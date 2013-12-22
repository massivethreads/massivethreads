/* 
 * dag_recorder_impl.h
 */

#pragma once

#include <dag_recorder.h>

/* a node of the dag, position independent */
struct dr_pi_dag_node {
  dr_dag_node_info info;
  long edges_begin;
  long edges_end;
  union {
    long child_offset;
    struct {
      long subgraphs_begin_offset;
      long subgraphs_end_offset;
    };
  };
};

typedef enum {
  dr_edge_kind_cont,
  dr_edge_kind_create,
  dr_edge_kind_end,
} dr_edge_kind_t;

typedef struct dr_pi_dag_edge {
  dr_edge_kind_t kind;
  long u;
  long v;
} dr_pi_dag_edge;

typedef struct dr_pi_dag {
  long n;			/* length of n */
  dr_pi_dag_node * T;		/* all nodes */
  long m;
  dr_pi_dag_edge * E;
} dr_pi_dag;

typedef enum {
  dr_event_kind_ready,
  dr_event_kind_start,
  dr_event_kind_end,
} dr_event_kind_t;

typedef struct {
  dr_clock_t t;			/* time at which it happened */
  dr_event_kind_t kind;		/* ready, start, or end */
  dr_pi_dag_node * u;		/* the node of the event */
  dr_pi_dag_node * pred;	/* the last predecessor */
  dr_edge_kind_t edge_kind;	/* kind of edges from pred -> this node */
} dr_event;

dr_event 
dr_mk_event(dr_clock_t t, 
	    dr_event_kind_t kind, 
	    dr_pi_dag_node * u,
	    dr_pi_dag_node * pred,
	    dr_edge_kind_t edge_kind);

typedef struct chronological_traverser chronological_traverser;
struct chronological_traverser {
  void (*process_event)(chronological_traverser * pp, dr_event evt);
};

void 
dr_pi_dag_chronological_traverse(dr_pi_dag * G,
				 chronological_traverser * ct);

dr_pi_dag_node *
dr_pi_dag_node_first(dr_pi_dag_node * g, dr_pi_dag * G);


dr_pi_dag_node *
dr_pi_dag_node_last(dr_pi_dag_node * g, dr_pi_dag * G);
