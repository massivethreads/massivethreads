#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <myth/myth.h>

#define max_n_keys 100
myth_key_t keys[max_n_keys];

typedef struct {
  long a;
  long b;
  long r;
  long n_keys;
} arg_t;
 
void * f(void * arg_) {
  arg_t * arg = (arg_t *)arg_;
  long a = arg->a, b = arg->b, n_keys = arg->n_keys;
  if (b - a == 1) {
    long i;
    unsigned short rg[3] = {
      (unsigned short)(     a  & 0xffff),
      (unsigned short)((a + 1) & 0xffff),
      (unsigned short)((a + 2) & 0xffff)
    };
    void * vals[n_keys];
    for (i = 0; i < n_keys; i++) {
      void * v = (void *)jrand48(rg);
      vals[i] = v;
      int r = myth_setspecific(keys[i], v);
      assert(r == 0);
    }
    for (i = 0; i < n_keys; i++) {
      void * v = myth_getspecific(keys[i]);
      assert(v == vals[i]);
    }
    arg->r = a;
  } else {
    long c = (a + b) / 2;
    arg_t cargs[2] = { { a, c, 0, n_keys }, { c, b, 0, n_keys } };
    myth_thread_t tid = myth_create(f, cargs);
    f(cargs + 1);
    myth_join(tid, 0);
    arg->r = cargs[0].r + cargs[1].r;
  }
  return 0;
}

double cur_time() {
  struct timeval tv[1];
  gettimeofday(tv, 0);
  return tv->tv_sec + tv->tv_usec * 1.0e-6;
}

int main(int argc, char ** argv) {
  long nthreads = (argc > 1 ? atol(argv[1]) : 100000);
  long n_keys = (argc > 2 ? atol(argv[2]) : 1);
  arg_t arg[1] = { { 0, nthreads, 0, n_keys } };
  long i;
  assert(n_keys <= max_n_keys);
  for (i = 0; i < n_keys; i++) {
    myth_key_create(&keys[i], 0);
  }
  for (i = 0; i < 3; i++) {
    double t0 = cur_time();
    myth_thread_t tid = myth_create(f, arg);
    myth_join(tid, 0);
    double t1 = cur_time();
    double dt = t1 - t0;
    if (arg->r == (nthreads - 1) * nthreads / 2) {
      printf("OK\n");
      printf("%ld thread creation/join in %.9f sec (%.3f per sec)\n",
	     nthreads, dt, nthreads / dt);
    } else {
      printf("NG\n");
      return 1;
    }
  }
  return 0;
}

