/*
 * tmfunc.c
 * Test timing function on different platforms
 *
 * by Nan Dun <dun@logos.ic.i.u-tokyo.ac.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

static inline uint64_t rdtsc()
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

int main(int argc, char **argv)
{	
	uint64_t t0, t1;
	int secs, i;
	
	if (argc < 2) {
		printf("usage: %s SECS\n", argv[0]);
		exit(0);
	}
	secs = atoi(argv[1]);

	t0 = t1 = 0;

	t0 = rdtsc();
//	for (i = 0; i < 10000000; i++)
//		secs += i;
	sleep(1);
	t1 = rdtsc();

	printf("ticks:%lu (%lu-%lu)\n", t1-t0, t1, t0);
	printf("%u\n", secs);

	exit(0);
}
