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

#if !defined(DAG_RECORDER_INLINE_PROFILE)
#define DAG_RECORDER_INLINE_PROFILE 1
#endif

#if DAG_RECORDER_INLINE_PROFILE
#define static_if_inline static
#else
#define static_if_inline
#endif

/* 
   task    ::= section* end
   section ::= task_group (section|create)* wait
 */

  typedef unsigned long long dr_clock_t;

  typedef struct dr_options {
    const char * dag_file;	/* filename of the dag */
    const char * stat_file;	/* filename of the dag */
    const char * gpl_file;	/* filename of the gpl */
    const char * dot_file;	/* filename of the dot */
    const char * sqlite_file;	/* filename of the sqlite3 */
    const char * text_file;	/* filename of the text */
    const char * nodes_file;	/* filename of the nodes */
    const char * edges_file;	/* filename of the edges */
    const char * strings_file;	/* filename of the strings */
    const char * text_file_sep;	/* separator for text file */

    dr_clock_t uncollapse_min;	/* minimum length that can be uncollapsed */
    dr_clock_t collapse_max;	/* maximum length that can be collpased */
    long node_count_target;	/* desired number of nodes */
    long prune_threshold;	/* prune nodes larger than node_count_target * prune_threshold */
    long alloc_unit_mb;	        /* node allocation unit in bytes */
    long pre_alloc;             /* pre-allocated units */
    int gpl_sz;			/* size of gpl file */
    char dbg_level;		/* level of debugging features */
    char verbose_level;		/* level of verbosity */
    char chk_level;		/* level of checks during run */
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
    (const char *)"00dr.dag",  /* dag_file */
    (const char *)"00dr.stat", /* stat_file */
    (const char *)"00dr.gpl",  /* gpl_file */
    (const char *)0,	       /* dot_file */
    (const char *)0,	       /* sqlite_file */
    (const char *)0,	       /* text_file */
    (const char *)0,	       /* nodes_file */
    (const char *)0,	       /* edges_file */
    (const char *)0,	       /* strings_file */
    (const char *)"|",	       /* text_file_sep */
    0,			       /* uncollapse_min; obsolete. */
    (1L << 60),	               /* collapse_max used to be (1L << 60), */
    0,	                       /* node_count_target */
    100000,                   /* prune_threshold */
    1,			       /* alloc unit in MB */
    0,			       /* the number of pre-allocations */
    4000,			/* gpl_sz */
    0,				/* dbg_level */
    0,				/* verbose_level */
    0,				/* chk_level */
  };

  typedef struct dr_dag_node dr_dag_node;

  static_if_inline void          dr_start_task__(dr_dag_node * parent, const char * file, int line, int worker);
  static_if_inline int           dr_start_cilk_proc__(const char * file, int line, int worker);
  static_if_inline void          dr_begin_section__(int worker);
  static_if_inline dr_dag_node * dr_enter_create_task__(dr_dag_node ** create, const char * file, int line, int worker);
  static_if_inline dr_dag_node * dr_enter_create_cilk_proc_task__(const char * file, int line, int worker);
  static_if_inline void          dr_return_from_create_task__(dr_dag_node * task, const char * file, int line, int worker);
  static_if_inline dr_dag_node * dr_enter_wait_tasks__(const char * file, int line, int worker);
  static_if_inline void          dr_return_from_wait_tasks__(dr_dag_node * task, const char * file, int line, int worker);
  static_if_inline void          dr_end_task__(const char * file, int line, int worker);
  void dr_options_default(dr_options * opts);
  void dr_start__(dr_options * opts, const char * file, int line, 
		  int worker, int num_workers);
  void dr_stop__(const char * file, int line, int worker);
  void dr_dump();
  int dr_read_and_analyze_dag(const char * filename);
  void dr_cleanup__(const char * file, int line, int worker, int num_workers);

#define dr_start_task_(parent, file, line)			\
  dr_start_task__(parent, file, line, dr_get_worker())

#define dr_start_task(parent) \
  dr_start_task_(parent, __FILE__, __LINE__)

#define dr_start_cilk_proc_(parent, file, line)			\
  dr_start_cilk_proc__(file, line, dr_get_worker())

#define dr_start_cilk_proc(parent) \
  dr_start_cilk_proc_(__FILE__, __LINE__)

#define dr_begin_section() \
  dr_begin_section__(dr_get_worker())

#define dr_enter_create_task_(create, file, line)		\
  dr_enter_create_task__(create, file, line, dr_get_worker())

#define dr_enter_create_task(create) \
  dr_enter_create_task_(create, __FILE__, __LINE__)

#define dr_enter_create_cilk_proc_task() \
  dr_enter_create_cilk_proc_task__(__FILE__, __LINE__, dr_get_worker())

#define dr_enter_create_cilk_proc_task_(file, line)		\
  dr_enter_create_cilk_proc_task__(file, line, dr_get_worker())

#define dr_return_from_create_task_(task, file, line)	\
  dr_return_from_create_task__(task, file, line, dr_get_worker())

#define dr_return_from_create_task(task) \
  dr_return_from_create_task_(task, __FILE__, __LINE__)

#define dr_enter_wait_tasks_(file, line)			\
  dr_enter_wait_tasks__(file, line, dr_get_worker())

#define dr_enter_wait_tasks() \
  dr_enter_wait_tasks_(__FILE__, __LINE__)

#define dr_return_from_wait_tasks_(task, file, line)		\
  dr_return_from_wait_tasks__(task, file, line, dr_get_worker())

#define dr_return_from_wait_tasks(task) \
  dr_return_from_wait_tasks_(task, __FILE__, __LINE__)

#define dr_end_task_(file, line)			\
  dr_end_task__(file, line, dr_get_worker())

#define dr_end_task() \
  dr_end_task_(__FILE__, __LINE__)

#define dr_start(opts) \
  dr_start__(opts, __FILE__, __LINE__, dr_get_worker(), dr_get_max_workers())
#define dr_stop() \
  dr_stop__(__FILE__, __LINE__, dr_get_worker())
#define dr_cleanup() \
  dr_cleanup__(__FILE__, __LINE__, dr_get_worker(), dr_get_max_workers())

#ifdef __cplusplus
}
#endif

/* all the above static functions are defined in 
   dag_recorder_inl.h.
   all the above external functions are defined in 
   dag_recorder.c
 */
#if DAG_RECORDER_INLINE_PROFILE
#include <dag_recorder_inl.h>
#endif


