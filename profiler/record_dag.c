/* 
 * dag_recorder.c
 */

#include <errno.h>
#include <string.h>
#include "dag_recorder_impl.h"

dr_global_state GS;
dr_thread_specific_state * TS = 0;

/* --------------------- dr_get_worker ------------------- */

dr_get_worker_key_struct dr_gwks = { 0, 0, 0 };

/* --------------------- free dag ------------------- */

void dr_free_dag_recursively(dr_dag_node * g) {
  if (g->info.kind == dr_dag_node_kind_create_task) {
    dr_free_dag_recursively(g->child);
    dr_free(g->child, sizeof(dr_dag_node));
  } else if (g->info.kind >= dr_dag_node_kind_section) {
    dr_dag_node_chunk * ch;
    for (ch = g->subgraphs->head; ch; ch = ch->next) {
      int i;
      for (i = 0; i < ch->n; i++) {
	dr_free_dag_recursively(&ch->a[i]);
      }
    }
    dr_dag_node_list_destroy(g->subgraphs);
    g->subgraphs = 0;
  }
}

/* --------------- depth first traverse a graph ------------ */

/* 
 stack of (dag nodes | string); infrastructure for
 printing hierarchical dag structure
 without recursion.
*/

typedef struct dr_dag_node_stack_cell {
  /* next cell in the stack or a free list */
  struct dr_dag_node_stack_cell * next;
  /* one and only one of these two fields are non-null */
  dr_dag_node * node;		/* this is a node */
} dr_dag_node_stack_cell;

typedef struct dr_dag_node_stack {
  /* free list (recycle popped cells) */
  dr_dag_node_stack_cell * freelist;
  /* the stack (linear list) */
  dr_dag_node_stack_cell * top;
} dr_dag_node_stack;

/* initialize the stack to be empty */
static void 
dr_dag_node_stack_init(dr_dag_node_stack * s) {
  s->freelist = 0;
  s->top = 0;
}

/* ensure the free list is not empty.
   get memory via malloc and fill the free list.
   return a pointer to a cell */
static dr_dag_node_stack_cell * 
ensure_freelist(dr_dag_node_stack * s) {
  dr_dag_node_stack_cell * f = s->freelist;
  if (f == 0) {
    int n = 100;
    f = (dr_dag_node_stack_cell *)
      dr_malloc(sizeof(dr_dag_node_stack_cell) * n);
    int i;
    for (i = 0; i < n - 1; i++) {
      f[i].next = &f[i + 1];
    }
    f[n - 1].next = 0;
    s->freelist = f;
  }
  return f;
}

/* push a dag node to the stack */
static void 
dr_dag_node_stack_push(dr_dag_node_stack * s, 
		       dr_dag_node * node) {
  dr_dag_node_stack_cell * f = ensure_freelist(s);
  f->node = node;
  s->freelist = f->next;
  f->next = s->top;
  s->top = f;
}

/* pop an element from the stack s.
   it is either a string or a dag node.
   the result is returned to one of 
   *xp (when it is a node) and *strp
   (when it is a string). the other one
   is filled with a null.
*/
static dr_dag_node * 
dr_dag_node_stack_pop(dr_dag_node_stack * s) {
  dr_dag_node_stack_cell * top = s->top;
  (void)dr_check(top);
  s->top = top->next;
  top->next = s->freelist;
  s->freelist = top;
  return top->node;
}

/* push the children of g in the reverse order,
   so that we later handle the children in the
   right order */
