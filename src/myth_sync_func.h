/* 
 * myth_sync_func.h
 */
#pragma once
#ifndef MYTH_SYNC_FUNC_H_
#define MYTH_SYNC_FUNC_H_

#include "myth/myth_sync.h"
#include "myth_sleep_queue_func.h"
#include "myth_internal_lock_func.h"

//#define ALIGN_MUTEX

/* ----------- common procedure for handling blocking/wake up ----------- */

/* put this thread in the q of waiting threads */
static inline long myth_sleep_queue_enq_th(myth_sleep_queue_t * q, 
					   myth_thread_t t) {
  return myth_sleep_queue_enq(q, (myth_sleep_queue_item_t)t);
}

/* dequeue a thread from the q of waiting threads;
   it returns null if q is found empty. */
static inline myth_thread_t myth_sleep_queue_deq_th(myth_sleep_queue_t * q) {
  return (myth_thread_t)myth_sleep_queue_deq(q);
}

/* put this thread in the q of waiting threads */
static inline long myth_sleep_stack_push_th(myth_sleep_stack_t * s, 
					    myth_thread_t t) {
  return myth_sleep_stack_push(s, (myth_sleep_queue_item_t)t);
}

/* dequeue a thread from the q of waiting threads;
   it returns null if q is found empty. */
static inline myth_thread_t myth_sleep_stack_pop_th(myth_sleep_stack_t * s) {
  return (myth_thread_t)myth_sleep_stack_pop(s);
}

/* called back right before switching context
   from the caller who is going to block to
   the next context to run. see the call to 
   myth_swap_context_withcall in myth_block_on_queue.
   myth_swap_context_withcall saves the context
   of the current thread, call myth_mutex_lock_1
   and then jump to the next context (another
   thread or the scheduler).  */

static inline int myth_mutex_unlock_body(myth_mutex_t * mutex);

MYTH_CTX_CALLBACK void myth_block_on_queue_cb(void *arg1,void *arg2,void *arg3) {
  myth_sleep_queue_t * q = arg1;
  myth_thread_t cur = arg2;
  myth_mutex_t * m = arg3;
  /* put the current thread to the sleep
     queue here.  it is important not to have
     done this until this point (i.e., after
     current thread's conetxt has been
     saved).  if we put it in the queue
     before calling context switch
     (myth_swap_context_withcall), another
     thread might unlock the mutex right
     after it enters the queue and access
     cur data structure before the context
     has been saved  */
  myth_sleep_queue_enq_th(q, cur);
  if (m) {
    myth_mutex_unlock_body(m);
  }
}

/* block the current thread on sleep_queue q */
static inline void myth_block_on_queue(myth_sleep_queue_t * q,
				       myth_mutex_t * m) {
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t cur = env->this_thread;
  /* pop next thread to run */
  myth_thread_t next = myth_queue_pop(&env->runnable_q);
  /* next context to run. either another thread
     or the scheduler */
  myth_context_t next_ctx;
  env->this_thread = next;
  if (next) {
    /* a runnable thread */
    next->env = env;
    next_ctx = &next->context;
  } else {
    /* no runnable thread -> scheduler */
    next_ctx = &env->sched.context;
  }
  /* now save the current context, myth_sleep_queue_enq_th(q, cur)
     to put cur in the q, and jump to next_ctx */
  myth_swap_context_withcall(&cur->context, next_ctx,
			     myth_block_on_queue_cb, q, cur, m);
}


MYTH_CTX_CALLBACK void myth_block_on_stack_cb(void *arg1,void *arg2,void *arg3) {
  myth_sleep_stack_t * s = arg1;
  myth_thread_t cur = arg2;
  myth_mutex_t * m = arg3;
  /* put the current thread to the sleep
     queue here.  it is important not to have
     done this until this point (i.e., after
     current thread's conetxt has been
     saved).  if we put it in the queue
     before calling context switch
     (myth_swap_context_withcall), another
     thread might unlock the mutex right
     after it enters the queue and access
     cur data structure before the context
     has been saved  */
  myth_sleep_stack_push_th(s, cur);
  if (m) {
    myth_mutex_unlock_body(m);
  }
}


