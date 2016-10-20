/* 
 * myth_internal_barrier.c
 */
#include <assert.h>

#include "myth_config.h"
#include "myth_real.h"
#include "myth_internal_barrier.h"

void myth_internal_barrier_init(myth_internal_barrier_t * b, int n) {
  real_pthread_mutex_init(b->mutex, 0);
  real_pthread_cond_init(b->cond, 0);
  b->n_threads = n;
  b->phase = 0;			/* 0 : 0 -> n; 1 : n -> 0 */
  b->cur[0] = b->cur[1] = 0;
}

void myth_internal_barrier_destroy(myth_internal_barrier_t * b) {
  real_pthread_mutex_destroy(b->mutex);
  real_pthread_cond_destroy(b->cond);
}

void myth_internal_barrier_wait(myth_internal_barrier_t * b) {
  real_pthread_mutex_lock(b->mutex);
  {
    int n_threads = b->n_threads;
    int phase = b->phase;
    assert(phase == 0 || phase == 1);
    assert(0 <= b->cur[phase]);
    assert(b->cur[phase] <= n_threads);
    b->cur[phase]++;
    if (b->cur[phase] < n_threads) {
      while (b->cur[phase] != n_threads) {
	real_pthread_cond_wait(b->cond, b->mutex);
      }
    } else {
      b->phase = 1 - phase;
      b->cur[b->phase] = 0;
      real_pthread_cond_broadcast(b->cond);
    }
  }
  real_pthread_mutex_unlock(b->mutex);
}


