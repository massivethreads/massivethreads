#ifndef MYTH_LOG_H_
#define MYTH_LOG_H_

#include <stdint.h>
#include "myth_config.h"

typedef enum
{
	MYTH_LOG_SWITCH=0,	//Start user application routine
}myth_log_type_t;

//ログをとるための構造体
typedef struct myth_log_entry
{
	uint64_t tsc;			//Time stamp counter from rdtsc
	myth_log_type_t type;
	union{
		char str[MYTH_THREAD_ANNOTATION_MAXLEN];
	}u;
}myth_log_entry,*myth_log_entry_t;

extern uint64_t g_tsc_base;

#endif /* MYTH_LOG_H_ */
