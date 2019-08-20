#pragma once
#ifndef MYTH_ADWS_SCHED_FUNC_H_
#define MYTH_ADWS_SCHED_FUNC_H_

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS

#include "myth_init.h"
#include "myth_context.h"
#include "myth_worker.h"
#include "myth_thread.h"
#include "myth_sched.h"
#include "myth_adws_sched.h"

#include "myth_workers_range_func.h"
#include "myth_context_func.h"
#include "myth_steal_range_func.h"

/*
 * search start/finish
 */

static inline void myth_adws_search_start(myth_workers_range_t workers_range, myth_running_env_t env) {
  myth_assert(env->is_searching == 0);
  myth_assert_workers_range(workers_range);

  myth_assert(workers_range.right_worker == env->rank);

  env->is_searching = 1;
  env->use_migration_q = 0;

  env->workers_range = workers_range;
}

static inline void myth_adws_search_finish(myth_running_env_t env) {
  myth_assert(env->is_searching == 1);

  env->is_searching = 0;

  myth_assert(env->cur_steal_range);
  if (!env->cur_steal_range->active) {
    myth_activate_steal_range(env->cur_steal_range);
  }

  if (env->workers_range.left_worker > env->rank + 1) {
    // notify search has finished too early (in case of the amount of work is incorrect or too coarse tasks)
    for (int w = env->rank + 1; w < env->workers_range.left_worker; w++) {
      myth_running_env_t target_env = &g_envs[w];
      target_env->cur_steal_range = env->cur_steal_range;
    }
  }
}

/*
 * search root thread get/set
 */

static inline myth_thread_t myth_adws_get_search_root_thread(myth_running_env_t env) {
  myth_thread_t ret = env->search_root_thread;
  if (ret) {
    env->search_root_thread = NULL;
    /* myth_rbarrier(); */
    myth_get_parent_steal_range(env);

    myth_assert(!env->is_searching);
    myth_adws_search_start(ret->workers_range, env);
  }
  return ret;
}

static inline void myth_adws_set_search_root_thread(myth_thread_t th, myth_running_env_t target_env) {
  myth_assert(target_env->search_root_thread == NULL);
  target_env->search_root_thread = th;
}

/*
 * thread pop/push
 */

static inline myth_thread_t myth_adws_pop_runnable_thread(myth_running_env_t env) {
  myth_thread_t next = myth_adws_get_search_root_thread(env);
  if (next) return next;

  if (env->use_migration_q) {
    next = myth_queue_pop(&env->migration_q);
    myth_assert(!next || next->workers_range.left_worker == next->workers_range.right_worker);
    if (!next) {
      next = myth_queue_pop(&env->runnable_q);
      env->use_migration_q = 0;
    }
  } else {
    next = myth_queue_pop(&env->runnable_q);
    if (!next) {
      next = myth_queue_pop(&env->migration_q);
      myth_assert(!next || next->workers_range.left_worker == next->workers_range.right_worker);
      if (next) env->use_migration_q = 1;
    }
  }
  return next;
}

static inline void myth_adws_push_thread(myth_thread_t th, myth_running_env_t env) {
  myth_thread_queue_t queue;
  if (env->use_migration_q) {
    myth_assert(th->workers_range.left_worker == th->workers_range.right_worker);
    queue = &env->migration_q;
  } else {
    queue = &env->runnable_q;
  }
  myth_queue_push(queue, th);
}

/*
 * migration back of search root thread
 */

MYTH_CTX_CALLBACK void myth_adws_migrate_search_root_thread_1(void* arg1, void* arg2, void* arg3) {
  myth_running_env_t target_env  = (myth_running_env_t)arg1;
  myth_thread_t      this_thread = (myth_thread_t)arg2;
  myth_running_env_t env         = (myth_running_env_t)arg3;

  this_thread->status = MYTH_STATUS_READY;
  myth_adws_set_search_root_thread(this_thread, target_env);
}