static void
dr_dag_node_stack_push_children(dr_dag_node_stack * s, 
				dr_dag_node * g) {
  if (g->info.kind < dr_dag_node_kind_section) {
    if (g->info.kind == dr_dag_node_kind_create_task
	&& g->child) {
      dr_dag_node_stack_push(s, g->child);
    }
  } else {
    (void)dr_check(g->subgraphs);
    dr_dag_node_chunk * head = g->subgraphs->head;
    dr_dag_node_chunk * tail = g->subgraphs->tail;
    dr_dag_node_chunk * ch;
    /* a bit complicated to reverse the children */

    /* 1. count the number of children */
    int n_children = 0;
    for (ch = head; ch; ch = ch->next) {
      n_children += ch->n;
    }
    /* 2. make the array of the right size and
       fill the array with children */
    dr_dag_node ** children
      = (dr_dag_node **)dr_malloc(sizeof(dr_dag_node *) * n_children);
    int idx = 0;
    dr_dag_node_kind_t K = g->info.kind;
    for (ch = head; ch; ch = ch->next) {
      int i;
      for (i = 0; i < ch->n; i++) {
	dr_dag_node_kind_t k = ch->a[i].info.kind;
	/* the entire if expression for checks */
	if (DAG_RECORDER_CHK_LEVEL>=1) {
	  if (K == dr_dag_node_kind_section) {
	    if (g->done && ch == tail && i == ch->n - 1) {
	      (void)dr_check(k == dr_dag_node_kind_wait_tasks);
	    } else {
	      (void)dr_check(k == dr_dag_node_kind_create_task 
			     || k == dr_dag_node_kind_section);
	    }
	  } else {
	    (void)dr_check(K == dr_dag_node_kind_task);
	    if (g->done && ch == tail && i == ch->n - 1) {
	      (void)dr_check(k == dr_dag_node_kind_end_task);
	    } else {
	      (void)dr_check(k == dr_dag_node_kind_section);
	    }
	  }
	}
	children[idx++] = &ch->a[i];
      }
    }
    assert(idx == n_children);
    /* 3. finally push them in the reverse order */
    for (idx = n_children - 1; idx >= 0; idx--) {
      dr_dag_node_stack_push(s, children[idx]);
    }
    dr_free(children, sizeof(dr_dag_node *) * n_children);
  }
}

/* --- make a position-independent copy of a graph --- */

/* copy g into p */
static void
dr_pi_dag_copy_1(dr_dag_node * g, 
		 dr_pi_dag_node * p, dr_pi_dag_node * lim) {
  assert(p < lim);
  p->info = g->info;
  g->forward = p;		/* record g was copied to p */
}

/* g has been copied, but its children have not been.
   copy g's children into q,
   and set g's children pointers to the copy,
   making g truly position independent now
 */
static dr_pi_dag_node * 
dr_pi_dag_copy_children(dr_dag_node * g, 
			dr_pi_dag_node * p, 
			dr_pi_dag_node * lim,
			dr_clock_t start_clock) {
  /* where g has been copied */
  dr_pi_dag_node * g_pi = g->forward;
  /* sanity check. g_pi should be a copy of g */
  assert(g_pi->info.start == g->info.start);
  /* make the time relative */
  g_pi->info.start -= start_clock;
  g_pi->info.end -= start_clock;

  if (g_pi->info.kind < dr_dag_node_kind_section) {
    /* copy the child if it is a create_task node */
    if (g_pi->info.kind == dr_dag_node_kind_create_task) {
      dr_pi_dag_copy_1(g->child, p, lim);
      /* install the (relative) pointer to the child */
      g_pi->child_offset = p - g_pi;
      p++;
    }
  } else {
    dr_dag_node_chunk * head = g->subgraphs->head;
    dr_dag_node_chunk * tail = g->subgraphs->tail;
    dr_dag_node_chunk * ch;
    if (DAG_RECORDER_CHK_LEVEL>=1) {
      if (head == tail && head->n == 0) {
	/* empty. the node must have been collapsed */
	(void)dr_check(g->collapsed);
      }
    }
    g_pi->subgraphs_begin_offset = p - g_pi;
    for (ch = head; ch; ch = ch->next) {
      int i;
      for (i = 0; i < ch->n; i++) {
	dr_pi_dag_copy_1(&ch->a[i], p, lim);
	p++;
      }
    }
    g_pi->subgraphs_end_offset = p - g_pi;
    if (DAG_RECORDER_CHK_LEVEL>=1) {
      if (g_pi->subgraphs_begin_offset == g_pi->subgraphs_end_offset) {
	(void)dr_check(g->collapsed);
      }
    }
  }
  return p;
}

