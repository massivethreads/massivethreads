/* 
 * OpenMP + dag_recorder
 */

#pragma once
#include <dag_recorder.h>

#define mk_task_group_no_prof
#define create_task_no_prof(statement) statement
#define wait_tasks_no_prof

#define mk_task_group_with_prof

#define create_task_with_prof(statement) do { \
    dr_dag_node * __c__;				   \
    dr_dag_node * __t__ = dr_enter_create_task(&__c__);	   \
    dr_start_task(__c__);				   \
    statement;						   \
    dr_end_task();					   \
    dr_return_from_create_task(__t__);			   \
  } while(0)
  
#define wait_tasks_with_prof				\
  dr_return_from_wait_tasks(dr_enter_wait_tasks())

#if DAG_RECORDER>=2

#define mk_task_group  mk_task_group_with_prof
#define create_task(s) create_task_with_prof(s)
#define wait_tasks     wait_tasks_with_prof

#else

#define mk_task_group  mk_task_group_no_prof
#define create_task(s) create_task_no_prof
#define wait_tasks     wait_tasks_no_prof


#endif

