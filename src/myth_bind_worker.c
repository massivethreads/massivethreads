/* 
 * myth_bind_worker.c
 */
#include <assert.h>
#include <ctype.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "myth_config.h"
#include "myth_bind_worker.h"
#include "myth_real.h"

#if defined(CPU_SETSIZE)
#define N_MAX_CPUS CPU_SETSIZE
#else
#define N_MAX_CPUS 1024
#endif

static int myth_cpu_list[N_MAX_CPUS];
static int worker_cpu[N_MAX_CPUS];
static int n_available_cpus = -1;

/* 
   When environment variable MYTH_CPU_LIST is set,
   it enforces a worker to cpu assignment.
   for example, if MYTH_CPU_LIST=3,5,7,9 
   then 
   worker 0 is bound to 3, 
   worker 1 is bound to 5, 
   worker 2 is bound to 7, 
   worker 3 is bound to 9,
   worker 4 is bound to 3, 
   worker 5 is bound to 5, 
   worker 6 is bound to 7, 
   worker 7 is bound to 9,
   ...
*/

/* character stream */
typedef struct char_stream {
  char * a;			/* underlying string */
  int i;			/* the current position */
  int ok_pos;			/* position successfully parsed so far */
} char_stream, * char_stream_t;

typedef struct int_list {
  int * a;
  int i;
  int n;
} int_list, * int_list_t;
 
/* make a character stream returning characters in A */
static char_stream_t init_char_stream(char_stream_t cs, char * a) {
  cs->a = a;
  cs->i = 0;
  cs->ok_pos = 0;
  return cs;
}

static int_list_t init_int_list(int_list_t il, int * a, int n) {
  il->a = a;
  il->i = 0;
  il->n = n;
  return il;
}

static int int_list_add(int_list_t il, int x) {
  int i = il->i;
  int n = il->n;
  if (i < n) {
    il->a[i] = x;
    il->i = i + 1;
    return 1;
  } else {
    return 0;
  }
}

static inline int cur_char(char_stream_t cs) {
  return cs->a[cs->i];
}

static inline int next_char(char_stream_t cs) {
  assert(cs->a[cs->i] != '\n');
  cs->i++;
  return cur_char(cs);
}

/* mark the current position as having been parsed successfully */
static inline void set_ok_pos(char_stream_t cs) {
  cs->ok_pos = cs->i;
}

static void parse_error(char_stream_t cs, char * msg) {
  int i;
  fprintf(stderr, "myth_parse_cpu_list: invalid resource list: %s\n", 
	  msg);
  fprintf(stderr, "  %s\n", cs->a);
  for (i = 0; i < 2 + cs->ok_pos; i++) fputc(' ', stderr);
  for (     ; i < 2 + cs->i; i++) fputc('^', stderr);
  fputc('\n', stderr);
}

/* get a non-negative number or return -1 */
static int parse_int(char_stream_t cs) {
  int x = 0;
  int n_digits = 0;
  while (isdigit(cur_char(cs))) {
    n_digits++;
    x = x * 10 + (cur_char(cs) - '0');
    next_char(cs);
  }
  if (n_digits == 0) { 
    parse_error(cs, "expected a digit"); 
    return -1; 
  }
  return x;
}

/* a, a-b, or a-b:c */
static int parse_range(char_stream_t cs, int_list_t il) {
  int a = parse_int(cs);
  if (a == -1) return 0;	/* NG */
  int b = a + 1;
  int c = 1;			/* stride */
  int x;
  if (cur_char(cs) == '-') {
    next_char(cs);
    b = parse_int(cs);
    if (b == -1) return 0;
    if (cur_char(cs) == ':') {
      next_char(cs);
      c = parse_int(cs);
      if (c == -1) return 0; /* NG */
    }
  } 
  for (x = a; x < b; x += c) {
    if (!int_list_add(il, x)) {
      parse_error(cs, 
		  "myth_parse_cpu_list: too many numbers in MYTH_CPU_LIST\n");
      return 0;
    }
  }
  return 1;			/* OK */
}

