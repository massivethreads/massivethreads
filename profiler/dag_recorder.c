/* 
 * dag_recorder.c
 */

#include <dag_recorder.h>

dr_global_state GS;
dr_thread_specific_state * TS;

/* --------------------- free dag ------------------- */

void dr_free_dag_recursively(dr_dag_node * g) {
  int i;
  if (g->kind == dr_dag_node_kind_create_task) {
    dr_free_dag_recursively(g->child);
    dr_free(g->child, sizeof(dr_dag_node));
  } else if (g->kind >= dr_dag_node_kind_section) {
    int n = dr_dag_node_list_size(&g->subgraphs);
    for (i = 0; i < n; i++) {
      dr_free_dag_recursively(&g->subgraphs.a[i]);
    }
    dr_dag_node_list_clear(&g->subgraphs);
  }
}

/* --------------------- print functions ------------------- */

static const char * 
dr_node_kind_str(dr_dag_node_kind_t kind) {
  switch (kind) {
  case dr_dag_node_kind_task_group:  return "task_group";
  case dr_dag_node_kind_create_task: return "create_task";
  case dr_dag_node_kind_wait_tasks:  return "wait_tasks";
  case dr_dag_node_kind_end_task:    return "end_task";
  case dr_dag_node_kind_section:     return "section";
  case dr_dag_node_kind_task:        return "task";
  default : assert(0);
  }
}

static void 
dr_print_dag(dr_dag_node * g, dr_clock_t start_clock,
	     FILE * wp, int indent) {
  int i;
  if (g->kind < dr_dag_node_kind_section) {
    for (i = 0; i < indent; i++) fprintf(wp, " ");
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
      dr_print_dag(g->child, start_clock, wp, indent + 1);
      for (i = 0; i < indent; i++) fprintf(wp, " ");
      fprintf(wp, "}\n");
    } else {
      fprintf(wp, "\n");
    }
  } else {
    dr_dag_node_kind_t K = g->kind;
    assert(K == dr_dag_node_kind_section
	   || K == dr_dag_node_kind_task);
    for (i = 0; i < indent; i++) fprintf(wp, " ");
    if (GS.opts.collapse) {
      fprintf(wp, "%s(%llu-%llu,est=%llu,W=%llu/%llu,sz=%ld/%ld by %d on %d @%p\n", 
	      dr_node_kind_str(g->kind), 
	      g->start - start_clock, 
	      g->end - start_clock, 
	      g->est, 
	      g->t_1, g->t_inf,
	      g->n_nodes, g->n_edges, g->worker, g->cpu,
	      g);
    } else {
      fprintf(wp, "%s(@%p\n", dr_node_kind_str(g->kind), g);
    }
    int n = dr_dag_node_list_size(&g->subgraphs);
    for (i = 0; i < n; i++) {
      dr_dag_node_kind_t k = g->subgraphs.a[i].kind;
      if (K == dr_dag_node_kind_section) {
	if (i == 0) {
	  assert(k == dr_dag_node_kind_task_group);
	} else if (g->done && i == n - 1) {
	  assert(k == dr_dag_node_kind_wait_tasks);
	} else {
	  assert(k == dr_dag_node_kind_create_task 
		 || k == dr_dag_node_kind_section);
	}
      } else {
	assert(K == dr_dag_node_kind_task);
	if (g->done && i == n - 1) {
	  assert(k == dr_dag_node_kind_end_task);
	} else {
	  assert(k == dr_dag_node_kind_section);
	}
      }
      dr_print_dag(&g->subgraphs.a[i], start_clock, wp, indent + 1);
    }
    for (i = 0; i < indent; i++) fprintf(wp, " ");
    fprintf(wp, ")\n");
  }
}

/* --------------------- gen_dot functions ------------------- */

static dr_dag_node * 
dr_first_interval(dr_dag_node * t) {
  dr_dag_node * s = t;
  while (s->kind >= dr_dag_node_kind_section) {
    int n = dr_dag_node_list_size(&s->subgraphs);
    if (n == 0) {
      assert(GS.opts.collapse);
      assert(s->collapsed);
      break;
    }
    s = &s->subgraphs.a[0];
  }
  return s;
}