/* --- count the number of nodes under g --- */

static long
dr_dag_count_nodes(dr_dag_node * g) {
  /* count the number of elements popped from stack */
  long n = 0;
  dr_dag_node_stack s[1];
  dr_dag_node_stack_init(s);
  dr_dag_node_stack_push(s, g);
  while (s->top) {
    dr_dag_node * x = dr_dag_node_stack_pop(s);
    n++;
    if (x->info.kind < dr_dag_node_kind_section) {
      if (x->info.kind == dr_dag_node_kind_create_task 
	  && x->child) {
	dr_dag_node_stack_push(s, x->child);
      }
    } else {
      dr_dag_node_stack_push_children(s, x);
    }
  }
  return n;
}

/* --- make position independent copy of g --- */

static void 
dr_pi_dag_init(dr_pi_dag * G) {
  G->n = 0;
  G->T = 0;
  G->m = 0;
  G->E = 0;
}

static void
dr_pi_dag_enum_nodes(dr_pi_dag * G,
		     dr_dag_node * g, dr_clock_t start_clock) {
  dr_dag_node_stack s[1];
  long n = dr_dag_count_nodes(g);
  dr_pi_dag_node * T 
    = (dr_pi_dag_node *)dr_malloc(sizeof(dr_pi_dag_node) * n);
  dr_pi_dag_node * lim = T + n; 
  dr_pi_dag_node * p = T; /* allocation pointer */

  dr_dag_node_stack_init(s);
  dr_pi_dag_copy_1(g, p, lim);
  p++;
  dr_dag_node_stack_push(s, g);
  while (s->top) {
    dr_dag_node * x = dr_dag_node_stack_pop(s);
    p = dr_pi_dag_copy_children(x, p, lim, start_clock);
    dr_dag_node_stack_push_children(s, x);
  }
  //dr_dag_node_stack_clear(s);
  G->n = n;
  G->T = T;
}

/* --------------------- enumurate edges ------------------- */

static long
dr_pi_dag_count_edges(dr_pi_dag * G) {
  long n_edges = 0;
  long i;
  for (i = 0; i < G->n; i++) {
    dr_pi_dag_node * u = &G->T[i];
    if (u->info.kind >= dr_dag_node_kind_section
	&& u->subgraphs_begin_offset < u->subgraphs_end_offset) {
      /* for each section or a task, count edges
	 between its direct children.
	 (1) task : each child's last node
	 -> the next child's first node
	 (2) section : the above +
	 each create_task -> the task's first insn
	 each task's last insn -> 
      */
      dr_pi_dag_node * ua = u + u->subgraphs_begin_offset;
      dr_pi_dag_node * ub = u + u->subgraphs_end_offset;
      dr_pi_dag_node * x;
      n_edges += ub - ua - 1;
      if (u->info.kind == dr_dag_node_kind_section) {
	for (x = ua; x < ub; x++) {
	  if (x->info.kind == dr_dag_node_kind_create_task) {
	    n_edges += 2;
	  }
	}
      }
    }
  }
  return n_edges;
}

static void 
dr_pi_dag_add_edge(dr_pi_dag_edge * e, dr_pi_dag_edge * lim, 
		   dr_edge_kind_t kind, long u, long v) {
  assert(e < lim);
  e->kind = kind;
  e->u = u;
  e->v = v;
}

dr_pi_dag_node *
dr_pi_dag_node_first(dr_pi_dag_node * g, dr_pi_dag * G) {
  dr_pi_dag_node * lim = G->T + G->n;
  assert(g < lim);
  while (g->info.kind >= dr_dag_node_kind_section
	 && g->subgraphs_begin_offset < g->subgraphs_end_offset) {
    g = g + g->subgraphs_begin_offset;
    assert(g < lim);
  }
  return g;
}

