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

//pthread functions
extern int (*real_pthread_key_create) (pthread_key_t *__key,void (*__destr_function) (void *));
extern int (*real_pthread_key_delete) (pthread_key_t __key);
extern void *(*real_pthread_getspecific)(pthread_key_t __key);
extern int (*real_pthread_setspecific) (pthread_key_t __key,__const void *__pointer);
extern int (*real_pthread_create) (pthread_t *__restrict __newthread,__const pthread_attr_t *__restrict __attr,void *(*__start_routine) (void *),void *__restrict __arg);
extern int (*real_pthread_join) (pthread_t __th, void **__thread_return);
extern int (*real_pthread_kill)(pthread_t __threadid, int __signo);
extern int (*real_pthread_sigmask)(int __how,__const __sigset_t *__restrict __newmask,__sigset_t *__restrict __oldmask);
extern int (*real_pthread_mutex_init) (pthread_mutex_t *__mutex,__const pthread_mutexattr_t *__mutexattr);
extern int (*real_pthread_mutex_destroy) (pthread_mutex_t *__mutex);
extern int (*real_pthread_mutex_lock) (pthread_mutex_t *__mutex);
extern int (*real_pthread_mutex_trylock) (pthread_mutex_t *__mutex);
extern int (*real_pthread_mutex_unlock) (pthread_mutex_t *__mutex);
extern int (*real_pthread_barrier_init) (pthread_barrier_t *__restrict __barrier,__const pthread_barrierattr_t *__restrict __attr, unsigned int __count);
extern int (*real_pthread_barrier_destroy) (pthread_barrier_t *__barrier);
extern int (*real_pthread_barrier_wait) (pthread_barrier_t *__barrier);
extern int (*real_pthread_setaffinity_np) (pthread_t __th, size_t __cpusetsize,__const cpu_set_t *__cpuset);
extern pthread_t (*real_pthread_self) (void);
extern int (*real_pthread_spin_init) (pthread_spinlock_t *__lock, int __pshared);
extern int (*real_pthread_spin_destroy) (pthread_spinlock_t *__lock);
extern int (*real_pthread_spin_lock) (pthread_spinlock_t *__lock);
extern int (*real_pthread_spin_trylock) (pthread_spinlock_t *__lock);
extern int (*real_pthread_spin_unlock) (pthread_spinlock_t *__lock);

extern int (*real_sched_yield)(void);

extern void *(*real_calloc)(size_t,size_t);
extern void *(*real_malloc)(size_t);
extern void (*real_free)(void *);
extern void *(*real_realloc)(void *,size_t);

extern int (*real_socket)(int __domain, int __type, int __protocol);
extern int (*real_connect)(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
extern int (*real_accept)(int __fd, __SOCKADDR_ARG __addr,socklen_t *__restrict __addr_len);
extern int (*real_bind)(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
extern int (*real_listen)(int __fd, int __n);
extern ssize_t (*real_send)(int __fd, __const void *__buf, size_t __n, int __flags);
extern ssize_t (*real_recv)(int __fd, void *__buf, size_t __n, int __flags);
extern int (*real_close)(int __fd);
extern int (*real_fcntl)(int __fd, int __cmd, ...);
extern int (*real_select)(int, fd_set*, fd_set*,fd_set *, struct timeval *);
extern ssize_t (*real_sendto)(int, const void *, size_t, int,const struct sockaddr *, socklen_t);
extern ssize_t (*real_recvfrom)(int , void *, size_t , int ,struct sockaddr *, socklen_t *);

//Initialize/Finalize
void myth_get_original_funcs(void);
void myth_free_original_funcs(void);

#endif /* MYTH_SYNC_H_ */