/* block the current thread on sleep_queue q */
static inline void myth_block_on_stack(myth_sleep_stack_t * s,
				       myth_mutex_t * m) {
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t cur = env->this_thread;
  /* pop next thread to run */
  myth_thread_t next = myth_queue_pop(&env->runnable_q);
  /* next context to run. either another thread
     or the scheduler */
  myth_context_t next_ctx;
  env->this_thread = next;
  if (next) {
    /* a runnable thread */
    next->env = env;
    next_ctx = &next->context;
  } else {
    /* no runnable thread -> scheduler */
    next_ctx = &env->sched.context;
  }
  /* now save the current context, myth_sleep_queue_enq_th(q, cur)
     to put cur in the q, and jump to next_ctx */
  myth_swap_context_withcall(&cur->context, next_ctx,
			     myth_block_on_stack_cb, s, cur, m);
}


typedef void * (*callback_on_wakeup_t)(void *);

/* wake up exactly one thread from the queue.
   you must gurantee there is one in the queue,
   or an attempt is perhaps concurrently made
   to insert one. wait until the queue becomes
   non-empty. */

static inline int myth_wake_one_from_queue(myth_sleep_queue_t * q,
					   callback_on_wakeup_t callback,
					   void * arg) {
  myth_running_env_t env = myth_get_current_env();
  /* wait until the queue becomes non-empty.
     necessary for example when lock/unlock
     are called almost at the same time on 
     a mutex. if lock was slightly ahead, the
     unlocker may observe the queue before
     the locker enters the queue */
  myth_thread_t to_wake = 0;
  while (!to_wake) {
    to_wake = myth_sleep_queue_deq_th(q);
  }
  /* wake up this guy */
  to_wake->env = env;
  /* do any action after dequeueing from the sleep queue
     but before really putting it in the run queue.
     (for mutex, we need to clear the locked flag
     exactly at this moment; (i) doing so before
     dequeueing from the sleep queue allows other
     thread to lock the mutex, unlock it, and dequeue
     the thread I am trying to wake; (ii) doing so
     after putting it in the run queue will allow
     the to_wake to run (by another worker), observe
     the lock bit still set, and sleep again. */
  if (callback) {
    callback(arg);
  }
  /* put the thread to wake up in run queue */
  myth_queue_push(&env->runnable_q, to_wake);
  return 1;
}

/* wake up exactly n threads from the q.
   used e.g., by barrier, where the thread
   that enters the barrier last knows how many
   threads should wake up */
static inline int myth_wake_many_from_queue(myth_sleep_queue_t * q,
					    callback_on_wakeup_t callback,
					    void * arg,
					    long n) {
  /* the following works only when some measures are taken to 
     guarantee that thread to wake up do not reenter the queue.
     currently, this procedure is called from barrier wait,
     when the caller finds it is the last guy so needs to 
     wake up others. a race would arise if those waking threads
     call barrier_wait again on the same barrier and reenter
     the queue, possibly BEFORE threads that should wake up now.

     thread 1 .. n
        barrier_wait
     thread 0
        barrier_wait -> wake up 1 .. n, but a thread, say thread 1,
                        has not yet entered the queue
     thread 1
        woke up and barrier_wait again

     in our current implementation, barrier_wait ensures that
     threads that enter barrier_wait while the wake up is
     in progress waits until all threads have been dequeued
     from the sleep queue.  this is done by temporarily making
     the state of barrier invalid, and having threads that
     see invalid state wait for it to become valid again.

     alternatively, we could (i) dequeue all threads without
     waking them up and (ii) putting them in the run queue.
  */
  myth_running_env_t env = myth_get_current_env();
  /* wait until the queue becomes non-empty.
     necessary for example when lock/unlock
     are called almost at the same time on 
     a mutex. if lock was slightly ahead, the
     unlocker may observe the queue before
     the locker enters the queue */
  myth_thread_t to_wake_head = 0;
  myth_thread_t to_wake_tail = 0;
  long i;
  for (i = 0; i < n; i++) {
    myth_thread_t to_wake = 0;
    while (!to_wake) {
      to_wake = myth_sleep_queue_deq_th(q);
    }
    to_wake->env = env;
    to_wake->next = 0;
    if (to_wake_tail) {
      to_wake_tail->next = to_wake;
    } else {
      to_wake_head = to_wake;
    }
    to_wake_tail = to_wake;
  }
  /* do any action after dequeueing from the sleep queue
     but before really putting it in the run queue.
     (for mutex, we need to clear the locked flag
     exactly at this moment; (i) doing so before
     dequeueing from the sleep queue allows other
     thread to lock the mutex, unlock it, and dequeue
     the thread I am trying to wake; (ii) doing so
     after putting it in the run queue will allow
     the to_wake to run (by another worker), observe
     the lock bit still set, and sleep again. */
  if (callback) {
    callback(arg);
  }
  /* put the thread to wake up in run queue */
  myth_thread_t to_wake = to_wake_head;
  for (i = 0; i < n; i++) {
    assert(to_wake);
    myth_thread_t next = to_wake->next;
    myth_queue_push(&env->runnable_q, to_wake);
    to_wake = next;
  }
  return n;
}

