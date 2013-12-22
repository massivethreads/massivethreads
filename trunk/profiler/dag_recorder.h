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
    const char * log_file;	/* filename of the log */
    const char * dot_file;	/* filename of the dot */
    const char * gpl_file;	/* filename of the gpl */
    int gpl_sz;			/* size of gpl file */
    char dbg_level;		/* level of debugging features */
    char verbose_level;		/* level of verbosity */
    char chk_level;		/* level of checks during run */
    char collapse;		/* collapse nodes if set */
    char dump_on_stop;		/* when set, dr_stop dumps the log */
  } dr_options;

  /* default values. written here for documentation purpose.
     there are two ways to overwrite them.
     (1) explicitly pass options to dr_start();
     dr_options opts; dr_options_default(&opts);
     opts.dbg_level = 2; dr_start(&opts);
     (2) set corresponding environment variables when
     you run the program. e.g.,
     DAG_RECORDER_DBG_LEVEL=2 ./a.out
  */
  static dr_options 
  dr_options_default_values __attribute__ ((unused)) = { 
    (const char *)0,		/* log_file */
    (const char *)0,		/* dot_file */
    (const char *)0,		/* gpl_file */
    4000,			/* gpl_sz */
    0,				/* dbg_level */
    1,				/* verbose_level */
    1,				/* chk_level */
    1,				/* collapse */
    0,				/* dump_on_stop */
  };

  typedef struct dr_dag_node dr_dag_node;

  static void          dr_start_task_(dr_dag_node * parent, int worker);
  static int           dr_start_cilk_proc_(int worker);
  static void          dr_begin_section_(int worker);
  static dr_dag_node * dr_enter_create_task_(dr_dag_node ** create, int worker);
  static dr_dag_node * dr_enter_create_cilk_proc_task_(int worker);
  static void          dr_return_from_create_task_(dr_dag_node * task, int worker);
  static dr_dag_node * dr_enter_wait_tasks_(int worker);
  static void          dr_return_from_wait_tasks_(dr_dag_node * task, int worker);
  static void          dr_end_task_(int worker);
  void dr_options_default(dr_options * opts);
  void dr_start_(dr_options * opts, int worker, int num_workers);
  void dr_stop_(int worker);
  void dr_dump();

#define dr_start_task(parent) \
  dr_start_task_(parent, dr_get_worker()) 

#define dr_start_cilk_proc(parent) \
  dr_start_cilk_proc_(dr_get_worker()) 

#define dr_begin_section() \
  dr_begin_section_(dr_get_worker())

#define dr_enter_create_task(create) \
  dr_enter_create_task_(create, dr_get_worker())

#define dr_enter_create_cilk_proc_task() \
  dr_enter_create_cilk_proc_task_(dr_get_worker())

#define dr_return_from_create_task(task) \
  dr_return_from_create_task_(task, dr_get_worker())

#define dr_enter_wait_tasks() \
  dr_enter_wait_tasks_(dr_get_worker())

#define dr_return_from_wait_tasks(task) \
  dr_return_from_wait_tasks_(task, dr_get_worker())

#define dr_end_task() \
  dr_end_task_(dr_get_worker())

#define dr_start(opts) \
  dr_start_(opts, dr_get_worker(), dr_get_max_workers())
#define dr_stop() \
  dr_stop_(dr_get_worker())

#ifdef __cplusplus
}
#endif

/* all the above static functions are defined in 
   dag_recorder_inl.h.
   all the above external functions are defined in 
   dag_recorder.c
 */
#include <dag_recorder_inl.h>

