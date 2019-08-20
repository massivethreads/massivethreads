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

#if !defined(MTHREAD_DISABLE_ADWS)
#define MTHREAD_DISABLE_ADWS 0
#endif

#if !MTHREAD_DISABLE_ADWS
#include <mtbb/task_group.h>
#define mk_task_group_w(W)           mtbb::task_group __tg__(W)
#define create_task0_w(statement, W) __tg__.run([=] { statement; }, W)
#define create_taskA_w(statement, W) __tg__.run([&] { statement; }, W)
#define create_taskc_w(callable, W)  __tg__.run(callable, W)
#else
#define mk_task_group_w(W)           mk_task_group
#define create_task0_w(statement, W) create_task0(statement)
#define create_taskA_w(statement, W) create_taskA(statement)
#define create_taskc_w(callable, W)  create_taskc(callable)
#endif

/* OpenMP */
#if TO_OMP

#include <tpswitch/omp_dr.h>
#define mk_task_group 
#define create_task0(statement)			\
  pragma_omp_task(, statement)
#define create_task0_(statement, file, line)    \
  pragma_omp_task_(, statement, file, line)
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
#define wait_tasks_(file, line) pragma_omp_taskwait_(file, line)

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
#define mit_spawn
#define cilk
#define cilk_static static


/* TBB, MassiveThredhads, Qthreads, Nanos++ */
#elif defined(__cplusplus) && (TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX)

#include <mtbb/task_group.h>

#define mk_task_group mtbb::task_group __tg__
#define create_task0(statement)			\
  __tg__.run_([=] { statement; }, __FILE__, __LINE__)
#define create_task0_(statement, file, line)    \
  __tg__.run_([=] { statement; }, file, line)
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
#define wait_tasks_(file, line) __tg__.wait_(file, line)

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
#define mit_spawn
#define cilk
#define cilk_static static
#define pragma_omp_parallel_single(clause, S) do { S } while(0)


/* No C++ or serial */
#elif TO_SERIAL || TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX

#if TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX
#warning "you define either TO_TBB, TO_MTHREAD, TO_MTHREAD_NATIVE, TO_QTHREAD, or TO_NANOX in your C program. create_task and other task parallel primitives are IGNORED in this file"
#endif

#include <tpswitch/serial_dr.h>

#define create_task0(statement)              create_task(statement)
#define create_task0_(statement, file, line) create_task_(statement, file, line)
#define create_task1(s0,statement)           create_task(statement)
#define create_task2(s0,s1,statement)        create_task(statement)
#define create_taskA(statement)              create_task(statement)
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
#define mit_spawn
#define cilk
#define cilk_static static
#define pragma_omp_parallel_single(clause, S) do { S } while(0)


/* Cilk */
#elif TO_CILK || TO_CILKPLUS

#if TO_CILK

#include <tpswitch/cilk_dr.cilkh>
#define mk_task_group

#define create_task0(spawn_stmt)       spawn_(spawn_stmt)
#define create_task0_(spawn_stmt, file, line)       spawn__(spawn_stmt, file, line)
#define create_task1(s0,spawn_stmt)    spawn_(spawn_stmt)
#define create_task2(s0,s1,spawn_stmt) spawn_(spawn_stmt)
#define create_taskA(spawn_stmt)       spawn_(spawn_stmt)

#define create_taskc(callable)            spawn_(spawn callable())
#define create_taskc_(callable)            spawn_(spawn callable())

#define create_task_and_wait(spawn_stmt)                \
  do { create_taskA(spawn_stmt); wait_tasks; } while(0)
#define create_taskc_and_wait(callable)			\
  do { create_taskc(callable); wait_tasks; } while(0)

#define call_task(spawn_stmt)          create_task_and_wait(spawn_stmt)
#define call_taskc(callable)              create_taskc_and_wait(callable)   

#define mit_spawn spawn
#define cilk_static cilk

#elif TO_CILKPLUS

#include <tpswitch/cilkplus_dr.h>
#define mk_task_group clkp_mk_task_group

#define create_task0(spawn_stmt)       spawn_(spawn_stmt)
#define create_task0_(spawn_stmt, file, line)       spawn__(spawn_stmt, file, line)
#define create_task1(s0,spawn_stmt)    spawn_(spawn_stmt)
#define create_task2(s0,s1,spawn_stmt) spawn_(spawn_stmt)
#define create_taskA(spawn_stmt)       spawn_(spawn_stmt)