/* wake up if any thread in the queue.
   used e.g., by a cond_signal which wakes up 
   if any thread is waiting.  */
static inline int myth_wake_if_any_from_queue(myth_sleep_queue_t * q,
					      callback_on_wakeup_t callback,
					      void * arg) {
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t to_wake = myth_sleep_queue_deq_th(q);
  /* no threads sleeping, done */
  if (!to_wake) return 0;	/* I did not wake up any */
  to_wake->env = env;
  /* any action after dequeue but before really waking him up */
  if (callback) {
    callback(arg);
  }
  /* put the thread that just woke up to the run queue */
  myth_queue_push(&env->runnable_q, to_wake);
  return 1;			/* I woke up one */
}

/* wake up all threads waiting on q.
   return the number of threads that it has waken  */
static inline int myth_wake_all_from_queue(myth_sleep_queue_t * q,
					   callback_on_wakeup_t callback,
					   void * arg) {
  int n = 0;
  while (1) {
    if (myth_wake_if_any_from_queue(q, callback, arg) == 0) {
      break;
    }
    n++;
  }
  return n;			
}


static inline int myth_wake_many_from_stack(myth_sleep_stack_t * s,
					    callback_on_wakeup_t callback,
					    void * arg,
					    long n) {
  /* the following works only when some measures are taken to 
     guarantee that thread to wake up do not reenter the queue.
     currently, this procedure is called from barrier wait,
     when the caller finds it is the last guy so needs to 
     wake up others. a race would arise if those waking threads
     call barrier_wait again on the same barrier and reenter
     the queue, possibly BEFORE threads that should wake up now.

     thread 1 .. n
        barrier_wait
     thread 0
        barrier_wait -> wake up 1 .. n, but a thread, say thread 1,
                        has not yet entered the queue
     thread 1
        woke up and barrier_wait again

     in our current implementation, barrier_wait ensures that
     threads that enter barrier_wait while the wake up is
     in progress waits until all threads have been dequeued
     from the sleep queue.  this is done by temporarily making
     the state of barrier invalid, and having threads that
     see invalid state wait for it to become valid again.

     alternatively, we could (i) dequeue all threads without
     waking them up and (ii) putting them in the run queue.
  */
  myth_running_env_t env = myth_get_current_env();
  /* wait until the queue becomes non-empty.
     necessary for example when lock/unlock
     are called almost at the same time on 
     a mutex. if lock was slightly ahead, the
     unlocker may observe the queue before
     the locker enters the queue */
  myth_thread_t to_wake_head = 0;
  myth_thread_t to_wake_tail = 0;
  long i;
  for (i = 0; i < n; i++) {
    myth_thread_t to_wake = 0;
    while (!to_wake) {
      to_wake = myth_sleep_stack_pop_th(s);
    }
    to_wake->env = env;
    to_wake->next = 0;
    if (to_wake_tail) {
      to_wake_tail->next = to_wake;
    } else {
      to_wake_head = to_wake;
    }
    to_wake_tail = to_wake;
  }
  /* do any action after dequeueing from the sleep queue
     but before really putting it in the run queue.
     (for mutex, we need to clear the locked flag
     exactly at this moment; (i) doing so before
     dequeueing from the sleep queue allows other
     thread to lock the mutex, unlock it, and dequeue
     the thread I am trying to wake; (ii) doing so
     after putting it in the run queue will allow
     the to_wake to run (by another worker), observe
     the lock bit still set, and sleep again. */
  if (callback) {
    callback(arg);
  }
  /* put the thread to wake up in run queue */
  myth_thread_t to_wake = to_wake_head;
  for (i = 0; i < n; i++) {
    assert(to_wake);
    myth_thread_t next = to_wake->next;
    myth_queue_push(&env->runnable_q, to_wake);
    to_wake = next;
  }
  return n;
}