static inline void myth_adws_migrate_search_root_thread(myth_running_env_t target_env, myth_running_env_t env) {
  myth_thread_t this_thread = env->this_thread;
  myth_thread_t next_thread = myth_adws_pop_runnable_thread(env);
  myth_context_t next_context;
  if (next_thread) {
    next_thread->env = env;
    env->this_thread = next_thread;
    //Switch to next runnable thread
    next_context = &next_thread->context;
  } else {
    //Since there is no runnable thread, switch to scheduler
    next_context = &env->sched.context;
  }
  // Make it a function call instead of inlining it for safety.
  // Some registers (e.g. floating-point, SIMD registers) can be destroyed across an inlined context switch.
  myth_swap_context_withcall_s(&this_thread->context, next_context, myth_adws_migrate_search_root_thread_1,
                               (void*)target_env, (void*)this_thread, (void*)env);
}

/*
 * task group started
 */

static inline myth_running_env_t myth_adws_task_group_start(myth_workers_range_t* workers_range, double w_total, myth_running_env_t env) {
  if (!g_sched_adws) {
    // This task group should be the root task group
    g_sched_adws = 1;

    myth_thread_t this_thread = env->this_thread;
    myth_assert_workers_range(this_thread->workers_range);
    g_root_workers_range = this_thread->workers_range;

    // When a search starts from the root thread, the rank of the worker must be 0
    if (env->rank != 0) {
      myth_running_env_t target_env = &g_envs[0];
      myth_adws_migrate_search_root_thread(target_env, env);
      // switched to worker 0
      env = target_env;
    } else {
      myth_adws_search_start(this_thread->workers_range, env);
    }
    myth_assert(myth_get_current_env()->rank == 0);
    myth_assert(myth_get_current_env()->is_searching == 1);
  }

  if (env->ready_to_activate_steal_range) {
    // Being ready_to_activate_steal_range == 1 means that in the previous task group, this flag is set.
    // Because this worker reached the next task group here, unset this flag.
    env->ready_to_activate_steal_range = 0;
  }

  myth_assert(w_total > 0);
  env->total_work = w_total;

  if (env->is_searching) {
    // set a new steal range at the beginning of a task group.
    // this steal range is popped when the task group is finished.
    myth_set_new_steal_range(env->workers_range, env);
    myth_assert_workers_range(env->workers_range);
    *workers_range = env->workers_range;
  } else {
    // dummy workers range
    myth_workers_range_t dummy_workers_range;
    dummy_workers_range.left_worker  = env->rank;
    dummy_workers_range.right_worker = env->rank;
    *workers_range = dummy_workers_range;
  }
  return env;
}

// the parameter workers_range is passed from task group
static inline void myth_adws_task_group_finish(const myth_workers_range_t workers_range, myth_running_env_t env) {
  // if this thread is non-search-root thread, immediately return
  if (workers_range.left_worker == workers_range.right_worker) return;

  myth_assert_workers_range(workers_range);
  myth_thread_t this_thread = env->this_thread;

  int is_root_task_group = myth_is_same_workers_range(g_root_workers_range, workers_range);

  // scan steal range to determine whether or not to start search from here
  int updated = myth_scan_steal_range(env);
  if (!is_root_task_group && (updated || !myth_is_same_workers_range(env->cur_steal_range->workers_range, workers_range))) {
    // if any ancestor has already been activated, don't migrate back the search-root thread and don't start search
    this_thread->workers_range.left_worker  = env->rank;
    this_thread->workers_range.right_worker = env->rank;
    return;
  }

  this_thread->workers_range = workers_range;

  if (workers_range.right_worker != env->rank) {
    // Migration back: return this thread (search-root thread) to its owner
    myth_assert(0 <= workers_range.right_worker && workers_range.right_worker < g_attr.n_workers);
    myth_running_env_t target_env = &g_envs[workers_range.right_worker];

    myth_adws_migrate_search_root_thread(target_env, env);
    // The search-root thread must be executed by the target worker (never be stolen)
    env = target_env;
    myth_scan_steal_range(env);
  } else {
    // This worker is the owner of this search-root thread, so immediately start search without migration back
    myth_adws_search_start(workers_range, env);
  }

  // Now, this search-root thread must be executed by the owner worker
  myth_assert(workers_range.right_worker == env->rank);

  if (is_root_task_group) {
    // This task group is the root task group
    myth_assert(g_sched_adws);
    g_sched_adws = 0;
    myth_assert(env->is_searching);
    myth_adws_search_finish(env);
  }

  myth_steal_range_t* steal_range = env->cur_steal_range;
  if (myth_is_same_workers_range(steal_range->workers_range, workers_range)) {
    // pop the current steal range
    myth_assert(steal_range->parent);
    env->cur_steal_range = steal_range->parent;
    myth_remove_steal_range(steal_range, env);
    // Don't activate the steal range of the parent task group yet. If this thread finishes immediately
    // without generating any task groups, the parent steal range is activated at the time.
    env->ready_to_activate_steal_range = 1;
  }
}

