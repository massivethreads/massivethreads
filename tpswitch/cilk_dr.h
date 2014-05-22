/* 
 * cilk or cilkplus + dag recorder
 */

#pragma once
#if DAG_RECORDER>=2
#include <dag_recorder.h>
#endif

/* absorb diff between MIT Cilk and Cilkplus */
#if __CILK__			/* MIT Cilk */
#define sync__   sync
#else                           /* GCC/ICC Cilkplus */
#define sync__   _Cilk_sync
#endif



#define cilk_proc_start_with_prof			   \
  int __cilk_proc_start__ = dr_start_cilk_proc()

#define cilk_proc_return_with_prof(x) do {		   \
    __cilk_proc_start__++;				   \
    dr_end_task();					   \
    return x;						   \
  } while(0);

#define cilk_proc_void_return_with_prof do {		   \
    __cilk_proc_start__++;				   \
    dr_end_task();					   \
    return;						   \
  } while(0);

#define spawn_with_prof(function_call) do {				\
    dr_dag_node * __t__ = dr_enter_create_cilk_proc_task();		\
    function_call;							\
    dr_return_from_create_task(__t__);					\
  } while (0)

#define sync_with_prof do {				       \
    dr_dag_node * __t__ = dr_enter_wait_tasks();	       \
    sync__;						       \
    dr_return_from_wait_tasks(__t__);			       \
  } while(0)

#if DAG_RECORDER>=2

#define cilk_proc_start          cilk_proc_start_with_prof
#define cilk_proc_return(x)      cilk_proc_return_with_prof(x)
#define cilk_proc_void_return    cilk_proc_void_return_with_prof
#define spawn_(function_call)    spawn_with_prof(function_call)
#define sync_                    sync_with_prof

#if __CILK__ 			/* MIT Cilk */
#define dr_get_max_workers()     Cilk_active_size
#define dr_get_worker()          Self
#else
#define dr_get_max_workers()     __cilkrts_get_nworkers()
#define dr_get_worker()          __cilkrts_get_worker_number()
#endif

#else

#define cilk_proc_start          int __dummy_cilk_proc_start__ = 0
#define cilk_proc_return(x)      return x
#define cilk_proc_void_return    return
#define spawn_(function_call)    function_call
#define sync_                    sync__

#endif
