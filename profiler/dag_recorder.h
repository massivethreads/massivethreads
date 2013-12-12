/*
 * dag recorder 2.0
 */

#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
   task    ::= section* end
   section ::= task_group (section|create)* wait
 */
  typedef enum { 
    dr_model_serial,
    dr_model_omp,
    dr_model_cilk,
    dr_model_tbb,
  } dr_model_t;

  typedef struct dr_options {
    int dbg_level;		/* level of checks during run */
    int collapse;		/* collapse nodes if set */
    const char * log_file;	/* filename of the log */
    int dump_on_stop;		/* when set, dr_stop dumps the log */
    dr_model_t model;
  } dr_options;

  /* default values. written here for documentation purpose */
  __attribute__ ((unused))
  static dr_options dr_options_default_values = { 
    0,
    1,
    "000dag_recorder.log",
    1,
    dr_model_omp,
  };

  typedef struct dr_dag_node dr_dag_node;

  static void          dr_start_task(dr_dag_node * parent);
  static dr_dag_node * dr_enter_task_group();
  static void          dr_return_from_task_group(dr_dag_node * task);
  static dr_dag_node * dr_enter_create_task(dr_dag_node ** create);
  static void          dr_return_from_create_task(dr_dag_node * task);
  static dr_dag_node * dr_enter_wait_tasks();
  static void          dr_return_from_wait_tasks(dr_dag_node * task);
  static void          dr_end_task();

  void dr_options_default(dr_options * opts);
  void dr_start(dr_options * opts);
  void dr_stop();

#ifdef __cplusplus
}
#endif

/* all the above static functions are defined in 
   dag_recorder_inl.h.
   all the above external functions are defined in 
   dag_recorder.c
 */
#include <dag_recorder_inl.h>

