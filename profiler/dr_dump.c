/* 
 * record_dag.c
 */

#include <errno.h>
#include <string.h>

#define DAG_RECORDER 2
#include "dag_recorder_impl.h"

extern dr_global_state GS;

/* --------------- depth first traverse a graph ------------ */

static void 
dr_dag_node_list_free(dr_dag_node_list * l, 
		      dr_dag_node_freelist * fl) 
  __attribute__ ((unused));

/* free all nodes in l */
static void 
dr_dag_node_list_free(dr_dag_node_list * l, 
		      dr_dag_node_freelist * fl) {
#if DAG_RECORDER_VALGRIND_MEM_DBG
  dr_dag_node * ch;
  dr_dag_node * next;
  for (ch = l->head; ch; ch = next) {
    next = ch->next;
    dr_dag_node_free(ch, fl);
  }
  dr_dag_node_list_init(l);
#else
  if (l->head) {
    (void)dr_check(l->tail);
    (void)dr_check(!l->tail->next);
    l->tail->next = fl->head;
    if (!fl->head)
      fl->tail = l->tail;
    fl->head = l->head;
    dr_dag_node_list_init(l);
  } else {
    (void)dr_check(!l->tail);
  }
#endif
  (void)dr_check(!l->head);
  (void)dr_check(!l->tail);
}

/* --- make a position-independent copy of a graph --- */

typedef struct dr_string_table_cell {
  struct dr_string_table_cell * next;
  const char * s;
} dr_string_table_cell;

typedef struct {
  dr_string_table_cell * head;
  dr_string_table_cell * tail;
  long n;
} dr_string_table;

static void 
dr_string_table_init(dr_string_table * t) {
  t->n = 0;
  t->head = t->tail = 0;
}

static void 
dr_string_table_destroy(dr_string_table * t) {
  dr_string_table_cell * c;
  dr_string_table_cell * next;
  for (c = t->head; c; c = next) {
    next = c->next;
    dr_free(c, sizeof(dr_string_table_cell));
  }
}

static long 
dr_string_table_find(dr_string_table * t, const char * s) {
  dr_string_table_cell * c;
  long i = 0;
  for (c = t->head; c; c = c->next) {
    assert(c->s);
    assert(s);
    if (strcmp(c->s, s) == 0) return i;
    i++;
  }
  (void)dr_check(i == t->n);
  return i;
}

static void
dr_string_table_append(dr_string_table * t, const char * s) {
  dr_string_table_cell * c 
    = (dr_string_table_cell *)dr_malloc(sizeof(dr_string_table_cell));
  c->s = s;
  c->next = 0;
  if (t->head) {
    (void)dr_check(t->tail);
    t->tail->next = c;
  } else {
    (void)dr_check(!t->tail);
    t->head = c;
  }
  t->tail = c;
  t->n++;
}

/* find s in the string table t.
   if not found, return a new index */
static long 
dr_string_table_intern(dr_string_table * t, const char * s) {
  long idx = dr_string_table_find(t, s);
  if (idx == t->n) {
    dr_string_table_append(t, s);
  }
  return idx;
}

/* given linked-list based string table t,
   flatten it.
   flattened table consists of
   a flat char array contigously
   storing all strings and an array
   of indexes into that string array;
   these two arrays are also stored
   contiguously in memory.
   there is also a header pointing to them.
   
   before:
   |abc|-->|defg|-->|hi|-->|

   after:
 +-0
 | 4 -----+            Index Array
 | 9 -----+-----+
 |        |     |
 +-> abc\0defg\0hi\0   Char Array
     0... .5... .10

*/
static dr_pi_string_table *
dr_string_table_flatten(dr_string_table * t) {
  long str_bytes = 0;		/* string length */
  int n = 0;
  dr_string_table_cell * c;
  for (c = t->head; c; c = c->next) {
    n++;
    str_bytes += strlen(c->s) + 1;
  }  
  {
    long header_bytes = sizeof(dr_pi_string_table);
    long table_bytes = n * sizeof(const char *);
    long total_bytes = header_bytes + table_bytes + str_bytes;
    void * a = dr_malloc(total_bytes); /* leaking */
    dr_pi_string_table * h = a;
    /* index array */
    long * I = a + header_bytes;
    /* char array */
    char * C = a + header_bytes + table_bytes;
    char * p = C;
    long i = 0;
    h->n = n;
    h->sz = total_bytes;
    h->I = I;
    h->C = C;
    for (c = t->head; c; c = c->next) {
      strcpy(p, c->s);
      I[i] = p - C;
      p += strlen(c->s) + 1;
      i++;
    }  
    (void)dr_check(i == n);
    (void)dr_check(p == C + str_bytes);
    return h;
  }
}