static int parse_range_list(char_stream_t cs, int_list_t il) {
  if (!parse_range(cs, il)) return 0;
  set_ok_pos(cs);
  while (cur_char(cs) == ',') {
    next_char(cs);
    if (!parse_range(cs, il)) return 0;
    set_ok_pos(cs);
  }
  if (cur_char(cs) != '\0') { 
    next_char(cs);
    parse_error(cs, "junk at the end of CPU list"); 
    return 0;
  }
  return 1;			/* OK */
}

/* entry: 
   parse_cpu_list("MYTH_CPU_LIST", a, n) 
   fills integer array a with CPU numbers
   specified in MYTH_CPU_LIST. n is the number
   of elements a can store.  if the number of
   CPUs listed in MYTH_CPU_LIST, then it returns
   -1.
*/
static int myth_parse_cpu_list(const char * var, int * a, int n) {
  char_stream cs[1];
  int_list il[1];
  char * s = getenv(var);
  init_int_list(il, a, n);
  if (s) {
    init_char_stream(cs, s);
    if (!parse_range_list(cs, il)) return -1;
  }
#if 0
  int i;
  for (i = 0; i < il->i; i++) {
    if (i > 0) printf(",");
    printf("%d", il->a[i]);
  }
  printf("\n");
#endif
  return il->i;
}

void myth_read_available_cpu_list(void) {
  int i;
  int n = myth_parse_cpu_list("MYTH_CPU_LIST", myth_cpu_list, CPU_SETSIZE);
  if (n == -1) {
    fprintf(stderr, "MYTH_CPU_LIST ignored\n");
  }
  /* initialize all cpusets to empty */
  for (i = 0; i < N_MAX_CPUS; i++){
    worker_cpu[i] = -1;
  }
  n_available_cpus = 0;

#if defined(HAVE_SCHED_GETAFFINITY)
  {
    cpu_set_t cset;
    /* get cpu set we can use */
    sched_getaffinity(getpid(), sizeof(cpu_set_t), &cset);
    if (n == 0) {
      /* MYTH_CPU_LIST unspecified. simply bind ith 
	 worker to ith available CPU */
      for (i = 0; i < N_MAX_CPUS; i++){
	if (CPU_ISSET(i, &cset)) {
	  worker_cpu[n_available_cpus] = i;
	  n_available_cpus++;
	}
      }
    } else {
      /* MYTH_CPU_LIST specified. bind ith 
	 worker to ith specified & available CPU */
      for (i = 0; i < n; i++){
	if (CPU_ISSET(myth_cpu_list[i], &cset)) {
	  worker_cpu[n_available_cpus] = myth_cpu_list[i];
	  n_available_cpus++;
	}
      }
    }
  }
#else  /* HAVE_SCHED_GETAFFINITY */
  if (n == 0) {
    for (i = 0; i < N_MAX_CPUS; i++){
      worker_cpu[n_available_cpus] = i;
      n_available_cpus++;
    }
  } else {
    for (i = 0; i < n; i++){
      worker_cpu[n_available_cpus] = myth_cpu_list[i];
      n_available_cpus++;
    }
  }
#endif	/* HAVE_SCHED_GETAFFINITY */
  
#if 1				/* debug */
  printf("%d available CPUs:", n_available_cpus);
  for (i = 0; i < n_available_cpus; i++) {
    printf(" ");
    printf("%d", worker_cpu[i]);
  }
  printf("\n");
#endif	/* debug */
}

static int myth_get_worker_cpu(int rank) {
  if (n_available_cpus <= 0) {
    myth_read_available_cpu_list();
    assert(n_available_cpus > 0);
  }
  return worker_cpu[rank % n_available_cpus];
}

//Return the number of CPU cores
int myth_get_cpu_num(void) {
  if (n_available_cpus <= 0) {
    myth_read_available_cpu_list();
    assert(n_available_cpus > 0);
  }
  return n_available_cpus;
}

void myth_bind_worker(int rank) {
#if HAVE_PTHREAD_AFFINITY_NP
  int cpu = myth_get_worker_cpu(rank);
  cpu_set_t cs;
  CPU_ZERO(&cs);
  CPU_SET(cpu, &cs);
  real_pthread_setaffinity_np(real_pthread_self(), sizeof(cpu_set_t), &cs);
#endif
}

