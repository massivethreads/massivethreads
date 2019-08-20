#pragma once
#ifndef MYTH_STEAL_RANGE_H_
#define MYTH_STEAL_RANGE_H_

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS

#include "myth_sched.h"

typedef struct myth_steal_range {
  // for freelist
  struct myth_steal_range *next;

  struct myth_steal_range *parent;
  myth_workers_range_t workers_range;
  volatile int active;
} myth_steal_range_t;

static inline void myth_activate_steal_range(myth_steal_range_t* steal_range);
static inline void myth_remove_steal_range(myth_steal_range_t* steal_range, myth_running_env_t env);
static inline void myth_remove_steal_range_until(myth_steal_range_t* until, myth_running_env_t env);
static inline int myth_scan_steal_range(myth_running_env_t env);
static inline void myth_get_parent_steal_range(myth_running_env_t env);
static inline void myth_set_new_steal_range(myth_workers_range_t workers_range, myth_running_env_t env);

#endif

#endif	/* MYTH_STEAL_RANGE_H_ */
