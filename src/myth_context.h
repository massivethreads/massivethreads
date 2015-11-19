/* 
 * myth_context.h ---
 *  data structure related to context and
 *  must be included by the user
 */
#ifndef MYTH_CONTEXT_H_
#define MYTH_CONTEXT_H_

#include <stdint.h>
#include "myth/myth_config.h"

#if defined MYTH_ARCH_i386 && !defined MYTH_FORCE_UCONTEXT
#define MYTH_CONTEXT_ARCH_i386

#elif defined MYTH_ARCH_amd64 && !defined MYTH_FORCE_UCONTEXT
#define MYTH_CONTEXT_ARCH_amd64

#elif defined MYTH_ARCH_sparc && !defined MYTH_FORCE_UCONTEXT
#define MYTH_CONTEXT_ARCH_sparc
  #ifdef MYTH_ARCH_sparc_v9
  #define FRAMESIZE 176   /* Keep consistent with myth_context.S */
  #define STACKBIAS 2047  /* Do not change this */
  #define SAVE_FP   128
  #define SAVE_I7   136
  #else
  #define FRAMESIZE 92
  #define SAVE_FP   68
  #define SAVE_I7   72
  #endif

#elif defined MYTH_ARCH_UNIVERSAL || defined MYTH_FORCE_UCONTEXT
#include <ucontext.h>

#define MYTH_CONTEXT_ARCH_UNIVERSAL
#undef MYTH_INLINE_CONTEXT
#define MYTH_CONTEXT_ARCH_UNIVERSAL

#else
#error "Specify architecture"
#endif

/* Execution context */
typedef struct myth_context {
#if defined MYTH_CONTEXT_ARCH_i386
  uint32_t esp;
#elif defined MYTH_CONTEXT_ARCH_amd64
  uint64_t rsp;
#elif defined MYTH_CONTEXT_ARCH_sparc
#ifdef MYTH_ARCH_sparc_v9
  uint64_t sp;
#else
  uint32_t sp;
#endif
#elif defined MYTH_CONTEXT_ARCH_UNIVERSAL
  ucontext_t uc;
#else
#error "Architecture not defined"
#endif
} myth_context, *myth_context_t;

#endif	/* MYTH_CONTEXT_H_ */
