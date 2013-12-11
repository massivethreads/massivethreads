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

  typedef struct dr_options {
    int dbg_level;
    int collapse;
  } dr_options;

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

