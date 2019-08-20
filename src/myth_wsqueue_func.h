/*
 * myth_wsqueue_func.h
 */
#pragma once
#ifndef MYTH_WSQUEUE_FUNC_H_
#define MYTH_WSQUEUE_FUNC_H_

#include "myth_config.h"
#include "myth_wsqueue.h"

#include "myth_spinlock_func.h"
#include "myth_misc_func.h"
#include "myth_mem_barrier_func.h"

//critical section for signal
static inline void myth_queue_enter_operation(myth_thread_queue_t q) {
#if USE_SIGNAL_CS
  assert(q->op_flag == 0);
  q->op_flag = 1;
  myth_wsqueue_wbarrier();
#else
  (void)q;
#endif
#if USE_THREAD_CS
  real_pthread_mutex_lock(&q->mtx);
#else
  (void)q;
#endif
}

static inline void myth_queue_exit_operation(myth_thread_queue_t q) {
#if USE_SIGNAL_CS
  assert(q->op_flag == 1);
  myth_wsqueue_wbarrier();
  q->op_flag = 0;
#else
  (void)q;
#endif
#if USE_THREAD_CS
  real_pthread_mutex_unlock(&q->mtx);
#else
  (void)q;
#endif
}

static inline int myth_queue_is_operating(myth_thread_queue_t q) {
  int ret = 0;
#if USE_SIGNAL_CS
  ret = ret && q->op_flag;
#else
  (void)q;
#endif
  return ret;
}

static inline void myth_queue_init(myth_thread_queue_t q) {
  myth_spin_init_body(&q->lock);
#if USE_SIGNAL_CS
  q->op_flag = 0;
#endif
#if USE_THREAD_CS
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK);
  real_pthread_mutex_init(&q->mtx,&attr);
  pthread_mutexattr_destroy(&attr);
#endif
  q->size = INITIAL_QUEUE_SIZE;
  q->ptr = (myth_thread_t*)myth_malloc(sizeof(myth_thread_t) * q->size);
  memset(q->ptr, 0, sizeof(myth_thread_t) * q->size);
  q->base = q->size / 2;
  q->top = q->base;
}

static inline void myth_queue_fini(myth_thread_queue_t q) {
  myth_queue_clear(q);
  myth_spin_destroy_body(&q->lock);
  myth_free_with_size(q->ptr, 0);
}

static inline void myth_queue_clear(myth_thread_queue_t q) {
  myth_queue_enter_operation(q);
  myth_spin_lock_body(&q->lock);
  myth_assert(q->top == q->base);
  q->base = q->size / 2;
  q->top = q->base;
  myth_spin_unlock_body(&q->lock);
  myth_queue_exit_operation(q);
}

//push/pop/peek:Owner thread operations
static inline void __attribute__((always_inline)) myth_queue_push(myth_thread_queue_t q, myth_thread_t th) {
  myth_queue_enter_operation(q);
  int top = q->top;
  // Check size
  if (top == q->size) {
    // Acquire lock
    myth_spin_lock_body(&q->lock);
    // Runqueue full?
    if (q->base == 0) {
      myth_assert(0);
      fprintf(stderr, "Fatal error:Runqueue overflow\n");
      abort();
      /* TODO:extend runqueue */
    } else {
      // Shift pointers
      int offset = (- q->base - 1) / 2;
      myth_assert(offset < 0);
      memmove(&q->ptr[q->base+offset], &q->ptr[q->base],
	      sizeof(myth_thread_t) * (q->top - q->base));
      q->top += offset;
      q->base += offset;
    }
    top = q->top;
    myth_assert(top < q->size);
    myth_spin_unlock_body(&q->lock);
  }
  // Do not need to extend of move.
  q->ptr[top] = th;
  myth_wbarrier(); // Guarantee W-W dependency
  q->top = top + 1;
  myth_queue_exit_operation(q);
}

