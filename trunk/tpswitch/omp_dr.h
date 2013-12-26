/* 
 * OpenMP + dag_recorder
 */

/* 
   this file provides macros with which users can
   easily turn on/off dag recorder for your OpenMP
   task parallel programs.

   provided macros are:

   (i) pragma_omp_task(option, statement)
   (ii) pragma_omp_taskc(option, callable)
   (iii) pragma_omp_taskwait

   they are respectively translated into
   #pragma omp task option
     statement

   #pragma omp task option
     callable()

   #pragma omp taskwait

   when DAG_RECORDER is set to a number >= 2,
   they insert instrumentation code for dag
   recorder.

   ideally we like to instrument OpenMP 
   programs written with the regular 
   pragma's, but I don't know how to do 
   it.  so we ask the programmer to write
   OpenMP fragment such as 

   #pragma omp task shared(x)
     x = foo();

   as
   
   pragma_omp_task(shared(x),
     x = foo());

 */


#pragma once
#include <dag_recorder.h>

#define do_pragma(x)               _Pragma( #x )
#define pragma_omp(x)              do_pragma(omp x)

#define pragma_omp_task_no_prof(options, statement) \
  pragma_omp(task options) do { statement; } while(0)

#define pragma_omp_taskc_no_prof(options, callable) \
  pragma_omp_task_no_prof(options, callable())

#define pragma_omp_taskwait_no_prof pragma_omp(taskwait)

#define pragma_omp_task_with_prof(options, statement) do { \
    dr_dag_node * __c__ = 0;				   \
    dr_dag_node * __t__ = dr_enter_create_task(&__c__);	   \
    pragma_omp(task options) do {			   \
      dr_start_task(__c__);				   \
      statement;					   \
      dr_end_task();					   \
    } while(0);						   \
    dr_return_from_create_task(__t__);			   \
  } while (0)

#define pragma_omp_taskc_with_prof(options, callable) \
  pragma_omp_task_with_prof(options, callable())

#define pragma_omp_taskwait_with_prof do {		       \
    dr_dag_node * __t__ = dr_enter_wait_tasks();	       \
    pragma_omp(taskwait);				       \
    dr_return_from_wait_tasks(__t__);			       \
  } while(0)

#if DAG_RECORDER>=2

#define pragma_omp_task(options, statement)	\
  pragma_omp_task_with_prof(options, statement)
#define pragma_omp_taskc(options, callable)	\
  pragma_omp_taskc_with_prof(options, callable)
#define pragma_omp_taskwait pragma_omp_taskwait_with_prof

#define dr_get_max_workers() (omp_in_parallel() ? omp_get_num_threads() : omp_get_max_threads())
#define dr_get_worker()      omp_get_thread_num()

#else

#define pragma_omp_task(options, statement)	\
  pragma_omp_task_no_prof(options, statement)
#define pragma_omp_taskc(options, callable)	\
  pragma_omp_taskc_no_prof(options, callable)
#define pragma_omp_taskwait pragma_omp_taskwait_no_prof


#endif

