/* 
 * gen_gpl.c
 */

#include <errno.h>
#include <string.h>
#include "dag_recorder_impl.h"

/* 
   this file calculates a time series
   of the number of running and ready
   tasks and generates a gnuplot file.
 */

typedef enum {
  dr_node_state_running,
  dr_node_state_ready_end,
  dr_node_state_ready_create,
  dr_node_state_ready_create_cont,
  dr_node_state_ready_wait_cont,
  dr_node_state_max,
} dr_node_state_t;


/* an entry in the parallelism profile. */
typedef struct {
  dr_clock_t t;			/* time */
  int a[dr_node_state_max];	/* count of nodes in each state */
} dr_para_prof_history_entry;

typedef struct {
  /* this must be the first element */
  void (*process_event)(chronological_traverser * ct, dr_event evt);
  int counts[dr_node_state_max];
  dr_clock_t last_time;
  dr_para_prof_history_entry * history;
  long hist_sz;
  long n_hists;
} dr_para_prof;

static void 
dr_para_prof_process_event(chronological_traverser * pp, dr_event evt);

static void 
dr_para_prof_init(dr_para_prof * pp, long hist_sz, dr_clock_t last_time) {
  int i;
  for (i = 0; i < dr_node_state_max; i++) {
    pp->counts[i] = 0;
  }
  pp->process_event = dr_para_prof_process_event;
  pp->last_time = last_time;
  pp->hist_sz = hist_sz;
  pp->n_hists = 0;
  pp->history = (dr_para_prof_history_entry *)
    dr_malloc(sizeof(dr_para_prof_history_entry) * hist_sz);
}

static void 
dr_para_prof_add_hist(dr_para_prof * pp, dr_clock_t t) {
  long sz = pp->hist_sz;
  long n = pp->n_hists;
  dr_para_prof_history_entry * hist = pp->history;
  //long last_t = (n ? hist[n-1].t : 0);
  // (last_t + (t - last_t) * (sz - n) >= pp->last_time)

  if (t / (double)pp->last_time > n / (double)sz) {
    int i;
    dr_para_prof_history_entry * h = &hist[n];
    assert(n < sz);
    h->t = t;
    for (i = 0; i < dr_node_state_max; i++) {
      h->a[i] = pp->counts[i];
    }
    pp->n_hists = n + 1;
  }
}

static void
dr_para_prof_check(dr_para_prof * pp) {
  int i;
  for (i = 0; i < dr_node_state_max; i++) {
    (void)dr_check(pp->counts[i] == 0);
  }
}

static void 
dr_para_prof_write_to_file(dr_para_prof * pp, FILE * wp) {
  long n = pp->n_hists;
  dr_para_prof_history_entry * h = pp->history;
  long i;
  int k;
  /* 
     dr_node_state_running,
     dr_node_state_ready_end,
     dr_node_state_ready_create,
     dr_node_state_ready_create_cont,
     dr_node_state_ready_wait_cont,

     the following must match the definition
     of dr_node_state_t
  */

  fprintf(wp,
	  "plot"
	  "  \"-\" u 1:2:3 w filledcurves title \"running\""
	  ", \"-\" u 1:2:3 w filledcurves title \"end\""
	  ", \"-\" u 1:2:3 w filledcurves title \"create\""
	  ", \"-\" u 1:2:3 w filledcurves title \"create cont\""
	  ", \"-\" u 1:2:3 w filledcurves title \"wait cont\""
	  "\n"
	  );
  for (k = 0; k < dr_node_state_max; k++) {
    for (i = 0; i < n; i++) {
      /* we transit from
	 (t_prev, y_prev) -> (t, y), we draw
	 the following *******

	         y           *-----+
		             *
		 y0   +*******
                      |      
                      |
                     t0      t
      */
      int j;
      int y_prev = 0;
      int y = 0;

      if (i == 0) {
	fprintf(wp, "%d %d %d\n", 0, 0, 0);
	fprintf(wp, "%llu %d %d\n", h[0].t, 0, 0);
      } else {
	for (j = 0; j < k; j++) {
	  y_prev += h[i-1].a[j];
	}
	fprintf(wp, "%llu %d %d\n", 
		h[i].t, y_prev, y_prev + h[i-1].a[k]);
      }
      for (j = 0; j < k; j++) {
	y += h[i].a[j];
      }
      fprintf(wp, "%llu %d %d\n", h[i].t, y, y + h[i].a[k]);
    }
    fprintf(wp, "e\n");
  }
  fprintf(wp, "pause -1\n");
}

static dr_node_state_t 
calc_node_state(dr_pi_dag_node * pred, dr_dag_edge_kind_t ek) {
  /* no predecessor. 
     i.e. root node. treat as if it is created */
  if (!pred) 
    return dr_node_state_ready_create;
  switch (ek) {
  case dr_dag_edge_kind_end:
    return dr_node_state_ready_end;
  case dr_dag_edge_kind_create:
    return dr_node_state_ready_create;
  case dr_dag_edge_kind_create_cont:
    return dr_node_state_ready_create_cont;
  case dr_dag_edge_kind_wait_cont:
    return dr_node_state_ready_wait_cont;
  default:
    (void)dr_check(0);
  }
  (void)dr_check(0);
  return dr_node_state_running;
}

static void 
dr_para_prof_show_counts(dr_para_prof * pp) {
  int k;
  printf(" node states:");
  for (k = 0; k < dr_node_state_max; k++) {
    printf(" %d", pp->counts[k]);
  }  
  printf("\n");
}

static void 
dr_para_prof_process_event(chronological_traverser * pp_, dr_event ev) {
  dr_para_prof * pp = (dr_para_prof *)pp_;
  switch (ev.kind) {
  case dr_event_kind_ready: {
    dr_node_state_t k = calc_node_state(ev.pred, ev.edge_kind);
    pp->counts[k]++;
    break;
  }
  case dr_event_kind_start: {
    dr_node_state_t k = calc_node_state(ev.pred, ev.edge_kind);
    pp->counts[k]--;
    pp->counts[dr_node_state_running]++;
    break;
  }
  case dr_event_kind_end: {
    pp->counts[dr_node_state_running]--;
    break;
  }
  default:
    assert(0);
    break;
  }
  if (GS.opts.dbg_level>=2) {
    dr_para_prof_show_counts(pp);
  }
  dr_para_prof_add_hist(pp, ev.t);
}

int 
dr_gen_gpl(dr_pi_dag * G) {
  FILE * wp = NULL;
  int must_close = 0;
  const char * filename = GS.opts.gpl_file;
  if (filename && strcmp(filename, "") != 0) {
    if (strcmp(filename, "-") == 0) {
      fprintf(stderr, "writing gnuplot to stdout\n");
      wp = stdout;
    } else {
      fprintf(stderr, "writing gnuplot to %s\n", filename);
      wp = fopen(filename, "wb");
      if (!wp) { 
	fprintf(stderr, "fopen: %s (%s)\n", strerror(errno), filename); 
	return 0;
      }
      must_close = 1;
    }
  } else {
    fprintf(stderr, "not writing gnuplot\n");
    return 1;
  }
  dr_pi_dag_node * last = dr_pi_dag_node_last(G->T, G);
  dr_para_prof pp[1];
  dr_para_prof_init(pp, GS.opts.gpl_sz, last->info.end.t);
  dr_pi_dag_chronological_traverse(G, (chronological_traverser *)pp);
  dr_para_prof_check(pp);
  dr_para_prof_write_to_file(pp, wp);
  if (must_close) fclose(wp);
  return 1;
}

