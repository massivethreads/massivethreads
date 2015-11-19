/* 
 * myth_internal_lock.h
 */
#pragma once
#ifndef MYTH_INTERNAL_LOCK_H_
#define MYTH_INTERNAL_LOCK_H_

#include "myth/myth_config.h"

#ifdef MYTH_INTERNAL_LOCK_MUTEX
typedef pthread_mutex_t myth_internal_lock_t;

#elif defined MYTH_INTERNAL_LOCK_SPINLOCK1
typedef pthread_spinlock_t myth_internal_lock_t;

#elif defined MYTH_INTERNAL_LOCK_SPINLOCK2
#if (defined MYTH_ARCH_i386) || (defined MYTH_ARCH_amd64)
typedef volatile int myth_internal_lock_t;

#elif (defined MYTH_ARCH_sparc)
#warning "Inlined spinlock is not provided in this architecture, substituted by pthread_spin"
typedef pthread_spinlock_t myth_internal_lock_t;

#else
#warning "Inlined spinlock is not provided in this architecture, substituted by pthread_spin"
typedef pthread_spinlock_t myth_internal_lock_t;

#endif
#else
#error "Please choose internal locking method"
#endif

#endif /* MYTH_INTERNAL_LOCK_H_ */