#define create_taskc(callable)            spawn_(_Cilk_spawn callable())

/* "mit_spawn" keyword is used with create_task_and_wait() and call_task(),
   by default "mit_spawn" translates to null when targeting cilk plus and to "spawn"
   when targeting mit cilk.
   In cilk plus target, "mit_spawn" would translate to "spawn" when the flag
   -Dcreate_task_and_wait_spawns=1 is set to 1, needed for old code which did not
   use "mit_spawn" yet, e.g.,

   #define create_task_and_wait_spawns 1
   #include <tpswitch/tpswitch.h>
*/
#if !defined(create_task_and_wait_spawns)
#define create_task_and_wait_spawns 0
#endif

#if create_task_and_wait_spawns
#define mit_spawn spawn
#else
#define mit_spawn
#endif

#define cilk
#define cilk_static static

#if create_task_and_wait_spawns
#define create_task_and_wait(spawn_stmt)                \
  do { create_taskA(spawn_stmt); wait_tasks; } while(0)
#define create_taskc_and_wait(callable)			\
  do { create_taskc(callable); wait_tasks; } while(0)
#define call_task(spawn_stmt)                           \
  do { mk_task_group; create_taskA(spawn_stmt); wait_tasks; } while(0)
#define call_taskc(callable)                            \
  do { mk_task_group; create_taskc(callable); wait_tasks; } while(0)
#else
#define create_task_and_wait(stmt)              \
  do { stmt; wait_tasks; } while(0)
#define create_taskc_and_wait(callable)         \
  do { callable(); wait_tasks; } while(0)
#define call_task(stmt)                         \
  do { stmt; } while(0)
#define call_taskc(callable)                    \
  do { callable(); } while(0)
#endif

#endif /* TO_CILKPLUS */

#define wait_tasks sync_
#define wait_tasks_(file,line) sync__(file,line)

#define pragma_omp_parallel_single(clause, S) do { S } while(0)

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


/*
  tpswitch parallel for (pfor)
   -->
  omp parallel for (OpenMP)
  cilk_for (Cilk Plus)
  mtbb::parallel_for (TBB-like)

  pfor<IntTy, StepIntTy, LeafFuncTy>(IntTy FIRST, IntTy LAST, StepIntTy STEP, IntTy GRAINSIZE, LeafFuncTy LEAF(IntTy first, IntTy last))
  (STEP > 0, GRAINSIZE > 0)

  pfor_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRSTVARIABLE, LASTVARIABLE, LEAF)

  pfor_backward<IntTy, StepIntTy, LeafFuncTy>(IntTy FIRST, IntTy LAST, StepIntTy STEP, IntTy GRAINSIZE, LeafFuncTy LEAF(IntTy first, IntTy last))
  (STEP > 0, GRAINSIZE > 0)

  pfor_backward_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRSTVARIABLE, LASTVARIABLE, LEAF)

  Example:

  //original for version:

  #include <stdio.h>

  int main() {
    int first = 0;
    int last = 10;
    int step = 1;
    for(int i = first; i < last; i += step) {
      char s[100];
      for (int i = innerFirst; i < innerLast; i += step)
        printf("processing %d\n", i);
    }
  }

  //pfor version.

  #include <stdio.h>

  //original for-loop parallelization if available (e.g., #omp parallel for)
  #define PFOR_TO_ORIGINAL
  //iteration space is divided by two until it becomes less than a certain size.
  //#define PFOR_TO_BISECTION
  //all tasks are created by the parent
  //#define PFOR_TO_ALLATONCE

  #include "tpswitch.h"

  int main() {
    int first = 0;
    int last = 10;
    int step = 1;
    int grainsize = 2;
    pfor(first, last, step, grainsize,
      [step] (int innerFirst, int innerLast) {
        char s[100];
        for (int i = innerFirst; i < innerLast; i += step)
          printf("processing %d\n", i);
      });
  }

  //pfor-c version.

  #define PFOR_TO_ORIGINAL
  #include "tpswitch.h"

  int main() {
    int first = 0;
    int last = 10;
    int step = 1;
    int grainsize = 2;
    pfor_c(int, first, last, step, grainsize, innerFirst, innerLast, {
        char s[100];
        for (int i = innerFirst; i < innerLast; i += step)
          printf("processing %d\n", i);
      });
  }
 
  //pfor-backward version.

  #include <stdio.h>
  #include "tpswitch.h"

  int main() {
    int first = 10 - 1;
    int last = 0;
    int step = -1;
    int grainsize = 2;
    pfor_backward(first, last, step, grainsize,
      [step] (int innerFirst, int innerLast) {
        char s[100];
        for (int i = innerFirst; i >= innerLast; i += step)
          printf("processing %d\n", i);
      });
  }

  //pfor-backward-c version.

  #include <stdio.h>
  #include "tpswitch.h"

  int main() {
    int first = 10 - 1;
    int last = 0;
    int step = -1;
    int grainsize = 2;
    pfor_backward_c(int, first, last, step, grainsize, innerFirst, innerLast, {
        char s[100];
        for (int i = innerFirst; i >= innerLast; i += step)
          printf("processing %d\n", i);
      });
  }
*/