dr_pi_dag_node *
dr_pi_dag_node_last(dr_pi_dag_node * g, dr_pi_dag * G) {
  dr_pi_dag_node * lim = G->T + G->n;
  assert(g < lim);
  while (g->info.kind >= dr_dag_node_kind_section 
	 && g->subgraphs_begin_offset < g->subgraphs_end_offset) {
    g = g + g->subgraphs_end_offset - 1;
    assert(g < lim);
  }
  return g;
}

/* count in G and list them. put the number of edges
   in G->m and array of edges in G->E */
static void 
dr_pi_dag_enum_edges(dr_pi_dag * G) {
  dr_pi_dag_node * T = G->T;
  long m = dr_pi_dag_count_edges(G);
  dr_pi_dag_edge * E
    = (dr_pi_dag_edge *)dr_malloc(sizeof(dr_pi_dag_edge) * m);
  dr_pi_dag_edge * e = E;
  dr_pi_dag_edge * E_lim = E + m;
  long i;
  for (i = 0; i < G->n; i++) {
    dr_pi_dag_node * u = T + i;
    if (u->info.kind >= dr_dag_node_kind_section) {
      dr_pi_dag_node * ua = u + u->subgraphs_begin_offset;
      dr_pi_dag_node * ub = u + u->subgraphs_end_offset;
      dr_pi_dag_node * x;
      for (x = ua; x < ub - 1; x++) {
	dr_pi_dag_node * t = dr_pi_dag_node_first(x + 1, G);
	/* x's last node -> x+1 */
	dr_pi_dag_node * s = dr_pi_dag_node_last(x, G);
	assert(e < E_lim);
	dr_pi_dag_add_edge(e, E_lim, dr_edge_kind_cont, s - T, t - T);
	e++;
	if (x->info.kind == dr_dag_node_kind_section) {
	  dr_pi_dag_node * xa = x + x->subgraphs_begin_offset;
	  dr_pi_dag_node * xb = x + x->subgraphs_end_offset;
	  dr_pi_dag_node * y;
	  for (y = xa; y < xb; y++) {
	    if (y->info.kind == dr_dag_node_kind_create_task) {
	      (void)dr_check(y < xb - 1);
	      /* y's last node -> x+1 */
	      dr_pi_dag_node * c = y + y->child_offset;
	      dr_pi_dag_node * z = dr_pi_dag_node_first(c, G);
	      dr_pi_dag_node * w = dr_pi_dag_node_last(c, G);
	      assert(e < E_lim);
	      dr_pi_dag_add_edge(e, E_lim, dr_edge_kind_create, y - T, z - T);
	      e++;
	      assert(e < E_lim);
	      dr_pi_dag_add_edge(e, E_lim, dr_edge_kind_end, w - T, t - T);
	      e++;
	    }
	  }
	}
      }
    }
  }
  assert(e == E_lim);
  G->m = m;
  G->E = E;
}

static int edge_cmp(const void * e_, const void * f_) {
  const dr_pi_dag_edge * e = e_;
  const dr_pi_dag_edge * f = f_;
  if (e->u < f->u) return -1;
  if (e->u > f->u) return 1;
  if (e->v < f->v) return -1;
  if (e->v > f->v) return 1;
  return 0;
}

static void 
dr_pi_dag_sort_edges(dr_pi_dag * G) {
  dr_pi_dag_edge * E = G->E;
  long m = G->m;
  qsort((void *)E, m, sizeof(dr_pi_dag_edge), edge_cmp);
}

/* we have G->E. put edges_begin/edges_end 
   ptrs of G-T */
