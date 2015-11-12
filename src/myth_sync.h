#ifndef MYTH_SYNC_H_
#define MYTH_SYNC_H_

#include "myth_desc.h"
#include "myth_sleep_queue.h"

/* ----------- mutex ----------- */

#define DEFAULT_FESIZE 10

typedef struct myth_mutexattr {
} myth_mutexattr_t;

typedef struct myth_mutex {
  myth_sleep_queue_t sleep_q[1];
  volatile long state;		/* n_waiters|locked */
} myth_mutex_t;

/* ----------- condition variable ----------- */

typedef struct myth_condattr {
} myth_condattr_t;

//Conditional variable data structure
typedef struct myth_cond
{
  struct myth_thread **wl;
  int wsize;
  int nw;
  struct myth_thread *def_wl[DEFAULT_FESIZE];
}__attribute__((aligned(CACHE_LINE_SIZE))) myth_cond_t;

/* ----------- barrier ----------- */

typedef struct myth_barrierattr { } myth_barrierattr_t;
typedef struct myth_barrier
{
  int rest;
  int nthreads;
  struct myth_thread **th;
  myth_internal_lock_t lock;
} __attribute__((aligned(CACHE_LINE_SIZE))) myth_barrier_t;

#define MYTH_BARRIER_SERIAL_THREAD PTHREAD_BARRIER_SERIAL_THREAD

/* ----------- join counter ----------- */

//Join counter data structure
typedef struct myth_join_counterattr { } myth_join_counterattr_t;
typedef struct myth_join_counter
{
  struct myth_thread *th;
  intptr_t count;
  myth_internal_lock_t lock;
  int initial;
} __attribute__((aligned(CACHE_LINE_SIZE))) myth_join_counter_t;

/* ----------- full empty lock ----------- */

//Full/empty lock data structure
typedef struct myth_felockattr { } myth_felockattr_t;

typedef struct myth_felock
{
  myth_mutex_t lock[1];
  struct myth_thread **fl;//full-waiting list
  struct myth_thread **el;//empty-waiting list
  volatile int fe;//full/empty
  int fsize;
  int esize;
  int nf;
  int ne;
  struct myth_thread *def_fl[DEFAULT_FESIZE],*def_el[DEFAULT_FESIZE];
}__attribute__((aligned(CACHE_LINE_SIZE))) myth_felock_t;

#endif /* MYTH_SYNC_H_ */
