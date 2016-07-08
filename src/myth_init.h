/* 
 * myth_init.h
 */
#pragma once
#ifndef MYTH_INIT_H
#define MYTH_INIT_H

//#include "myth_original_lib.h"

#include <pthread.h>

#include "myth_io.h"
#include "myth_log.h"

//Environment value of worker threads
#define ENV_MYTH_WORKER_NUM "MYTH_WORKER_NUM"
#define ENV_MYTH_DEF_STKSIZE "MYTH_DEF_STKSIZE"
#define ENV_MYTH_BIND_WORKERS "MYTH_BIND_WORKERS"

enum {
  myth_init_state_uninit,
  myth_init_state_initializing,
  myth_init_state_initialized
};
extern myth_attr_t g_attr;
extern volatile int g_myth_init_state;

int myth_get_attr_default(myth_attr_t * attr);
int myth_set_attr(const myth_attr_t * attr);
int myth_init_ex_body(const myth_attr_t * attr);
int myth_fini_body(void);

static inline int myth_ensure_init_ex(myth_attr_t * attr) {
  if (g_myth_init_state == myth_init_state_initialized) {
    return 1;
  } else {
    return myth_init_ex_body(attr);
  }
}

static inline int myth_ensure_init(void) {
  return myth_ensure_init_ex(0);
}

#endif
