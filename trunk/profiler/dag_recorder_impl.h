/* 
 * dag_recorder_impl.h
 */

#pragma once

#include <dag_recorder.h>

/* a node of the dag, position independent */
struct dr_pi_dag_node {
  /* misc. information about this node */
  dr_dag_node_info info;
  /* two indexes in the edges array, pointing to 
     the begining and the end of edges from this node */
  long edges_begin;	 
  long edges_end;
  union {
    /* valid when this node is a create node.
       index of its child */
    long child_offset;
    /* valid when this node is a section or task node
       begin/end indexes of its subgraphs */
    struct {
      long subgraphs_begin_offset;
      long subgraphs_end_offset;
    };
  };
};

typedef struct dr_pi_dag_edge {
  dr_dag_edge_kind_t kind;
  long u;
  long v;
} dr_pi_dag_edge;

/* 
   n
   sz
   I ------+
   S ------+----+
   I[0] <--+    |
   I[1]         |
   ...          |
   I[n-1]       |
   ...  <-------+
   
 */

typedef struct {
  long n;			/* number of strings */
  long sz;			/* total bytes including headers */
  long * I;			/* index I[0] .. I[n-1] */
  const char * C;		/* char array */
} dr_pi_string_table;

/* the toplevel structure of a position-independent dag.
   when dumped into a file, we make sure each integer 
   fields occupies 8 bytes */
typedef struct dr_pi_dag {
  long n;			/* length of T */
  long m;			/* length of E */
  long num_workers;		/* number of workers */
  dr_pi_dag_node * T;		/* all nodes in a contiguous array */
  dr_pi_dag_edge * E;		/* all edges in a contiguous array */
  dr_pi_string_table * S;
} dr_pi_dag;

typedef enum {
  dr_event_kind_ready,
  dr_event_kind_start,
  dr_event_kind_last_start,
  dr_event_kind_end,
} dr_event_kind_t;

typedef struct {
  dr_clock_t t;			/* time at which it happened */
  dr_event_kind_t kind;		/* ready, start, or end */
  dr_pi_dag_node * u;		/* the node of the event */
  dr_pi_dag_node * pred;	/* the last predecessor */
  dr_dag_edge_kind_t edge_kind;	/* kind of edges from pred -> this node */
} dr_event;

dr_event 
dr_mk_event(dr_clock_t t, 
	    dr_event_kind_t kind, 
	    dr_pi_dag_node * u,
	    dr_pi_dag_node * pred,
	    dr_dag_edge_kind_t edge_kind);

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

int dr_gen_dot(dr_pi_dag * G);
int dr_gen_gpl(dr_pi_dag * G);
int dr_gen_basic_stat(dr_pi_dag * G);

