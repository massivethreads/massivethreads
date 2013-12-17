/*
 * dag recorder 2.0
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifdef __cplusplus
extern "C" {
#endif

  /* a kind of nodes */
  typedef enum {
    dr_dag_node_kind_create_task,
    dr_dag_node_kind_wait_tasks,
    dr_dag_node_kind_end_task,
    dr_dag_node_kind_section,
    dr_dag_node_kind_task
  } dr_dag_node_kind_t;
  
  typedef struct dr_dag_node_list dr_dag_node_list;

  typedef unsigned long long dr_clock_t;

  /* a node of the dag */
  struct dr_dag_node {
    dr_dag_node_kind_t kind;
    dr_clock_t start; 
    dr_clock_t est;
    dr_clock_t end; 
    dr_clock_t t_1;
    dr_clock_t t_inf;
    long n_nodes; 
    long n_edges; 
    int worker;
    int cpu;
    union {
      dr_dag_node * child;		/* kind == create_task */
      struct {
	dr_dag_node_list * subgraphs;	/* kind == section/task */
	int done;
	int collapsed;
      };
    };
  };

  enum { dr_dag_node_chunk_sz = 5 };
  typedef struct dr_dag_node_chunk {
    struct dr_dag_node_chunk * next;
    dr_dag_node a[dr_dag_node_chunk_sz];
    int n;
  } dr_dag_node_chunk;

  struct dr_dag_node_list {
    dr_dag_node_chunk * tail;
    dr_dag_node_chunk head[1];
  };

  typedef struct dr_thread_specific_state {
    union {
      struct {
	dr_dag_node * task;		/* current task */
	/* only used in Cilk: it holds a pointer to 
	   the interval that just created a task */
	dr_dag_node * parent;
      };
      char minimum_size[64];
    };
  } dr_thread_specific_state;

  typedef struct dr_global_state {
    /* root of the task graph. 
       used (only) by print_task_graph */
    dr_dag_node * root;
    /* the clock when dr_start() was called */
    dr_clock_t start_clock;
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

  extern dr_thread_specific_state * TS;
  extern dr_global_state GS;

  /* malloc and free */

  static void * 
  dr_malloc(size_t sz) {
    void * a = malloc(sz);
    if (!a) { perror("malloc"); exit(1); }
    return a;
  }

  static void
  dr_free(void * a, size_t sz) {
    if (a) {
      if (DAG_RECORDER_DBG_LEVEL>=1) memset(a, 222, sz);
      free(a);
    } else {
      (void)dr_check(sz == 0);
    }
  }

  /* chunk */

  static inline void
  dr_dag_node_chunk_init(dr_dag_node_chunk * ch) {
    ch->next = 0;
    ch->n = 0;
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_last(dr_dag_node_chunk * ch) {
    (void)dr_check(ch->n > 0);
    return &ch->a[ch->n - 1];
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_first(dr_dag_node_chunk * ch) {
    (void)dr_check(ch->n > 0);
    return &ch->a[0];
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_push_back(dr_dag_node_chunk * ch) {
    int n = ch->n;
    (void)dr_check(n < dr_dag_node_chunk_sz);
    ch->n = n + 1;
    return &ch->a[n];
  }

  /* list */

  static inline void 
  dr_dag_node_list_init(dr_dag_node_list * l) {
    dr_dag_node_chunk_init(l->head);
    l->tail = l->head;
  }

  static inline dr_dag_node_list * 
  dr_mk_dag_node_list() {
    dr_dag_node_list * l 
      = (dr_dag_node_list *)dr_malloc(sizeof(dr_dag_node_list));
    dr_dag_node_list_init(l);
    return l;
  }

  static inline int
  dr_dag_node_list_empty(dr_dag_node_list * l) {
    if (l->head != l->tail) return 0;
    if (l->head->n) return 0;
    return 1;
  }

  static inline dr_dag_node *
  dr_dag_node_list_first(dr_dag_node_list * l) {
    return dr_dag_node_chunk_first(l->head);
  }

  static inline dr_dag_node *
  dr_dag_node_list_last(dr_dag_node_list * l) {
    return dr_dag_node_chunk_last(l->tail);
  }

  static inline dr_dag_node_chunk *
  dr_dag_node_list_add_chunk(dr_dag_node_list * l) {
    dr_dag_node_chunk * ch 
      = (dr_dag_node_chunk *)dr_malloc(sizeof(dr_dag_node_chunk));
    ch->next = 0;
    ch->n = 0;
    l->tail->next = ch;
    l->tail = ch;
    return ch;
  }

  static inline dr_dag_node *
  dr_dag_node_list_push_back(dr_dag_node_list * l) {
    dr_dag_node_chunk * tail = l->tail;
    if (tail->n == dr_dag_node_chunk_sz) {
      tail = dr_dag_node_list_add_chunk(l);
    }
    (void)dr_check(tail->n < dr_dag_node_chunk_sz);
    return dr_dag_node_chunk_push_back(tail);
  }

  static inline void 
  dr_dag_node_list_clear(dr_dag_node_list * l) {
    dr_dag_node_chunk * ch;
    dr_dag_node_chunk * next;
    for (ch = l->head->next; ch; ch = next) {
      next = ch->next;
      dr_free(ch, sizeof(dr_dag_node_chunk));
    }
    dr_dag_node_list_init(l);
  }

  static inline void 
  dr_dag_node_list_destroy(dr_dag_node_list * l) {
    dr_dag_node_list_clear(l);
    dr_free(l, sizeof(dr_dag_node_list));
  }

#if defined(__x86_64__)

  static inline unsigned long long dr_rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc;shlq $32,%%rdx;orq %%rdx,%%rax":"=a"(u)::"%rdx");
    return u;
  }
  
#elif defined(__sparc__) && defined(__arch64__)
  
  static inline unsigned long long dr_rdtsc(void) {
    unsigned long long u;
    asm volatile("rd %%tick, %0" : "=r" (u));
    return u;
  }

#else
  
  static inline unsigned long long dr_rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc" : "=A" (u));
    return u;
  }
  
#endif

  static inline dr_clock_t 
  dr_get_tsc() {
    return dr_rdtsc();
  }

  static inline dr_dag_node * 
  dr_get_cur_task_(int worker) {
    return TS[worker].task;
  }

  static inline void 
  dr_set_cur_task_(int worker, dr_dag_node * t) {
    TS[worker].task = t;
  }

  /* initialize dag node n to become a section- or a task-type node */
  static inline void 
  dr_dag_node_init_section_or_task(dr_dag_node * n,
				   dr_dag_node_kind_t kind) {
    (void)dr_check(kind >= dr_dag_node_kind_section);
    n->kind = kind;
    n->done = 0;
    n->collapsed = 0;
    n->subgraphs = dr_mk_dag_node_list();
  }

  /* add a new section as a child of s (either a section or task) */
  static inline dr_dag_node *
  dr_push_back_section(dr_dag_node * g) {
    if (dr_check(g->kind >= dr_dag_node_kind_section)) {
      dr_dag_node * s = dr_dag_node_list_push_back(g->subgraphs);
      dr_dag_node_init_section_or_task(s, dr_dag_node_kind_section);
      return s;
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* allocate a new dag node of a task type */
  static inline dr_dag_node * 
  dr_mk_dag_node_task() {
    dr_dag_node * t = (dr_dag_node *)dr_malloc(sizeof(dr_dag_node));
    dr_dag_node_init_section_or_task(t, dr_dag_node_kind_task);
    return t;
  }

  /* end an interval, 
     called by start_{task_group,create_task,wait_tasks} */
  static inline void 
  dr_end_interval_(dr_dag_node * dn, int worker, dr_clock_t start, 
		   dr_clock_t est, dr_clock_t end, 
		   dr_dag_node_kind_t kind) {
    (void)dr_check(kind < dr_dag_node_kind_section);
    dn->kind = kind;
    dn->start = start;
    dn->est = est;
    dn->n_nodes = 1;
    dn->n_edges = 0;
    dn->worker = worker;
    dn->cpu = sched_getcpu();
    dn->end = end;
    dn->t_inf = dn->t_1 = dn->end - dn->start;
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

  static inline dr_dag_node * 
  dr_task_active_node(dr_dag_node * t) {
    dr_dag_node * s = t;
    (void)dr_check(t->kind == dr_dag_node_kind_task);
    while (1) {
      if (dr_dag_node_list_empty(s->subgraphs)) {
	/* no subsection in s 
	   -> no active sections under s
	   -> return s */
	return s;
      } else {
	/* look at the rightmost child */
	dr_dag_node * c = dr_dag_node_list_last(s->subgraphs);
	/* the rightmost child is a leaf
	   -> it means it has finished
	   -> no active sections under s */
	if (c->kind < dr_dag_node_kind_section) return s;
	/* something is broken if we have a task node 
	   as a child! */
	(void)dr_check(c->kind == dr_dag_node_kind_section);
	/* the rightmost child is a section that has finished 
	   -> no active sections under s */
	if (c->done) return s;
	/* the rightmost child is an unfinished section.
	   -> descend */
	s = c;
      }
    }
    /* never reach. suppress cilkc warning */
    (void)dr_check(0);
    return 0;
  }

  static inline dr_dag_node * 
  dr_task_last_section_or_create(dr_dag_node * t) {
    if (dr_check(t->kind == dr_dag_node_kind_task)) {
      dr_dag_node * s = dr_task_active_node(t);
      dr_dag_node * i = dr_dag_node_list_last(s->subgraphs);
      (void)dr_check(i->kind == dr_dag_node_kind_section
		     || i->kind == dr_dag_node_kind_create_task);
      return i;
    } else {
      return 0;
    }
  }

  static inline dr_dag_node * 
  dr_task_ensure_session(dr_dag_node * t) {
    dr_dag_node * s = dr_task_active_node(t);
    if (s->kind == dr_dag_node_kind_task) {
      s = dr_push_back_section(s);
    }
    (void)dr_check(s->kind == dr_dag_node_kind_section);
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

  /* 
     task    ::= section* end 

     the interval that is just beginning may 
     be a task_group or end_task.
     we don't know which until end of this
     interval.
     at this point we just record the information
     about the opening interval in t
  */

  static void 
  dr_start_task_(dr_dag_node * p, int worker) {
    if (TS) {
      /* make a task, section, and interval */
      dr_dag_node * nt = dr_mk_dag_node_task();
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_start_task(parent=%p) by %d new task=%p\n", 
	       p, worker, nt);
      }
      /* register this task as the child of p */
      if (p) {
	(void)dr_check(p->kind == dr_dag_node_kind_create_task);
	(void)dr_check(p->child == 0);
	p->child = nt;
	nt->est = p->est + p->t_inf;
      } else {
	nt->est = 0;
      }
      /* set current * */
      dr_set_cur_task_(worker, nt);
      nt->start = dr_get_tsc();
    }
  }
  
  static int 
  dr_start_cilk_proc_(int worker) {
    if (TS) {
      dr_start_task_(TS[worker].parent, worker);
    }
    return 0;
  }

  static void
  dr_begin_section_(int worker) {
    if (TS) {
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_active_node(t);
      dr_push_back_section(s);
    }
  }


  /* end current interval and start create_task 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static dr_dag_node * 
  dr_enter_create_task_(dr_dag_node ** c, int worker) {
    if (TS) {
      dr_clock_t end = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      /* ensure t has a session */
      dr_dag_node * s = dr_task_ensure_session(t);
      /* add a new node to s */
      dr_dag_node * ct = dr_dag_node_list_push_back(s->subgraphs);
      ct->kind = dr_dag_node_kind_create_task;
      ct->child = 0;
      // dr_dag_node * ct = dr_task_add_create(t);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_enter_create_task() by %d task=%p, new interval=%p\n", 
	       worker, t, ct);
      }
      dr_end_interval_(ct, worker, t->start, t->est, end, 
		       dr_dag_node_kind_create_task);
      *c = ct;
      return t;
    } else {
      return (dr_dag_node *)0;
    }
  }

  static dr_dag_node *
  dr_enter_create_cilk_proc_task_(int worker) {
    if (TS) {
      return dr_enter_create_task_(&TS[worker].parent, worker);
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* resume from create_task 
     section ::= task_group (section|create)* wait
  */
  static void 
  dr_return_from_create_task_(dr_dag_node * t, int worker) {
    if (TS) {
      dr_dag_node * ct = dr_task_last_section_or_create(t);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_return_from_create_task(task=%p) by %d interval=%p\n", 
	       t, worker, ct);
      }
      (void)dr_check(ct->kind == dr_dag_node_kind_create_task);
      dr_set_cur_task_(worker, t);
      t->est = ct->est + ct->t_inf;
      t->start = dr_get_tsc();
    }
  }

  /* end current interval and start wait_tasks 
     section ::= task_group (section|create)* wait
  */
  static dr_dag_node *
  dr_enter_wait_tasks_(int worker) {
    if (TS) {
      dr_clock_t end = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_ensure_session(t);
      dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_enter_wait_tasks() by %d task=%p, "
	       "section=%p, new interval=%p\n", 
	       worker, t, s, i);
      }
      (void)dr_check(s->done == 0);
      s->done = 1;
      dr_end_interval_(i, worker, t->start, t->est, end, 
		       dr_dag_node_kind_wait_tasks);
      return t;
    } else {
      return (dr_dag_node *)0;
    }
  }

  /* look at subgraphs of s.
     if it is collapsable, collapse it */
  static void 
  dr_collapse_section_or_task(dr_dag_node * s) {
    if (dr_check(s->kind >= dr_dag_node_kind_section)
	&& dr_check(!dr_dag_node_list_empty(s->subgraphs))) {
      dr_dag_node * first = dr_dag_node_list_first(s->subgraphs);
      dr_dag_node * last = dr_dag_node_list_last(s->subgraphs);
      /* initialize the result */
      s->start   = first->start;
      s->est     = first->est;
      s->end     = last->end;
      s->t_1     = 0;
      s->t_inf   = 0;
      s->n_nodes = 0;
      s->n_edges = 0;
      s->worker  = first->worker;
      s->cpu     = first->cpu;

      {
	/* look through all chunks */
	dr_clock_t t_inf = 0;
	dr_dag_node_chunk * head = s->subgraphs->head;
	dr_dag_node_chunk * ch;
	for (ch = head; ch; ch = ch->next) {
	  int i;
	  for (i = 0; i < ch->n; i++) {
	    dr_dag_node * x = &ch->a[i];
	    s->t_1     += x->t_1;
	    s->t_inf   += x->t_inf;
	    s->n_nodes += x->n_nodes;
	    s->n_edges += x->n_edges + ((ch != head || i) ? 1 : 0);
	    if (s->worker != x->worker) s->worker = -1;
	    if (s->cpu    != x->cpu)    s->cpu    = -1;
	    if (x->kind == dr_dag_node_kind_create_task) {
	      dr_dag_node * y = x->child;
	      (void)dr_check(y);
	      if (y->end > s->end) s->end = y->end;
	      s->t_1     += y->t_1;
	      if (s->t_inf + y->t_inf > t_inf) 
		t_inf = s->t_inf + y->t_inf;
	      s->n_nodes += y->n_nodes;
	      // dr_check((ch != head) || i);
	      s->n_edges += y->n_edges + 2;
	      if (s->worker != y->worker) s->worker = -1;
	      if (s->cpu    != y->cpu)    s->cpu    = -1;
	    }
	  }
	  if (t_inf > s->t_inf) s->t_inf = t_inf;
	}
      }
      /* now check if we can collapse it */
      if (s->worker != -1) {
	/* free the graph of its children */
	dr_dag_node_chunk * head = s->subgraphs->head;
	dr_dag_node_chunk * ch;
	/* for now, we collapse it if it 
	   was executed on a single worker */
	s->collapsed = 1;
	for (ch = head; ch; ch = ch->next) {
	  int i;
	  for (i = 0; i < ch->n; i++) {
	    dr_dag_node * x = &ch->a[i];
	    if (x->kind == dr_dag_node_kind_create_task) {
	      /* children must have been collapsed */
	      dr_dag_node * c = x->child;
	      (void)dr_check(c);
	      (void)dr_check(c->kind == dr_dag_node_kind_task);
	      (void)dr_check(c->subgraphs);
	      (void)dr_check(dr_dag_node_list_empty(c->subgraphs));
	      dr_free(c->subgraphs, sizeof(dr_dag_node_list));
	      dr_free(c, sizeof(dr_dag_node));
	    } else if (x->kind == dr_dag_node_kind_section) {
	      (void)dr_check(x->subgraphs);
	      (void)dr_check(dr_dag_node_list_empty(x->subgraphs));
	      dr_free(x->subgraphs, sizeof(dr_dag_node_list));
	    }
	  }
	}
	dr_dag_node_list_clear(s->subgraphs);
      }
    }
  }

  /* resume from wait_tasks 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static void 
  dr_return_from_wait_tasks_(dr_dag_node * t, int worker) {
    if (TS) {
      /* get the section that finished last */
      dr_dag_node * s = dr_task_last_section_or_create(t);
      if (dr_check(s->kind == dr_dag_node_kind_section)) {
	dr_dag_node * p = dr_dag_node_list_last(s->subgraphs);
	(void)dr_check(p->kind == dr_dag_node_kind_wait_tasks);
	if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	  printf("dr_return_from_wait_tasks(task=%p) by %d section=%p, pred=%p\n", 
		 t, worker, s, p);
	}
	{
	  /* calc EST of the interval to start */
	  dr_clock_t est = p->est + p->t_inf;
	  dr_dag_node_chunk * head = s->subgraphs->head;
	  dr_dag_node_chunk * ch;
	  for (ch = head; ch; ch = ch->next) {
	    int i;
	    for (i = 0; i < ch->n; i++) {
	      dr_dag_node * sc = &ch->a[i];
	      (void)dr_check(sc->kind < dr_dag_node_kind_task); 
	      if (sc->kind == dr_dag_node_kind_create_task) {
		dr_dag_node * ct = sc->child;
		if (dr_check(ct)) {
		  dr_clock_t x = ct->est + ct->t_inf;
		  if (est < x) est = x;
		}
	      }
	    }
	  }
	  if (GS.opts.collapse) dr_collapse_section_or_task(s);
	  dr_set_cur_task_(worker, t);
	  t->est = est;
	  t->start = dr_get_tsc();
	}
      }
    }
  }

  static void 
  dr_end_task_(int worker) {
    if (TS) {
      dr_clock_t end = dr_get_tsc();
      dr_dag_node * t = dr_get_cur_task_(worker);
      dr_dag_node * s = dr_task_active_node(t);
      dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
      if (DAG_RECORDER_VERBOSE_LEVEL>=2) {
	printf("dr_end_task() by %d task=%p, section=%p, "
	       "new interval=%p\n", 
	       worker, t, s, i);
      }
      dr_end_interval_(i, worker, t->start, t->est, end, 
		       dr_dag_node_kind_end_task);
      (void)dr_check(t->done == 0);
      t->done = 1;
      if (GS.opts.collapse) dr_collapse_section_or_task(t);
    }
  }

  /*  */



  void dr_free_dag_recursively(dr_dag_node * g);
  void dr_print_task_graph(const char * filename);
  void dr_gen_dot_task_graph(const char * filename);


  /* dummy function to supress many
     "static function defined but not called" 
     errors */
#if ! __CILK__
  __attribute__((unused)) 
#endif
  static void dr_dummy_call_static_functions() {
    dr_dag_node * t;
    dr_dag_node * c;
    dr_start_(0, 0, 1);
    dr_begin_section_(0);
    t = dr_enter_create_task_(&c, 0);
    dr_enter_create_cilk_proc_task_(0);
    dr_start_task_(c, 0);
    dr_start_cilk_proc_(0);
    dr_end_task_(0);
    dr_return_from_create_task_(t, 0);
    t = dr_enter_wait_tasks_(0);
    dr_return_from_wait_tasks_(t, 0);
    dr_stop_(0);
    dr_print_task_graph(NULL);
    dr_gen_dot_task_graph("a.dot");
    dr_free_dag_recursively(t);
    dr_free(t, sizeof(dr_dag_node));
  }

#ifdef __cplusplus
}
#endif

