/* 
 * gen_basic.c
 */

#include <errno.h>
#include "dag_recorder_impl.h"

typedef struct {
  void (*process_event)(chronological_traverser * ct, dr_event evt);
  dr_pi_dag * G;
  dr_clock_t cum_running;
  dr_clock_t cum_delay;
  dr_clock_t cum_no_work;
  dr_clock_t t;
  long n_running;
  long n_ready;
  long n_workers;
  dr_clock_t total_elapsed;
  dr_clock_t total_t_1;
  long * edge_counts;		/* kind,u,v */
} dr_basic_stat;

static void 
dr_basic_stat_process_event(chronological_traverser * ct, 
			    dr_event evt);

static void
dr_calc_inner_delay(dr_basic_stat * bs, dr_pi_dag * G) {
  long n = G->n;
  long i;
  dr_clock_t total_elapsed = 0;
  dr_clock_t total_t_1 = 0;
  dr_pi_dag_node * T = G->T;
  for (i = 0; i < n; i++) {
    dr_pi_dag_node * t = &T[i];
    if (t->info.kind < dr_dag_node_kind_section
	|| t->subgraphs_begin_offset == t->subgraphs_end_offset) {
      total_elapsed += t->info.end.t - t->info.start.t;
      total_t_1 += t->info.t_1;
      if (total_elapsed < total_t_1) {
	fprintf(stderr,
		"warning: node %ld has negative"
		" inner delay (start=%llu, end=%llu,"
		" t_1=%llu, end - start - t_1 = -%llu\n",
		i,
		t->info.start.t, t->info.end.t, t->info.t_1,
		total_t_1 - total_elapsed);
      }
    }
  }
  bs->total_elapsed = total_elapsed;
  bs->total_t_1 = total_t_1;
}

static void
dr_calc_edges(dr_basic_stat * bs, dr_pi_dag * G) {
  long n = G->n;
  long m = G->m;
  long nw = G->num_workers;
  long * C_ = (long *)dr_malloc(sizeof(long) * dr_dag_edge_kind_max * nw * nw);
#define EDGE_COUNTS(k,i,j) C_[k*nw*nw+i*nw+j]
  dr_dag_edge_kind_t k;
  long i, j;
  for (k = 0; k < dr_dag_edge_kind_max; k++) {
    for (i = 0; i < nw; i++) {
      for (j = 0; j < nw; j++) {
	EDGE_COUNTS(k,i,j) = 0;
      }
    }
  }
  for (i = 0; i < n; i++) {
    dr_pi_dag_node * t = &G->T[i];
    if (t->info.kind >= dr_dag_node_kind_section
	&& t->subgraphs_begin_offset == t->subgraphs_end_offset) {
      for (k = 0; k < dr_dag_edge_kind_max; k++) {
	int w = t->info.worker;
	if (w == -1) {
	  fprintf(stderr, 
		  "warning: %ld (k=%d w=%d)\n",
		  i, t->info.kind, w);
	} else {
	  (void)dr_check(w >= 0);
	  (void)dr_check(w < nw);
	  EDGE_COUNTS(k, w, w) += t->info.edge_counts[k];
	}
      }
    }    
  }
  for (i = 0; i < m; i++) {
    dr_pi_dag_edge * e = &G->E[i];
    int uw = G->T[e->u].info.worker;
    int vw = G->T[e->v].info.worker;
    if (uw == -1 || vw == -1) {
      fprintf(stderr, "warning: %ld (k=%d w=%d) -> %ld (k=%d w=%d)\n",
	      e->u, G->T[e->u].info.kind, uw, 
	      e->v, G->T[e->v].info.kind, vw);
    } else {
      (void)dr_check(uw >= 0);
      (void)dr_check(uw < nw);
      (void)dr_check(vw >= 0);
      (void)dr_check(vw < nw);
      EDGE_COUNTS(e->kind, uw, vw)++;
    }
  }
#undef EDGE_COUNTS
  bs->edge_counts = C_;
}


static void 
dr_basic_stat_init(dr_basic_stat * bs, dr_pi_dag * G) {
  bs->process_event = dr_basic_stat_process_event;
  bs->G = G;
  bs->n_running = 0;
  bs->n_ready = 0;
  bs->n_workers = G->num_workers;
  bs->cum_running = 0;		/* cumulative running cpu time */
  bs->cum_delay = 0;		/* cumulative delay cpu time */
  bs->cum_no_work = 0;		/* cumulative no_work cpu time */
  bs->t = 0;			/* time of the last event */
}

static void 
dr_basic_stat_process_event(chronological_traverser * ct, 
			    dr_event evt) {
  dr_basic_stat * bs = (dr_basic_stat *)ct;
  dr_clock_t dt = evt.t - bs->t;

  int n_running = bs->n_running;
  int n_delay, n_no_work;
  if (bs->n_running >= bs->n_workers) {
    /* great, all workers are running */
    n_delay = 0;
    n_no_work = 0;
    if (bs->n_running > bs->n_workers) {
      fprintf(stderr, 
	      "warning: n_running = %ld"
	      " > n_workers = %ld (clock skew?)\n",
	      bs->n_running, bs->n_workers);
    }
    n_delay = 0;
    n_no_work = 0;
  } else if (bs->n_running + bs->n_ready <= bs->n_workers) {
    /* there were enough workers to run ALL ready tasks */
    n_delay = bs->n_ready;
    n_no_work = bs->n_workers - (bs->n_running + bs->n_ready);
  } else {
    n_delay = bs->n_workers - bs->n_running;
    n_no_work = 0;
  }
  bs->cum_running += n_running * dt;
  bs->cum_delay   += n_delay * dt;
  bs->cum_no_work += n_no_work * dt;

  switch (evt.kind) {
  case dr_event_kind_ready: {
    bs->n_ready++;
    break;
  }
  case dr_event_kind_start: {
    bs->n_ready--;
    bs->n_running++;
    break;
  }
  case dr_event_kind_end: {
    bs->n_running--;
    break;
  }
  default:
    assert(0);
    break;
  }

  bs->t = evt.t;
}

