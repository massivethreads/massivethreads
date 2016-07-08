#ifndef MYTH_SYNC_H_
#define MYTH_SYNC_H_

#include <stddef.h>
#include <stdint.h>

#include "myth/myth_config.h"
#include "myth/myth_sleep_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* ----------- mutex ----------- */

#define DEFAULT_FESIZE 10

  typedef struct myth_mutexattr {
  } myth_mutexattr_t;

  typedef struct myth_mutex {
    myth_sleep_queue_t sleep_q[1];
    volatile long state;		/* n_waiters|locked */
  } myth_mutex_t;

#define MYTH_MUTEX_INITIALIZER { { MYTH_SLEEP_QUEUE_INITIALIZER }, 0 }

  /* ----------- condition variable ----------- */

  typedef struct myth_condattr {
  } myth_condattr_t;

  //Conditional variable data structure
  typedef struct myth_cond {
    myth_sleep_queue_t sleep_q[1];
  } myth_cond_t;

#define MYTH_COND_INITIALIZER { { MYTH_SLEEP_QUEUE_INITIALIZER } }

  /* ----------- barrier ----------- */

  typedef struct myth_barrierattr { } myth_barrierattr_t;
  typedef struct myth_barrier {
    long n_threads;
    volatile long state;
    //myth_sleep_queue_t sleep_q[1];
    myth_sleep_stack_t sleep_s[1];
  } myth_barrier_t;

#define MYTH_BARRIER_SERIAL_THREAD PTHREAD_BARRIER_SERIAL_THREAD

  /* ----------- join counter ----------- */

  //Join counter data structure
  typedef struct myth_join_counterattr { } myth_join_counterattr_t;
  typedef struct myth_join_counter {
    /* TODO: conserve space? */
    long n_threads;		/* const : number of decrements to see */
    int n_threads_bits;		/* number of bits to represent n_threads */
    long state_mask;		/* (1 << n_threads_bits) - 1 */
    volatile long state;
    myth_sleep_queue_t sleep_q[1];
  } myth_join_counter_t;

  /* ----------- full empty lock ----------- */

  //Full/empty lock data structure
  typedef struct myth_felockattr { } myth_felockattr_t;

  typedef struct myth_felock {
    myth_mutex_t mutex[1];
    myth_cond_t cond[2];
    int status;
  } myth_felock_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* MYTH_SYNC_H_ */
