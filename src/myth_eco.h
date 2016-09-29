/* 
 * myth_eco.h
 */
#pragma once
#ifndef MYTH_ECO_H_
#define MYTH_ECO_H_

#include <linux/futex.h>
#include <limits.h>
#include <semaphore.h>

#include "myth_config.h"
#include "myth_worker.h"

/* TODO: probably merge them into one  */
#define MYTH_ECO_DEBUG 0
#define MYTH_ECO_MODE_DEBUG 0

typedef struct myth_sleeping_workers_queue_item {
  struct myth_sleeping_workers_queue_item * next;
  int *head_sem;
  int head_rank;
} myth_sleeping_workers_queue_item;

typedef struct {
  pthread_mutex_t lock;
  int n_sleepers;
  myth_sleeping_workers_queue_item * head;
  myth_sleeping_workers_queue_item * tail;
} myth_sleeping_workers_queue;

extern myth_sleeping_workers_queue g_sleeping_workers;
#ifdef TASK_NUM
extern int task_num;
#endif
extern struct myth_running_env *g_envs;
extern int g_eco_mode_enabled;

#if MYTH_ECO_MODE
static void myth_eco_sched_loop(myth_running_env_t env);
#endif
myth_thread_t myth_eco_steal(int rank);
myth_thread_t myth_eco_all_task_check(myth_running_env_t env);

void myth_sleep_1(void);
void myth_sleep_2(int num);
void myth_go_asleep(void);
int myth_wakeup_one(void);
void myth_wakeup_all(void);
void myth_wakeup_all_force(void);
void myth_eco_init(void);
void myth_eco_des(void);

int myth_sleeper_push(int *sem, int rank, int num);
myth_sleeping_workers_queue_item * myth_sleeper_pop(void);
int futex_wait( void *futex, int comparand );
int futex_wakeup_one( void *futex );
int futex_wakeup_n( void *futex, int n );
int futex_wakeup_all( void *futex );
int fetch_and_store(volatile void *ptr, int addend);

#endif /* MYTH_ECO_H_ */
