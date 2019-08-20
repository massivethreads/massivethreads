/*
 * myth_thread_pool.h
 */
#pragma once
#ifndef MYTH_THREAD_POOL_H_
#define MYTH_THREAD_POOL_H_

#include "myth/myth.h"
#include "myth_config.h"

#include "myth_worker.h"
#include "myth_misc.h"

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE

typedef struct myth_thread_pool_freelist {
  myth_freelist_cell_t* head;
  myth_freelist_cell_t* tail;
} myth_thread_pool_freelist_t;

typedef struct myth_thread_pool_buf {
  myth_thread_pool_freelist_t* fl;
  int count;
} myth_thread_pool_buf_t;

typedef struct myth_thread_pool {
  myth_freelist_mpsc_t   freelist_desc;  // Freelist of thread descriptor
  myth_thread_pool_buf_t buf_desc;       // Buffer of thread descriptors to be returned to the owner
#if MYTH_SPLIT_STACK_DESC
  myth_freelist_mpsc_t   freelist_stack; // Freelist of stack
  myth_thread_pool_buf_t buf_stack;      // Buffer of stacks to be returned to the owner
#endif
} myth_thread_pool_t;

#else

typedef struct myth_thread_pool {
  myth_freelist_t freelist_desc;  // Freelist of thread descriptor
#if MYTH_SPLIT_STACK_DESC
  myth_freelist_t freelist_stack; // Freelist of stack
#endif
} myth_thread_pool_t;

#endif

static inline void myth_thread_pool_init(myth_thread_pool_t* thread_pool, myth_running_env_t env);
static inline void myth_thread_pool_push_thread_desc(myth_thread_pool_t* thread_pool, myth_thread_t th);
static inline myth_thread_t myth_thread_pool_pop_thread_desc(myth_thread_pool_t* thread_pool);
static inline void myth_thread_pool_push_thread_stack(myth_thread_pool_t* thread_pool, void* st);
static inline void* myth_thread_pool_pop_thread_stack(myth_thread_pool_t* thread_pool);

static inline myth_thread_t myth_alloc_new_thread_desc(myth_running_env_t env, int num);
static inline myth_thread_t myth_get_new_thread_desc(myth_running_env_t env);
static inline void* myth_alloc_new_thread_stack(myth_running_env_t env, int num);
static inline void* myth_get_new_thread_stack(myth_running_env_t env, size_t size_in_bytes);
static inline void myth_free_thread_desc(myth_running_env_t env, myth_thread_t th);
static inline void myth_free_thread_stack(myth_running_env_t env, myth_thread_t th);

#endif //MYTH_THREAD_POOL_FUNC_H_