/* ----------- mutex ----------- */

/* initialize mutex */
static inline int myth_mutex_init_body(myth_mutex_t * mutex, 
				       const myth_mutexattr_t * attr) {
  (void)attr;
  myth_sleep_queue_init(mutex->sleep_q);
  mutex->state = 0;
  return 0;
}

/* destroy mutex */
static inline int myth_mutex_destroy_body(myth_mutex_t * mutex)
{
  myth_sleep_queue_destroy(mutex->sleep_q);
  return 0;
}

/* lock mutex.

   state = 2 * (the number of threads blocked
                on the mutex)
         + locked_bit (1 if locked)

   note that a mutex may be not locked (the
   last bit is zero) even if there are
   blocked threads in the queue.  this
   happens right after unlocking a mutex that
   has more than one threads blocked on
   it. we unblock one of them AND CLEAR THE
   LOCKED BIT.  we could leave the bit set,
   so that the unblocked thread is guaranteed
   to be the next one to obtain the lock.
   however, this approach leads to a live
   lock if another thread repeats
   mutex_trylock on such a mutex.  we instead
   clear the locked bit and let the unblocked
   one compete with other threads trying to
   lock it.  this approach may lead to
   spuriously waking up many threads only one
   of whom may be able to acquire a lock.
 */
static inline int myth_mutex_lock_body(myth_mutex_t * mutex) {
  /* TODO: spin block */
  while (1) {
    long s = mutex->state;
    assert(s >= 0);
    /* check lock bit */
    if ((s & 1) == 0) {
      /* lock bit clear -> try to become the one who set it */
      if (__sync_bool_compare_and_swap(&mutex->state, s, s + 1)) {
	break;
      }
    } else {
      /* lock bit set. indicate I am going to block on it.
	 I am competing with a thread who is trying to unlock it */
      if (__sync_bool_compare_and_swap(&mutex->state, s, s + 2)) {
	/* OK, I reserved a seat in the queue. even if the mutex is
	   unlocked by another thread right after the above cas, 
	   he will learn I am going to be in the queue soon, so should
	   wake me up */
	myth_block_on_queue(mutex->sleep_q, 0);
      }
    }
  }
  return 0;
}
  
/* clear lock bit, done after dequeueing
   a thread to wake up from the sleep queue and
   before putting it in the run queue */
static void * myth_mutex_clear_lock_bit(void * mutex_) {
  myth_mutex_t * mutex = mutex_;
  assert(mutex->state & 1);
  __sync_fetch_and_sub(&mutex->state, 1);
  return 0;
}

/* unlock a mutex */
static inline int myth_mutex_unlock_body(myth_mutex_t * mutex) {
  while (1) {
    long s = mutex->state;
    /* the mutex must be locked now (by me). 
       TODO: a better diagnosis message */
    if (!(s & 1)) {
      /* lock bit clear. the programmer must have called this
	 on a mutex that is not locked */
      fprintf(stderr, "myth_mutex_unlock : called on unlocked mutex, abort.\n");
      /* TODO: call myth_abort() rather than directly calling exit
	 show more useful info */
      exit(1);
    }
    if (s > 1) {
      /* some threads are blocked (or just have decided to block) 
	 on the queue. decrement it (while still keeping the lock bit)
	 wake up one, and then clear the lock bit */
      if (__sync_bool_compare_and_swap(&mutex->state, s, s - 2)) {
	// myth_mutex_wake_one_from_queue(mutex);
	myth_wake_one_from_queue(mutex->sleep_q, 
				 myth_mutex_clear_lock_bit, mutex);
	break;
      }
    } else {
      /* nobody waiting. clear the lock bit and done */
      assert(s == 1);
      if (__sync_bool_compare_and_swap(&mutex->state, 1, 0)) {
	break;
      }
    }
  }
  return 0;
}

