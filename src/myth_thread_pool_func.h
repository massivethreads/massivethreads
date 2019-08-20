/*
 * myth_thread_pool_func.h
 */
#pragma once
#ifndef MYTH_THREAD_POOL_FUNC_H_
#define MYTH_THREAD_POOL_FUNC_H_

#include "myth/myth.h"
#include "myth_config.h"

#include "myth_thread_pool.h"
#include "myth_init.h"
#include "myth_worker.h"
#include "myth_thread.h"
#include "myth_misc.h"
#include "myth_adws_sched.h"

#include "myth_misc_func.h"

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
static inline void myth_thread_pool_buf_init(myth_thread_pool_buf_t* buf, int num_workers) {
  buf->fl = (myth_thread_pool_freelist_t*)myth_malloc(sizeof(myth_thread_pool_freelist_t) * num_workers);
  for (int i = 0; i < num_workers; i++) {
    buf->fl[i].head = NULL;
    buf->fl[i].tail = NULL;
  }
  buf->count = 0;
}
#endif

static inline void myth_thread_pool_init(myth_thread_pool_t* thread_pool, myth_running_env_t env) {
#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE

  myth_thread_t th = myth_alloc_new_thread_desc(env, 1);
  myth_freelist_mpsc_init(&thread_pool->freelist_desc, th);
  myth_thread_pool_buf_init(&thread_pool->buf_desc, g_attr.n_workers);
#if MYTH_SPLIT_STACK_DESC
  void* st = myth_alloc_new_thread_stack(env, 1);
  myth_freelist_mpsc_init(&thread_pool->freelist_stack, st);
  myth_thread_pool_buf_init(&thread_pool->buf_stack, g_attr.n_workers);
#endif

#else

  myth_freelist_init(&thread_pool->freelist_desc);
#if MYTH_SPLIT_STACK_DESC
  myth_freelist_init(&thread_pool->freelist_stack);
#endif

#endif
}

static inline void myth_thread_pool_push_thread_desc(myth_thread_pool_t* thread_pool, myth_thread_t th) {
#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
  myth_freelist_mpsc_push(&thread_pool->freelist_desc, (void*)th);
#else
  myth_freelist_push(&thread_pool->freelist_desc, (void*)th);
#endif
}

static inline myth_thread_t myth_thread_pool_pop_thread_desc(myth_thread_pool_t* thread_pool) {
#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
  return (myth_thread_t)myth_freelist_mpsc_pop(&thread_pool->freelist_desc);
#else
  return (myth_thread_t)myth_freelist_pop(&thread_pool->freelist_desc);
#endif
}

static inline void myth_thread_pool_push_thread_stack(myth_thread_pool_t* thread_pool, void* st) {
#if MYTH_SPLIT_STACK_DESC

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
  myth_freelist_mpsc_push(&thread_pool->freelist_stack, st);
#else
  myth_freelist_push(&thread_pool->freelist_stack, st);
#endif

#else
  (void)thread_pool;
  (void)st;
#endif
}

static inline void* myth_thread_pool_pop_thread_stack(myth_thread_pool_t* thread_pool) {
#if MYTH_SPLIT_STACK_DESC

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
  return myth_freelist_mpsc_pop(&thread_pool->freelist_stack);
#else
  return myth_freelist_pop(&thread_pool->freelist_stack);
#endif

#else
  (void)thread_pool;
  return NULL;
#endif
}

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE

static inline void myth_thread_pool_fl_push(myth_thread_pool_freelist_t* fl, void* h_) {
  myth_freelist_cell_t* h = (myth_freelist_cell_t*)h_;
  h->next = fl->head;
  fl->head = h;
  if (!fl->tail) {
    fl->tail = h;
  }
}

static inline int myth_thread_pool_buf_push(myth_thread_pool_buf_t* buf, int target_rank, void* h_) {
  myth_thread_pool_fl_push(&buf->fl[target_rank], h_);
  buf->count++;
  return buf->count;
}

