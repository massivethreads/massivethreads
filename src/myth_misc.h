/* 
 * myth_misc.h
 */
#pragma once
#ifndef MYTH_MISC_H_
#define MYTH_MISC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sched.h>

#include "config.h"

#include "myth/myth_config.h"
#include "myth_real_fun.h"

//Variable attribute which may be unused to supress warnings
#define MAY_BE_UNUSED __attribute__((unused))

//Sanity check which can be removed for performance
//Do not write expressions with side-effect! It may not be executed!
#ifdef MYTH_SANITY_CHECK
static inline void myth_assert(expr){ assert(expr); }
#else
#define myth_assert(expr)
#endif


#define GCC_VERSION (__GNUC__ * 10000		\
		     + __GNUC_MINOR__ * 100	\
		     + __GNUC_PATCHLEVEL__)


//Unreachable marker that causes segmentation fault
//useful for debugging context-switching codes
#if defined USE_MYTH_UNREACHABLE
#if (defined MYTH_ARCH_i386 || defined MYTH_ARCH_amd64)
#define myth_unreachable() asm volatile("ud2\n")
#elif GCC_VERSION >= 40500
#define myth_unreachable() __builtin_unreachable()
#else
#define myth_unreachable()
#endif
#else
#define myth_unreachable()
#endif



typedef struct myth_freelist_cell {
  struct myth_freelist_cell * next;
} myth_freelist_cell_t;
  
typedef struct {
  myth_freelist_cell_t * head;
} myth_freelist_t;

static inline void myth_freelist_init(myth_freelist_t * fl);
static inline void myth_freelist_push(myth_freelist_t * fl, void * h_);
static inline void * myth_freelist_pop(myth_freelist_t * fl);

static inline void * myth_malloc(size_t size);
static inline void myth_free(void *ptr,size_t size);
static inline void myth_free_no_size(void *ptr);
static inline void *myth_realloc(void *ptr,size_t size);
static inline void *myth_mmap(void *addr,size_t length,int prot,
			      int flags,int fd,off_t offset);
static inline int myth_munmap(void *addr,size_t length);

void myth_init_read_available_cpu_list(void);
int myth_get_cpu_num(void);
cpu_set_t myth_get_worker_cpuset(int rank);

#endif /* MYTH_MISC_H_ */
