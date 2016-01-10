/* 
 * myth_thraed.h ---
 *  data structure related to thread and
 *  must be included by the user
 */
#ifndef MYTH_THREAD_H_
#define MYTH_THREAD_H_

#include <stdlib.h>

#include "myth/myth.h"

#include "myth_context.h"
#include "myth_internal_lock.h"

/* Thread status constants */
typedef enum {
  // Executable
  MYTH_STATUS_READY = 0,
  // Blocked
  MYTH_STATUS_BLOCKED = 1,
  // Execution finished. But you have to obtain lock before release
  MYTH_STATUS_FREE_READY = 2,
  //Execution finished. Feel free to release
  MYTH_STATUS_FREE_READY2 = 3,
} myth_status_t;

/* Thread descriptor */
// typedef 
struct myth_thread {
  struct myth_thread * next;
  // A thread which is waiting for this
  struct myth_thread* join_thread;
  myth_func_t entry_func;
  // Return value
  void *result;
  // Context
  myth_context context;
  // Pointer to stack
  void *stack;
  // Pointer to worker thread
  struct myth_running_env* env;
  // Lock
  myth_internal_lock_t lock;
#if 0
  // Data for runqueue (not used)
  myth_queue_data queue_data;
#endif
  // Status
  volatile myth_status_t status;
  uint8_t detached;
  uint8_t cancelled;
  uint8_t cancel_enabled;
#if defined MYTH_ENABLE_THREAD_ANNOTATION && defined MYTH_COLLECT_LOG
  char annotation_str[MYTH_THREAD_ANNOTATION_MAXLEN];
  int recycle_count;
#endif
#ifdef MYTH_DESC_REUSE_CHECK
  myth_internal_lock_t sanity_check;
#endif
  void *custom_data_ptr;
  int custom_data_size;
} ;

// __attribute__((aligned(CACHE_LINE_SIZE))) myth_thread, *myth_thread_t;

// typedef 
struct myth_pickle {
  struct myth_thread desc;
  size_t stack_size;
  char *stack;
} ;
// myth_pickle_t;

#endif	/* MYTH_THREAD_H_ */
