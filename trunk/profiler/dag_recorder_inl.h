/*
 * dag recorder 2.0
 */

#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

  int dr_get_worker();
  int dr_get_num_workers();
  int dr_get_cpu();

  typedef enum {
    dr_dag_node_kind_task_group,
    dr_dag_node_kind_create_task,
    dr_dag_node_kind_wait_tasks,
    dr_dag_node_kind_end_task,
    dr_dag_node_kind_section,
    dr_dag_node_kind_task
  } dr_dag_node_kind_t;
  
  typedef struct dr_dag_node dr_dag_node;

  typedef struct dr_dag_node_list {
    dr_dag_node * a;
    int n;
    int sz;
  } dr_dag_node_list;

  typedef unsigned long long dr_clock_t;

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
	dr_dag_node_list subgraphs;	/* kind == section/task */
	int done;
	int collapsed;
      };
    };
  };


  typedef struct dr_thread_specific_state {
    union {
      struct {
	dr_dag_node * task;		/* current task */
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
      if (GS.opts.dbg_level>=1) memset(a, 222, sz);
      free(a);
    } else {
      assert(sz == 0);
    }
  }

  /* list */

  static inline void 
  dr_dag_node_list_init(dr_dag_node_list * l) {
    l->n = 0;
    l->sz = 0;
    l->a = 0;
  }

  static inline int 
  dr_dag_node_list_size(dr_dag_node_list * l) {
    return l->n;
  }

  static inline dr_dag_node *
  dr_dag_node_list_last(dr_dag_node_list * l) {
    int n = dr_dag_node_list_size(l);
    assert(n > 0);
    return &l->a[n - 1];
  }

  static inline void 
  dr_dag_node_list_resize(dr_dag_node_list * l, int sz) {
    if (sz > l->sz) {
      int new_sz = (3 * (sz + 1)) / 2;
      dr_dag_node * a 
	= (dr_dag_node *)dr_malloc(sizeof(dr_dag_node) * new_sz);
      memcpy(a, l->a, sizeof(dr_dag_node) * l->n);
      dr_free(l->a, sizeof(dr_dag_node) * l->sz);
      l->sz = new_sz;
      l->a = a;
    }
    assert(l->n < l->sz);
  }

  static inline dr_dag_node *
  dr_dag_node_list_push_back(dr_dag_node_list * l) {
    int n = dr_dag_node_list_size(l);
    dr_dag_node_list_resize(l, n + 1);
    assert(l->n < l->sz);
    l->n++;
    return &l->a[n];
  }

  static inline void 
  dr_dag_node_list_clear(dr_dag_node_list * l) {
    dr_free(l->a, sizeof(dr_dag_node) * l->sz);
    dr_dag_node_list_init(l);
  }

#if defined(__x86_64__)

  static inline unsigned long long rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc;shlq $32,%%rdx;orq %%rdx,%%rax":"=a"(u)::"%rdx");
    return u;
  }
  
#elif defined(__sparc__) && defined(__arch64__)
  
  static inline unsigned long long rdtsc(void) {
    unsigned long long u;
    asm volatile("rd %%tick, %0" : "=r" (u));
    return u;
  }

#else
  
  static inline unsigned long long rdtsc() {
    unsigned long long u;
    asm volatile ("rdtsc" : "=A" (u));
    return u;
  }
  
