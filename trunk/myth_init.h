#ifndef MYTH_INIT_H
#define MYTH_INIT_H

#include "myth_original_lib.h"

#include <pthread.h>

#include "myth_io.h"
#include "myth_log.h"

//Environment value of worker threads
#define ENV_MYTH_WORKER_NUM "MYTH_WORKER_NUM"
#define ENV_MYTH_WORKER_OFFSET "MYTH_WORKER_OFFSET"
#define ENV_MYTH_WORKER_STEP "MYTH_WORKER_STEP"

void myth_init_body(void);
void myth_fini_body(void);
int myth_init_ex_body(int worker_num);
void myth_fini_ex_body(void);
void myth_notify_workers_exit(void);

#endif
