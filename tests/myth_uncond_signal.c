#include <assert.h>
#include <stdio.h>
#include <myth/myth.h>

/* 
   this program illustrates how to use unconditional variable (myth_uncond_t).

   a producer thread and a consumer thread communicate values
   through a single variable with a value presense/absence bit + a waiter
   presence/absense bit.
   a myth_uncond_t is used to block when a thread cannot proceed.

   bit[0] : value presence bit
   bit[1] : waiter presence bit
 */

enum {
  status_full = 1,
  status_sleeping = 2,
};

void sleep_exit(long sec) {
  sleep(sec);
  exit(1);
}

int put(volatile long * p, long x, myth_uncond_t * u, long n_tries) {
  for (long i = 0; i < n_tries; i++) {
    long p_old = *p;
    if (p_old & status_full) {
      /* full. sleep if I am ahead of the consumer */
      assert((p_old & status_sleeping) == 0);
      if (__sync_bool_compare_and_swap(p, p_old, p_old | status_sleeping)) {
	myth_uncond_wait(u);
      }
    } else {
      /* empty. make it full, no one sleeping */
      if (__sync_bool_compare_and_swap(p, p_old, (x << 2) | status_full)) {
	if (p_old & status_sleeping) {
	  /* consumer is sleeping. wake up */
	  myth_uncond_signal(u);
	}
	return 1;
      }
    }
  }
  fprintf(stderr, "could not put after %ld trials. something must be wrong\n",
	  n_tries);
  sleep_exit(1);
  return 0;
}

long get(volatile long * p, myth_uncond_t * u, long n_tries) {
  for (long i = 0; i < n_tries; i++) {
    long p_old = *p;
    if (p_old & status_full) {
      /* full. make it empty, no one sleeping */
      if (__sync_bool_compare_and_swap(p, p_old, 0)) {
	if (p_old & status_sleeping) {
	  /* consumer is sleeping. wake up */
	  myth_uncond_signal(u);
	}
	return (p_old >> 2);
      }
    } else {
      /* empty. sleep if I am ahead of the producer */
      assert((p_old & status_sleeping) == 0);
      if (__sync_bool_compare_and_swap(p, p_old, p_old | status_sleeping)) {
	myth_uncond_wait(u);
      }
    }
  }
  fprintf(stderr, "could not get after %ld trials. something must be wrong\n",
	  n_tries);
  sleep_exit(1);
  return -1;
}

long producer(volatile long * p, long n, myth_uncond_t * u, long n_tries) {
  for (long i = 0; i < n; i++) {
    put(p, i, u, n_tries);
  }
  return 1;			/* OK */
}

long consumer(volatile long * p, long n, myth_uncond_t * u, long n_tries) {
  for (long i = 0; i < n; i++) {
    long x = get(p, u, n_tries);
    assert(x == i);
  }
  return 1;			/* OK */
}

typedef struct {
  long n;
  long n_tries;
  volatile long * p;
  myth_uncond_t * u;
  long rank;
  myth_thread_t tid;
  void * ret;
} thread_arg_t;

void * producer_or_consumer(void * arg_) {
  thread_arg_t * a = (thread_arg_t *)arg_;
  if (a->rank == 0) {
    return (void *)producer(a->p, a->n, a->u, a->n_tries);
  } else if (a->rank == 1) {
    return (void *)consumer(a->p, a->n, a->u, a->n_tries);
  } else {
    assert(0);
  }
}

int main(int argc, char ** argv) {
  long n        = (argc > 1 ? atol(argv[1]) :  100 * 1000);
  long n_tries  = (argc > 2 ? atol(argv[2]) : 1000 * 1000 * 1000);
  volatile long p[1] = { 0 };
  myth_uncond_t u[1] = { { 0 } };
  thread_arg_t args[2];
  for (int rank = 0; rank < 2; rank++) {
    thread_arg_t * a = &args[rank];
    a->n = n;
    a->n_tries = n_tries;
    a->p = p;
    a->u = u;
    a->rank = rank;
    a->tid = myth_create(producer_or_consumer, a);
    a->ret = 0;
  }
  long ok = 0;
  for (int rank = 0; rank < 2; rank++) {
    myth_join(args[rank].tid, &args[rank].ret);
    if (args[rank].ret) ok++;
  }
  if (ok == 2) {
    printf("OK\n");
  }
  return 0;
}