/*
 * cleanup
 */

static inline void myth_adws_cleanup_reached(myth_running_env_t env) {
  if (env->is_searching == 1) {
    myth_adws_search_finish(env);
  }
  if (env->ready_to_activate_steal_range) {
    // This worker reached the end of this thread without generating task groups since the previous task group,
    // so activate the steal range of the parent task group.
    env->ready_to_activate_steal_range = 0;
    myth_activate_steal_range(env->cur_steal_range);
  }
}

//Switch to next_thread
MYTH_CTX_CALLBACK void myth_adws_entry_point_cleanup_1(void* arg1, void* arg2, void* arg3) {
  myth_running_env_t env         = (myth_running_env_t)arg1;
  myth_thread_t      this_thread = (myth_thread_t)arg2;
  (void)arg3;
  (void)env;

  myth_assert(!this_thread->detached);
  this_thread->status = MYTH_STATUS_FREE_READY2;
  myth_spin_unlock_body(&this_thread->lock);

  // do not free a stack object
}

static inline void myth_adws_entry_point_cleanup(myth_thread_t this_thread) {
  myth_running_env_t env = this_thread->env;
  myth_adws_cleanup_reached(env);
  myth_context_t next_context = myth_entry_point_cleanup_impl(this_thread);
  myth_set_context_withcall(next_context, myth_adws_entry_point_cleanup_1,
                            (void*)env, (void*)this_thread, NULL);
}

/*
 * entry point
 */

static void __attribute__((unused)) myth_adws_entry_point(void) {
  myth_running_env_t env         = myth_get_current_env();
  myth_thread_t      this_thread = env->this_thread;
  //Execute a thread function
  this_thread->result = (*(this_thread->entry_func))(this_thread->result);
  myth_adws_entry_point_cleanup(this_thread);
}

/*
 * create
 */

MYTH_CTX_CALLBACK void myth_adws_create_1(void* arg1, void* arg2, void* arg3) {
  myth_running_env_t env         = (myth_running_env_t)arg1;
  myth_func_t        fn          = (myth_func_t)arg2;
  myth_thread_t      new_thread  = (myth_thread_t)arg3;
  myth_thread_t      this_thread = env->this_thread;
  // Push current thread to runqueue
  myth_adws_push_thread(this_thread, env);
  env->this_thread = new_thread;
  // Call entry point function
  new_thread->result = (*fn)(new_thread->result);

  myth_adws_entry_point_cleanup(new_thread);
}

