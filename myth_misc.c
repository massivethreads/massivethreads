#include "myth_misc.h"

#ifdef MYTH_FLMALLOC_TLS
__thread myth_freelist_t *g_myth_freelist;
#else
myth_freelist_t **g_myth_freelist;
#endif

//Global variabled declaration

//uint64_t g_mmap_total=0,g_mmap_count=0;

__thread unsigned int g_myth_random_temp=0;

__thread uint64_t g_myth_flmalloc_cycles=0,g_myth_flmalloc_cnt=0;
__thread uint64_t g_myth_flfree_cycles=0,g_myth_flfree_cnt=0;
