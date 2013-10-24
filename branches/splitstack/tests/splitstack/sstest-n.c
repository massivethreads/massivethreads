#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <myth.h>
#include <sys/time.h>

double cur_time() {
  struct timeval tp[1];
  gettimeofday(tp, NULL);
  return tp->tv_sec + tp->tv_usec * 1.0e-6;
}

typedef struct fib_arg {
  myth_thread_t tid;
  int n;
  int x;
} fib_arg, * fib_arg_t;

void __attribute__((noinline)) recursion(int cnt)
{
  volatile char dummy[1024];
  memset((char*)dummy,0,1024);
  cnt--;
  if (cnt>0){
    recursion(cnt);
  }
  memset((char*)dummy,0,1024);
}

int rec_cnt=0;

void * fib(void * a_) {
  //recursion(rec_cnt);
  volatile int arr[128];
  fib_arg_t a = a_;
  int n = a->n;
  unsigned int seed = (unsigned int)n;
  int i;
  for (i=0;i<128;i++){
    arr[i]=rand_r(&seed);
  }
  if (n < 2) {
    a->x = 1;
  } else {
    fib_arg c[2] = { { 0, n - 1, 0 }, 
		     { 0, n - 2, 0 } };
    c[0].tid=myth_create(fib, &c[0]);
    c[1].tid=myth_create(fib, &c[1]);
    recursion(rec_cnt);
    myth_join(c[0].tid, NULL);
    myth_join(c[1].tid, NULL);
    a->x = c[0].x + c[1].x;
  }
  seed = n;
  for (i=0;i<128;i++){
    if (arr[i] != rand_r(&seed)){
      fprintf(stderr,"Error: stack broken\n");
      assert(0);
    }
  }
  return a;
}

void *fn(void *arg)
{
  return NULL;
}

int main(int argc, char ** argv) {
  int n = (argc > 1 ? atoi(argv[1]) : 10);
  rec_cnt = (argc > 2 ? atoi(argv[2]) : 0);
  fib_arg a = { 0, n, 0 };
  double t0,t1;
  t0 = cur_time();
  a.tid=myth_create(fib, &a);
  myth_join(a.tid, NULL);
  t1 = cur_time();
  fprintf(stderr,"fib(%d) = %d\n", a.n, a.x);
  fprintf(stderr,"stack cleared for each recursion: %d KB\n", rec_cnt);
  printf("Elapsed time: %lf sec\n",t1-t0);
  return 0;
}
