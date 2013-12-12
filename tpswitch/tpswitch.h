/* 
 * tpswitch.h --- switch between task parallel systems
 */

/* serial */
#if TO_SERIAL
#include <tpswitch/serial_dr.h>

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
#define create_taskA(statement)			\
  pragma_omp_task(default(shared),statement)
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
#define create_taskA(statement)			\
  __tg__.run([&] { statement; })

#define wait_tasks __tg__.wait()

/* Cilk (not implemented yet) */
#if TO_CILK
#include <tpswitch/cilk_dr.h>

#else
#error "neither TO_SERIAL, TO_OMP, TO_TBB, TO_CILK, TO_CILKPLUS, TO_MTHREAD, TO_MTHREAD_NATIVE, TO_QTHREAD, nor TO_NANOX defined"
#endif
