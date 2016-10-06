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
    //int ws_victim;
#if 0
#if MYTH_SCHED_LOOP_DEBUG
    myth_dprintf("env %p is trying to steal thread from %p...\n",env,busy_env);
#endif
#endif
    //ws_victim=busy_env->rank;
    //Try to steal thread
    next_run = myth_queue_take(&busy_env->runnable_q);
    if (next_run){
#if MYTH_SCHED_LOOP_DEBUG
      myth_dprintf("env %p is stealing thread %p from %p...\n",env,steal_th,busy_env);
#endif
      myth_assert(next_run->status==MYTH_STATUS_READY);
      //Change worker thread descriptor
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

static long * myth_steal_prob_table;

static myth_running_env_t
myth_env_choose_victim(myth_running_env_t e) {
  /* P[i] <= X < P[i+1] ==> i */
  int n = g_attr.n_workers;
  if (n == 1) {
    return NULL;
  } else {
    long * P = e->steal_prob;
    long x = nrand48(e->steal_rg);
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

int myth_scheduler_worker_init(int rank, int nw) {
  myth_running_env_t env = &g_envs[rank];
  env->steal_prob = &myth_steal_prob_table[rank * nw];
  env->steal_rg[0] = rank;
  env->steal_rg[1] = rank + 1;
  env->steal_rg[2] = rank + 2;
  return 0;
}

#endif	/* EXPERIMENTAL_SCHEDULER */