static void
dr_write_edge_counts(dr_basic_stat * bs, FILE * wp) {
  dr_dag_edge_kind_t k;
  int i, j;
  int nw = bs->n_workers;
  long * C_ = bs->edge_counts;
#define EDGE_COUNTS(k,i,j) C_[k*nw*nw+i*nw+j]
  for (k = 0; k < dr_dag_edge_kind_max; k++) {
    switch (k) {
    case dr_dag_edge_kind_end:
      fprintf(wp, "end-parent edges:\n");
      break;
    case dr_dag_edge_kind_create:
      fprintf(wp, "create-child edges:\n");
      break;
    case dr_dag_edge_kind_create_cont:
      fprintf(wp, "create-cont edges:\n");
      break;
    case dr_dag_edge_kind_wait_cont:
      fprintf(wp, "wait-cont edges:\n");
      break;
    default:
      (void)dr_check(0);
      break;
    }
    for (i = 0; i < nw; i++) {
      for (j = 0; j < nw; j++) {
	long c = EDGE_COUNTS(k,i,j);
	fprintf(wp, " %ld", c);
      }
      fprintf(wp, "\n");
    }
  }
}


static void
dr_basic_stat_write_to_file(dr_basic_stat * bs, FILE * wp) {
  dr_pi_dag * G = bs->G;
  long * nc = G->T[0].info.node_counts;
  long n_tasks = nc[dr_dag_node_kind_create_task];
  long n_waits = nc[dr_dag_node_kind_wait_tasks];
  long n_ends = nc[dr_dag_node_kind_end_task];
  long n_nodes = n_tasks + n_waits + n_ends;
  dr_clock_t t_inf = G->T[0].info.t_inf;
  dr_clock_t work = bs->total_t_1;
  dr_clock_t delay = bs->cum_delay + (bs->total_elapsed - bs->total_t_1);
  dr_clock_t no_work = bs->cum_no_work;
  double nw = bs->n_workers;

  assert(work == G->T[0].info.t_1);

  fprintf(wp, "create_task           = %ld\n", n_tasks);
  fprintf(wp, "wait_tasks            = %ld\n", n_waits);
  fprintf(wp, "end_task              = %ld\n", n_ends);
  //fprintf(wp, "local edges           = %ld\n", bs->n_local_edges);
  //fprintf(wp, "remote edges          = %ld\n", bs->n_remote_edges);
  fprintf(wp, "work (T1)             = %llu\n", work);
  fprintf(wp, "delay                 = %llu\n", delay);
  fprintf(wp, "no_work               = %llu\n", no_work);
  fprintf(wp, "critical_path (T_inf) = %llu\n", t_inf);
  fprintf(wp, "n_workers (P)         = %ld\n", bs->n_workers);
  fprintf(wp, "elapsed               = %llu\n", bs->t);

  if (work + delay + no_work != bs->n_workers * bs->t) {
    fprintf(wp, 
	    "*** warning (running + delay + no_work)"
	    " / n_workers = %llu != elapsed_time\n", 
	    (work + delay + no_work) / bs->n_workers);
  }
  fprintf(wp, "T1/P                  = %.3f\n", 
	  work/nw);
  fprintf(wp, "T1/P+T_inf            = %.3f\n", 
	  work/nw + t_inf);
  fprintf(wp, "T1/T_inf              = %.3f\n", 
	  work/(double)t_inf);
  fprintf(wp, "greedy speedup        = %.3f\n", 
	  work/(work/nw + t_inf));
  fprintf(wp, "observed speedup      = %.3f\n", 
	  work/(double)bs->t);
  fprintf(wp, "observed/greedy       = %.3f\n", 
	  (work/nw + t_inf)/bs->t);
  fprintf(wp, "task granularity      = %.3f\n", 
	  bs->cum_running/(double)n_tasks);
  fprintf(wp, "node granularity      = %.3f\n", 
	  bs->cum_running/(double)n_nodes);
  dr_write_edge_counts(bs, wp);
}

int 
dr_gen_basic_stat(dr_pi_dag * G) {
  FILE * wp = NULL;
  int must_close = 0;
  const char * filename = GS.opts.stat_file;
  if (filename && strcmp(filename, "") != 0) {
    if (strcmp(filename, "-") == 0) {
      fprintf(stderr, "writing stat to stdout\n");
      wp = stdout;
    } else {
      fprintf(stderr, "writing stat to %s\n", filename);
      wp = fopen(filename, "wb");
      if (!wp) { 
	fprintf(stderr, "fopen: %s (%s)\n", strerror(errno), filename); 
	return 0;
      }
      must_close = 1;
    }
  } else {
    fprintf(stderr, "not writing stat\n");
    return 1;
  }
  dr_basic_stat bs[1];
  dr_basic_stat_init(bs, G);
  dr_calc_inner_delay(bs, G);
  dr_calc_edges(bs, G);
  dr_pi_dag_chronological_traverse(G, (chronological_traverser *)bs);
  dr_basic_stat_write_to_file(bs, wp);
  if (must_close) fclose(wp);
  return 1;
}


