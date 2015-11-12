#ifndef MYTH_SLEEP_QUEUE_H_
#define MYTH_SLEEP_QUEUE_H_

#include "myth_desc.h"

#ifndef MYTH_SLEEP_QUEUE_DBG
#define MYTH_SLEEP_QUEUE_DBG 0
#endif

typedef struct {
  volatile myth_thread_t head;
  volatile myth_thread_t tail;
} myth_sleep_queue_t;

#endif	/* MYTH_SLEEP_QUEUE_H_ */
