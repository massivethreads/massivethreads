/* Measure the overhead of setjmp/longjmp routines */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>

static jmp_buf buf_main, buf_func1, buf_func2;
static uint64_t t0, t1, t2, t3, t4, t5;

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

static inline uint64_t get_rdtsc(void)
{
#if defined __i386__ || defined __x86_64__
  uint32_t hi, lo;
  asm volatile("lfence\nrdtsc\n" : "=a"(lo),"=d"(hi));
  return ((uint64_t)hi)<<32 | lo;
#elif __sparc__
  uint64_t tick;
  asm volatile("rd %%tick, %0" : "=r" (tick));
  return tick;
#else
#error "get_rdtsc() not implemented"
  return 0;
#endif
}

static void func1(void)
{
  t2 = get_rdtsc();
  //printf("func1: started\n");
  //printf("func1: setjmp(buf_func1)\n");
  if (setjmp(buf_func1) == 0)
    longjmp(buf_func2, 1);
  t4 = get_rdtsc();
  //printf("func1: returning\n");
  longjmp(buf_main, 1);
}

static void func2(void)
{
  t1 = get_rdtsc();
  //printf("func2: started\n");
  //printf("func2: setjmp(buf_func2)\n");
  if (setjmp(buf_func2) == 0)
    func1();
  t3 = get_rdtsc();
  //printf("func2: returning\n");
  longjmp(buf_func1, 1);
}

int main(int argc, char *argv[])
{
  //printf("main: setjmp(buf_main)\n");
  t0 = get_rdtsc();
  if (setjmp(buf_main) == 0) 
    func2();
  t5 = get_rdtsc();
  printf("main: setjmp/longjmp: %llu, %llu, %llu, %llu, %llu\n", 
    t1-t0, t2-t1, t3-t2, t4-t3, t5-t4);
  //printf("main: exiting\n");
  exit(EXIT_SUCCESS);
}