static inline int myth_mutex_trylock_body(myth_mutex_t * mutex) {
  /* TODO: spin block */
  while (1) {
    long s = mutex->state;
    /* check the lock bit */
    if (s & 1) {
      /* lock bit set. do nothing and go home */
      return EBUSY;
    } else if (__sync_bool_compare_and_swap(&mutex->state, s, s + 1)) {
      /* I set the lock bit */
      return 0;
    } else {
      continue;
    }
  }
}

/* ----------- condition variable ----------- */

static inline int myth_cond_init_body(myth_cond_t * cond, 
				      const myth_condattr_t * attr) {
  (void)attr;
  myth_sleep_queue_init(cond->sleep_q);
  return 0;
}

static inline int myth_cond_destroy_body(myth_cond_t * cond) {
  myth_sleep_queue_destroy(cond->sleep_q);
  return 0;
}

static inline int myth_cond_broadcast_body(myth_cond_t * cond) {
  myth_wake_all_from_queue(cond->sleep_q, 0, 0);
  return 0;
}

static inline int myth_cond_signal_body(myth_cond_t * cond) {
  myth_wake_if_any_from_queue(cond->sleep_q, 0, 0);
  return 0;
}

static inline int myth_cond_wait_body(myth_cond_t * cond, myth_mutex_t * mutex) {
  myth_block_on_queue(cond->sleep_q, mutex);
  return myth_mutex_lock(mutex);
}

/* ----------- barrier ----------- */

static inline int myth_barrier_init_body(myth_barrier_t * barrier, 
					 const myth_barrierattr_t * attr, 
					 long n_threads) {
  (void)attr;
  /* 2 *(number of threads that reached) + invalid */
  barrier->state = 0;
  barrier->n_threads = n_threads;
  //myth_sleep_queue_init(barrier->sleep_q);
  myth_sleep_stack_init(barrier->sleep_s);
  return 0;
}

static inline int myth_barrier_destroy_body(myth_barrier_t * barrier) {
  assert(barrier->state == 0);
  //myth_sleep_queue_destroy(barrier->sleep_q);
  myth_sleep_stack_destroy(barrier->sleep_s);
  return 0;
}

static inline int myth_barrier_wait_body(myth_barrier_t * barrier) {
  while (1) {
    long c = barrier->state;
    if (c >= barrier->n_threads) {
      /* TODO: set errno and return */
      fprintf(stderr, 
	      "myth_barrier_wait : excess threads (> %ld) enter barrier_wait\n",
	      barrier->n_threads);
      exit(1);
    }
    if (! __sync_bool_compare_and_swap(&barrier->state, c, c + 1)) {
      continue;
    }
    if (c == barrier->n_threads - 1) {
      /* I am the last one. wake up all guys.
	 TODO: spin block */
      barrier->state = 0;	/* reset state */
      //myth_wake_many_from_queue(barrier->sleep_q, 0, 0, c);
      myth_wake_many_from_stack(barrier->sleep_s, 0, 0, c);
      return MYTH_BARRIER_SERIAL_THREAD;
    } else {
      //myth_block_on_queue(barrier->sleep_q, 0);
      myth_block_on_stack(barrier->sleep_s, 0);
      return 0;
    }
  }
}

/* ----------- join counter ----------- */

/* calc number of bits enough to represent x
   (1 -> 1, 2 -> 2, 3 -> 2, 4 -> 3, etc.)
*/
static inline int calc_bits(long x) {
  int b = 0;
  while (x >= (1L << b)) {
    b++;
  }
  assert(x < (1L << b));
  return b;
}

static inline int myth_join_counter_init_body(myth_join_counter_t * jc, 
					      const myth_join_counterattr_t * attr, 
					      long n_threads) {
  //Create a join counter with initial value val
  //val must be positive
  (void)attr;
  int b = calc_bits(n_threads);
  long mask = (1L << b) - 1;
  jc->n_threads = n_threads;
  jc->n_threads_bits = b;
  jc->state_mask = mask;
  assert((n_threads & mask) == n_threads);
  /* number of waiters|number of decrements so far */
  jc->state = 0;
  myth_sleep_queue_init(jc->sleep_q);
  return 0;
}

