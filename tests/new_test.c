
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <myth/myth.h>

#if 0
int myth_reinit_mutex_PTHREAD_MUTEX_INITIALIZER(pthread_mutex_t * pm) {
  myth_mutex_t * m = (myth_mutex_t *)pm;
  volatile int * magic_p = (volatile int *)&m->magic;
  int magic = * magic_p;
  if (magic != myth_mutex_magic_no) {
    if (magic != myth_mutex_magic_no_initializing
	&& __sync_bool_compare_and_swap(magic_p, magic, myth_mutex_magic_no_initializing)) {
      printf("reinit\n");
      myth_mutex_t mi = MYTH_MUTEX_INITIALIZER;
      mi.magic = myth_mutex_magic_no_initializing;
      *m = mi;
      __sync_synchronize();
      *magic_p = myth_mutex_magic_no;
    } else {
      while (*magic_p == myth_mutex_magic_no_initializing) { }
      assert(*magic_p == myth_mutex_magic_no);
    }
  }

#if 0
  pthread_mutex_t pmi[1] = { PTHREAD_MUTEX_INITIALIZER };
  myth_mutex_t mmi[1] = { MYTH_MUTEX_INITIALIZER };
  printf("sizeof myth_mutex_t = %ld\n", sizeof(myth_mutex_t));
  printf("attr.type [%ld] = %d\n", (long)&((myth_mutex_t *)0)->attr.type, mmi->attr.type);
  printf("sleep_q->ilock->locked [%ld] = %d\n", (long)&((myth_mutex_t *)0)->sleep_q->ilock->locked, mmi->sleep_q->ilock->locked);
  printf("sleep_q->head [%ld] = %p\n", (long)&((myth_mutex_t *)0)->sleep_q->head, mmi->sleep_q->head);
  printf("sleep_q->tail [%ld] = %p\n", (long)&((myth_mutex_t *)0)->sleep_q->tail, mmi->sleep_q->tail);
  printf("state [%ld] = %ld\n", (long)&((myth_mutex_t *)0)->state, mmi->state);
#endif
  return 0;
}

int pthread_mutex_lock_x(pthread_mutex_t * m) {
  myth_reinit_mutex_PTHREAD_MUTEX_INITIALIZER(m);
  return pthread_mutex_lock(m);
}
#endif

typedef struct {
  long ninc_per_thread;
  long a;
  long b;
  long r;
  long * p;
  pthread_mutex_t * m;
} arg_t;

void * f(void * arg_) {
  arg_t * arg = (arg_t *)arg_;
  long a = arg->a, b = arg->b;
  long ninc_per_thread = arg->ninc_per_thread;
  if (b - a == 1) {
    int i;
    for (i = 0; i < ninc_per_thread; i++) {
      pthread_mutex_lock(arg->m);
      arg->p[0]++;
      pthread_mutex_unlock(arg->m);
    }
    arg->r = a;
  } else {
    long c = (a + b) / 2;
    arg_t cargs[2] = { { ninc_per_thread, a, c, 0, arg->p, arg->m }, 
		       { ninc_per_thread, c, b, 0, arg->p, arg->m } };
    pthread_t tid;
    pthread_create(&tid, 0, f, cargs);
    f(cargs + 1);
    pthread_join(tid, 0);
    arg->r = cargs[0].r + cargs[1].r;
  }
  return 0;
}

pthread_mutex_t m[1] = { PTHREAD_MUTEX_INITIALIZER };

int mainx(int argc, char ** argv) {
  size_t i;
  size_t sz = sizeof(pthread_mutex_t);
  char * p = (char *)m;
  printf("sizeof pthread_mutex_t = %ld\n", sz);
  for (i = 0; i < sz; i++) {
    printf("%d ", p[i]);
  }
  printf("\n");
  return 0;
}

int main(int argc, char ** argv) {
  long nthreads        = (argc > 1 ? atol(argv[1]) : 100);
  long ninc_per_thread = (argc > 2 ? atol(argv[2]) : 10000);

  long p[1] = { 0 };
  arg_t arg[1] = { { ninc_per_thread, 0, nthreads, 0, p, m } };
  pthread_t tid;

  pthread_mutex_init(m, 0);

  pthread_create(&tid, 0, f, arg);
  pthread_join(tid, 0);

  if (arg->r == (nthreads - 1) * nthreads / 2
      && arg->p[0] == nthreads * ninc_per_thread) {
    printf("OK\n");
    return 0;
  } else {
    printf("NG: p = %ld != nthreads * ninc_per_thread = %ld\n",
	   arg->p[0], nthreads * ninc_per_thread);
    return 1;
  }
}

