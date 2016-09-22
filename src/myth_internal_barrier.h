/* 
 * myth_internal_barrier.h
 */
#pragma once
#ifndef MYTH_INTERNAL_BARRIER_H_
#define MYTH_INTERNAL_BARRIER_H_

#include "myth_config.h"

typedef struct {
  pthread_mutex_t mutex[1];
  pthread_cond_t cond[1];
  int n_threads;
  int phase;
  int cur[2];
} myth_internal_barrier_t;

void myth_internal_barrier_init(myth_internal_barrier_t * b, int n);
void myth_internal_barrier_destroy(myth_internal_barrier_t * b);
void myth_internal_barrier_wait(myth_internal_barrier_t * b);

#endif	/* MYTH_INTERNAL_BARRIER_H_ */