static inline int myth_join_counter_wait_body(myth_join_counter_t * jc) {
  while (1) {
    long s = jc->state;
    if ((s & jc->state_mask) == jc->n_threads) {
      return 0;
    }
    /* try to indicate I am going to sleep. */
    long new_s = s + (1L << jc->n_threads_bits);
    if (! __sync_bool_compare_and_swap(&jc->state, s, new_s)) {
      /* another thread may have just decrement it, so I may
	 have to keep going */
      continue;
    }
    myth_block_on_queue(jc->sleep_q, 0);
    assert((jc->state & jc->state_mask) == jc->n_threads);
  }
}

static inline int myth_join_counter_dec_body(myth_join_counter_t * jc) {
  while (1) {
    long s = jc->state;
    long n_decs = s & jc->state_mask;
    if (n_decs >= jc->n_threads) {
      /* TODO: set errno and return */
      fprintf(stderr, 
	      "myth_join_counter_dec : excess threads (> %ld) enter join_counter_dec\n",
	      jc->n_threads);
      exit(1);
    }
    assert(((s + 1) & jc->state_mask) == (n_decs + 1));
    if (!__sync_bool_compare_and_swap(&jc->state, s, s + 1)) {
      continue;
    }
    if (n_decs == jc->n_threads - 1) {
      /* I am the last one. wake up all guys.
	 TODO: spin block */
      long n_threads_to_wake = (s >> jc->n_threads_bits);
      myth_wake_many_from_queue(jc->sleep_q, 0, 0, n_threads_to_wake);
    }
    break;
  }
  return 0;
}

#if 1				/* felock */

static inline int myth_felock_init_body(myth_felock_t * fe,
					const myth_felockattr_t * attr) {
  (void)attr;
  myth_mutex_init_body(fe->mutex, 0);
  myth_cond_init_body(&fe->cond[0], 0);
  myth_cond_init_body(&fe->cond[1], 0);
  fe->status = 0;
  return 0;
}

static inline int myth_felock_destroy_body(myth_felock_t * fe) {
  myth_mutex_destroy_body(fe->mutex);
  myth_cond_destroy_body(&fe->cond[0]);
  myth_cond_destroy_body(&fe->cond[1]);
  return 0;
}

static inline int myth_felock_lock_body(myth_felock_t * fe) {
  return myth_mutex_lock_body(fe->mutex);
}

static inline int myth_felock_unlock_body(myth_felock_t * fe) {
  return myth_mutex_unlock_body(fe->mutex);
}

static inline int myth_felock_wait_and_lock_body(myth_felock_t * fe, 
						 int status_to_wait) {
  myth_mutex_lock_body(fe->mutex);
  while (fe->status != status_to_wait) {
    myth_cond_wait(&fe->cond[status_to_wait], fe->mutex);
  }
  return 0;
}

static inline int myth_felock_mark_and_signal_body(myth_felock_t * fe,
						   int status_to_signal) {
  fe->status = status_to_signal;
  myth_cond_signal(&fe->cond[status_to_signal]);
  return myth_mutex_unlock_body(fe->mutex);
}

static inline int myth_felock_status_body(myth_felock_t * fe) {
  return fe->status;
}


#else

static inline int myth_felock_init_body(myth_felock_t * felock,
					const myth_felockattr_t * attr)
{
  (void)attr;
  myth_mutex_init_body(felock->lock, 0);
  felock->fe=0;
  felock->fsize = felock->esize = DEFAULT_FESIZE;
  felock->ne = felock->nf = 0;
  felock->el = felock->def_el;
  felock->fl = felock->def_fl;
  return 0;
}

static inline int myth_felock_destroy_body(myth_felock_t * felock)
{
  myth_running_env_t env;
  env = myth_get_current_env();
  myth_mutex_destroy_body(felock->lock);
  if (felock->el != felock->def_el){
    assert(felock->esize>DEFAULT_FESIZE);
    myth_flfree(env->rank, sizeof(myth_thread_t) * felock->esize, felock->el);
  }
  if (felock->fl != felock->def_fl){
    assert(felock->fsize > DEFAULT_FESIZE);
    myth_flfree(env->rank, sizeof(myth_thread_t) * felock->fsize, felock->fl);
  }
  return 0;
}