#if defined(PFOR_TO_ALLATONCE) || defined(PFOR_TO_BISECTION) || defined(PFOR_TO_ORIGINAL)

#if __cplusplus >= 201103L
  #if PFOR_TO_ORIGINAL
    #define PFOR_IMPL pfor_original
    #if TO_SERIAL
      template<typename IntTy, typename StepIntTy, typename LeafFuncTy> static void pfor_original(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
        leaffunc(first,last);
      }
    #elif TO_OMP
      template<typename IntTy, typename StepIntTy, typename LeafFuncTy> static void pfor_original(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
        const IntTy elementnum = (last - first) / step;
        const IntTy tasknum = (elementnum + grainsize - 1) / grainsize;
        pragma_omp(parallel for)
        for(IntTy i = 0; i < tasknum; i++) {
          IntTy leaf_first = first + i * step * grainsize;
          IntTy leaf_last  = first + (i + 1) * step * grainsize;
          if (leaf_last > last)
            leaf_last = last;
          leaffunc(leaf_first,leaf_last);
        }
      }
    #elif TO_CILKPLUS
      template<typename IntTy, typename StepIntTy, typename LeafFuncTy> static void pfor_original(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
        const IntTy elementnum = (last - first) / step;
        const IntTy tasknum = (elementnum + grainsize - 1) / grainsize;
        cilk_for (IntTy i = 0; i < tasknum; i++) {
          IntTy leaf_first = first + i * step * grainsize;
          IntTy leaf_last  = first + (i + 1) * step * grainsize;
          if (leaf_last > last)
            leaf_last = last;
          leaffunc(leaf_first,leaf_last);
        }
      }
    #elif TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX
      #include <mtbb/parallel_for.h>
      template<typename IntTy, typename StepIntTy, typename LeafFuncTy> static void pfor_original(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
        mtbb::parallel_for(first, last, step, grainsize, leaffunc);
      }
    #endif
  #elif PFOR_TO_BISECTION
    #define PFOR_IMPL pfor_bisection
    template<typename IntTy, typename StepIntTy, typename LeafFuncTy> cilk_static void pfor_bisection_aux(IntTy first, IntTy a, IntTy b, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
      cilk_begin;
      if (b - a <= grainsize) {
        leaffunc(first + a * step, first + b * step);
      } else {
        mk_task_group;
        const IntTy c = a + (b - a) / 2;
        create_task0_(spawn pfor_bisection_aux(first, a, c, step, grainsize, leaffunc, file, line), file, line);
        create_task_and_wait(mit_spawn pfor_bisection_aux(first, c, b, step, grainsize, leaffunc, file, line));
      }
      cilk_void_return;
    }
    template<typename IntTy, typename StepIntTy, typename LeafFuncTy> cilk_static void pfor_bisection(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
      IntTy a = 0;
      IntTy b = (last - first + step - 1) / step;
      call_task(mit_spawn pfor_bisection_aux(first, a, b, step, grainsize, leaffunc, file, line));
    }
  #elif PFOR_TO_ALLATONCE
    #define PFOR_IMPL pfor_allatonce
    template<typename IntTy, typename StepIntTy, typename LeafFuncTy> cilk_static void pfor_allatonce_aux(IntTy first, IntTy a, IntTy b, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
      mk_task_group;
      IntTy ia = a;
      IntTy ib = a;
      while (ib < b) {
        ib += grainsize;
        if (ib > b)
          ib = b;
        create_task0_(spawn leaffunc(first + ia * step, first + ib * step), file, line);
        ia = ib;
      }
      wait_tasks;
    }
    template<typename IntTy, typename StepIntTy, typename LeafFuncTy> cilk_static void pfor_allatonce(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
      IntTy a = 0;
      IntTy b = (last - first + step - 1) / step;
      call_task(mit_spawn pfor_allatonce_aux(first, a, b, step, grainsize, leaffunc, file, line));
    }
  #endif
  #ifdef PFOR_IMPL
    #include <type_traits>
    //__VA_ARGS__ is to avoid the well-known comma-in-macro problem.
    //std::decay is to get base type (i.e., remove const)
    #define pfor(FIRST, ...) PFOR_IMPL <std::decay<decltype(FIRST)>::type>(FIRST, __VA_ARGS__, __FILE__, __LINE__)
    #define pfor_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) PFOR_IMPL <INTTYPE>(FIRST, LAST, STEP, GRAINSIZE, [=](INTTYPE FIRST_VAR, INTTYPE LAST_VAR) {__VA_ARGS__}, __FILE__, __LINE__)

    template<typename IntTy, typename StepIntTy, typename LeafFuncTy> static void PFOR_BACKWARD_IMPL(IntTy first, IntTy last, StepIntTy step, IntTy grainsize, LeafFuncTy leaffunc, const char * file, int line) {
      IntTy newfirst = 0;
      IntTy newlast  = first - last + 1;
      IntTy newstep  = -step;
      auto PFOR_BACKWARD_FUNC = [first, leaffunc] (IntTy _first, IntTy _last) -> void { 
        leaffunc(first - _first, first - _last + 1);
      };
      PFOR_IMPL(newfirst, newlast, newstep, grainsize, PFOR_BACKWARD_FUNC, file, line);
    }
    #define pfor_backward(FIRST, ...) PFOR_BACKWARD_IMPL <std::decay<decltype(FIRST)>::type>(FIRST, __VA_ARGS__, __FILE__, __LINE__)
    #define pfor_backward_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) PFOR_BACKWARD_IMPL <INTTYPE>(FIRST, LAST, STEP, GRAINSIZE, [=](INTTYPE FIRST_VAR, INTTYPE LAST_VAR) {__VA_ARGS__}, __FILE__, __LINE__)
  #endif
