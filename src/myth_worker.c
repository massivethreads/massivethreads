/* 
 * myth_worker.c
 */

#include "myth_config.h"
#include "config.h"

#include "myth_worker.h"
#include "myth_worker_func.h"

#if EXPERIMENTAL_SCHEDULER
static myth_thread_t myth_steal_func_with_prob(int rank);
myth_steal_func_t g_myth_steal_func = myth_steal_func_with_prob;
#else
myth_thread_t myth_default_steal_func(int rank);
myth_steal_func_t g_myth_steal_func = myth_default_steal_func;
#endif

myth_steal_func_t myth_wsapi_set_stealfunc(myth_steal_func_t fn)
{
  myth_steal_func_t prev=g_myth_steal_func;
  g_myth_steal_func=fn;
  return prev;
}

extern myth_running_env_t g_envs;

myth_thread_t myth_default_steal_func(int rank) {
  myth_running_env_t env,busy_env;
  myth_thread_t next_run = NULL;
#if MYTH_WS_PROF_DETAIL
  uint64_t t0, t1;
  t0 = myth_get_rdtsc();
#endif
  //Choose a worker thread that seems to be busy
  env = &g_envs[rank];
  busy_env = myth_env_get_first_busy(env);
  if (busy_env){
    //Try to steal thread
    next_run = myth_queue_take(&busy_env->runnable_q);
    if (next_run){
#if MYTH_SCHED_LOOP_DEBUG
      myth_dprintf("env %p is stealing thread %p from %p...\n",env,steal_th,busy_env);
#endif
      myth_assert(next_run->status==MYTH_STATUS_READY);
    }
  }
#if MYTH_WS_PROF_DETAIL
  t1 = myth_get_rdtsc();
  if (g_sched_prof){
    env->prof_data.ws_attempt_count[busy_env->rank]++;
    if (next_run){
      env->prof_data.ws_hit_cycles += t1 - t0;
      env->prof_data.ws_hit_cnt++;
    }else{
      env->prof_data.ws_miss_cycles += t1 - t0;
      env->prof_data.ws_miss_cnt++;
    }
  }
#endif
  return next_run;
}

#if EXPERIMENTAL_SCHEDULER

static unsigned long * myth_steal_prob_table;

static myth_running_env_t
myth_env_choose_victim(myth_running_env_t e) {
  /* P[i] <= X < P[i+1] ==> i */
  int n = g_attr.n_workers;
  if (n == 1) {
    return NULL;
  } else {
    unsigned long * P = e->steal_prob;
    unsigned long x = (unsigned long)nrand48(e->steal_rg);
    if (P[n - 1] <= x) {
      return &g_envs[n - 1];
    } else {
      int a = 0; int b = n - 1;
      assert(P[a] <= x);
      assert(x < P[b]);
      while (b - a > 1) {
	int c = (a + b) / 2;
	if (P[c] <= x) {
	  a = c;
	} else {
	  b = c;
	}
	assert(P[a] <= x);
	assert(x < P[b]);
      }
      assert(0 <= a);
      assert(a < n - 1);
      assert(P[a] <= x);
      assert(x < P[a + 1]);
      assert(a != e->rank);
      return &g_envs[a];
    }
  }
}

static myth_thread_t myth_steal_func_with_prob(int rank) {
  myth_running_env_t env = &g_envs[rank];
  myth_running_env_t busy_env = myth_env_choose_victim(env);
  if (busy_env){
    myth_thread_t next_run = myth_queue_take(&busy_env->runnable_q);
    if (next_run){
      myth_assert(next_run->status == MYTH_STATUS_READY);
    }
    return next_run;
  } else {
    return NULL;
  }
}

