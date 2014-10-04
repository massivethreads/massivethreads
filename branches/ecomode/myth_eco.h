#ifndef _MYTH_ECO_H_
#define _MYTH_ECO_H_

#include "myth_original_lib.h"
#include "myth_worker.h"
#include "myth_desc.h"
#include <linux/futex.h>
#include <limits.h>
#include <semaphore.h>

typedef struct sleep_queue {
  struct sleep_queue *next;
  struct sleep_queue *tail;
  int *head_sem;
  int head_rank;
}sleep_queue,*sleep_queue_t;


sleep_queue_t g_sleep_queue;
pthread_mutex_t *queue_lock;
int sleeper;
int task_num;
extern myth_running_env *g_envs;

static void myth_eco_sched_loop(myth_running_env_t env);
myth_thread_t myth_eco_steal(int rank);
myth_thread_t myth_eco_all_task_check(myth_running_env_t env);

void myth_sleep();
void myth_sleep_2(int num);
void myth_go_asleep();
int myth_wakeup_one();
void myth_wakeup_all();
void myth_eco_init();
void myth_eco_des();

int myth_sleeper_push(int *sem, int rank, int num);
sleep_queue_t myth_sleeper_pop();
int futex_wait( void *futex, int comparand );
int futex_wakeup_one( void *futex );
int futex_wakeup_n( void *futex, int n );
int futex_wakeup_all( void *futex );
int fetch_and_store(volatile void *ptr, int addend);

#endif /* _MYTH_ECO_H_ */