/* copy g into p */
static void
dr_pi_dag_copy_1(dr_dag_node * g, 
		 dr_pi_dag_node * p, dr_pi_dag_node * lim,
		 dr_string_table * st) {
  assert(p < lim);
  p->info = g->info;
  p->info.start.pos.file = 0;
  assert(g->info.start.pos.file);
  assert(strlen(g->info.start.pos.file) > 0);
  p->info.start.pos.file_idx
    = dr_string_table_intern(st, g->info.start.pos.file);
  p->info.end.pos.file = 0;
  p->info.end.pos.file_idx
    = dr_string_table_intern(st, g->info.end.pos.file);
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
			dr_clock_t start_clock,
			dr_string_table * st) {
  /* where g has been copied */
  dr_pi_dag_node * g_pi = g->forward;
  /* sanity check. g_pi should be a copy of g */
  assert(g_pi->info.start.t == g->info.start.t);
  /* make the time relative */
  g_pi->info.start.t       -= start_clock;
  g_pi->info.end.t         -= start_clock;
  g_pi->info.first_ready_t -= start_clock;
  g_pi->info.last_start_t  -= start_clock;

  if (g_pi->info.kind < dr_dag_node_kind_section) {
    /* copy the child if it is a create_task node */
    if (g_pi->info.kind == dr_dag_node_kind_create_task) {
      dr_pi_dag_copy_1(g->child, p, lim, st);
      /* install the (relative) pointer to the child */
      g_pi->child_offset = p - g_pi;
      p++;
    }
  } else {
    dr_dag_node * head = g->subgraphs->head;
    dr_dag_node * ch;
    g_pi->subgraphs_begin_offset = p - g_pi;
    for (ch = head; ch; ch = ch->next) {
      dr_pi_dag_copy_1(ch, p, lim, st);
      p++;
    }
    g_pi->subgraphs_end_offset = p - g_pi;
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
  dr_dag_node_stack_fini(s);
  return n;
}


/* --- make position independent copy of g --- */

static void 
dr_pi_dag_init(dr_pi_dag * G) {
  G->n = 0;
  G->T = 0;
  G->m = 0;
  G->E = 0;
  G->S = 0;
}

static void
dr_pi_dag_enum_nodes(dr_pi_dag * G,
		     dr_dag_node * g, dr_clock_t start_clock,
		     dr_string_table * st) {
  dr_dag_node_stack s[1];
  long n = dr_dag_count_nodes(g);
  dr_pi_dag_node * T 
    = (dr_pi_dag_node *)dr_malloc(sizeof(dr_pi_dag_node) * n);
  dr_pi_dag_node * lim = T + n; 
  dr_pi_dag_node * p = T; /* allocation pointer */

  memset(T, 0, sizeof(dr_pi_dag_node) * n);

  dr_dag_node_stack_init(s);
  dr_pi_dag_copy_1(g, p, lim, st);
  p++;
  dr_dag_node_stack_push(s, g);
  while (s->top) {
    dr_dag_node * x = dr_dag_node_stack_pop(s);
    p = dr_pi_dag_copy_children(x, p, lim, start_clock, st);
    dr_dag_node_stack_push_children(s, x);
  }
  //dr_dag_node_stack_clear(s);
  G->n = n;
  G->T = T;
  dr_dag_node_stack_fini(s);
}

/* --------------------- enumurate edges ------------------- */

/* count the number of edges left uncollapsed in the memory */
static long
dr_pi_dag_count_edges_uncollapsed(dr_pi_dag * G) {
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
      /* edges between u's i-th child to (i+1)-th child */
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
		   dr_dag_edge_kind_t kind, long u, long v) {
  assert(e < lim);
  memset(e, 0, sizeof(dr_pi_dag_edge));
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
  long m = dr_pi_dag_count_edges_uncollapsed(G);
  dr_pi_dag_edge * E
    = (dr_pi_dag_edge *)dr_malloc(sizeof(dr_pi_dag_edge) * m); /* leaking */
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
	/* x's last node -> x+1 */
	dr_pi_dag_node * s = dr_pi_dag_node_last(x, G);
	dr_pi_dag_node * t = dr_pi_dag_node_first(x + 1, G);
	assert(e < E_lim);
	switch (t->info.in_edge_kind) {
	case dr_dag_edge_kind_create_cont:
	case dr_dag_edge_kind_wait_cont:
	case dr_dag_edge_kind_other_cont:
	  dr_pi_dag_add_edge(e, E_lim, t->info.in_edge_kind, 
			     s - T, t - T);
	  break;
	case dr_dag_edge_kind_end:
	  dr_pi_dag_add_edge(e, E_lim, dr_dag_edge_kind_wait_cont, 
			     s - T, t - T);
	  break;
	default:
	  /* case dr_dag_edge_kind_end should not happen */
	  /* create_cont can't happen. */
	  (void)dr_check(t->info.in_edge_kind 
			 != dr_dag_edge_kind_create);
	  (void)dr_check(0);
	  break;
	}
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
	      dr_pi_dag_add_edge(e, E_lim, dr_dag_edge_kind_create, y - T, z - T);
	      e++;
	      assert(e < E_lim);
	      dr_pi_dag_add_edge(e, E_lim, dr_dag_edge_kind_end, w - T, t - T);
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

static int 
edge_cmp(const void * e_, const void * f_) {
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
dr_pi_dag_set_string_table(dr_pi_dag * G, dr_string_table * st) {
  G->S = dr_string_table_flatten(st); /* G->S */
}

static int 
dr_get_number_of_workers() {
  if (GS.worker_specific_state_array) {
    return GS.worker_specific_state_array_sz;
  } else {
    dr_worker_specific_state * wss;
    int n = 0;
    for (wss = GS.worker_specific_state_list; wss; wss = wss->next) {
      n++;
    }
    return n;
  }
}

/* convert pointer-based dag structure (g)
   into a "position-independent" format (G)
   suitable for dumping into disk */
static void
dr_make_pi_dag(dr_pi_dag * G, dr_dag_node * g, 
	       dr_clock_t start_clock) {
  dr_string_table st[1];
  dr_string_table_init(st);
  G->num_workers = dr_get_number_of_workers();
  G->start_clock = start_clock;
  dr_pi_dag_init(G);
  dr_pi_dag_enum_nodes(G, g, start_clock, st); /* G->T */
  dr_pi_dag_enum_edges(G);	/* G->E */
  dr_pi_dag_sort_edges(G);
  dr_pi_dag_set_edge_ptrs(G);
  dr_pi_dag_set_string_table(G, st); /* G->S */
  dr_string_table_destroy(st);
}

static void
dr_destroy_pi_dag(dr_pi_dag * G) {
  dr_free(G->T, G->n * sizeof(dr_pi_dag_node));
  dr_free(G->E, G->m * sizeof(dr_pi_dag_edge));
  dr_free(G->S, G->S->sz);	/* string table */
}

/* format of the dag file
   n 
   m 
   num_workers
   array of nodes
   array of edges
   number of strings
   string offset table
   flat string arrays
 */

static int 
dr_pi_dag_dump(dr_pi_dag * G, FILE * wp, 
	       const char * filename) {
  long total_sz = 0;
  total_sz += sizeof(G->n);
  total_sz += sizeof(G->m);
  total_sz += sizeof(G->start_clock);
  total_sz += sizeof(G->num_workers);
  total_sz += sizeof(dr_pi_dag_node) * G->n;
  total_sz += sizeof(dr_pi_dag_edge) * G->m;
  total_sz += G->S->sz;
  if (GS.opts.verbose_level >= 1) 
    fprintf(stderr, "dr_pi_dag_dump: %ld bytes\n", total_sz);

  if (fwrite(DAG_RECORDER_HEADER, DAG_RECORDER_HEADER_LEN, 1, wp) != 1
      || fwrite(&G->n, sizeof(G->n), 1, wp) != 1
      || fwrite(&G->m, sizeof(G->m), 1, wp) != 1
      || fwrite(&G->start_clock, sizeof(G->start_clock), 1, wp) != 1
      || fwrite(&G->num_workers, sizeof(G->num_workers), 1, wp) != 1
      || (long)fwrite(G->T, sizeof(dr_pi_dag_node), G->n, wp) != G->n
      || (long)fwrite(G->E, sizeof(dr_pi_dag_edge), G->m, wp) != G->m
      || fwrite(G->S, G->S->sz, 1, wp) != 1) {
    fprintf(stderr, "fwrite: %s (%s)\n", 
	    strerror(errno), filename);
    return 0;
  } else {
    return 1;
  }
}

/* dump the position-independent dag into a file */
static int dr_gen_pi_dag(dr_pi_dag * G) {
  if (GS.opts.dag_file_yes) {
    const char * prefix = GS.opts.dag_file_prefix;
    const char * ext = ".dag";
    const int len = strlen(prefix) + strlen(ext) + 1;
    char * filename = dr_malloc(len);
    FILE * wp;
    strcpy(filename, prefix);
    strcat(filename, ext);
    if (GS.opts.verbose_level >= 1)
      fprintf(stderr, "dag_recorder: writing dag to %s\n", filename);
    wp = fopen(filename, "wb");
    if (!wp) { 
      fprintf(stderr, "error: fopen: %s (%s)\n", strerror(errno), filename); 
      dr_free(filename, len);
      return 0;
    }
    dr_pi_dag_dump(G, wp, filename);
    dr_free(filename, len);
    fclose(wp);
  } else {
    if (GS.opts.verbose_level >= 1)
      fprintf(stderr, "not writing dag\n");
  }
  return 1;
}

/* open filename to write position independent dag 
   (this function does not belong to this file...) */
FILE *
dr_pi_dag_open_to_write(const char * prefix, const char * ext,
			const char * file_kind, 
			int show_message) {
  int len = strlen(prefix) + strlen(ext) + 1;
  char * filename = dr_malloc(len);
  FILE * wp;
  strcpy(filename, prefix);
  strcat(filename, ext);
  if (show_message) 
    fprintf(stderr, "dag recorder: writing %s to %s\n", file_kind, filename);
  wp = fopen(filename, "wb");
  if (!wp) { 
    fprintf(stderr, "error: fopen: %s (%s)\n", strerror(errno), filename); 
  }
  dr_free(filename, len);
  return wp;
}

/* --------------------- free dag ------------------- */

void dr_dump_() {
  if (GS.root) {
    dr_pi_dag G[1];
    dr_make_pi_dag(G, GS.root, GS.start_clock);
    interpolate_counters(G);
    dr_gen_pi_dag(G);
    dr_gen_basic_stat(G);
    dr_gen_gpl(G);
    dr_gen_dot(G);
    dr_gen_text(G);
    dr_destroy_pi_dag(G);
  }
}