#endif

  static inline dr_clock_t 
  dr_get_tsc() {
    return rdtsc();
  }

  static inline dr_dag_node * 
  dr_get_cur_task_(dr_thread_specific_state * tss) {
    return tss->task;
  }

  static inline dr_dag_node * 
  dr_get_cur_task() {
    return dr_get_cur_task_(&TS[dr_get_worker()]);
  }

  static inline void 
  dr_set_cur_task_(dr_thread_specific_state * tss, dr_dag_node * t) {
    tss->task = t;
  }

  static inline void 
  dr_set_cur_task(dr_dag_node * t) {
    dr_set_cur_task_(&TS[dr_get_worker()], t);
  }

  /* initialize dag node n to become a section- or a task-type node */
  static inline void 
  dr_dag_node_init_section_or_task(dr_dag_node * n,
				   dr_dag_node_kind_t kind) {
    assert(kind >= dr_dag_node_kind_section);
    n->kind = kind;
    n->done = 0;
    n->collapsed = 0;
    dr_dag_node_list_init(&n->subgraphs);
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
  dr_end_interval(dr_dag_node * dn, dr_clock_t start, 
		  dr_clock_t est, dr_clock_t end, 
		  dr_dag_node_kind_t kind) {
    assert(kind < dr_dag_node_kind_section);
    dn->kind = kind;
    dn->start = start;
    dn->est = est;
    dn->n_nodes = 1;
    dn->n_edges = 0;
    dn->worker = dr_get_worker();
    dn->cpu = dr_get_cpu();
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
    assert(t->kind == dr_dag_node_kind_task);
    dr_dag_node * s = t;
    while (1) {
      int n = dr_dag_node_list_size(&s->subgraphs);
      /* no subsection in s 
	 -> no active sections under s
	 -> return s */
      if (n == 0) return s;
      /* look at the rightmost child */
      dr_dag_node * c = &s->subgraphs.a[n - 1];
      /* the rightmost child is a leaf
	 -> it means it has finished
	 -> no active sections under s */
      if (c->kind < dr_dag_node_kind_section) return s;
      /* something is broken if we have a task node 
	 as a child! */
      assert(c->kind == dr_dag_node_kind_section);
      /* the rightmost child is a section that has finished 
	 -> no active sections under s */
      if (c->done) return s;
      /* the rightmost child is an unfinished section.
	 -> descend */
      s = c;
    }
  }

  static inline dr_dag_node * 
  dr_task_last_section_or_create(dr_dag_node * t) {
    assert(t->kind == dr_dag_node_kind_task);
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_last(&s->subgraphs);
    assert(i->kind == dr_dag_node_kind_section
	   || i->kind == dr_dag_node_kind_create_task);
    return i;
  }

  static inline dr_dag_node * 
  dr_task_add_section(dr_dag_node * t) {
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(&s->subgraphs);
    dr_dag_node_init_section_or_task(i, dr_dag_node_kind_section);
    return i;
  }

  static inline dr_dag_node * 
  dr_task_add_create(dr_dag_node * t) {
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(&s->subgraphs);
    i->kind = dr_dag_node_kind_create_task;
    i->child = 0;
    return i;
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
  dr_start_task(dr_dag_node * p) {
    /* make a task, section, and interval */
    dr_dag_node * nt = dr_mk_dag_node_task();
    if (GS.opts.dbg_level>=2) {
      printf("start_task(parent=%p) by %d new task=%p\n", 
	     p, dr_get_worker(), nt);
    }
    /* register this task as the child of p */
    if (p) {
      assert(p->kind == dr_dag_node_kind_create_task);
      assert(p->child == 0);
      p->child = nt;
      nt->est = p->est + p->t_inf;
    } else {
      nt->est = 0;
    }
    /* set current * */
    dr_set_cur_task(nt);
    nt->start = dr_get_tsc();
  }


  /* end current interval and start task_group
  
     task    ::= section* end 
     section ::= task_group (section|create)* wait
   
     we have just learned the current interval is
     a task group, and so, we open a new section

  */

  static dr_dag_node *
  dr_enter_task_group() {
    /* get clock as early as possible */
    dr_clock_t end = dr_get_tsc();
    /* get current task (set by start_task or end_*) */
    dr_dag_node * t = dr_get_cur_task();
    /* we are beginning a new section; add it to an
       appropriate place */
    dr_dag_node * s = dr_task_add_section(t);
    dr_dag_node * tg = dr_dag_node_list_push_back(&s->subgraphs);
    if (GS.opts.dbg_level>=2) {
      printf("start_task_group() by %d task=%p, "
	     "new section=%p, new interval=%p\n", 
	     dr_get_worker(), t, s, tg);
    }
    dr_end_interval(tg, t->start, t->est, end, 
		    dr_dag_node_kind_task_group);
    return t;
  }

  /* resume from task_group 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

     we just resume from task_group.
     the predecessor node should be an interval
     ended with task_group.

  */

  static void 
  dr_return_from_task_group(dr_dag_node * t) {
    /* get the current (unfinished) section of the task */
    dr_dag_node * s = dr_task_active_node(t);
    assert(s->kind == dr_dag_node_kind_section);
    assert(s->subgraphs.n == 1);
    dr_dag_node * p = dr_dag_node_list_last(&s->subgraphs);
    if (GS.opts.dbg_level>=2) {
      printf("end_task_group(task=%p) by %d pred=%p\n", 
	     t, dr_get_worker(), p);
    }
    dr_set_cur_task(t);
    /* previous interval must be the one that started task_group */
    t->est = p->est + p->t_inf;
    /* get clock as late as possible */
    t->start = dr_get_tsc();
  }

  /* end current interval and start create_task 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static dr_dag_node * 
  dr_enter_create_task(dr_dag_node ** c) {
    dr_clock_t end = dr_get_tsc();
    dr_dag_node * t = dr_get_cur_task();
    /* get the current (unfinished) section of the task */
    dr_dag_node * ct = dr_task_add_create(t);
    if (GS.opts.dbg_level>=2) {
      printf("start_create_task() by %d task=%p, new interval=%p\n", 
	     dr_get_worker(), t, ct);
    }
    dr_end_interval(ct, t->start, t->est, end, 
		    dr_dag_node_kind_create_task);
    *c = ct;
    return t;
  }

  /* resume from create_task 
     section ::= task_group (section|create)* wait
  */
  static void 
  dr_return_from_create_task(dr_dag_node * t) {
    dr_dag_node * ct = dr_task_last_section_or_create(t);
    if (GS.opts.dbg_level>=2) {
      printf("end_create_task(task=%p) by %d interval=%p\n", 
	     t, dr_get_worker(), ct);
    }
    assert(ct->kind == dr_dag_node_kind_create_task);
    dr_set_cur_task(t);
    t->est = ct->est + ct->t_inf;
    t->start = dr_get_tsc();
  }

  /* end current interval and start wait_tasks 
     section ::= task_group (section|create)* wait
  */
  static dr_dag_node *
  dr_enter_wait_tasks() {
    dr_clock_t end = dr_get_tsc();
    dr_dag_node * t = dr_get_cur_task();
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(&s->subgraphs);
    if (GS.opts.dbg_level>=2) {
      printf("start_wait_tasks() by %d task=%p, "
	     "section=%p, new interval=%p\n", 
	     dr_get_worker(), t, s, i);
    }
    assert(s->done == 0);
    s->done = 1;
    dr_end_interval(i, t->start, t->est, end, 
		    dr_dag_node_kind_wait_tasks);
    return t;
  }

  static void 
  dr_collapse_section_or_task(dr_dag_node * s) {
    assert(s->kind >= dr_dag_node_kind_section);
    int n = dr_dag_node_list_size(&s->subgraphs);
    int i;
    assert(n > 0);
    s->start   = s->subgraphs.a[0].start;
    s->est     = s->subgraphs.a[0].est;
    s->end     = s->subgraphs.a[n - 1].end;
    s->t_1     = 0;
    s->t_inf   = 0;
    s->n_nodes = 0;
    s->n_edges = 0;
    s->worker  = s->subgraphs.a[0].worker;
    s->cpu     = s->subgraphs.a[0].cpu;

    dr_clock_t t_inf = 0;
    for (i = 0; i < n; i++) {
      dr_dag_node * x = &s->subgraphs.a[i];
      s->t_1     += x->t_1;
      s->t_inf   += x->t_inf;
      s->n_nodes += x->n_nodes;
      s->n_edges += x->n_edges + (i ? 1 : 0);
      if (s->worker != x->worker) s->worker = -1;
      if (s->cpu    != x->cpu)    s->cpu    = -1;
      if (x->kind == dr_dag_node_kind_create_task) {
	dr_dag_node * y = x->child;
	assert(y);
	if (y->end > s->end) s->end = y->end;
	s->t_1     += y->t_1;
	if (s->t_inf + y->t_inf > t_inf) t_inf = s->t_inf + y->t_inf;
	s->n_nodes += y->n_nodes;
	assert(i > 0);
	s->n_edges += y->n_edges + 2; /* ??? */
	if (s->worker != y->worker) s->worker = -1;
	if (s->cpu    != y->cpu)    s->cpu    = -1;
      }
    }
    if (t_inf > s->t_inf) s->t_inf = t_inf;
    if (s->worker != -1) {
      s->collapsed = 1;
      if (s->kind == dr_dag_node_kind_section) {
	int i;
	int n = dr_dag_node_list_size(&s->subgraphs);
	for (i = 0; i < n; i++) {
	  dr_dag_node * x = &s->subgraphs.a[i];
	  if (x->kind == dr_dag_node_kind_create_task) {
	    /* children must have been collapsed */
	    dr_dag_node * c = x->child;
	    assert(c);
	    assert(c->kind == dr_dag_node_kind_task);
	    assert(c->subgraphs.n == 0);
	    dr_free(c, sizeof(dr_dag_node));
	  } else if (x->kind == dr_dag_node_kind_section) {
	    assert(dr_dag_node_list_size(&x->subgraphs) == 0);
	  }
	}
      } else {
	assert(s->kind == dr_dag_node_kind_task);
	int i;
	int n = dr_dag_node_list_size(&s->subgraphs);
	for (i = 0; i < n; i++) {
	  dr_dag_node * x = &s->subgraphs.a[i];
	  if (x->kind == dr_dag_node_kind_section) {
	    /* subsections must have been collapsed */
	    assert(dr_dag_node_list_size(&x->subgraphs) == 0);
	  }
	}
      }
      dr_dag_node_list_clear(&s->subgraphs);
    }
  }

  /* resume from wait_tasks 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static void 
  dr_return_from_wait_tasks(dr_dag_node * t) {
    /* get the section that finished last */
    dr_dag_node * s = dr_task_last_section_or_create(t);
    assert(s->kind == dr_dag_node_kind_section);
    dr_dag_node * p = dr_dag_node_list_last(&s->subgraphs);
    assert(p->kind == dr_dag_node_kind_wait_tasks);
    if (GS.opts.dbg_level>=2) {
      printf("end_wait_tasks(task=%p) by %d section=%p, pred=%p\n", 
	     t, dr_get_worker(), s, p);
    }
    /* calc EST of the interval to start */
    dr_clock_t est = p->est + p->t_inf;
    int i;
    int n = dr_dag_node_list_size(&s->subgraphs);
    for (i = 0; i < n; i++) {
      dr_dag_node * sc = &s->subgraphs.a[i];
      assert(sc->kind < dr_dag_node_kind_task); 
      if (sc->kind == dr_dag_node_kind_create_task) {
	assert(sc->child);
	dr_dag_node * ct = sc->child;
	dr_clock_t x = ct->est + ct->t_inf;
	if (est < x) est = x;
      }
    }
    if (GS.opts.collapse) dr_collapse_section_or_task(s);
    dr_set_cur_task(t);
    t->est = est;
    t->start = dr_get_tsc();
  }

  static void 
  dr_end_task() {
    dr_clock_t end = dr_get_tsc();
    dr_dag_node * t = dr_get_cur_task();
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(&s->subgraphs);
    if (GS.opts.dbg_level>=2) {
      printf("end_task() by %d task=%p, section=%p, "
	     "new interval=%p\n", 
	     dr_get_worker(), t, s, i);
    }
    dr_end_interval(i, t->start, t->est, end, 
		    dr_dag_node_kind_end_task);
    assert(t->done == 0);
    t->done = 1;
    if (GS.opts.collapse) dr_collapse_section_or_task(t);
  }

  void dr_free_dag_recursively(dr_dag_node * g);
  void dr_print_task_graph(const char * filename);
  void dr_gen_dot_task_graph(const char * filename);

  void dr_start(dr_options * opts);
  void dr_stop();


  /* dummy function to supress many
     "static function defined but not called" 
     errors */
  __attribute__((unused)) static void dr_dummy_call_static_functions() {
    dr_start(0);
    dr_dag_node * t = dr_enter_task_group();
    dr_dag_node * c;
    dr_return_from_task_group(t);
    t = dr_enter_create_task(&c);
    dr_start_task(dr_task_last_section_or_create(t));
    dr_end_task();
    dr_return_from_create_task(t);
    t = dr_enter_wait_tasks();
    dr_return_from_wait_tasks(t);
    dr_stop();
    dr_print_task_graph(NULL);
    dr_gen_dot_task_graph("a.dot");
    dr_free_dag_recursively(t);
    dr_free(t, sizeof(dr_dag_node));
  }

#ifdef __cplusplus
}
#endif

