#pragma once
#ifndef MYTH_STEAL_RANGE_FUNC_H_
#define MYTH_STEAL_RANGE_FUNC_H_

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS

#include "myth_sched.h"
#include "myth_worker.h"
#include "myth_misc.h"

static inline void myth_activate_steal_range(myth_steal_range_t* steal_range) {
  myth_assert(steal_range);
  steal_range->active = 1;
}

static inline void myth_remove_steal_range(myth_steal_range_t* steal_range, myth_running_env_t env) {
  myth_assert(steal_range->workers_range.right_worker == env->rank);
  steal_range->active = 0;
  myth_freelist_push(&env->freelist_steal_range, steal_range);
}

static inline void myth_remove_steal_range_until(myth_steal_range_t* until, myth_running_env_t env) {
  myth_steal_range_t* steal_range = env->cur_steal_range;
  myth_assert(steal_range);
  while (steal_range != until && steal_range != NULL) {
    if (steal_range->workers_range.right_worker == env->rank) {
      myth_remove_steal_range(steal_range, env);
    }
    steal_range = steal_range->parent;
  }
}

// Returns 1 when current steal range is updated, otherwise 0
static inline int myth_scan_steal_range(myth_running_env_t env) {
  int ret = 0;
  myth_steal_range_t* steal_range = env->cur_steal_range->parent;
  while (steal_range != NULL) {
    if (steal_range->active) {
      myth_remove_steal_range_until(steal_range, env);
      env->cur_steal_range = steal_range;
      ret = 1;
    }
    steal_range = steal_range->parent;
  }
  myth_assert(env->cur_steal_range);
  return ret;
}

static inline void myth_get_parent_steal_range(myth_running_env_t env) {
  if (env->parent_steal_range) {
    myth_assert(env->parent_steal_range->workers_range.left_worker >= env->rank &&
        env->rank >= env->parent_steal_range->workers_range.right_worker);
    myth_remove_steal_range_until(NULL, env);
    env->cur_steal_range = env->parent_steal_range;
    env->parent_steal_range = NULL;
  }
}

static inline void myth_set_new_steal_range(myth_workers_range_t workers_range, myth_running_env_t env) {
  myth_steal_range_t* new_steal_range = (myth_steal_range_t*)myth_freelist_pop(&env->freelist_steal_range);
  if (!new_steal_range) {
    new_steal_range = (myth_steal_range_t*)myth_malloc(sizeof(struct myth_steal_range));
  }
  myth_steal_range_t* parent = env->cur_steal_range;
  myth_assert(parent != new_steal_range);
  myth_assert(workers_range.right_worker == env->rank);

  new_steal_range->workers_range = workers_range;
  new_steal_range->active        = 0;
  new_steal_range->parent        = parent;

  env->cur_steal_range = new_steal_range;
}

#endif

#endif	/* MYTH_STEAL_RANGE_FUNC_H_ */
