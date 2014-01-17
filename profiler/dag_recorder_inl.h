/*
 * dag recorder 2.0
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

/* 
     task ::= section* end
     section ::= task_group (section|create)* wait 

 */

#if !defined(DAG_RECORDER_VERBOSE_LEVEL)
#define DAG_RECORDER_VERBOSE_LEVEL GS.opts.verbose_level
#endif

#if !defined(DAG_RECORDER_DBG_LEVEL)
#define DAG_RECORDER_DBG_LEVEL GS.opts.dbg_level
#endif

#if !defined(DAG_RECORDER_CHK_LEVEL)
#define DAG_RECORDER_CHK_LEVEL GS.opts.chk_level
#endif

#if !defined(DAG_RECORDER_RECORD_CPU)
#define DAG_RECORDER_RECORD_CPU 0
#endif

#if !defined(DAG_RECORDER_RECORD_POS)
#define DAG_RECORDER_RECORD_POS 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /* a kind of nodes.
     in many places the code assumes
     k < dr_dag_node_kind_section
     iff k is a primitive node */
  typedef enum {
    dr_dag_node_kind_create_task,
    dr_dag_node_kind_wait_tasks,
    dr_dag_node_kind_end_task,
    dr_dag_node_kind_section,
    dr_dag_node_kind_task,
  } dr_dag_node_kind_t;
  
  /* the state of a node.
     a node is either 
     (i) not ready,
     (ii) ready but not running, 
     (iii) running, or 
     (iv) finished
     we do not explicitly count (i) or (iv).
     ready tasks (category (ii)) are further
     classified by the event that made them
     ready.
     
     create : a task created a task and the
     node is the first node the create task
     create_cont : a task created a task and the
     node is the first in the parent task after 
     create_task
     end : a task issued a wait_task at one
     point, when one of the waited children
     has not yet finished. later the last
     child finished and the task is now ready.
     the node is the first node after wait_tasks
     wait_cont : a task issued a wait_task,
     when all waited children have finished.
     the node is the first node after wait_tasks
  */
  typedef enum {
    dr_dag_edge_kind_end,    /* end -> parent */
    dr_dag_edge_kind_create, /* create -> child */
    dr_dag_edge_kind_create_cont, /* create -> next */
    dr_dag_edge_kind_wait_cont,	/* wait -> next */
    dr_dag_edge_kind_max,
  } dr_dag_edge_kind_t;

  typedef struct dr_dag_node_list dr_dag_node_list;
  typedef struct dr_dag_node_chunk dr_dag_node_chunk;

  /* list of dag nodes */
  struct dr_dag_node_list {
    dr_dag_node_chunk * head;
    dr_dag_node_chunk * tail;
  };

  typedef struct {
    /* pointer to filename. valid in dr_dag_node */
    const char * file;
    /* index in dr_flat_string_table. valid in dr_pi_dag_node */    
    long file_idx;
    /* line number */
    long line;
  } code_pos;

  typedef struct {
    dr_clock_t t;		/* clock */
    code_pos pos;		/* code position */
  } dr_clock_pos;

  typedef struct dr_dag_node_info {
    dr_clock_pos start; /* start clock,position */
    dr_clock_pos end;	 /* end clock,position */
    dr_clock_t est;	 /* earliest start time */
    dr_clock_t t_1;	 /* work */
    dr_clock_t t_inf;	 /* critical path */
    /* number of nodes in this subgraph */
    long node_counts[dr_dag_node_kind_section];
    /* number of edges connecting nodes in this subgraph */
    long edge_counts[dr_dag_edge_kind_max];
    /* direct children of create_task type */
    long n_child_create_tasks;
    int worker;
    int cpu;
    dr_dag_node_kind_t kind;
    dr_dag_node_kind_t last_node_kind;
  } dr_dag_node_info;

  typedef struct dr_pi_dag_node dr_pi_dag_node;

  /* a node of the in-memory, growing/shrinking dag */
  /* size = 128 bytes? */
  struct dr_dag_node {
    dr_dag_node_info info;
    /* a pointer used to recursively
       convert the graph into the 
       position independent format */
    dr_pi_dag_node * forward;
    /* todo: done and collapsed are actually redundant.
       done is used only for sanity checks
       collapsed <-> subgraphs are empty
    */
    union {
      dr_dag_node * child;		/* kind == create_task */
      struct {				/* kind == section/task */
	dr_dag_node_list subgraphs[1];
	union {
	  dr_dag_node * parent_section; /* kind == section */
	  dr_dag_node * active_section; /* kind == task */
	};
      };
    };
  };

  /* list of dr_dag_node, used to dynamically
     grow the subgraphs of section/task */
  enum { dr_dag_node_chunk_sz = 3 };
  /* 16 + 128 * 7 */
  typedef struct dr_dag_node_chunk {
    struct dr_dag_node_chunk * next;
    int n;
    dr_dag_node a[dr_dag_node_chunk_sz];
  } dr_dag_node_chunk;

  typedef struct dr_thread_specific_state {
    union {
      struct {
	dr_dag_node * task;		/* current task */
	//dr_dag_node_chunk_freelist freelist[1];
	dr_dag_node_list freelist[1];
	/* only used in Cilk: it holds a pointer to 
	   the interval that just created a task */
	dr_dag_node * parent;
      };
      char minimum_size[64];
    };
  } dr_thread_specific_state;

  typedef struct dr_global_state {
    int initialized;
    int num_workers;
    /* root of the task graph. 
       used (only) by print_task_graph */
    dr_dag_node * root;
    /* the clock when dr_start() was called */
    dr_clock_t start_clock;
    dr_thread_specific_state * thread_specific; /* allocate at init */
    dr_thread_specific_state * ts; /* null when not profiling */
    dr_options opts;
  } dr_global_state;

  static int dr_check_(int condition, const char * condition_s, 
		       const char * __file__, int __line__, 
		       const char * func) {
    if (!condition) {
      fprintf(stderr, "%s:%d:%s: dag recorder check failed : %s\n", 
	      __file__, __line__, func, condition_s); 
      exit(1);
    }
    return 1;
  }

