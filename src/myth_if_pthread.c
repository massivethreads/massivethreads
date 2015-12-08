/* 
 * myth_pthread_if.c : pthread-like interface
 */
#include "myth/myth.h"

#include "myth_init.h"
#include "myth_sched.h"
#include "myth_worker.h"
#include "myth_io.h"

#include "myth_sched_proto.h"
#include "myth_io_proto.h"
#include "myth_tls_proto.h"

#include "myth_worker_func.h"
#include "myth_io_func.h"
#include "myth_sync_func.h"
#include "myth_sched_func.h"
#include "myth_tls_func.h"

#include "myth_if_pthread.h"

int sched_yield(void) {
  real_sched_yield();
  myth_yield_body(1);
  return 0;
}

pthread_t pthread_self(void) {
  return (pthread_t)myth_self_body();
}

int pthread_create(pthread_t *pth, const pthread_attr_t * attr, 
		   void *(*func)(void*),void *args) {
  assert(sizeof(myth_thread_attr_t) <= sizeof(pthread_attr_t));
  assert(sizeof(myth_thread_t) <= sizeof(pthread_t));
  (void)attr;
  return myth_create_ex_body((myth_thread_t *)pth, (myth_thread_attr_t *)attr, func, args);
}

int pthread_join(pthread_t th, void ** ret) {
  return myth_join_body((myth_thread_t)th, ret);
}

void pthread_exit(void *ret) {
  myth_exit_body(ret);
  //To avoid warning, this code is unreachable
  while (1) { }
}

int pthread_detach(pthread_t th) {
  return myth_detach_body((myth_thread_t)th);
}

int pthread_setcancelstate(int state, int *oldstate) {
  return myth_setcancelstate_body(state, oldstate);
}

int pthread_setcanceltype(int type, int *oldtype) {
  return myth_setcanceltype_body(type, oldtype);
}

int pthread_cancel (pthread_t th) {
  return myth_cancel_body((myth_thread_t)th);
}

void pthread_testcancel(void) {
  myth_testcancel_body();
}

int pthread_key_create(pthread_key_t *key, void (*destructor) (void *)) {
  assert(sizeof(myth_key_t) <= sizeof(pthread_key_t));
  return myth_key_create_body((myth_key_t*)key,destructor);
}

int pthread_key_delete (pthread_key_t key) {
  return myth_key_delete_body((myth_key_t)key);
}

void *pthread_getspecific (pthread_key_t key) {
  return myth_getspecific_body((myth_key_t)key);
}

int pthread_setspecific (pthread_key_t key, const void *ptr) {
  return myth_setspecific_body((myth_key_t)key, (void*)ptr);
}

/* ---------- pthread_mutex ---------- */

static inline void check_mutex_initializer(pthread_mutex_t * mtx) {
#ifdef MYTH_SUPPORT_MUTEX_INITIALIZER


  /* TODO: develop a portable/efficient/safe checking method */
  static int checked = 1;


  if (!checked) {
    /* it looks mtx has been initialized with = PTHREAD_MUTEX_INITIALIZER,
       we can only hope this is equivaent to MYTH_MUTEX_INITIALIZER.
       the strategy to overwrite it with MYTH_MUTEX_INITIALIZER 
       is not guaranteed to work unless we make it atomic */
    /* TODO: can we do anything better */

    /* TODO: the method below is broken; 
       myth_mutex_t has a gap between fields, so 
       MYTH_MUTEX_INITIALIZER does not uniquely determine
       a bit image. the check may undeterministically fail.
       we can fix the definition of myth_mutex_t so as
       not to leave any gap, but, besides being artificial,
       there is no hope if PTHREAD_MUTEX_INITIALIZER has a gap.

       after all we know PTHREAD_MUTEX_INITIALIZER is all-zeros,
       and it just works fine for myth_mutex_t too.
    */
    pthread_mutex_t p = PTHREAD_MUTEX_INITIALIZER;
    if (memcmp(&p, mtx, sizeof(pthread_mutex_t)) == 0) {
      myth_mutex_t m = MYTH_MUTEX_INITIALIZER;
      if (memcmp(&p, &m, sizeof(myth_mutex_t)) != 0) {
	fprintf(stderr, 
		"error: it appears your program uses PTHREAD_MUTEX_INITIALIZER"
		" (or uninitialized mutex), which are not supported on this"
		" platform. sorry, you should change the program to explicitly"
		" call pthread_mutex_init\n");
	exit(1);
      }
      checked = 1;
    }
  }
#endif
}