static inline void myth_thread_pool_flush_buffered_thread_desc(myth_running_env_t env) {
  myth_thread_pool_buf_t* buf = &env->thread_pool.buf_desc;
  for (int i = 0; i < g_attr.n_workers; i++) {
    if (i == env->rank) continue;
    myth_thread_pool_freelist_t* fl        = &buf->fl[i];
    myth_freelist_mpsc_t*        target_fl = &g_envs[i].thread_pool.freelist_desc;
    if (fl->head == NULL) continue;
    myth_freelist_mpsc_pass(target_fl, fl->head, fl->tail);
    fl->head = NULL;
    fl->tail = NULL;
  }
  buf->count = 0;
}

static inline void myth_thread_pool_flush_buffered_thread_stack(myth_running_env_t env) {
#if MYTH_SPLIT_STACK_DESC
  myth_thread_pool_buf_t* buf = &env->thread_pool.buf_stack;
  for (int i = 0; i < g_attr.n_workers; i++) {
    if (i == env->rank) continue;
    myth_thread_pool_freelist_t* fl        = &buf->fl[i];
    myth_freelist_mpsc_t*        target_fl = &g_envs[i].thread_pool.freelist_stack;
    if (fl->head == NULL) continue;
    myth_freelist_mpsc_pass(target_fl, fl->head, fl->tail);
    fl->head = NULL;
    fl->tail = NULL;
  }
  buf->count = 0;
#else
  (void)env;
#endif
}

#endif

static inline myth_thread_t myth_alloc_new_thread_desc(myth_running_env_t env, int num) {
  myth_thread_t ret;
  //Allocate
#if MYTH_SPLIT_STACK_DESC
  size_t st_size    = 0;
#else
  size_t st_size    = g_attr.stacksize;
#endif
  size_t th_size    = st_size + sizeof(struct myth_thread);
  size_t alloc_size = th_size * num;
#if MYTH_ALLOC_PROF
  env->prof_data.dmalloc_cnt++;
  uint64_t t0 = myth_get_rdtsc();
#endif

#if ALLOCATE_STACK_BY_MALLOC
  void * th_ptr = myth_flmalloc(env->rank, alloc_size);
#else
  alloc_size +=  0xFFF;
  alloc_size &= ~0xFFF;
  char* th_ptr = (char*)myth_mmap(NULL, alloc_size, PROT_READ|PROT_WRITE,
                                  MAP_PRIVATE|MYTH_MAP_ANON|MYTH_MAP_STACK, -1, 0);
#endif

#if MYTH_ALLOC_PROF
  uint64_t t1 = myth_get_rdtsc();
  env->prof_data.dmalloc_cycles += t1 - t0;
  uint64_t t2 = myth_get_rdtsc();
#endif
  th_ptr += st_size;
  for (int i = 0; i < num; i++) {
    ret = (myth_thread_t)th_ptr;
#if !MYTH_SPLIT_STACK_DESC
    ret->stack = (th_ptr - sizeof(void*));
    ret->stack_size = st_size;
#endif
#if MYTH_DESC_REUSE_CHECK
    myth_spin_init_body(&ret->sanity_check);
#endif
    myth_spin_init_body(&ret->lock);
    if (i < num - 1) {
      myth_thread_pool_push_thread_desc(&env->thread_pool, ret);
      th_ptr += th_size;
    }
  }
#if MYTH_ALLOC_PROF
  uint64_t t3 = myth_get_rdtsc();
  env->prof_data.daddlist_cycles += t3 - t2;
#endif
  return ret;
}

// Return a new thread descriptor
static inline myth_thread_t myth_get_new_thread_desc(myth_running_env_t env) {
#if MYTH_ALLOC_PROF
  env->prof_data.dalloc_cnt++;
#endif
  myth_thread_t ret = myth_thread_pool_pop_thread_desc(&env->thread_pool);
  if (!ret) {
#if MYTH_DISALLOW_ALLOC_NEW_THREADS
    fprintf(stderr, "Fatal error: allocation of new threads is not allowed.\n");
    abort();
#else
    ret = myth_alloc_new_thread_desc(env, STACK_ALLOC_UNIT);
#endif
  }
  myth_assert(ret);
#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
  ret->owner_worker = env->rank;
#endif
  return ret;
}