#else
  //old __cplusplus, so avoid to use lambda
  #if PFOR_TO_ORIGINAL
    #if TO_SERIAL
      #define pfor_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE FIRST_VAR=(FIRST);\
          INTTYPE LAST_VAR =(LAST);\
          {__VA_ARGS__};\
        } while(0)
      #define pfor_backward_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE FIRST_VAR=(FIRST);\
          INTTYPE LAST_VAR =(LAST);\
          {__VA_ARGS__};\
        } while(0)
    #elif TO_OMP
      #define pfor_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE eval_first     = (FIRST);\
          INTTYPE eval_last      = (LAST);\
          int     eval_step      = (STEP);\
          INTTYPE eval_grainsize = (GRAINSIZE);\
          const INTTYPE MACRO_elementnum = (eval_last - eval_first) / eval_step;\
          const INTTYPE MACRO_tasknum    = (MACRO_elementnum + eval_grainsize - 1) / eval_grainsize;\
          pragma_omp(parallel for)\
          for(INTTYPE MACRO_i = 0; MACRO_i < MACRO_tasknum; MACRO_i++) {\
            INTTYPE FIRST_VAR = eval_first + MACRO_i * eval_step * eval_grainsize;\
            INTTYPE LAST_VAR  = eval_first + (MACRO_i + 1) * eval_step * eval_grainsize;\
            if (LAST_VAR > eval_last)\
              LAST_VAR = eval_last;\
            {__VA_ARGS__};\
          }\
        } while(0)
      #define pfor_backward_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE eval_first     = (FIRST);\
          INTTYPE eval_last      = (LAST);\
          int     eval_step      = (STEP);\
          INTTYPE eval_grainsize = (GRAINSIZE);\
          const INTTYPE MACRO_elementnum = (eval_last - eval_first) / eval_step;\
          const INTTYPE MACRO_tasknum    = (MACRO_elementnum + eval_grainsize - 1) / eval_grainsize;\
          pragma_omp(parallel for)\
          for(INTTYPE MACRO_i = 0; MACRO_i < MACRO_tasknum; MACRO_i++) {\
            INTTYPE FIRST_VAR = eval_first + MACRO_i * eval_step * eval_grainsize;\
            INTTYPE LAST_VAR  = eval_first + (MACRO_i + 1) * eval_step * eval_grainsize;\
            if (LAST_VAR < eval_last)\
              LAST_VAR = eval_last;\
            {__VA_ARGS__};\
          }\
        } while(0)
    #elif TO_CILKPLUS
      #define pfor_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE eval_first     = (FIRST);\
          INTTYPE eval_last      = (LAST);\
          int     eval_step      = (STEP);\
          INTTYPE eval_grainsize = (GRAINSIZE);\
          const INTTYPE MACRO_elementnum = (eval_last - eval_first) / eval_step;\
          const INTTYPE MACRO_tasknum    = (MACRO_elementnum + eval_grainsize - 1) / eval_grainsize;\
          cilk_for(INTTYPE MACRO_i = 0; MACRO_i < MACRO_tasknum; MACRO_i++) {\
            INTTYPE FIRST_VAR = eval_first + MACRO_i * eval_step * eval_grainsize;\
            INTTYPE LAST_VAR  = eval_first + (MACRO_i + 1) * eval_step * eval_grainsize;\
            if (LAST_VAR > eval_last)\
              LAST_VAR = eval_last;\
            {__VA_ARGS__};\
          }\
        } while(0)
      #define pfor_backward_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) \
        do {\
          INTTYPE eval_first     = (FIRST);\
          INTTYPE eval_last      = (LAST);\
          int     eval_step      = (STEP);\
          INTTYPE eval_grainsize = (GRAINSIZE);\
          const INTTYPE MACRO_elementnum = (eval_last - eval_first) / eval_step;\
          const INTTYPE MACRO_tasknum    = (MACRO_elementnum -eval_grainsize - 1) / eval_grainsize;\
          cilk_for(INTTYPE MACRO_i = 0; MACRO_i < MACRO_tasknum; MACRO_i++) {\
            INTTYPE FIRST_VAR = eval_first + MACRO_i * eval_step * eval_grainsize;\
            INTTYPE LAST_VAR  = eval_first + (MACRO_i + 1) * eval_step * eval_grainsize;\
            if (LAST_VAR < eval_last)\
              LAST_VAR = eval_last;\
            {__VA_ARGS__};\
          }\
        } while(0)
    #elif TO_TBB || TO_MTHREAD || TO_MTHREAD_NATIVE || TO_QTHREAD || TO_NANOX
      #error "error: pfor (parallel for) for tbb/mth/qth/nanox needs C++11; add a flag -std=c++11"
    #endif
    #define pfor_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) pfor_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, __VA_ARGS__)
    #define pfor_backward_c(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, ...) pfor_backward_original_no_cpp11(INTTYPE, FIRST, LAST, STEP, GRAINSIZE, FIRST_VAR, LAST_VAR, __VA_ARGS__)
  #elif PFOR_TO_BISECTION
    #error "error: pfor_bisecion (parallel for) needs C++11; add a flag -std=c++11"
  #elif PFOR_TO_ALLATONCE
    #error "error: pfor_allatonce (parallel for) needs C++11; add a flag -std=c++11"
  #endif
#endif

#endif


#if TO_TBB
//It is necessary in tp_init()
#include <tbb/task_scheduler_init.h>
#endif

inline void tp_init() {
#if TO_QTHREAD
  qthread_initialize();
#elif TO_TBB
  /* it is possible that it is included from C file,
     in which case we do not call it.
     we assume there is still a main C++ file
     and the one defined in C does not get called */
  const char* TBB_NTHREADS = "TBB_NTHREADS";
  if(char *tbb_nthreads = getenv(TBB_NTHREADS)) {
    int num_workers = atoi(tbb_nthreads);
    if(num_workers <= 0) {
      fprintf(stderr, "could not parse environment variable %s as a number (treated as 1)\n", TBB_NTHREADS);
      num_workers = 1;
    }
    new tbb::task_scheduler_init(num_workers);
  } else {
    fprintf(stderr, "could not get number of workers (set %s)\n", TBB_NTHREADS);
    //Use a default value (= a number of cores)
  }
#endif
}