#define dr_check(x) (!DAG_RECORDER_CHK_LEVEL || dr_check_(((x)?1:0), #x, __FILE__, __LINE__, __func__))

  extern dr_global_state GS;

  /* malloc and free */

  static void * 
  dr_malloc(size_t sz) {
    void * a = malloc(sz);
    if (DAG_RECORDER_CHK_LEVEL) {
      if (!a) { perror("malloc"); exit(1); }
    }
    if (DAG_RECORDER_VERBOSE_LEVEL>=3) {
      printf("dr_malloc(%ld) -> %p\n", sz, a);
    }
    if (DAG_RECORDER_DBG_LEVEL>=2) {
      memset(a, 255, sz);
    }
    return a;
  }

  static void
  dr_free(void * a, size_t sz) {
    if (DAG_RECORDER_VERBOSE_LEVEL>=3) {
      printf("dr_free(%p, %ld)\n", a, sz);
    }
    if (DAG_RECORDER_CHK_LEVEL) {
      if (a) {
	if (DAG_RECORDER_DBG_LEVEL>=1) memset(a, 222, sz);
	free(a);
      } else {
	(void)dr_check(sz == 0);
      }
    } else {
      free(a);
    }
  }

  static dr_dag_node_chunk *
  dr_dag_node_chunk_alloc(dr_dag_node_list * fl) {
    dr_dag_node_chunk * head = fl->head;
    if (head) {
      (void)dr_check(fl->tail);
      dr_dag_node_chunk * next = head->next;
      fl->head = next;
      if (!next) fl->tail = 0;
      if (DAG_RECORDER_VERBOSE_LEVEL>=3) {
	printf("dr_dag_node_chunk_alloc(%p) -> %p\n", 
	       fl, head);
      }
      return head;
    } else {
      head = (dr_dag_node_chunk *)dr_malloc(sizeof(dr_dag_node_chunk));
      if (DAG_RECORDER_VERBOSE_LEVEL>=3) {
	printf("dr_dag_node_chunk_alloc(%p) -> %p (via malloc)\n", 
	       fl, head);
      }
      (void)dr_check(!fl->tail);
      return head;
    }
  }

  /* chunk */

  static dr_dag_node * 
  dr_dag_node_chunk_last(dr_dag_node_chunk * ch) {
    (void)dr_check(ch->n > 0);
    return &ch->a[ch->n - 1];
  }

  static dr_dag_node * 
  dr_dag_node_chunk_first(dr_dag_node_chunk * ch) {
    (void)dr_check(ch->n > 0);
    return &ch->a[0];
  }

  static dr_dag_node * 
  dr_dag_node_chunk_push_back(dr_dag_node_chunk * ch) {
    int n = ch->n;
    (void)dr_check(n < dr_dag_node_chunk_sz);
    ch->n = n + 1;
    return &ch->a[n];
  }

  /* list */

  static void 
  dr_dag_node_list_init(dr_dag_node_list * l) {
    l->tail = l->head = 0;
  }

  static int
  dr_dag_node_list_empty(dr_dag_node_list * l) {
    if (l->head) {
      (void)dr_check(l->tail);
      return 0;
    } else {
      (void)dr_check(!l->tail);
      return 1;
    }
  }

  static dr_dag_node *
  dr_dag_node_list_first(dr_dag_node_list * l) {
    return dr_dag_node_chunk_first(l->head);
  }

  static dr_dag_node *
  dr_dag_node_list_last(dr_dag_node_list * l) {
    return dr_dag_node_chunk_last(l->tail);
  }

  /* pop a chunk from fl, append it at the end of l */
  static dr_dag_node_chunk *
  dr_dag_node_list_add_chunk(dr_dag_node_list * l,
			     dr_dag_node_list * fl) {
    dr_dag_node_chunk * ch 
      = dr_dag_node_chunk_alloc(fl);
    ch->next = 0;
    ch->n = 0;
    if (l->tail) {
      l->tail->next = ch;
    } else {
      (void)dr_check(!l->head);
      l->head = ch;
    }
    l->tail = ch;
    return ch;
  }

  /* extend l by one element and return a point
     to the new element */
  static dr_dag_node *
  dr_dag_node_list_push_back(dr_dag_node_list * l,
			     dr_dag_node_list * fl) {
    dr_dag_node_chunk * tail = l->tail;
    if (!tail || tail->n == dr_dag_node_chunk_sz) {
      tail = dr_dag_node_list_add_chunk(l, fl);
    }
    (void)dr_check(tail->n < dr_dag_node_chunk_sz);
    return dr_dag_node_chunk_push_back(tail);
  }

  /* put everything in l at the head of fl */
  static void 
  dr_dag_node_list_clear(dr_dag_node_list * l, 
			 dr_dag_node_list * fl) {
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
  }

