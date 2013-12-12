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

  /* they must be provide by the underlying
     system (or by the user) */
  int dr_get_worker();
  int dr_get_num_workers();
  int dr_get_cpu();

  /* a kind of nodes */
  typedef enum {
    dr_dag_node_kind_task_group,
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

  /* chunk */

  static inline void
  dr_dag_node_chunk_init(dr_dag_node_chunk * ch) {
    ch->next = 0;
    ch->n = 0;
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_last(dr_dag_node_chunk * ch) {
    assert(ch->n > 0);
    return &ch->a[ch->n - 1];
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_first(dr_dag_node_chunk * ch) {
    assert(ch->n > 0);
    return &ch->a[0];
  }

  static inline dr_dag_node * 
  dr_dag_node_chunk_push_back(dr_dag_node_chunk * ch) {
    int n = ch->n;
    assert(n < dr_dag_node_chunk_sz);
    ch->n = n + 1;
    return &ch->a[n];
  }

  /* list */

  static inline void 
  dr_dag_node_list_init(dr_dag_node_list * l) {
    dr_dag_node_chunk_init(l->head);
    l->tail = l->head;
  }

  dr_dag_node_list * 
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
    assert(tail->n < dr_dag_node_chunk_sz);
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
    n->subgraphs = dr_mk_dag_node_list();
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
      if (dr_dag_node_list_empty(s->subgraphs)) {
	/* no subsection in s 
	   -> no active sections under s
	   -> return s */
	return s;
      }
      /* look at the rightmost child */
      dr_dag_node * c = dr_dag_node_list_last(s->subgraphs);
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
    dr_dag_node * i = dr_dag_node_list_last(s->subgraphs);
    assert(i->kind == dr_dag_node_kind_section
	   || i->kind == dr_dag_node_kind_create_task);
    return i;
  }

  static inline dr_dag_node * 
  dr_task_add_section(dr_dag_node * t) {
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
    dr_dag_node_init_section_or_task(i, dr_dag_node_kind_section);
    return i;
  }

  static inline dr_dag_node * 
  dr_task_add_create(dr_dag_node * t) {
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
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
  dr_start_task_(dr_dag_node * p) {
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
  dr_enter_task_group_() {
    /* get clock as early as possible */
    dr_clock_t end = dr_get_tsc();
    /* get current task (set by start_task or end_*) */
    dr_dag_node * t = dr_get_cur_task();
    /* we are beginning a new section; add it to an
       appropriate place */
    dr_dag_node * s = dr_task_add_section(t);
    dr_dag_node * tg = dr_dag_node_list_push_back(s->subgraphs);
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
  dr_return_from_task_group_(dr_dag_node * t) {
    /* get the current (unfinished) section of the task */
    dr_dag_node * s = dr_task_active_node(t);
    assert(s->kind == dr_dag_node_kind_section);
    assert(s->subgraphs->tail == s->subgraphs->head);
    assert(s->subgraphs->tail->n == 1);
    dr_dag_node * p = dr_dag_node_list_last(s->subgraphs);
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
  dr_enter_create_task_(dr_dag_node ** c) {
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
  dr_return_from_create_task_(dr_dag_node * t) {
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
  dr_enter_wait_tasks_() {
    dr_clock_t end = dr_get_tsc();
    dr_dag_node * t = dr_get_cur_task();
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
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

  /* look at subgraphs of s.
     if it is collapsable, collapse it */
  static void 
  dr_collapse_section_or_task(dr_dag_node * s) {
    assert(s->kind >= dr_dag_node_kind_section);
    assert(!dr_dag_node_list_empty(s->subgraphs));
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
	  assert(y);
	  if (y->end > s->end) s->end = y->end;
	  s->t_1     += y->t_1;
	  if (s->t_inf + y->t_inf > t_inf) 
	    t_inf = s->t_inf + y->t_inf;
	  s->n_nodes += y->n_nodes;
	  assert((ch != head) || i);
	  s->n_edges += y->n_edges + 2;
	  if (s->worker != y->worker) s->worker = -1;
	  if (s->cpu    != y->cpu)    s->cpu    = -1;
	}
      }
      if (t_inf > s->t_inf) s->t_inf = t_inf;
    }
    /* now check if we can collapse it */
    if (s->worker != -1) {
      /* for now, we collapse it if it 
	 was executed on a single worker */
      s->collapsed = 1;
      /* free the graph of its children */
      dr_dag_node_chunk * ch;
      for (ch = s->subgraphs->head; ch; ch = ch->next) {
	int i;
	for (i = 0; i < ch->n; i++) {
	  dr_dag_node * x = &ch->a[i];
	  if (x->kind == dr_dag_node_kind_create_task) {
	    /* children must have been collapsed */
	    dr_dag_node * c = x->child;
	    assert(c);
	    assert(c->kind == dr_dag_node_kind_task);
	    assert(c->subgraphs);
	    assert(dr_dag_node_list_empty(c->subgraphs));
	    dr_free(c->subgraphs, sizeof(dr_dag_node_list));
	    dr_free(c, sizeof(dr_dag_node));
	  } else if (x->kind == dr_dag_node_kind_section) {
	    assert(x->subgraphs);
	    assert(dr_dag_node_list_empty(x->subgraphs));
	    dr_free(x->subgraphs, sizeof(dr_dag_node_list));
	  }
	}
      }
      dr_dag_node_list_clear(s->subgraphs);
    }
  }

  /* resume from wait_tasks 

     task    ::= section* end 
     section ::= task_group (section|create)* wait

  */
  static void 
  dr_return_from_wait_tasks_(dr_dag_node * t) {
    /* get the section that finished last */
    dr_dag_node * s = dr_task_last_section_or_create(t);
    assert(s->kind == dr_dag_node_kind_section);
    dr_dag_node * p = dr_dag_node_list_last(s->subgraphs);
    assert(p->kind == dr_dag_node_kind_wait_tasks);
    if (GS.opts.dbg_level>=2) {
      printf("end_wait_tasks(task=%p) by %d section=%p, pred=%p\n", 
	     t, dr_get_worker(), s, p);
    }
    /* calc EST of the interval to start */
    dr_clock_t est = p->est + p->t_inf;
    dr_dag_node_chunk * ch;
    for (ch = s->subgraphs->head; ch; ch = ch->next) {
      int i;
      for (i = 0; i < ch->n; i++) {
	dr_dag_node * sc = &ch->a[i];
	assert(sc->kind < dr_dag_node_kind_task); 
	if (sc->kind == dr_dag_node_kind_create_task) {
	  assert(sc->child);
	  dr_dag_node * ct = sc->child;
	  dr_clock_t x = ct->est + ct->t_inf;
	  if (est < x) est = x;
	}
      }
    }
    if (GS.opts.collapse) dr_collapse_section_or_task(s);
    dr_set_cur_task(t);
    t->est = est;
    t->start = dr_get_tsc();
  }

  static void 
  dr_end_task_() {
    dr_clock_t end = dr_get_tsc();
    dr_dag_node * t = dr_get_cur_task();
    dr_dag_node * s = dr_task_active_node(t);
    dr_dag_node * i = dr_dag_node_list_push_back(s->subgraphs);
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

  /*  */

  static int is_omp_like(dr_model_t o) {
    switch (o) {
    case dr_model_omp:
    case dr_model_cilk:
      return 1;
    case dr_model_tbb:
    case dr_model_serial:
      return 0;
    default:
      assert(0);
      return 0;
    }
  }

  static void 
  dr_start_task(dr_dag_node * p) {
    dr_start_task_(p);
    if (is_omp_like(GS.opts.model)) {
      dr_return_from_task_group_(dr_enter_task_group_());
    }
  }

  static dr_dag_node *
  dr_enter_task_group() {
    return dr_enter_task_group_();
  }

  static void
  dr_return_from_task_group(dr_dag_node * t) {
    dr_return_from_task_group_(t);
  }

  static dr_dag_node * 
  dr_enter_create_task(dr_dag_node ** c) {
    return dr_enter_create_task_(c);
  }

  static void 
  dr_return_from_create_task(dr_dag_node * t) {
    dr_return_from_create_task_(t);
  }

  static dr_dag_node *
  dr_enter_wait_tasks() {
    return dr_enter_wait_tasks_();
  }

  static void 
  dr_return_from_wait_tasks(dr_dag_node * t) {
    dr_return_from_wait_tasks_(t);
    if (is_omp_like(GS.opts.model)) {
      dr_return_from_task_group_(dr_enter_task_group_());
    }
  }

  static void 
  dr_end_task() {
    if (is_omp_like(GS.opts.model)) {
      dr_return_from_wait_tasks_(dr_enter_wait_tasks_());
    }
    dr_end_task_();
  }

  void dr_free_dag_recursively(dr_dag_node * g);
  void dr_print_task_graph(const char * filename);
  void dr_gen_dot_task_graph(const char * filename);

  void dr_start(dr_options * opts);
  void dr_stop();


  /* dummy function to supress many
     "static function defined but not called" 
     errors */
  __attribute__((unused)) 
  static void dr_dummy_call_static_functions() {
    dr_start(0);
    dr_dag_node * t = dr_enter_task_group();
    dr_dag_node * c;
    dr_return_from_task_group(t);
    t = dr_enter_create_task(&c);
    dr_start_task(c);
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