static void 
dr_pi_dag_set_edge_ptrs(dr_pi_dag * G) {
  dr_pi_dag_node * T = G->T;
  dr_pi_dag_edge * E = G->E;
  long m = G->m, n = G->n;
  long i = 0;
  long j;
  T[i].edges_begin = 0;
  for (j = 0; j < m; j++) {
    long u = E[j].u;
    while (i < u) {
      T[i].edges_end = j;
      T[i+1].edges_begin = j;
      i++;
    }
  }
  while (i < n - 1) {
    T[i].edges_end = m;
    T[i+1].edges_begin = m;
    i++;
  }
  T[n - 1].edges_end = m;
}

static void
dr_make_pi_dag(dr_pi_dag * G,
	       dr_dag_node * g, dr_clock_t start_clock) {
  dr_pi_dag_init(G);
  dr_pi_dag_enum_nodes(G, g, start_clock);
  dr_pi_dag_enum_edges(G);
  dr_pi_dag_sort_edges(G);
  dr_pi_dag_set_edge_ptrs(G);
}


/* initialization */

static int 
getenv_bool(const char * v, char * y) {
  char * x = getenv(v);
  if (!x) return 0;
  if (strcasecmp(x, "true") == 0
      || strcasecmp(x, "yes") == 0) {
    *y = 1;
  } else {
    *y = atoi(x); 
  } 
  return 1;
} 

static int 
getenv_byte(const char * v, char * y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = atoi(x);
  return 1;
} 

static int 
getenv_int(const char * v, int * y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = atoi(x);
  return 1;
} 

static int 
getenv_str(const char * v, const char ** y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = strdup(x);
  return 1;
} 

void dr_options_default(dr_options * opts) {
  * opts = dr_options_default_values;

  if (getenv_str("DAG_RECORDER_LOG_FILE",    &opts->log_file)
      || getenv_str("DR_LOG_FILE",           &opts->log_file)) {}
  if (getenv_str("DAG_RECORDER_DOT_FILE",    &opts->dot_file)
      || getenv_str("DR_DOT",                &opts->dot_file)) {}
  if (getenv_str("DAG_RECORDER_GPL_FILE",    &opts->gpl_file)
      || getenv_str("DR_GPL",                &opts->gpl_file)) {}
  if (getenv_int("DAG_RECORDER_GPL_SIZE",    &opts->gpl_sz)
      || getenv_int("DR_GPL_SZ",             &opts->gpl_sz)) {}

  if (getenv_byte("DAG_RECORDER_DBG_LEVEL",  &opts->dbg_level)
      || getenv_byte("DR_DBG",               &opts->dbg_level)) {}
  if (getenv_bool("DAG_RECORDER_COLLAPSE",   &opts->collapse)
      || getenv_bool("DR_COLLAPSE",          &opts->collapse)) {}
  if (getenv_bool("DAG_RECORDER_DUMP_ON_STOP", &opts->dump_on_stop)
      || getenv_bool("DR_DUMP",              &opts->dump_on_stop)) {}
}

void dr_start_(dr_options * opts, int worker, int num_workers) {
  const char * dag_recorder = getenv("DAG_RECORDER");
  if (!dag_recorder || atoi(dag_recorder)) {
    dr_options opts_[1];
    if (!opts) {
      opts = opts_;
      dr_options_default(opts);
    }
    TS = (dr_thread_specific_state *)
      dr_malloc(sizeof(dr_thread_specific_state) 
		* num_workers);
    GS.opts = *opts;
    GS.start_clock = dr_get_tsc();
    dr_start_task_(0, worker);
    GS.root = dr_get_cur_task_(worker);
  }
}

int dr_gen_dot(dr_pi_dag * G);
int dr_gen_gpl(dr_pi_dag * G);

void dr_dump() {
  dr_pi_dag G[1];
  dr_make_pi_dag(G, GS.root, GS.start_clock);
  dr_free_dag_recursively(GS.root); GS.root = NULL;
  dr_gen_dot(G);
  dr_gen_gpl(G);
}

void dr_stop_(int worker) {
  dr_end_task_(worker);
  if (GS.opts.dump_on_stop) {
    dr_dump();
  }
}