static inline int myth_felock_lock_body(myth_felock_t * felock)
{
  return myth_mutex_lock_body(felock->lock);
}

MYTH_CTX_CALLBACK void myth_felock_wait_lock_1(void *arg1,void *arg2,void *arg3)
{
  myth_running_env_t env = (myth_running_env_t)arg1;;
  myth_thread_t next = (myth_thread_t)arg2;
  myth_felock_t * fe = (myth_felock_t *)arg3;
  env->this_thread=next;
  if (next){
    next->env=env;
  }
  myth_mutex_unlock_body(fe->lock);
}

static inline int myth_felock_wait_lock_body(myth_felock_t * felock, int val)
{
  myth_running_env_t e = myth_get_current_env();
  myth_thread_t this_thread = e->this_thread;
  while (1) {
    myth_mutex_lock_body(felock->lock);
    volatile myth_running_env_t ev;
    ev = myth_get_current_env();
    myth_running_env_t env;
    env = ev;
    assert(this_thread == env->this_thread);
    if ((!!val) == (!!felock->fe)){
      return 0;
    }
    //add to queue
#if 1
    if (val){
      //wait for fe to be !0
      //check size and reallocate
      if (felock->fsize <= felock->nf){
	if (felock->fsize == DEFAULT_FESIZE){
	  felock->fl = myth_flmalloc(env->rank, 
				     sizeof(myth_thread_t) * felock->fsize * 2);
	  memcpy(felock->fl, felock->def_fl, 
		 sizeof(myth_thread_t) * felock->fsize);
	} else {
	  felock->fl = myth_flrealloc(env->rank,
				      sizeof(myth_thread_t) * felock->fsize,
				      felock->fl,
				      sizeof(myth_thread_t) * felock->fsize * 2);
	}
	felock->fsize *= 2;
      }
      //add
      felock->fl[felock->nf] = this_thread;
      felock->nf++;
    } else {
      //wait for fe to be 0
      //check size and reallocate
      if (felock->esize <= felock->ne){
	if (felock->esize == DEFAULT_FESIZE){
	  felock->el = myth_flmalloc(env->rank, 
				     sizeof(myth_thread_t) * felock->esize * 2);
	  memcpy(felock->el, felock->def_el, 
		 sizeof(myth_thread_t) * felock->esize);
	} else {
	  felock->el = myth_flrealloc(env->rank,
				      sizeof(myth_thread_t) * felock->esize,
				      felock->el,
				      sizeof(myth_thread_t) * felock->esize * 2);
	}
	felock->esize *= 2;
      }
      //add
      felock->el[felock->ne] = this_thread;
      felock->ne++;
    }
    myth_thread_t next;
    myth_context_t next_ctx;
    next = myth_queue_pop(&env->runnable_q);
    if (next){
      next->env = env;
      next_ctx = &next->context;
    }
    else{
      next_ctx = &env->sched.context;
    }
    myth_swap_context_withcall(&this_thread->context, next_ctx, 
			       myth_felock_wait_lock_1,
			       (void*)env, (void*)next, (void*)felock);
#else
    myth_mutex_unlock_body(&felock->lock);
    myth_yield_body();
#endif
  }
}

static inline int myth_felock_unlock_body(myth_felock_t * felock)
{
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t next;
  if (felock->fe){
    if (felock->nf){
      felock->nf--;
      next = felock->fl[felock->nf];
      next->env = env;
      myth_queue_push(&env->runnable_q, next);
    }
  } else {
    if (felock->ne){
      felock->ne--;
      next = felock->el[felock->ne];
      next->env = env;
      myth_queue_push(&env->runnable_q, next);
    }
  }
  return myth_mutex_unlock_body(felock->lock);
}

static inline int myth_felock_set_unlock_body(myth_felock_t * felock, int val)
{
  felock->fe = val;
  return myth_felock_unlock_body(felock);
}

static inline int myth_felock_status_body(myth_felock_t * felock)
{
  return felock->fe;
}


#endif

#endif /* MYTH_SYNC_FUNC_H_ */
