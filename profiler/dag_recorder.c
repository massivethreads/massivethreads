/* 
 * dag_recorder.c
 */

#include <dag_recorder.h>

dr_global_state GS;
dr_thread_specific_state * TS = 0;

/* --------------------- free dag ------------------- */

void dr_free_dag_recursively(dr_dag_node * g) {
  if (g->kind == dr_dag_node_kind_create_task) {
    dr_free_dag_recursively(g->child);
    dr_free(g->child, sizeof(dr_dag_node));
  } else if (g->kind >= dr_dag_node_kind_section) {
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

/* --------------------- print functions ------------------- */

static const char * 
dr_node_kind_str(dr_dag_node_kind_t kind) {
  switch (kind) {
  case dr_dag_node_kind_create_task: return "create_task";
  case dr_dag_node_kind_wait_tasks:  return "wait_tasks";
  case dr_dag_node_kind_end_task:    return "end_task";
  case dr_dag_node_kind_section:     return "section";
  case dr_dag_node_kind_task:        return "task";
  default : (void)dr_check(0);
  }
  return (const char *)0;
}

/* print or dump dag, without recursion */

typedef struct dr_dag_node_stack_node {
  struct dr_dag_node_stack_node * next;
  dr_dag_node * node;
  const char * str;
} dr_dag_node_stack_node;

typedef struct dr_dag_node_stack {
  dr_dag_node_stack_node * free;
  dr_dag_node_stack_node * top;
} dr_dag_node_stack;

static void 
dr_dag_node_stack_init(dr_dag_node_stack * s) {
  s->free = 0;
  s->top = 0;
}

static dr_dag_node_stack_node * 
ensure_free(dr_dag_node_stack * s) {
  dr_dag_node_stack_node * f = s->free;
  if (f == 0) {
    int n = 100;
    f = (dr_dag_node_stack_node *)
      dr_malloc(sizeof(dr_dag_node_stack_node) * n);
    int i;
    for (i = 0; i < n - 1; i++) {
      f[i].next = &f[i + 1];
    }
    f[n - 1].next = 0;
    s->free = f;
  }
  return f;
}

static void 
dr_dag_node_stack_push_node(dr_dag_node_stack * s, 
			    dr_dag_node * node) {
  dr_dag_node_stack_node * f = ensure_free(s);
  f->node = node;
  f->str = (const char *)0;
  s->free = f->next;
  f->next = s->top;
  s->top = f;
}

static void 
dr_dag_node_stack_push_string(dr_dag_node_stack * s, 
			      const char * str) {
  dr_dag_node_stack_node * f = ensure_free(s);
  f->node = (dr_dag_node *)0;
  f->str = str;
  s->free = f->next;
  f->next = s->top;
  s->top = f;
}

static void
dr_dag_node_stack_pop_node_or_string(dr_dag_node_stack * s,
				     dr_dag_node ** xp,
				     const char ** strp) {
  dr_dag_node_stack_node * top = s->top;
  dr_check(top);
  s->top = top->next;
  top->next = s->free;
  s->free = top;
  assert(top->node || top->str);
  assert(!top->node || !top->str);
  *xp = top->node;
  *strp = top->str;
}

static void
dr_dag_node_stack_push_children(dr_dag_node_stack * s, 
				dr_dag_node * g) {
  (void)dr_check(g->subgraphs);
  dr_dag_node_chunk * head = g->subgraphs->head;
  dr_dag_node_chunk * tail = g->subgraphs->tail;
  dr_dag_node_chunk * ch;
  int n_children = 0;
  for (ch = head; ch; ch = ch->next) {
    n_children += ch->n;
  }
  dr_dag_node ** children
    = (dr_dag_node **)dr_malloc(sizeof(dr_dag_node *) * n_children);
  int idx = 0;
  dr_dag_node_kind_t K = g->kind;
  for (ch = head; ch; ch = ch->next) {
    int i;
    for (i = 0; i < ch->n; i++) {
      dr_dag_node_kind_t k = ch->a[i].kind;
      /* the entire if expression for checks */
      if (DAG_RECORDER_CHK_LEVEL) {
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
  for (idx = n_children - 1; idx >= 0; idx--) {
    dr_dag_node_stack_push_node(s, children[idx]);
  }
  dr_free(children, sizeof(dr_dag_node *) * n_children);
}

static void 
dr_print_dag(dr_dag_node * g, dr_clock_t start_clock, FILE * wp);

static void 
dr_print_dag_1(dr_dag_node * g, dr_clock_t start_clock, FILE * wp) {
  fprintf(wp, 
	  "%s %s %llu - %llu, est=%llu, T1/Tinf=%llu/%llu, nodes/edges=%ld/%ld by %d on %d @%p",
	  dr_node_kind_str(g->kind),
	  dr_node_kind_str(g->last_node_kind),
	  g->start - start_clock, 
	  g->end - start_clock, 
	  g->est, 
	  g->t_1, g->t_inf,
	  g->n_nodes, g->n_edges, g->worker, g->cpu,
	  g);
}

static void 
dr_print_dag(dr_dag_node * g, dr_clock_t start_clock,
	     FILE * wp) {
  dr_dag_node_stack s[1];
  dr_dag_node_stack_init(s);
  dr_dag_node_stack_push_node(s, g);
  while (s->top) {
    dr_dag_node * x = 0;
    const char * str = 0;
    dr_dag_node_stack_pop_node_or_string(s, &x, &str);
    if (str) {
      fprintf(wp, "%s", str);
    } else {
      dr_print_dag_1(x, start_clock, wp);
      if (x->kind < dr_dag_node_kind_section) {
	if (x->kind == dr_dag_node_kind_create_task && x->child) {
	  fprintf(wp, " {\n");
	  dr_dag_node_stack_push_string(s, "}\n");
	  dr_dag_node_stack_push_node(s, x->child);
	} else {
	  fprintf(wp, "\n");
	}
      } else {
	fprintf(wp, " (\n");
	dr_dag_node_stack_push_string(s, ")\n");
	dr_dag_node_stack_push_children(s, x);
      }
    }
  }
}

static void 
dr_print_dag_rec(dr_dag_node * g, dr_clock_t start_clock,
		 FILE * wp, int indent) {
  int i;
  int real_indent = indent;
  if (real_indent > 10) real_indent = 10;

  if (g->kind < dr_dag_node_kind_section) {
    for (i = 0; i < real_indent; i++) fputc(' ', wp);
    fprintf(wp, 
	    "%s[%llu-%llu,est=%llu,W=%llu/%llu,sz=%ld/%ld by %d on %d @%p]",
	    dr_node_kind_str(g->kind),
	    g->start - start_clock, 
	    g->end - start_clock, 
	    g->est, 
	    g->t_1, g->t_inf,
	    g->n_nodes, g->n_edges, g->worker, g->cpu,
	    g);
    if (g->kind == dr_dag_node_kind_create_task && g->child) {
      fprintf(wp, " {\n");
      dr_print_dag_rec(g->child, start_clock, wp, indent + 1);
      for (i = 0; i < real_indent; i++) fputc(' ', wp);
      fprintf(wp, "}\n");
    } else {
      fprintf(wp, "\n");
    }
  } else {
    dr_dag_node_kind_t K = g->kind;
    (void)dr_check(K == dr_dag_node_kind_section
		   || K == dr_dag_node_kind_task);
    for (i = 0; i < real_indent; i++) fputc(' ', wp);
    if (GS.opts.collapse) {
      fprintf(wp, "%s(%llu-%llu,est=%llu,W=%llu/%llu,sz=%ld/%ld by %d on %d collapsed=%d @%p", 
	      dr_node_kind_str(g->kind), 
	      g->start - start_clock, 
	      g->end - start_clock, 
	      g->est, 
	      g->t_1, g->t_inf,
	      g->n_nodes, g->n_edges, g->worker, g->cpu,
	      g->collapsed,
	      g);
    } else {
      fprintf(wp, "%s(@%p", dr_node_kind_str(g->kind), g);
    }
    (void)dr_check(g->subgraphs);
    dr_dag_node_chunk * head = g->subgraphs->head;
    dr_dag_node_chunk * tail = g->subgraphs->tail;
    dr_dag_node_chunk * ch;
    for (ch = head; ch; ch = ch->next) {
      for (i = 0; i < ch->n; i++) {
	dr_dag_node_kind_t k = ch->a[i].kind;
	/* the entire if expression for checks */
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
	dr_print_dag_rec(&ch->a[i], start_clock, wp, indent + 1);
      }
    }
    for (i = 0; i < real_indent; i++) fputc(' ', wp);
    fprintf(wp, ")\n");
  }
}

/* --------------------- gen_dot functions ------------------- */

static dr_dag_node * 
dr_first_interval(dr_dag_node * t) {
  dr_dag_node * s = t;
  while (s->kind >= dr_dag_node_kind_section) {
    if (dr_dag_node_list_empty(s->subgraphs)) {
      (void)dr_check(GS.opts.collapse);
      (void)dr_check(s->collapsed);
      break;
    }
    s = &s->subgraphs->head->a[0];
  }
  return s;
}

static dr_dag_node * 
dr_last_interval(dr_dag_node * t) {
  dr_dag_node * s = t;
  while (s->kind >= dr_dag_node_kind_section) {
    if (dr_dag_node_list_empty(s->subgraphs)) {
      (void)dr_check(GS.opts.collapse);
      (void)dr_check(s->collapsed);
      break;
    }
    dr_dag_node_chunk * tail = s->subgraphs->tail;
    s = &tail->a[tail->n - 1];
  }
  return s;
}

static void
dr_gen_dot_edge_i_i(dr_dag_node * A, dr_dag_node * B, FILE * wp) {
  fprintf(wp, "T%lu -> T%lu;\n", 
	  (unsigned long)A, (unsigned long)B);
}

/* A is a section; generate edges from each of create_task's
   last node in S to B. */
static void 
dr_gen_dot_edges_s_i(dr_dag_node * A, dr_dag_node * B, FILE * wp) {
  (void)dr_check(A->kind >= dr_dag_node_kind_section);
  dr_dag_node_chunk * ch;
  for (ch = A->subgraphs->head; ch; ch = ch->next) {
    int i;
    for (i = 0; i < ch->n; i++) {
      dr_dag_node * c = &ch->a[i];
      if (c->kind == dr_dag_node_kind_create_task) {
	dr_gen_dot_edge_i_i(dr_last_interval(c->child), B, wp);
      }
    }
  }
}

static void 
dr_gen_dot_dag(dr_dag_node * g, dr_clock_t start_clock, FILE * wp) {
  if (g->kind < dr_dag_node_kind_section
      || g->collapsed) {
    fprintf(wp, 
	    "T%lu [label=\"%s\\n"
	    "%llu-%llu est=%llu\\n"
	    "W=%llu/%llu,sz=%ld/%ld by %d on %d\"];\n",
	    (unsigned long)g,
	    dr_node_kind_str(g->kind),
	    g->start - start_clock, 
	    g->end - start_clock, 
	    g->est, 
	    g->t_1, g->t_inf,
	    g->n_nodes, g->n_edges, g->worker, g->cpu);
    if (g->kind == dr_dag_node_kind_create_task && g->child) {
      dr_gen_dot_edge_i_i(g, dr_first_interval(g->child), wp);
      dr_gen_dot_dag(g->child, start_clock, wp);
    }
  } else {
    dr_dag_node_kind_t K = g->kind;
    dr_dag_node_chunk * head = g->subgraphs->head;
    dr_dag_node_chunk * tail = g->subgraphs->tail;
    dr_dag_node_chunk * ch;
    for (ch = head; ch; ch = ch->next) {
      int i;
      for (i = 0; i < ch->n; i++) {
	dr_dag_node_kind_t k = ch->a[i].kind;
	/* the entire if expression is just for sanity check */
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
	/* here is the real body */
	if (ch != head || i > 0) { 
	  dr_gen_dot_edge_i_i(dr_last_interval(&ch->a[i - 1]), 
			      dr_first_interval(&ch->a[i]), wp);
	  if (ch->a[i - 1].kind == dr_dag_node_kind_section) {
	    dr_gen_dot_edges_s_i(&ch->a[i - 1], 
				 dr_first_interval(&ch->a[i]), wp);
	  }
	}
	dr_gen_dot_dag(&ch->a[i], start_clock, wp);
      }
    }
  }
}

void dr_print_task_graph(const char * filename) {
  if (GS.root) {
    if (GS.opts.dbg_level>=1) {
      if (filename) {
	fprintf(stderr, "generating log file to \"%s\"\n", 
		filename);
      } else {
	fprintf(stderr, "generating log file to stdout\n");
      }
    }
    {
      FILE * wp = (filename ? fopen(filename, "wb") : stdout);
      if (!wp) { perror("fopen"); exit(1); }
      dr_print_dag(GS.root, GS.start_clock, wp);
    }
  } else {
    fprintf(stderr, "dr_print_task_graph: no task graph to print!\n");
  }
}

void dr_gen_dot_task_graph(const char * filename) {
  if (GS.root) {
    if (GS.opts.dbg_level>=1) {
      if (filename) {
	fprintf(stderr, "generating dot file to \"%s\"\n", 
		filename);
      } else {
	fprintf(stderr, "generating dot file to stdout\n");
      }
    }
    {
      FILE * wp = (filename ? fopen(filename, "wb") : stdout);
      if (!wp) { perror("fopen"); exit(1); }
      fprintf(wp, "digraph G {\n");
      dr_gen_dot_dag(GS.root, GS.start_clock, wp);
      fprintf(wp, "}\n");
      fclose(wp);
    }
  } else {
    fprintf(stderr, "dr_gen_dot_task_graph: no task graph to dot!\n");
  }
}

int getenv_bool(const char * v, char * y) {
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

int getenv_int(const char * v, int * y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = atoi(x);
  return 1;
} 

int getenv_byte(const char * v, char * y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = atoi(x);
  return 1;
} 

int getenv_str(const char * v, const char ** y) {
  char * x = getenv(v);
  if (!x) return 0;
  *y = strdup(x);
  return 1;
} 

void dr_options_default(dr_options * opts) {
  * opts = dr_options_default_values;

  getenv_byte("DAG_RECORDER_DBG_LEVEL",    &opts->dbg_level);
  getenv_bool("DAG_RECORDER_COLLAPSE",     &opts->collapse);
  getenv_bool("DAG_RECORDER_DUMP_ON_STOP", &opts->dump_on_stop);
  getenv_str("DAG_RECORDER_LOG_FILE",      &opts->log_file);
  getenv_str("DAG_RECORDER_DOT_FILE",      &opts->dot_file);
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

void dr_stop_(int worker) {
  dr_end_task_(worker);
  if (GS.opts.dump_on_stop) {
    dr_print_task_graph(GS.opts.log_file);
    //dr_gen_dot_task_graph(GS.opts.dot_file);
  }
}


