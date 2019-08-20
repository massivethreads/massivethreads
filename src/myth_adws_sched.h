/*
 * myth_adws_sched.h
 */
#pragma once
#ifndef MYTH_ADWS_SCHED_H_
#define MYTH_ADWS_SCHED_H_

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS

#include "myth_worker.h"

extern myth_workers_range_t g_root_workers_range;
extern volatile int g_sched_adws;
extern volatile int g_adws_stealable;

static inline myth_thread_t myth_adws_get_search_root_thread(myth_running_env_t env);
static inline void myth_adws_set_search_root_thread(myth_thread_t th, myth_running_env_t target_env);
static inline myth_thread_t myth_adws_pop_runnable_thread(myth_running_env_t env);
static inline void myth_adws_push_thread(myth_thread_t th, myth_running_env_t env);
static inline int myth_adws_create_ex_impl(myth_thread_t*      id,
                                           myth_thread_attr_t* attr,
                                           myth_func_t         func,
                                           void*               arg,
                                           double              work,
                                           myth_running_env_t  env);
static inline int myth_adws_create_ex_first_body(myth_thread_t*        id,
                                                 myth_thread_attr_t*   attr,
                                                 myth_func_t           func,
                                                 void*                 arg,
                                                 double                work,
                                                 double                w_total,
                                                 myth_workers_range_t* workers_range);
static inline int myth_adws_create_ex_body(myth_thread_t*      id,
                                           myth_thread_attr_t* attr,
                                           myth_func_t         func,
                                           void*               arg,
                                           double              work);
static inline int myth_adws_join_body(myth_thread_t th, void** result);
static inline int myth_adws_join_last_body(myth_thread_t th, void** result, myth_workers_range_t workers_range);
static inline int myth_adws_get_stealable_body();
static inline void myth_adws_set_stealable_body(int flag);

#endif

#endif //MYTH_ADWS_SCHED_H_
