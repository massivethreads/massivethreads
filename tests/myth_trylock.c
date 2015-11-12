
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <myth.h>

#define nthreads 1000
#define ninc_per_thread 1000

typedef struct {
  long a;
  long b;
  long r;
  long * p;
  myth_mutex_t * m;
} arg_t;
 
void * f(void * arg_) {
  arg_t * arg = arg_;
  long a = arg->a, b = arg->b;
  if (b - a == 1) {
    int i;
    for (i = 0; i < ninc_per_thread; ) {
      if (myth_mutex_trylock(arg->m) == 0) {
	arg->p[0]++;
	myth_mutex_unlock(arg->m);
	i++;
      }
    }
    arg->r = a;
  } else {
    long c = (a + b) / 2;
    arg_t cargs[2] = { { a, c, 0, arg->p, arg->m }, 
		       { c, b, 0, arg->p, arg->m } };
    myth_thread_t tid = myth_create(f, cargs);
    f(cargs + 1);
    myth_join(tid, 0);
    arg->r = cargs[0].r + cargs[1].r;
  }
  return 0;
}

int main(int argc, char ** argv) {
  (void)argc;
  (void)argv;
  myth_mutex_t m[1];
  myth_mutex_init(m, 0);
  long p[1] = { 0 };
  arg_t arg[1] = { { 0, nthreads, 0, p, m } };
  myth_thread_t tid = myth_create(f, arg);
  myth_join(tid, 0);

  if (arg->r == (nthreads - 1) * nthreads / 2
      && arg->p[0] == nthreads * ninc_per_thread) {
    printf("OK\n");
    return 0;
  } else {
    printf("NG\n");
    return 1;
  }
}