static inline int myth_adws_create_ex_impl(myth_thread_t*      id,
                                           myth_thread_attr_t* attr,
                                           myth_func_t         func,
                                           void*               arg,
                                           double              work,
                                           myth_running_env_t  env) {
  myth_thread_t new_thread = myth_create_new_thread(attr, arg, env);
  size_t stk_size = new_thread->stack_size - sizeof(void*) * 2;
  int child_first;

  if (env->is_searching) {
    // search
    double worker_amount = myth_size_of_workers_range(env->workers_range);
    double left_workers  = work / env->total_work * worker_amount;

    myth_workers_range_t left_workers_range, right_workers_range;
    myth_divide_workers_range(env->workers_range, &left_workers_range, &right_workers_range, left_workers);

    int boundary_worker = left_workers_range.right_worker;
    new_thread->workers_range = left_workers_range;

    if (boundary_worker == env->rank) {
      /* go to left */
      env->workers_range = left_workers_range;
      child_first = 1;
    } else {
      /* go to right */
      env->total_work -= work;
      if (env->total_work < 0) env->total_work = 0;
      env->workers_range = right_workers_range;
      // migrate the new task
      new_thread->entry_func = func;
      myth_make_context_voidcall(&new_thread->context, myth_adws_entry_point, new_thread->stack, stk_size);
      myth_running_env_t target_env = &g_envs[boundary_worker];
      // sanity check
      myth_assert(target_env->rank != env->rank);
      myth_assert(target_env->rank < g_attr.n_workers);

      if (left_workers_range.left_worker == boundary_worker) {
        myth_queue_pass(&target_env->migration_q, new_thread);
      } else {
        // target worker should start search from new_thread
        myth_assert(target_env->parent_steal_range == NULL);
        target_env->parent_steal_range = env->cur_steal_range;
        myth_wbarrier();
        myth_adws_set_search_root_thread(new_thread, target_env);
      }
      child_first = 0;
    }
  } else {
    // default
    new_thread->workers_range.left_worker  = env->rank;
    new_thread->workers_range.right_worker = env->rank;
    // child-first
    child_first = 1;
  }

  if (child_first) {
    myth_make_context_empty(&new_thread->context, new_thread->stack, stk_size);
    myth_swap_context_withcall(&env->this_thread->context, &new_thread->context,
                               myth_adws_create_1, (void*)env, (void*)func, (void*)new_thread);
  }

  id[0] = new_thread;
  return 0;
}

static inline int myth_adws_create_ex_first_body(myth_thread_t*        id,
                                                 myth_thread_attr_t*   attr,
                                                 myth_func_t           func,
                                                 void*                 arg,
                                                 double                work,
                                                 double                w_total,
                                                 myth_workers_range_t* workers_range) {
  myth_running_env_t env = myth_get_current_env();
  myth_running_env_t new_env = myth_adws_task_group_start(workers_range, w_total, env);
  return myth_adws_create_ex_impl(id, attr, func, arg, work, new_env);
}

static inline int myth_adws_create_ex_body(myth_thread_t*      id,
                                           myth_thread_attr_t* attr,
                                           myth_func_t         func,
                                           void*               arg,
                                           double              work) {
  myth_running_env_t env = myth_get_current_env();
  return myth_adws_create_ex_impl(id, attr, func, arg, work, env);
}

/*
 * join
 */

static inline void myth_adws_join_reached(myth_running_env_t env) {
  if (env->is_searching == 1) {
    myth_adws_search_finish(env);
  }
}

static inline void myth_adws_join_completed(myth_thread_t th, void** result) {
  if (result != NULL) {
    *result = th->result;
  }
  // do not free a thread object
}

static inline int myth_adws_join_body(myth_thread_t th, void** result) {
  myth_running_env_t env = myth_get_current_env();
  myth_adws_join_reached(env);
  myth_join_body_impl(th, env);
  myth_adws_join_completed(th, result);
  return 0;
}

static inline int myth_adws_join_last_body(myth_thread_t th, void** result, const myth_workers_range_t workers_range) {
  myth_running_env_t env = myth_get_current_env();
  myth_adws_join_reached(env);
  // myth_join_body_impl should NOT be inlined because some registers are not saved across a context switch in it.
  // In this case, floating-point variables in workers_range are sometimes saved into %xmm registers, and their value
  // can be destroyed by context switching. We can avoid this problem by making myth_join_body_impl an usual function
  // call. %xmm registers are safely saved across a function call by the compiler.
  myth_running_env_t joined_env = myth_join_body_impl_noinline(th, env);
  myth_adws_join_completed(th, result);
  myth_adws_task_group_finish(workers_range, joined_env);
  return 0;
}

/*
 * settings
 */

static inline int myth_adws_get_stealable_body() {
  return g_adws_stealable;
}

static inline void myth_adws_set_stealable_body(int flag) {
  g_adws_stealable = flag;
}

#endif

#endif	/* MYTH_ADWS_SCHED_FUNC_H_ */