static inline myth_thread_t __attribute__((always_inline)) myth_queue_pop(myth_thread_queue_t q) {
  myth_queue_enter_operation(q);
#if QUICK_CHECK_ON_POP
  if (q->top <= q->base) {
    return NULL;
  }
#endif
  myth_thread_t ret;
  int top, base;
  top = q->top;
  top--;
  q->top = top;
  // Decrement and check top
  myth_rwbarrier();
  base = q->base;
  if (base + 1 < top) {
    ret = q->ptr[top];
  } else {
    myth_spin_lock_body(&q->lock);
    base = q->base;
    if (base > top) {
      q->top = q->size / 2;
      q->base = q->size / 2;
      myth_spin_unlock_body(&q->lock);
      return NULL;
    }
    ret = q->ptr[top];
    myth_spin_unlock_body(&q->lock);
  }
  myth_queue_exit_operation(q);
  return ret;
}

static inline myth_thread_t myth_queue_take_unsafe(myth_thread_queue_t q) {
  myth_thread_t ret;
  int base, top;
  //Increment base
  base = q->base;
  q->base = base + 1;
  myth_rwbarrier();
  top = q->top;
  if (base >= top) {
    q->base = base;
    return NULL;
  }
  ret = q->ptr[base];
  return ret;
}

//take/pass:Non-owner functions
static inline myth_thread_t myth_queue_take(myth_thread_queue_t q) {
  myth_thread_t ret;
#if QUICK_CHECK_ON_STEAL
  if (q->top - q->base <= 0) {
    return NULL;
  }
#endif
  myth_spin_lock_body(&q->lock);

  ret = myth_queue_take_unsafe(q);

  myth_spin_unlock_body(&q->lock);
  return ret;
}

static inline myth_thread_t myth_queue_peek(myth_thread_queue_t q) {
  myth_thread_t ret;
  int base, top;
#if QUICK_CHECK_ON_STEAL
  if (q->top - q->base <= 0) {
    return NULL;
  }
#endif
  base = q->base;
  top = q->top;
  if (base < top){
    ret = q->ptr[base];
  }else{
    ret = NULL;
  }
  return ret;
}

static inline int myth_queue_trypass(myth_thread_queue_t q, myth_thread_t th) {
  int ret;
  if (!myth_spin_trylock_body(&q->lock)) return 0;
  if (q->base == 0) {
    ret = 0;
  } else {
    int base = q->base;
    base--;
    q->ptr[base] = th;
    myth_wbarrier();
    q->base = base;
    ret = 1;
  }
  myth_spin_unlock_body(&q->lock);
  return ret;
}

static inline void myth_queue_pass(myth_thread_queue_t q, myth_thread_t th) {
  int ret;
  do {
    ret = myth_queue_trypass(q,th);
  } while (ret == 0);
}

//put:Owner function: put a thread to the tail of the queue
static inline void myth_queue_put(myth_thread_queue_t q, myth_thread_t th) {
  myth_queue_enter_operation(q);
  myth_spin_lock_body(&q->lock);

  if (q->base == 0) {
    /* queue underflow at the bottom. move the contents higher */
    if (q->top == q->size) {
      fprintf(stderr, "Fatal error:Runqueue overflow\n");
      abort();
    } else {
      int offset = (q->size - q->top + 1) / 2;
      myth_assert(offset > 0);
      memmove(&q->ptr[q->base + offset], &q->ptr[q->base],
	      sizeof(myth_thread_t) * (q->top - q->base));
      q->top += offset;
      q->base += offset;
      myth_assert(q->base > 0);
    }
  }
  int base = q->base;
  myth_assert(base > 0);
  base--;
  q->ptr[base] = th;
  myth_wbarrier();
  q->base = base;

  myth_spin_unlock_body(&q->lock);
  myth_queue_exit_operation(q);
}

#endif /* MYTH_WSQUEUE_FUNC_H_ */
