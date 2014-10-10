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

#define cilk_begin				\
  int __cilk_begin__ = 0

#define cilk_return(x) do {				   \
    (void)__cilk_begin__;				   \
    return x;						   \
  } while(0)

#define cilk_return_t(type_of_x, x) cilk_return(x)

#define cilk_void_return do {				   \
    (void)__cilk_begin__;				   \
    return;						   \
  } while(0)

#define cilk_spawn
#define _Cilk_spawn
#define spawn


/* TBB, MassiveThredhads, Qthreads, Nanos++ */
#elif defined(__cplusplus) && (TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX)

#include <mtbb/task_group.h>

#define mk_task_group mtbb::task_group __tg__
#define create_task0(statement)			\
  __tg__.run_([=] { statement; }, __FILE__, __LINE__)
#define create_task1(s0,statement)		\
  __tg__.run_([=,&s0] { statement; }, __FILE__, __LINE__)
#define create_task2(s0,s1,statement)		\
  __tg__.run_([=,&s0,&s1] { statement; }, __FILE__, __LINE__)
#define create_taskc(callable)			\
  __tg__.run_(callable, __FILE__, __LINE__)
#define create_taskA(statement)			\
  __tg__.run_([&] { statement; }, __FILE__, __LINE__)
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
#define wait_tasks __tg__.wait_(__FILE__, __LINE__)

#define cilk_begin				\
  int __cilk_begin__ = 0

#define cilk_return(x) do {				   \
    (void)__cilk_begin__;				   \
    return x;						   \
  } while(0)

#define cilk_return_t(type_of_x, x) cilk_return(x)

#define cilk_void_return do {				   \
    (void)__cilk_begin__;				   \
    return;						   \
  } while(0)

#define cilk_spawn
#define _Cilk_spawn
#define spawn


/* No C++ or serial */
#elif TO_SERIAL || TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX

#if TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX
#warning "you define either TO_TBB, TO_MTHREAD, TO_MTHREAD_NATIVE, TO_QTHREAD, or TO_NANOX in your C program. create_task and other task parallel primitives are IGNORED in this file"
#endif

#include <tpswitch/serial_dr.h>

#define create_task0(statement)       create_task(statement)
#define create_task1(s0,statement)    create_task(statement)
#define create_task2(s0,s1,statement) create_task(statement)
#define create_taskA(statement)       create_task(statement)
#define call_task(statement)          do { statement; } while(0)
#define call_taskc(callable)          callable()

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

#define cilk_begin				\
  int __cilk_begin__ = 0

#define cilk_return(x) do {				   \
    (void)__cilk_begin__;				   \
    return x;						   \
  } while(0)

#define cilk_return_t(type_of_x, x) cilk_return(x)

#define cilk_void_return do {				   \
    (void)__cilk_begin__;				   \
    return;						   \
  } while(0)


#define cilk_spawn
#define _Cilk_spawn
#define spawn

/* Cilk */
#elif TO_CILK || TO_CILKPLUS

#if TO_CILK
#include <tpswitch/cilk_dr.cilkh>
#elif TO_CILKPLUS
#include <tpswitch/cilkplus_dr.h>
#endif

//#define mk_task_group int __mk_task_group__ __attribute__((unused)) = 0
#define mk_task_group 
#define create_task0(spawn_stmt)       spawn_(spawn_stmt)
#define create_task1(s0,spawn_stmt)    spawn_(spawn_stmt)
#define create_task2(s0,s1,spawn_stmt) spawn_(spawn_stmt)
#define create_taskA(spawn_stmt)       spawn_(spawn_stmt)
#if TO_CILK
#define create_taskc(callable)            spawn_(spawn callable())
#else
#define create_taskc(callable)            spawn_(_Cilk_spawn callable())
#endif
#define create_task_and_wait(spawn_stmt)			\
  do { create_taskA(spawn_stmt); wait_tasks; } while(0)
#define create_taskc_and_wait(callable)			\
  do { create_taskc(callable); wait_tasks; } while(0)
#define call_task(spawn_stmt)          create_task_and_wait(spawn_stmt)
#define call_taskc(callable)              create_taskc_and_wait(callable)   

#define wait_tasks sync_


#else
#error "neither TO_SERIAL, TO_OMP, TO_TBB, TO_CILK, TO_CILKPLUS, TO_MTHREAD, TO_MTHREAD_NATIVE, TO_QTHREAD, nor TO_NANOX defined"
#endif


/* common once we define create_taskXX etc. */

#define create_task0_if(X,E) \
do { if (X) { create_task0(E); } else { call_task(E); } } while(0)
#define create_task1_if(X,s0,E) \
do { if (X) { create_task1(s0,E); } else { call_task(E); } } while(0)
#define create_task2_if(X,s0,s1,E) \
do { if (X) { create_task2(s0,s1,E); } else { call_task(E); } } while(0)
#define create_taskA_if(X,E) \
do { if (X) { create_taskA(E); } else { call_task(E); } } while(0)
#define create_taskc_if(X,E) \
do { if (X) { create_taskc(E); } else { call_taskc(E); } } while(0)
#define create_task_and_wait_if(X,E) \
do { if (X) { create_task_and_wait(E); } else { call_task(E); } } while(0)
#define create_taskc_and_wait_if(X,E) \
do { if (X) { create_taskc_and_wait(E); } else { call_taskc(E); } } while(0)