int pthread_mutex_init(pthread_mutex_t *mutex,
		       const pthread_mutexattr_t *attr) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  assert(attr == 0 || sizeof(myth_mutexattr_t) <= sizeof(pthread_mutexattr_t));
  return myth_mutex_init_body((myth_mutex_t *)mutex, 
			      (const myth_mutexattr_t *)attr);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  check_mutex_initializer(mutex);
  return myth_mutex_destroy_body((myth_mutex_t *)mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  check_mutex_initializer(mutex);
  return myth_mutex_trylock_body((myth_mutex_t *)mutex);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  check_mutex_initializer(mutex);
  return myth_mutex_lock_body((myth_mutex_t *)mutex);
}

int pthread_mutex_unlock (pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  check_mutex_initializer(mutex);
  return myth_mutex_unlock_body((myth_mutex_t *)mutex);
}

/* TODO: move to myth_config.h */
#ifndef MYTH_SUPPORT_COND_INITIALIZER
#define MYTH_SUPPORT_COND_INITIALIZER 1
#endif

static inline void check_cond_initializer(pthread_cond_t * cond) {
#ifdef MYTH_SUPPORT_COND_INITIALIZER
  static int checked = 0;
  if (!checked) {
    /* it looks mtx has been initialized with = PTHREAD_COND_INITIALIZER,
       we can only hope this is equivaent to MYTH_COND_INITIALIZER.
       the strategy to overwrite it with MYTH_COND_INITIALIZER 
       is not guaranteed to work unless we make it atomic */
    /* TODO: can we do anything better */
    pthread_cond_t p = PTHREAD_COND_INITIALIZER;
    if (memcmp(&p, cond, sizeof(pthread_cond_t)) == 0) {
      myth_cond_t m = MYTH_COND_INITIALIZER;
      if (memcmp(&p, &m, sizeof(myth_cond_t)) != 0) {
	fprintf(stderr, 
		"error: it appears your program uses PTHREAD_COND_INITIALIZER"
		" (or uninitialized mutex), which are not supported on this"
		" platform. sorry, you should change the program to explicitly"
		" call pthread_mutex_init\n");
	exit(1);
      }
      checked = 1;
    }
  }
#endif
}

int pthread_cond_init(pthread_cond_t * cond,
		      const pthread_condattr_t *cond_attr) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  assert(sizeof(myth_condattr_t) <= sizeof(pthread_condattr_t));
  return myth_cond_init_body((myth_cond_t *)cond, 
			     (const myth_condattr_t *)cond_attr);
}

int pthread_cond_destroy(pthread_cond_t *cond) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  check_cond_initializer(cond);
  return myth_cond_destroy_body((myth_cond_t *)cond);
}

int pthread_cond_signal(pthread_cond_t * cond) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  check_cond_initializer(cond);
  return myth_cond_signal_body((myth_cond_t *)cond);
}

int pthread_cond_broadcast(pthread_cond_t * cond) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  check_cond_initializer(cond);
  return myth_cond_broadcast_body((myth_cond_t *)cond);
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  check_cond_initializer(cond);
  return myth_cond_wait_body((myth_cond_t *)cond, (myth_mutex_t *)mutex);
}

int pthread_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, 
			 unsigned int count) {
  assert(sizeof(myth_barrier_t) <= sizeof(pthread_barrier_t));
  assert(sizeof(myth_barrierattr_t) <= sizeof(pthread_barrierattr_t));
  return myth_barrier_init_body((myth_barrier_t *)barrier,
				(myth_barrierattr_t *)attr,
				count);
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
  assert(sizeof(myth_barrier_t) <= sizeof(pthread_barrier_t));
  return myth_barrier_destroy_body((myth_barrier_t *)barrier);
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
  assert(sizeof(myth_barrier_t) <= sizeof(pthread_barrier_t));
  return myth_barrier_wait_body((myth_barrier_t*)barrier);
}