#if MYTH_SPLIT_STACK_DESC
static inline void* myth_alloc_new_thread_stack(myth_running_env_t env, int num) {
  void* ret;
  //Allocate
  size_t st_size = g_attr.stacksize;
#if USE_STACK_GUARDPAGE
  st_size += PAGE_SIZE;
#endif /* USE_STACK_GUARDPAGE */
  size_t alloc_size = st_size * num;
#if MYTH_ALLOC_PROF
  env->prof_data.smalloc_cnt ++;
  uint64_t t0 = myth_get_rdtsc();
#endif /* MYTH_ALLOC_PROF */

#if ALLOCATE_STACK_BY_MALLOC
  void* st_ptr = myth_flmalloc(env->rank, alloc_size);
#else
  alloc_size +=  0xFFF;
  alloc_size &= ~0xFFF;
  char* st_ptr = (char*)myth_mmap(NULL, alloc_size, PROT_READ|PROT_WRITE,
                                  MAP_PRIVATE|MYTH_MAP_ANON|MYTH_MAP_STACK, -1, 0);
#endif /* ALLOCATE_STACK_BY_MALLOC */

#if MYTH_ALLOC_PROF
  uint64_t t1 = myth_get_rdtsc();
  env->prof_data.smalloc_cycles += t1 - t0;
  uint64_t t2 = myth_get_rdtsc();
#endif /* MYTH_ALLOC_PROF */
#if USE_STACK_GUARDPAGE
  char *pr_ptr = st_ptr;
  for (int i = 0; i < num; i++) {
    mprotect(pr_ptr, PAGE_SIZE, PROT_NONE);
    pr_ptr += st_size;
  }
#endif /* USE_STACK_GUARDPAGE */
  st_ptr += st_size - (sizeof(void*) * 2);
  for (int i = 0; i < num; i++) {
    ret = (void**)st_ptr;
    uintptr_t *blk_size = (uintptr_t*)(st_ptr + sizeof(void*));
    *blk_size = 0;	  //indicates default
    if (i < num - 1) {
      myth_thread_pool_push_thread_stack(&env->thread_pool, ret);
      st_ptr += st_size;
    }
  }
#if MYTH_ALLOC_PROF
  uint64_t t3 = myth_get_rdtsc();
  env->prof_data.saddlist_cycles += t3 - t2;
#endif /* MYTH_ALLOC_PROF */
  return ret;
}
#else
static inline void* myth_alloc_new_thread_stack(myth_running_env_t env, int num) {
  (void)env;
  (void)num;
  return NULL;
}
#endif

#if MYTH_SPLIT_STACK_DESC
// fast when size_in_bytes == 0
// Return a new thread descriptor
static inline void* myth_get_new_thread_stack(myth_running_env_t env, size_t size_in_bytes) {
#if MYTH_ALLOC_PROF
  env->prof_data.salloc_cnt ++;
#endif /* MYTH_ALLOC_PROF */
  if (size_in_bytes) {
    /* let's say sizeof(void*) == 8, original size_in_bytes is tiny,
       then size_in_bytes is rounded up to 4096.
       for brevity let's say the bottom address is zero.

          4088-4095: size_in_bytes
st_ptr -> 4080-4087:
            ...
     */

    //Round up to 4KB
    size_in_bytes +=  0xFFF;
    size_in_bytes &= ~0xFFF;
    char* st_ptr = (char*)myth_flmalloc(env->rank, size_in_bytes);
    st_ptr += size_in_bytes - (sizeof(void*) * 2);
    uintptr_t *blk_size = (uintptr_t*)(st_ptr + sizeof(void*));
    *blk_size = size_in_bytes;
    return st_ptr;
  }
  void* ret = myth_thread_pool_pop_thread_stack(&env->thread_pool);
  if (!ret) {
#if MYTH_DISALLOW_ALLOC_NEW_THREADS
    fprintf(stderr, "Fatal error: allocation of new threads is not allowed.\n");
    abort();
#else
    ret = myth_alloc_new_thread_stack(env, STACK_ALLOC_UNIT);
#endif
  }
  return ret;
}
#else
static inline void* myth_get_new_thread_stack(myth_running_env_t env, size_t size_in_bytes) {
  (void)env;
  (void)size_in_bytes;
  return NULL;
}
#endif

