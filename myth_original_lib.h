#ifndef MYTH_ORIGINAL_LIB_H_
#define MYTH_ORIGINAL_LIB_H_

#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "myth_config.h"

#ifdef MYTH_WRAP_PTHREAD_FUNCTIONS
//pthread functions
extern int (*real_pthread_key_create) (pthread_key_t *,void (*)(void *));
extern int (*real_pthread_key_delete) (pthread_key_t);
extern void *(*real_pthread_getspecific)(pthread_key_t);
extern int (*real_pthread_setspecific) (pthread_key_t,const void *);
extern int (*real_pthread_create) (pthread_t *,const pthread_attr_t *,void *(*)(void *),void *);
extern int (*real_pthread_join) (pthread_t, void **);
extern int (*real_pthread_kill)(pthread_t, int);
extern int (*real_pthread_sigmask)(int,const sigset_t *,sigset_t *);
extern int (*real_pthread_mutex_init) (pthread_mutex_t *,const pthread_mutexattr_t *);
extern int (*real_pthread_mutex_destroy) (pthread_mutex_t *);
extern int (*real_pthread_mutex_lock) (pthread_mutex_t *);
extern int (*real_pthread_mutex_trylock) (pthread_mutex_t *);
extern int (*real_pthread_mutex_unlock) (pthread_mutex_t *);
extern int (*real_pthread_barrier_init) (pthread_barrier_t *,const pthread_barrierattr_t *, unsigned int);
extern int (*real_pthread_barrier_destroy) (pthread_barrier_t *);
extern int (*real_pthread_barrier_wait) (pthread_barrier_t *);
extern int (*real_pthread_setaffinity_np) (pthread_t, size_t,cpu_set_t *);
extern pthread_t (*real_pthread_self) (void);
extern int (*real_pthread_spin_init) (pthread_spinlock_t *, int);
extern int (*real_pthread_spin_destroy) (pthread_spinlock_t *);
extern int (*real_pthread_spin_lock) (pthread_spinlock_t *);
extern int (*real_pthread_spin_trylock) (pthread_spinlock_t *);
extern int (*real_pthread_spin_unlock) (pthread_spinlock_t *);

extern int (*real_sched_yield)(void);
#else
//pthread functions
#define real_pthread_key_create pthread_key_create
#define real_pthread_key_delete pthread_key_delete
#define real_pthread_getspecific pthread_getspecific
#define real_pthread_setspecific pthread_setspecific
#define real_pthread_create pthread_create
#define real_pthread_join pthread_join
#define real_pthread_kill pthread_kill
#define real_pthread_sigmask pthread_sigmask
#define real_pthread_mutex_init pthread_mutex_init
#define real_pthread_mutex_destroy pthread_mutex_destroy
#define real_pthread_mutex_lock pthread_mutex_lock
#define real_pthread_mutex_trylock pthread_mutex_trylock
#define real_pthread_mutex_unlock pthread_mutex_unlock
#define real_pthread_barrier_init pthread_barrier_init
#define real_pthread_barrier_destroy pthread_barrier_destroy
#define real_pthread_barrier_wait pthread_barrier_wait
#define real_pthread_setaffinity_np pthread_setaffinity_np
#define real_pthread_self pthread_self
#define real_pthread_spin_init pthread_spin_init
#define real_pthread_spin_destroy pthread_spin_destroy
#define real_pthread_spin_lock pthread_spin_lock
#define real_pthread_spin_trylock pthread_spin_trylock
#define real_pthread_spin_unlock pthread_spin_unlock

#define real_sched_yield sched_yield
#endif

#ifdef MYTH_WRAP_MALLOC

extern void *(*real_calloc)(size_t,size_t);
extern void *(*real_malloc)(size_t);
extern void (*real_free)(void *);
extern void *(*real_realloc)(void *,size_t);

#ifdef MYTH_WRAP_MALLOC_RUNTIME
extern int (*real_posix_memalign)(void **, size_t, size_t);
extern void *(*real_valloc)(size_t);
extern int g_wrap_malloc_completed;
extern int g_wrap_malloc;
#define ENV_MYTH_DONT_WRAP_MALLOC "MYTH_DONT_WRAP_MALLOC"
#endif

#else

#define real_calloc calloc
#define real_malloc malloc
#define real_free free
#define real_realloc realloc

#ifdef MYTH_WRAP_MALLOC_RUNTIME
#define real_posix_memalign posix_memalign
#define real_valloc valloc
extern int g_wrap_malloc_completed;
extern int g_wrap_malloc;
#define ENV_MYTH_DONT_WRAP_MALLOC "MYTH_DONT_WRAP_MALLOC"
#endif

#endif

#ifdef MYTH_WRAP_SOCKIO_FUNCTIONS
extern int (*real_socket)(int, int, int);
extern int (*real_connect)(int, const struct sockaddr *, socklen_t);
extern int (*real_accept)(int , struct sockaddr *,socklen_t *);
extern int (*real_bind)(int , const struct sockaddr *, socklen_t);
extern int (*real_listen)(int , int);
extern ssize_t (*real_send)(int , const void *, size_t , int );
extern ssize_t (*real_recv)(int , void *, size_t , int );
extern int (*real_close)(int );
extern int (*real_fcntl)(int , int , ...);
extern int (*real_select)(int, fd_set*, fd_set*,fd_set *, struct timeval *);
extern ssize_t (*real_sendto)(int, const void *, size_t, int,const struct sockaddr *, socklen_t);
extern ssize_t (*real_recvfrom)(int , void *, size_t , int ,struct sockaddr *, socklen_t *);
#else
#define real_socket socket
#define real_connect connect
#define real_accept accept
#define real_bind bind
#define real_listen listen
#define real_send send
#define real_recv recv
#define real_close close
#define real_fcntl fnctl
#define real_select select
#define real_sendto sendto
#define real_recvfrom recvfrom
#endif

//Initialize/Finalize
void myth_get_original_funcs(void);
void myth_free_original_funcs(void);

#endif /* MYTH_SYNC_H_ */
