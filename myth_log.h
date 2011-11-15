#ifndef MYTH_LOG_H_
#define MYTH_LOG_H_

#include "stdint.h"

typedef enum
{
	MYTH_LOG_USER=0,	//Start user application routine
	MYTH_LOG_INT,		//Internal processing
	MYTH_LOG_WS,		//Start work stealing
	MYTH_LOG_QUIT,		//Quit application
}myth_log_type_t;

static const char s_log_type_to_name[]=
{
		'U','I','W','Q',
};

//ログをとるための構造体
typedef struct myth_log_entry
{
	uint64_t tsc;			//Time stamp counter from rdtsc
	int ws_victim;
	myth_log_type_t type;
}myth_log_entry,*myth_log_entry_t;

typedef struct myth_textlog_entry
{
	uint64_t ts,te;
	int cat;
	int id_a,id_b;
}myth_textlog_entry,*myth_textlog_entry_t;

int myth_textlog_entry_compare(const void *pa,const void *pb);

#endif /* MYTH_LOG_H_ */