#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
static inline void myth_buffer_or_return_thread_desc(myth_running_env_t env, myth_thread_t th) {
  int owner = th->owner_worker;
  if (owner != env->rank) {
    int count = myth_thread_pool_buf_push(&env->thread_pool.buf_desc, owner, th);
    if (count >= MYTH_THREAD_POOL_RETURN_THRESHOLD) {
      myth_thread_pool_flush_buffered_thread_desc(env);
    }
  } else {
    myth_thread_pool_push_thread_desc(&env->thread_pool, th);
  }
}
#endif

// Release thread descriptor
static inline void myth_free_thread_desc(myth_running_env_t env, myth_thread_t th) {
  myth_assert(th);
#if FREE_MYTH_THREAD_STRUCT_DEBUG
  myth_dprintf("thread descriptor %p is freed\n",th);
#endif
#if MYTH_DESC_REUSE_CHECK
  myth_spin_unlock_body(&th->sanity_check);
#endif
#if MYTH_THREAD_POOL_POLICY == MYTH_THREAD_POOL_RETURN_ALWAYS
  myth_buffer_or_return_thread_desc(env, th);
#elif MYTH_THREAD_POOL_POLICY == MYTH_THREAD_POOL_RETURN_ONLY_ADWS && MYTH_ENABLE_ADWS
  if (g_sched_adws) {
    myth_buffer_or_return_thread_desc(env, th);
  } else {
    myth_thread_pool_push_thread_desc(&env->thread_pool, th);
  }
#else
  myth_thread_pool_push_thread_desc(&env->thread_pool, th);
#endif
}

#if MYTH_SPLIT_STACK_DESC
#if MYTH_THREAD_POOL_POLICY != MYTH_THREAD_POOL_LOCAL_CACHE
static inline void myth_buffer_or_return_thread_stack(myth_running_env_t env, myth_thread_t th, void** ptr) {
  int owner = th->owner_worker;
  if (owner != env->rank) {
    int count = myth_thread_pool_buf_push(&env->thread_pool.buf_stack, owner, ptr);
    if (count >= MYTH_THREAD_POOL_RETURN_THRESHOLD) {
      myth_thread_pool_flush_buffered_thread_stack(env);
    }
  } else {
    myth_thread_pool_push_thread_stack(&env->thread_pool, ptr);
  }
}
#endif

// Release thread descriptor
static inline void myth_free_thread_stack(myth_running_env_t env, myth_thread_t th) {
  myth_assert(th);
#if FREE_MYTH_THREAD_STRUCT_DEBUG
  myth_dprintf("thread stack %p is freed\n",th);
#endif
  if (th->stack) {
    //Add to a freelist
    void** ptr = (void**)th->stack;
    uintptr_t* blk_size = (uintptr_t*)((uint8_t*)ptr + sizeof(void*));
    if (*blk_size == 0) {
#if MYTH_THREAD_POOL_POLICY == MYTH_THREAD_POOL_RETURN_ALWAYS
      myth_buffer_or_return_thread_stack(env, th, ptr);
#elif MYTH_THREAD_POOL_POLICY == MYTH_THREAD_POOL_RETURN_ONLY_ADWS && MYTH_ENABLE_ADWS
      if (g_sched_adws) {
        myth_buffer_or_return_thread_stack(env, th, ptr);
      } else {
        myth_thread_pool_push_thread_stack(&env->thread_pool, ptr);
      }
#else
      myth_thread_pool_push_thread_stack(&env->thread_pool, ptr);
#endif
    } else {
      void* stack_start = (uint8_t*)ptr - *blk_size + sizeof(void*) * 2;
      myth_flfree(env->rank, (size_t)(*blk_size), stack_start);
    }
  }
}
#else
static inline void myth_free_thread_stack(myth_running_env_t env, myth_thread_t th) {
  (void)env;
  (void)th;
}
#endif

#endif //MYTH_THREAD_POOL_FUNC_H_