static dr_dag_node * 
dr_last_interval(dr_dag_node * t) {
  dr_dag_node * s = t;
  while (s->kind >= dr_dag_node_kind_section) {
    int n = dr_dag_node_list_size(&s->subgraphs);
    if (n == 0) {
      assert(GS.opts.collapse);
      assert(s->collapsed);
      break;
    }
    s = &s->subgraphs.a[n - 1];
  }
  return s;
}

static void
dr_gen_dot_edge_i_i(dr_dag_node * A, dr_dag_node * B, FILE * wp) {
  fprintf(wp, "T%lu -> T%lu;\n", (unsigned long)A, (unsigned long)B);
}

/* A is a section; generate edges from each of create_task's
   last node in S to B. */
static void 
dr_gen_dot_edges_s_i(dr_dag_node * A, dr_dag_node * B, FILE * wp) {
  assert(A->kind >= dr_dag_node_kind_section);
  int n = dr_dag_node_list_size(&A->subgraphs);
  int i;
  for (i = 0; i < n; i++) {
    dr_dag_node * c = &A->subgraphs.a[i];
    if (c->kind == dr_dag_node_kind_create_task) {
      dr_gen_dot_edge_i_i(dr_last_interval(c->child), B, wp);
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
    int n = dr_dag_node_list_size(&g->subgraphs);
    int i;
    for (i = 0; i < n; i++) {
      dr_dag_node_kind_t k = g->subgraphs.a[i].kind;
      /* the entire if expression is just for sanity check */
      if (K == dr_dag_node_kind_section) {
	if (i == 0) {
	  assert(k == dr_dag_node_kind_task_group);
	} else if (g->done && i == n - 1) {
	  assert(k == dr_dag_node_kind_wait_tasks);
	} else {
	  assert(k == dr_dag_node_kind_create_task 
		 || k == dr_dag_node_kind_section);
	}
      } else {
	assert(K == dr_dag_node_kind_task);
	if (g->done && i == n - 1) {
	  assert(k == dr_dag_node_kind_end_task);
	} else {
	  assert(k == dr_dag_node_kind_section);
	}
      }
      /* here is the real body */
      if (i > 0) { 
	dr_gen_dot_edge_i_i(dr_last_interval(&g->subgraphs.a[i - 1]), 
			    dr_first_interval(&g->subgraphs.a[i]), wp);
	if (g->subgraphs.a[i - 1].kind == dr_dag_node_kind_section) {
	  dr_gen_dot_edges_s_i(&g->subgraphs.a[i - 1], 
			       dr_first_interval(&g->subgraphs.a[i]), wp);
	}
      }
      dr_gen_dot_dag(&g->subgraphs.a[i], start_clock, wp);
    }
  }
}

void dr_print_task_graph(const char * filename) {
  if (GS.root) {
    FILE * wp = (filename ? fopen(filename, "wb") : stdout);
    if (!wp) { perror("fopen"); exit(1); }
    dr_print_dag(GS.root, GS.start_clock, stdout, 0);
  } else {
    printf("no task graph to print!\n");
  }
}

void dr_gen_dot_task_graph(const char * filename) {
  if (GS.root) {
    FILE * wp = (filename ? fopen(filename, "wb") : stdout);
    if (!wp) { perror("fopen"); exit(1); }
    fprintf(wp, "digraph G {\n");
    dr_gen_dot_dag(GS.root, GS.start_clock, wp);
    fprintf(wp, "}\n");
    fclose(wp);
  } else {
    printf("no task graph to dot!\n");
  }
}

void dr_options_default(dr_options * opts) {
  opts->dbg_level = 0;
  opts->collapse = 1;
}

void dr_start(dr_options * opts) {
  dr_options opts_[1];
  if (!opts) {
    opts = opts_;
    dr_options_default(opts);
  }
  TS = (dr_thread_specific_state *)
    dr_malloc(sizeof(dr_thread_specific_state) * dr_get_num_workers());
  GS.opts = *opts;
  GS.start_clock = dr_get_tsc();
  dr_start_task(0);
  GS.root = dr_get_cur_task();
}

void dr_stop() {
  dr_end_task();
}


