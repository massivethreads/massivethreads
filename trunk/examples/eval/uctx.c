/* Measure the overhead of ucontext routines */

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;
static uint64_t t0, t1, t2, t3, t4, t5, t6, t7;

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
  t3 = get_rdtsc();
  printf("func1: started\n");
  printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
  t4 = get_rdtsc();
  if (swapcontext(&uctx_func1, &uctx_func2)  == -1)
    handle_error("swapcontext");
  t6 = get_rdtsc();
  printf("func1: returning\n");
}

static void func2(void)
{
  t1 = get_rdtsc();
  printf("func2: started\n");
  printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
  t2 = get_rdtsc();
  if (swapcontext(&uctx_func2, &uctx_func1) == -1)
    handle_error("swapcontext");
  t5 = get_rdtsc();
  printf("func2: returning\n");
}

int main(int argc, char *argv[])
{
  char func1_stack[16384];
  char func2_stack[16384];
  
  t0 = get_rdtsc();
  if (getcontext(&uctx_func1) == -1)
    handle_error("getcontext");
  t1 = get_rdtsc();
  uctx_func1.uc_stack.ss_sp = func1_stack;
  uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
  uctx_func1.uc_link = &uctx_main;
  makecontext(&uctx_func1, func1, 0);
  t2 = get_rdtsc();
  printf("main: getcontext: %llu, makecontext: %llu\n", t1 - t0, t2 - t1);
  
  t0 = get_rdtsc();
  if (getcontext(&uctx_func2) == -1)
    handle_error("getcontext");
  t1 = get_rdtsc();
  uctx_func2.uc_stack.ss_sp = func2_stack;
  uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
  uctx_func2.uc_link = &uctx_func1;
  makecontext(&uctx_func2, func2, 0);
  t2 = get_rdtsc();
  printf("main: getcontext: %llu, makecontext: %llu\n", t1 - t0, t2 - t1);

  printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
  t0 = get_rdtsc();
  if (swapcontext(&uctx_main, &uctx_func2) == -1)
    handle_error("swapcontext");
  t7 = get_rdtsc();
  
  printf("main: swapcontext: %llu, %llu, %llu, %llu\n", t1-t0, t3-t2, t5-t4, t7-t6);
  printf("main: exiting\n");
  exit(EXIT_SUCCESS);
}