#if defined(__x86_64__)

  static unsigned long long dr_rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc;shlq $32,%%rdx;orq %%rdx,%%rax":"=a"(u)::"%rdx");
    return u;
  }
  
#elif defined(__sparc__) && defined(__arch64__)
  
  static unsigned long long dr_rdtsc(void) {
    unsigned long long u;
    asm volatile("rd %%tick, %0" : "=r" (u));
    return u;
  }

#else
  
  static unsigned long long dr_rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc" : "=A" (u));
    return u;
  }
  
#endif

  static dr_clock_t 
  dr_get_tsc() {
    return dr_rdtsc();
  }

  /* a hopefully portable way to get a unique worker id.
     you can roll your own 
     dr_get_worker and dr_get_max_workers.
     this one is used as the last resort */

  typedef struct {
    pthread_key_t worker_key;
    volatile int worker_key_state;
    int worker_counter;
  } dr_get_worker_key_struct;
  
  extern dr_get_worker_key_struct dr_gwks;
  
  static inline pthread_key_t dr_get_worker_key() {
    if (dr_gwks.worker_key_state == 2) return dr_gwks.worker_key;
    if (__sync_bool_compare_and_swap(&dr_gwks.worker_key_state, 0, 1)) {
      pthread_key_create(&dr_gwks.worker_key, NULL);
      dr_gwks.worker_key_state = 2;
    } else {
      while (dr_gwks.worker_key_state < 2) ;
    }
    return dr_gwks.worker_key;
  }
  
  static int worker_counter_get_next() {
    return __sync_fetch_and_add(&dr_gwks.worker_counter, 1);
  }
  
  static inline int dr_get_worker_by_pthread_key() {
    pthread_key_t wk = dr_get_worker_key();
    void * x = pthread_getspecific(wk);
    if (x) {
      int w = (long)x - 1;
      return w;
    } else {
      int c = worker_counter_get_next();
      pthread_setspecific(wk, (void *)((long)c + 1));
      int w = c;
      return w;
    }
  }

  static int 
  dr_get_num_workers_env(const char * v) {
    char * s = getenv(v);
    int x;
    if (!s) {
      fprintf(stderr, 
	      "error: could not get number of workers\n"
	      "set environment variable %s\n", v);
      return 0;
    }
    x = atoi(s);
    if (x <= 0) {
      fprintf(stderr, 
	      "error: invalid value in environment varible %s (%s)\n"
	      "set a positive integer\n", v, s);
      return 0;
    }
    return x;
  } 

  static inline int dr_tbb_num_workers() {
    return dr_get_num_workers_env("TBB_NTHREADS");
  }

  static inline int dr_nanox_num_workers() {
    return dr_get_num_workers_env("NX_PES");
  }

  static dr_dag_node * 
  dr_get_cur_task_(int worker) {
    return GS.ts[worker].task;
  }

  static void 
  dr_set_cur_task_(int worker, dr_dag_node * t) {
    GS.ts[worker].task = t;
  }

  /* initialize dag node n to become a section- or a task-type node */
  static void 
  dr_dag_node_init_section_or_task(dr_dag_node * n,
				   dr_dag_node_kind_t kind,
				   dr_dag_node * p) {
    (void)dr_check(kind >= dr_dag_node_kind_section);
    n->info.kind = kind;
    dr_dag_node_list_init(n->subgraphs);

    if (kind == dr_dag_node_kind_section) {
      n->parent_section = p;
    } else {
      (void)dr_check(kind == dr_dag_node_kind_task);
      n->active_section = 0;
    }
  }

  static dr_dag_node * 
  dr_task_active_node(dr_dag_node * t);

  /* add a new section as a child of s (either a section or task) */
  static dr_dag_node *
  dr_push_back_section(dr_dag_node * t, dr_dag_node * s, 
		       dr_dag_node_list * fl) {
    if (dr_check(s->info.kind >= dr_dag_node_kind_section)) {
      dr_dag_node * new_s 
	= dr_dag_node_list_push_back(s->subgraphs, fl);
      dr_dag_node_init_section_or_task(new_s, dr_dag_node_kind_section, s);
      t->active_section = new_s;
      (void)dr_check(dr_task_active_node(t) == t->active_section);
      return new_s;
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* allocate a new dag node of a task type */
  static dr_dag_node * 
  dr_mk_dag_node_task() {
    dr_dag_node * t = (dr_dag_node *)dr_malloc(sizeof(dr_dag_node));
    dr_dag_node_init_section_or_task(t, dr_dag_node_kind_task, 0);
    t->active_section = t;
    return t;
  }

#if __cplusplus 
  extern "C" {
#endif
    int sched_getcpu();
#if __cplusplus 
  }
#endif

  static int dr_getcpu() {
#if DAG_RECORDER_RECORD_CPU
    return sched_getcpu();
#else
    return 0;
#endif
  }

  static inline dr_clock_t
  dr_max_clock(dr_clock_t x, dr_clock_t y) {
    return (x < y ? y : x);
  }

  static inline int
  dr_meet_ints(int x, int y) {
    return (x == y ? x : -1);
  }

  /* end an interval, 
     called by start_{task_group,create_task,wait_tasks} */
  static void 
  dr_end_interval_(dr_dag_node * dn, int worker, 
		   dr_dag_node_kind_t kind,
		   dr_clock_t end_t, 
		   const char * file, int line,
		   dr_clock_t est, 
		   dr_clock_pos start) {
    int k;
    (void)dr_check(kind < dr_dag_node_kind_section);
    dn->info.start = start;
    dn->info.kind = kind;
    dn->info.last_node_kind = kind;
    dn->info.est = est;
    dn->info.n_child_create_tasks = 0;
    dn->info.t_inf = dn->info.t_1 = end_t - start.t;
    dn->info.end.t = end_t;
    dn->info.end.pos.file = file;
    dn->info.end.pos.line = line;
    for (k = 0; k < dr_dag_node_kind_section; k++) {
      dn->info.node_counts[k] = 0;
    }
    dn->info.node_counts[kind] = 1;
    for (k = 0; k < dr_dag_edge_kind_max; k++) {
      dn->info.edge_counts[k] = 0;
    }
    dn->info.worker = worker;
    dn->info.cpu = dr_getcpu();
  }

  /* auxiliary functions that modify or query task and section */

  /* 

     return the current (unfinished) section of section s.  in other
     words, it returns the section to which a new element should added
     when the program calls create_task or task_group next time.

     section ::= task_group (section|create)* wait 
  */


  /* it returns a dag node to which a new interval 
     should added when the program performs an action
     that needs one.
     starting from t, it descends the rightmost child,
     until it finds that a node's rightmost child is
     not an unfinished section.
  */

  static dr_dag_node * 
  dr_task_active_node(dr_dag_node * t) {
    return t->active_section;
  }

  static dr_dag_node * 
  dr_task_last_section_or_create(dr_dag_node * t) {
    if (dr_check(t->info.kind == dr_dag_node_kind_task)) {
      dr_dag_node * s = dr_task_active_node(t);
      dr_dag_node * i = dr_dag_node_list_last(s->subgraphs);
      (void)dr_check(i->info.kind == dr_dag_node_kind_section
		     || i->info.kind == dr_dag_node_kind_create_task);
      return i;
    } else {
      return 0;
    }
  }

  static dr_dag_node * 
  dr_task_ensure_section(dr_dag_node * t, dr_dag_node_list * fl) {
    dr_dag_node * s = dr_task_active_node(t);
    if (s == t) {
      (void)dr_check(s->info.kind == dr_dag_node_kind_task);
      s = dr_push_back_section(t, s, fl);
    }
    (void)dr_check(s->info.kind == dr_dag_node_kind_section);
    return s;
  }

  /* ------------- main instrumentation functions ------------- 
     start_task
     start_task_group
     end_task_group
     start_create_task
     end_create_task
     start_wait_tasks
     end_wait_tasks
     end_task
  */


  /* called when we start a task */

  static void
  dr_set_start_info(dr_clock_pos * p, const char * file, int line) {
    p->pos.file = file;
    p->pos.line = line;
    p->t = dr_get_tsc();
  }

  /* 
     task    ::= section* end 

     the interval that is just beginning may 
     be a task_group or end_task.
     we don't know which until end of this
     interval.
     at this point we just record the information
     about the opening interval in t
  */

  static_if_inline void 
  dr_start_task__(dr_dag_node * p, 
		  const char * file, int line,
		  int worker) {
    if (GS.ts) {
      /* make a task, section, and interval */
      dr_dag_node * nt = dr_mk_dag_node_task();
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_start_task(parent=%p) by %d new task=%p\n", 
	       p, worker, nt);
      }
      /* register this task as the child of p */
      if (p) {
	(void)dr_check(p->info.kind == dr_dag_node_kind_create_task);
	(void)dr_check(p->child == 0);
	p->child = nt;
	nt->info.est = p->info.est + p->info.t_inf;
      } else {
	nt->info.est = 0;
      }
      /* set current task */
      dr_set_cur_task_(worker, nt);
      /* record info on the point of start */
      dr_set_start_info(&nt->info.start, file, line);
    }
  }
  
  static_if_inline int 
  dr_start_cilk_proc__(const char * file, int line,
		       int worker) {
    if (GS.ts) {
      dr_start_task__(GS.ts[worker].parent, file, line, worker);
    }
    return 0;
  }

  static_if_inline void
  dr_begin_section__(int worker) {
    if (GS.ts) {
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_active_node(t);
      dr_dag_node * new_s = dr_push_back_section(t, s, GS.ts[worker].freelist);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_begin_section() by %d task=%p,"
	       " parent section=%p, new section = %p\n", 
	       worker, t, s, new_s);
      }
    }
  }


  /* end current interval and start create_task 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static_if_inline dr_dag_node * 
  dr_enter_create_task__(dr_dag_node ** c, 
			 const char * file, int line,
			 int worker) {
    if (GS.ts) {
      dr_clock_t end_t = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      /* ensure t has a session */
      dr_dag_node * s = dr_task_ensure_section(t, GS.ts[worker].freelist);
      /* add a new node to s */
      dr_dag_node * ct 
	= dr_dag_node_list_push_back(s->subgraphs, GS.ts[worker].freelist);
      ct->info.kind = dr_dag_node_kind_create_task;
      ct->child = 0;
      // dr_dag_node * ct = dr_task_add_create(t);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_enter_create_task() by %d task=%p, section=%p, new interval=%p\n", 
	       worker, t, s, ct);
      }
      dr_end_interval_(ct, worker, dr_dag_node_kind_create_task,
		       end_t, file, line,
		       t->info.est, t->info.start);
      *c = ct;
      return t;
    } else {
      return (dr_dag_node *)0;
    }
  }

  static_if_inline dr_dag_node *
  dr_enter_create_cilk_proc_task__(const char * file, int line,
				   int worker) {
    if (GS.ts) {
      return dr_enter_create_task__(&GS.ts[worker].parent, 
				    file, line, worker);
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* resume from create_task 
     section ::= task_group (section|create)* wait
  */
  static_if_inline void 
  dr_return_from_create_task__(dr_dag_node * t, 
			       const char * file, int line,
			       int worker) {
    if (GS.ts) {
      dr_dag_node * ct = dr_task_last_section_or_create(t);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_return_from_create_task(task=%p) by %d interval=%p\n", 
	       t, worker, ct);
      }
      (void)dr_check(ct->info.kind == dr_dag_node_kind_create_task);
      dr_set_cur_task_(worker, t);
      t->info.est = ct->info.est + ct->info.t_inf;
      dr_set_start_info(&t->info.start, file, line);
    }
  }

  /* end current interval and start wait_tasks 
     section ::= task_group (section|create)* wait
  */
  static_if_inline dr_dag_node *
  dr_enter_wait_tasks__(const char * file, int line, int worker) {
    if (GS.ts) {
      dr_clock_t end_t = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_ensure_section(t, GS.ts[worker].freelist);
      dr_dag_node * i 
	= dr_dag_node_list_push_back(s->subgraphs, GS.ts[worker].freelist);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_enter_wait_tasks() by %d task=%p, "
	       "section=%p, new interval=%p\n", 
	       worker, t, s, i);
      }
      t->active_section = s->parent_section;
      (void)dr_check(dr_task_active_node(t) == t->active_section);
      dr_end_interval_(i, worker, dr_dag_node_kind_wait_tasks,
		       end_t, file, line, 
		       t->info.est, t->info.start);
      return t;
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* look at subgraphs of s.
     if it is collapsable, collapse it */
  static void 
  dr_summarize_section_or_task(dr_dag_node * s, dr_dag_node_list * fl) {
    if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
      if (s->info.kind == dr_dag_node_kind_section) {
	printf("dr_summarize_section(section=%p)\n", s);
      } else if (s->info.kind == dr_dag_node_kind_task) {
	printf("dr_summarize_task(task=%p)\n", s);
      }
    }
    if (dr_check(s->info.kind >= dr_dag_node_kind_section)
	&& dr_check(!dr_dag_node_list_empty(s->subgraphs))) {
      dr_dag_node * first = dr_dag_node_list_first(s->subgraphs);
      dr_dag_node * last = dr_dag_node_list_last(s->subgraphs);
      int i;
      /* initialize the result */
      s->info.start   = first->info.start;
      s->info.end     = last->info.end;
      s->info.worker  = first->info.worker;
      s->info.cpu     = first->info.cpu;
      s->info.est     = first->info.est;
      s->info.last_node_kind = last->info.last_node_kind;
      s->info.t_1     = 0;
      s->info.t_inf   = 0;
      for (i = 0; i < dr_dag_node_kind_section; i++) {
	s->info.node_counts[i] = 0;
      }
      for (i = 0; i < dr_dag_edge_kind_max; i++) {
	s->info.edge_counts[i] = 0;
      }
      s->info.n_child_create_tasks = 0;

      {
	/* look through all chunks */
	dr_clock_t t_inf = 0;
	dr_dag_node_chunk * head = s->subgraphs->head;
	dr_dag_node_chunk * ch;
	for (ch = head; ch; ch = ch->next) {
	  int i;
	  for (i = 0; i < ch->n; i++) {
	    dr_dag_node * x = &ch->a[i];
	    int k;
	    /* accumulate t_1 */
	    s->info.t_1     += x->info.t_1;
	    /* accumulate t_inf along the sequential chain */
	    s->info.t_inf   += x->info.t_inf;
	    /* meet workers and cpus */
	    s->info.worker = dr_meet_ints(s->info.worker, x->info.worker);
	    s->info.cpu = dr_meet_ints(s->info.cpu, x->info.cpu);
	    /* accumulate node counts of each type */
	    for (k = 0; k < dr_dag_node_kind_section; k++) {
	      s->info.node_counts[k] += x->info.node_counts[k];
	    }
	    /* accumulate edge counts of each type */
	    for (k = 0; k < dr_dag_edge_kind_max; k++) {
	      s->info.edge_counts[k] += x->info.edge_counts[k];
	    }
	    switch (x->info.kind) {
	    case dr_dag_node_kind_create_task: {
	      /* besides, we need to count edges
		 from x to its successor */
	      (void)dr_check(ch->next || i < ch->n - 1);
	      s->info.edge_counts[dr_dag_edge_kind_create]++;
	      s->info.edge_counts[dr_dag_edge_kind_create_cont]++;
	      s->info.n_child_create_tasks++;
	      /* similar accumulation for x's child task */
	      dr_dag_node * y = x->child;
	      (void)dr_check(y);
	      s->info.end.t = dr_max_clock(y->info.end.t, s->info.end.t);
	      s->info.last_node_kind 
		= (y->info.end.t > s->info.end.t
		   ? y->info.last_node_kind : s->info.last_node_kind);
	      s->info.t_1     += y->info.t_1;
	      t_inf = dr_max_clock(s->info.t_inf + y->info.t_inf, t_inf);
	      s->info.worker = dr_meet_ints(s->info.worker, y->info.worker);
	      s->info.cpu = dr_meet_ints(s->info.cpu, y->info.cpu);

	      for (k = 0; k < dr_dag_node_kind_section; k++) {
		s->info.node_counts[k] += y->info.node_counts[k];
	      }
	      for (k = 0; k < dr_dag_edge_kind_max; k++) {
		s->info.edge_counts[k] += y->info.edge_counts[k];
	      }
	      break;
	    }
	    case dr_dag_node_kind_wait_tasks: 
	    case dr_dag_node_kind_end_task: {
	      (void)dr_check(!ch->next && i == ch->n - 1);
	      break;
	    }
	    case dr_dag_node_kind_section:
	      if (ch->next || i < ch->n - 1) {
		s->info.edge_counts[dr_dag_edge_kind_wait_cont]++;
		s->info.edge_counts[dr_dag_edge_kind_end] 
		  += x->info.n_child_create_tasks;
	      }
	      break;
	    default:
	      (void)dr_check(0);
	      break;
	    }
	  }
	}
	s->info.t_inf = dr_max_clock(t_inf, s->info.t_inf);
      }
      /* now check if we can collapse it */
      /* for now, we collapse it if it 
	 was executed on a single worker
	 and it isn't too large */
      if (s->info.worker != -1
	  && s->info.end.t - s->info.start.t < GS.opts.collapse_max) {
	/* free the graph of its children */
	dr_dag_node_chunk * head = s->subgraphs->head;
	dr_dag_node_chunk * ch;
	for (ch = head; ch; ch = ch->next) {
	  int i;
	  for (i = 0; i < ch->n; i++) {
	    dr_dag_node * x = &ch->a[i];
	    if (x->info.kind == dr_dag_node_kind_create_task) {
	      /* children must have been collapsed */
	      dr_dag_node * c = x->child;
	      (void)dr_check(c);
	      (void)dr_check(c->info.kind == dr_dag_node_kind_task);
	      (void)dr_check(c->subgraphs);
	      /* free subgraphs */
	      (void)dr_check(dr_dag_node_list_empty(c->subgraphs));
	      dr_free(c, sizeof(dr_dag_node));
	    } else if (x->info.kind == dr_dag_node_kind_section) {
	      (void)dr_check(x->subgraphs);
	      /* free subgraphs */
	      (void)dr_check(dr_dag_node_list_empty(x->subgraphs));
	    }
	  }
	}
	dr_dag_node_list_clear(s->subgraphs, fl);
      }
    }
  }


  /* resume from wait_tasks 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static_if_inline void 
  dr_return_from_wait_tasks__(dr_dag_node * t, 
			      const char * file, int line,
			      int worker) {
    if (GS.ts) {
      /* get the section that finished last */
      dr_dag_node * s = dr_task_last_section_or_create(t);
      if (dr_check(s->info.kind == dr_dag_node_kind_section)) {
	dr_dag_node * p = dr_dag_node_list_last(s->subgraphs);
	(void)dr_check(p->info.kind == dr_dag_node_kind_wait_tasks);
	if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	  printf("dr_return_from_wait_tasks(task=%p)"
		 " by %d section=%p, pred=%p\n", 
		 t, worker, s, p);
	}
	{
	  /* calc EST of the interval to start */
	  dr_clock_t est = p->info.est + p->info.t_inf;
	  dr_dag_node_chunk * head = s->subgraphs->head;
	  dr_dag_node_chunk * ch;
	  for (ch = head; ch; ch = ch->next) {
	    int i;
	    for (i = 0; i < ch->n; i++) {
	      dr_dag_node * sc = &ch->a[i];
	      (void)dr_check(sc->info.kind < dr_dag_node_kind_task); 
	      if (sc->info.kind == dr_dag_node_kind_create_task) {
		dr_dag_node * ct = sc->child;
		if (dr_check(ct)) {
		  dr_clock_t x = ct->info.est + ct->info.t_inf;
		  if (est < x) est = x;
		}
	      }
	    }
	  }
	  dr_summarize_section_or_task(s, GS.ts[worker].freelist);
	  dr_set_cur_task_(worker, t);
	  t->info.est = est;
	  dr_set_start_info(&t->info.start, file, line);
	}
      }
    }
  }

  static_if_inline void 
  dr_end_task__(const char * file, int line, int worker) {
    if (GS.ts) {
      dr_clock_t end_t = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_active_node(t);
      dr_dag_node * i 
	= dr_dag_node_list_push_back(s->subgraphs, GS.ts[worker].freelist);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_end_task() by %d task=%p, section=%p, "
	       "new interval=%p\n", 
	       worker, t, s, i);
      }
      dr_end_interval_(i, worker, dr_dag_node_kind_end_task,
		       end_t, file, line, 
		       t->info.est, t->info.start);
      dr_summarize_section_or_task(t, GS.ts[worker].freelist);
    }
  }

  /* dummy function to supress many
     "static function defined but not called" 
     errors */
#if ! __CILK__
  __attribute__((unused)) 
#endif
  static void dr_dummy_call_static_functions() {
    dr_dag_node * t;
    dr_dag_node * c;
    dr_start__(NULL, "", 1, 1, 1);
    dr_begin_section__(1);
    t = dr_enter_create_task__(&c, "", 1, 1);
    dr_enter_create_cilk_proc_task__("", 1, 1);
    dr_start_task__(c, "", 1, 1);
    dr_start_cilk_proc__("", 1, 1);
    dr_end_task__("", 1, 1);
    dr_return_from_create_task__(t, "", 1, 1);
    t = dr_enter_wait_tasks__("", 1, 1);
    dr_return_from_wait_tasks__(t, "", 1, 1);
    dr_stop__("", 1, 1);
    dr_free(t, sizeof(dr_dag_node));
  }

#ifdef __cplusplus
}
#endif

