/*
 * myth_sched.h
 */
#pragma once
#ifndef MYTH_SCHED_H_
#define MYTH_SCHED_H_

#include <stdint.h>

#include "myth/myth.h"
#include "myth_config.h"

#include "myth_context.h"

//Thread scheduler
typedef struct myth_sched {
  void *stack;//Call stack
  myth_context context;//Scheduler context
} myth_sched, * myth_sched_t;

typedef struct myth_running_env* myth_running_env_t;

//External Global variables
extern int g_log_worker_stat;
extern int g_sched_prof;

//Cancel constants, set as the same as those of pthreads
#define MYTH_CANCEL_DEFERRED PTHREAD_CANCEL_DEFERRED
#define MYTH_CANCEL_ENABLE PTHREAD_CANCEL_ENABLE
#define MYTH_CANCEL_DISABLE PTHREAD_CANCEL_DISABLE
#define MYTH_CANCELED PTHREAD_CANCELED

static inline myth_thread_t myth_create_new_thread(myth_thread_attr_t* attr, void* arg, myth_running_env_t env);
static inline int myth_create_ex_body(myth_thread_t* id, myth_thread_attr_t* attr, myth_func_t func, void* arg);
static inline int myth_yield_ex_body(int yield_opt);
static inline int myth_yield_body(void);
static inline myth_running_env_t myth_join_body_impl(myth_thread_t th, myth_running_env_t env);
static myth_running_env_t __attribute__((noinline, unused)) myth_join_body_impl_noinline(myth_thread_t th, myth_running_env_t env);
static inline int myth_join_body(myth_thread_t th, void** result);
static inline int myth_detach_body(myth_thread_t th);
static void __attribute__((unused)) myth_entry_point(void);
static inline myth_context_t myth_entry_point_cleanup_impl(myth_thread_t this_thread);
static inline void myth_entry_point_cleanup(myth_thread_t this_thread);
static inline void myth_cleanup_thread_body(myth_thread_t th);

static inline void myth_sched_prof_start_body(void);
static inline void myth_sched_prof_pause_body(void);

#endif //MYTH_SCHED_H_
