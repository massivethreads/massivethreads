// Functions to support GCC's split stack

// IMPORTANT:
// We, MassiveThreads developers, read libgcc to write this code,
// but this file is fully written by us, and does not include
// any piece of derivative code from libgcc.
// Thus we consider this file is not a derivative work from libgcc,
// and we do not have to apply GPL to the source files.
// Please contact us if you have any question.

#include "myth_ss.h"

#ifdef MYTH_SPLIT_STACK
int g_myth_ss_enabled=1;

void myth_ss_init(void)
{
	char *env;
	env=getenv("MYTH_SPLIT_STACK");
	if (env){
		g_myth_ss_enabled=atoi(env);
	}
}

void myth_ss_fini(void)
{
}

void myth_ss_init_worker(int rank)
{
	if (myth_is_ss_enabled()){
		if (rank!=0){
			__stack_split_initialize();
		}
	}
}

void myth_ss_fini_worker(int rank)
{
	if (myth_is_ss_enabled()){
		if (rank!=0){
			__morestack_release_segments(__morestack_segments, 1);
		}
	}
}
#endif
