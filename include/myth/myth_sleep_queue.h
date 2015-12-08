/* 
 * myth_sleep_queue.h
 */
#pragma once
#ifndef MYTH_SLEEP_QUEUE_H_
#define MYTH_SLEEP_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MYTH_SLEEP_QUEUE_DBG
#define MYTH_SLEEP_QUEUE_DBG 0
#endif

#ifndef MYTH_SLEEP_QUEUE_LOCK
#define MYTH_SLEEP_QUEUE_LOCK 1
#endif

#if MYTH_SLEEP_QUEUE_LOCK
#include <myth/myth_internal_lock.h>
#endif

  /* myth_sleep_queue_item_t represents a pointer 
     to any data structure that has "next" field
     in its head */
  typedef struct myth_sleep_queue_item {
    struct myth_sleep_queue_item * next;
  } myth_sleep_queue_item, * myth_sleep_queue_item_t;

  typedef struct {
    volatile myth_sleep_queue_item_t head;
    volatile myth_sleep_queue_item_t tail;
#if MYTH_SLEEP_QUEUE_LOCK
    myth_internal_lock_t ilock[1];
#endif
  } myth_sleep_queue_t;
  
  /* TODO: the last element initializes internal lock.
     this works only when myth_internal_lock is a hand-made 
     spin lock. I'll rather make it always so, as doing
     pthread_mutex here does not make much sense*/
#define MYTH_SLEEP_QUEUE_INITIALIZER { 0, 0, { 0 } }

  typedef struct {
    volatile myth_sleep_queue_item_t top;
  } myth_sleep_stack_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* MYTH_SLEEP_QUEUE_H_ */
