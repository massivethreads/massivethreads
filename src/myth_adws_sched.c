/*
 * myth_adws_sched.c
 */

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS
myth_workers_range_t g_root_workers_range;
int g_sched_adws = 0;
int g_adws_stealable = 1;
#endif
