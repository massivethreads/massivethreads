/* 
 * myth_init_func.h
 */
#pragma once
#ifndef MYTH_INIT_FUNC_H_
#define MYTH_INIT_FUNC_H_

#include "myth_config.h"
#include "myth_init.h"
#include "myth_misc.h"
#include "myth_bind_worker.h"

#include "myth_misc_func.h"

static inline int myth_ensure_init_ex(myth_globalattr_t * attr) {
  if (g_myth_init_state == myth_init_state_initialized) {
    return 1;
  } else {
    return myth_init_ex_body(attr);
  }
}

static inline int myth_ensure_init(void) {
  return myth_ensure_init_ex(0);
}

static inline size_t myth_globalattr_default_stacksize(void) {
  /* default stack size */
  size_t sz = 0;
  char * env = getenv(ENV_MYTH_DEF_STKSIZE);
  if (env) {
    sz = atoi(env);
  }
  if (sz <= 0) {
    sz = MYTH_DEF_STACK_SIZE;
  }
  return sz;
}

static inline size_t myth_globalattr_default_guardsize(void) {
  /* default guard size */
  size_t sz = 0;
  char * env = getenv(ENV_MYTH_DEF_GUARDSIZE);
  if (env) {
    sz = atoi(env);
  }
  if (sz <= 0) {
    sz = MYTH_DEF_GUARD_SIZE;
  }
  return sz;
}

static inline size_t myth_globalattr_default_num_workers(void) {
  /* number of workers */
  /* we used to use MYTH_WORKER_NUM, but 
     now MYTH_NUM_WORKERS supercedes */
  int nw = 0;
  char * env = getenv(ENV_MYTH_NUM_WORKERS);
  if (env) {
    nw = atoi(env);
  } else {
    env = getenv(ENV_MYTH_WORKER_NUM);
    if (env) {
      fprintf(stderr,
	      "myth: environment variable MYTH_WORKER_NUM will be superceded by MYTH_NUM_WORKERS in future; consider switching to it now\n");
      nw = atoi(env);
    }
  }
  if (nw <= 0) {
    nw = myth_get_n_available_cpus();
  }
  return nw;
}

static inline size_t myth_globalattr_default_bind_workers(void) {
  /* bind workers */
  int bw = MYTH_DEFAULT_BIND_WORKERS;
  char * env = getenv(ENV_MYTH_BIND_WORKERS);
  if (env){
    bw = atoi(env);
  }
  return bw;
}

static inline size_t myth_globalattr_default_child_first(void) {
  /* child first */
  int cf = MYTH_CHILD_FIRST;
  char * env = getenv(ENV_MYTH_CHILD_FIRST);
  if (env){
    cf = atoi(env);
  }
  return cf;
}

static inline int myth_globalattr_init_body(myth_globalattr_t * attr) {
  myth_globalattr_t a;
  a.initialized = 1;
  a.stacksize = myth_globalattr_default_stacksize();
  a.guardsize = myth_globalattr_default_guardsize();
  a.n_workers = myth_globalattr_default_num_workers();
  a.bind_workers = myth_globalattr_default_bind_workers();
  a.child_first = myth_globalattr_default_child_first();
  *attr = a;
  return 0;
}

static inline int myth_globalattr_destroy_body(myth_globalattr_t * attr) {
  (void)attr;
  return 0;
}

static inline int
myth_globalattr_get_stacksize_body(const myth_globalattr_t * attr,
				   size_t *stacksize) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  *stacksize = attr->stacksize;
  return 0;
}

static inline int
myth_globalattr_set_stacksize_body(myth_globalattr_t * attr,
				   size_t stacksize) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  attr->stacksize = stacksize;
  return 0;
}

static inline int
myth_globalattr_get_guardsize_body(const myth_globalattr_t * attr,
				   size_t *guardsize) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  *guardsize = attr->guardsize;
  return 0;
}

static inline int
myth_globalattr_set_guardsize_body(myth_globalattr_t * attr,
				   size_t guardsize) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  attr->guardsize = guardsize;
  return 0;
}

static inline int
myth_globalattr_get_n_workers_body(const myth_globalattr_t * attr,
				   size_t *n_workers) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  *n_workers = attr->n_workers;
  return 0;
}

static inline int
myth_globalattr_set_n_workers_body(myth_globalattr_t * attr,
				   size_t n_workers) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  attr->n_workers = n_workers;
  return 0;
}

static inline int
myth_globalattr_get_bind_workers_body(const myth_globalattr_t * attr,
				      int *bind_workers) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  *bind_workers = attr->bind_workers;
  return 0;
}

static inline int
myth_globalattr_set_bind_workers_body(myth_globalattr_t * attr,
				      int bind_workers) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  attr->bind_workers = bind_workers;
  return 0;
}

static inline int
myth_globalattr_get_child_first_body(const myth_globalattr_t * attr,
				     int *child_first) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  *child_first = attr->child_first;
  return 0;
}

static inline int
myth_globalattr_set_child_first_body(myth_globalattr_t * attr,
				     int child_first) {
  if (!attr) {
    if (!g_attr.initialized) myth_globalattr_init_body(&g_attr);
    attr = &g_attr;
  }
  attr->child_first = child_first;
  return 0;
}

#endif	/* MYTH_INIT_FUNC_H_ */
