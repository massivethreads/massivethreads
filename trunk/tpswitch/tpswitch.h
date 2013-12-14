/* 
 * tpswitch.h --- switch between task parallel systems
 */

/* 
   this file provides a _uniform_ task parallel
   interface on top of various underlying
   task parallel systems (TBB, OpenMP, etc.).

   provided macros are:

   (i)   mk_task_group
   (ii)  create_taskXX (various variations, see below)
   (iii) wait_tasks

   create_taskXX has the following variations

   (a) create_task0(statement) 
   (b) create_task1(s0,statement)
   (c) create_task2(s0,s1,statement)
   (d) create_taskc(callable)
   (e) create_taskA(statement)
   (f) call_task(statement)
   (g) call_taskc(callable)
   (h) create_task_and_wait(statement)
   (i) create_taskc_and_wait(callable)

   numbers 0,1,2 represent the number of shared
   variables.

   taskc variants will take a callable (a function 
   taking no arguments, or any object supporting 
   operator()) instead of a statement.

   call_task{c} is a serial version. on
   systems (such as MIT-cilk) still need 
   to spawn a task.

   create_task{c}_and_wait variants behave
   differently among systems.  on systems
   that support explicit task groups (TBB-like), 
   it simply executes the statement (or the
   callable) sequentially and then waits.
   you can change this behavior to the one
   that creates a task for the statement,
   by setting 
   create_task_and_wait_creates_task
   macro to one (e.g.,
   -Dcreate_task_and_wait_creates_task=1).
   this is sometimes useful to avoid stack
   overflows.

   on other systems (OpenMP and Cilk), it
   spawns a task and then waits.

   in all cases, if you set DAG_RECORDER=2,
   they are profiled by dag recorder.

 */


#pragma once

/* serial */
#if TO_SERIAL
#include <tpswitch/serial_dr.h>

#define create_task0(statement)       create_task(statement)
#define create_task1(s0,statement)    create_task(statement)
#define create_task2(s0,s1,statement) create_task(statement)
#define create_taskA(statement)       create_task(statement)
#define call_task(statement)          do { statement; } while(0)
#define call_taskc(callable)          callable()
#define create_task_and_wait(statement)			\
  do { call_taskA(statement); wait_tasks; } while(0)
#define create_taskc_and_wait(callable)			\
  do { call_taskc(callable); wait_tasks; } while(0)

/* OpenMP */
#if TO_OMP
#include <tpswitch/omp_dr.h>
#define mk_task_group 
#define create_task0(statement)			\
  pragma_omp_task(, statement)
#define create_task1(s0,statement)		\
  pragma_omp_task(shared(s0), statement)
#define create_task2(s0,s1,statement)		\
  pragma_omp_task(shared(s0,s1), statement)
#define create_taskc(callable)			\
  pragma_omp_taskc(,callable)
#define create_taskA(statement)			\
  pragma_omp_task(default(shared),statement)
#define call_task(statement)			\
  do { statement; } while(0)
#define call_taskc(callable)			\
  callable()
#define create_task_and_wait(statement)		\
  do { create_taskA(statement); wait_tasks; } while(0)
#define create_taskc_and_wait(C)		\
  do { create_taskc(C); wait_tasks; } while(0)
#define wait_tasks pragma_omp_taskwait

/* TBB, MassiveThredhads, Qthreads, Nanos++ */
#if TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX
#include <mtbb/task_group.h>

#define mk_task_group mtbb::task_group __tg__
#define create_task0(statement)			\
  __tg__.run([=] { statement; })
#define create_task1(s0,statement)		\
  __tg__.run([=,&s0] { statement; })
#define create_task2(s0,s1,statement)		\
  __tg__.run([=,&s0,&s1] { statement; })
#define create_taskc(callable)			\
  __tg__.run(callable)
#define create_taskA(statement)			\
  __tg__.run([&] { statement; })
#define call_task(statement)			\
  do { statement; } while(0)
#define call_taskc(callable)			\
  callable()

#if create_task_and_wait_creates_task
#define create_task_and_wait(statement) \
  do { create_taskA(statement); wait_tasks; } while(0)
#define create_taskc_and_wait(callable) \
  do { create_taskc(callable); wait_tasks; } while(0)
#else
#define create_task_and_wait(statement) \
  do { call_task(statement); wait_tasks; } while(0)
#define create_taskc_and_wait(callable) \
  do { call_taskc(callable); wait_tasks; } while(0)
#endif
#define wait_tasks __tg__.wait()

/* Cilk (not implemented yet) */
#if TO_CILK
#include <tpswitch/cilk_dr.h>

#else
#error "neither TO_SERIAL, TO_OMP, TO_TBB, TO_CILK, TO_CILKPLUS, TO_MTHREAD, TO_MTHREAD_NATIVE, TO_QTHREAD, nor TO_NANOX defined"
#endif