#if 1
unsigned long * myth_read_prob_file(FILE * fp, int nw) {
  /* read a file describing the (relative) probability a worker chooses
     a victime
     
     N
     P0,0 P0,1 P0,2 ... P0,N-1
     P1,0 P1,1 P1,2 ... P1,N-1
          ...
     PN-1,0,        ... PN-1,N-1

  */
  /* nw is the number of workers used in this execution
     of the program; n_workers_in_file is the number of workers
     described in the file. they may be different. */
  int n_workers_in_file;
  if (fp) {
    char buf[100] = { 0 };
    char * s = fgets(buf, sizeof(buf), fp);
    n_workers_in_file = (s ? atoi(s) : nw);
  } else {
    /* no file. default (uniform distribution) */
    n_workers_in_file = nw;
  }
  /* p[i][j] is a relative probability that i chooses j as a victim */
  double * p = myth_malloc(nw * nw * sizeof(double));
  int i, j;
  for (i = 0; i < n_workers_in_file; i++) {
    for (j = 0; j < n_workers_in_file; j++) {
      if (fp) {
	if (i < nw && j < nw) {
	  int x = fscanf(fp, "%lf", &p[i * nw + j]);
	  assert(x == 1);
	} else {
	  /* skip */
	}
      } else {
	p[i * nw + j] = (i == j ? 0.0 : 1.0);
      }
    }
  }
  /* when n_workers_in_file < nw, we repeat */
  for (i = 0; i < nw; i++) {
    for (j = 0; j < nw; j++) {
      if (i >= n_workers_in_file && j >= n_workers_in_file) {
	int i_ = i % n_workers_in_file;
	int j_ = j % n_workers_in_file;
	p[i * nw + j] = p[i_ * nw + j_];
      }
    }
  }
  /* set diagonal lines to zero */
  for (i = 0; i < nw; i++) {
    p[i * nw + i] = 0.0;
  }

  for (i = 0; i < nw; i++) {
    double t = 0;
    for (j = 0; j < nw; j++) {
      t += p[i * nw + j];
    }
    for (j = 0; j < nw; j++) {
      p[i * nw + j] /= t;
    }
  }
  
  /* convert p into P, whose P[i][j] is 2^31 x the 
     probability that i chooses one of 0 ... j-1 as a victim.
     the probablity i steals from j is (P[i][j+1] - P[i][j]) / 2^31.
     the victim is chosen by drawing a random number x from [0,2^31],
     and find j s.t. P[i][j] <= x < P[i][j+1] (binary search)
  */
  unsigned long * P = myth_malloc(nw * nw * sizeof(long));
  for (i = 0; i < nw; i++) {
    double x = 0.0;
    for (j = 0; j < nw; j++) {
      P[i * nw + j] = x * (1UL << 31);
      x += p[i * nw + j];
    }
  }
  myth_free(p);
  return P;
}

int myth_scheduler_global_init(int nw) {
  char * prob_file = getenv("MYTH_PROB_FILE");
  if (prob_file) {
    FILE * fp = fopen(prob_file, "rb");
    if (!fp) { perror("fopen"); exit(1); }
    myth_steal_prob_table = myth_read_prob_file(fp, 0);
  } else {
    myth_steal_prob_table = myth_read_prob_file(0, nw);
  }
  return 0;
}

#else
int myth_scheduler_global_init(int nw) {
  long * P = myth_malloc(nw * nw * sizeof(long));
  double * q = myth_malloc(nw * sizeof(double));
  long i, j;
  for (i = 0; i < nw; i++) {
    double x = 0;
    for (j = 0; j < nw; j++) {
      q[j] = x;
      if (i != j) x += 1.0;
    }
    for (j = 0; j < nw; j++) {
      P[i * nw + j] = (long)((q[j] * (double)(1L << 31)) / x);
    }
  }
  myth_free(q);
  myth_steal_prob_table = P;
  return 0;
}
#endif

int myth_scheduler_worker_init(int rank, int nw) {
  myth_running_env_t env = &g_envs[rank];
  env->steal_prob = &myth_steal_prob_table[rank * nw];
  /* random seed */
  env->steal_rg[0] = rank;
  env->steal_rg[1] = rank + 1;
  env->steal_rg[2] = rank + 2;
  return 0;
}

#endif	/* EXPERIMENTAL_SCHEDULER */
