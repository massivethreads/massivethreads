#include <stdlib.h>
// Prototype for GCC's split stack

// IMPORTANT:
// We, MassiveThreads developers, read libgcc to write this code,
// but this file is fully written by us, and does not include
// any piece of derivative code from libgcc.
// Thus we consider this file is not a derivative work from libgcc,
// and we do not have to apply GPL to the source files.
// Please contact us if you have any question.

// Initialize
//extern void __stack_split_initialize(void);
//extern void* __morestack_release_segments(void *pp, int);

// Context management
extern void __splitstack_getcontext(void *ctx[10]) __attribute__((weak));
extern void __splitstack_setcontext(void *ctx[10]) __attribute__((weak));
extern void *__splitstack_makecontext(size_t stack_size, void *ctx[10],size_t *size) __attribute__((weak));
extern void __splitstack_releasecontext(void *ctx[10]) __attribute__((weak));

void myth_ss_init_worker(int rank);
void myth_ss_fini_worker(int rank);
void myth_ss_init(void);
void myth_ss_fini(void);
int myth_ss_status(void);