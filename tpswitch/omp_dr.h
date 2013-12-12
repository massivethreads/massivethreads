/* 
 * OpenMP + dag_recorder
 */

#pragma once
#include <dag_recorder.h>

#define do_pragma(x)               _Pragma( #x )
//#define pragma_omp(x)              do_pragma(omp x)
#define pragma_omp(x)

#define pragma_omp_task_no_prof(options, statement) \
  pragma_omp(task options) do { statement; } while(0)

#define pragma_omp_taskwait_no_prof pragma_omp(taskwait)

#define pragma_omp_task_with_prof(options, statement) do { \
    dr_dag_node * __c__;				   \
    dr_dag_node * __t__ = dr_enter_create_task(&__c__);	   \
    pragma_omp(task options) do {			   \
      dr_start_task(__c__);				   \
      statement;					   \
      dr_end_task();					   \
    } while(0);						   \
    dr_return_from_create_task(__t__);			   \
  } while (0)

#define pragma_omp_taskwait_with_prof do {		       \
    dr_dag_node * __t__ = dr_enter_wait_tasks();	       \
    pragma_omp(taskwait);				       \
    dr_return_from_wait_tasks(__t__);			       \
  } while(0)

#if DAG_RECORDER>=2

#define pragma_omp_task(options, statement)	\
  pragma_omp_task_with_prof(options, statement)
#define pragma_omp_taskwait pragma_omp_taskwait_with_prof

#else

#define pragma_omp_task(options, statement)	\
  pragma_omp_task_no_prof(options, statement)
#define pragma_omp_taskwait pragma_omp_taskwait_no_prof


#endif

