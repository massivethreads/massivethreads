/* 
 * myth_real_fun.h
 */
#pragma once
#ifndef MYTH_REAL_FUN_H_
#define MYTH_REAL_FUN_H_

#define MYTH_WRAP_PTHREAD 0
//#define MYTH_WRAP_MALLOC 0
#define MYTH_WRAP_IO 0

#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>


int real_pthread_key_create(pthread_key_t * key, void (*destructor)(void *));
int real_pthread_key_delete(pthread_key_t key);
void * real_pthread_getspecific(pthread_key_t key);
int real_pthread_setspecific(pthread_key_t key, const void * val);
int real_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
			void *(*start_routine)(void *), void * arg);
int real_pthread_join(pthread_t thread, void ** status);
int real_pthread_kill(pthread_t thread, int sig);
int real_pthread_sigmask(int how, const sigset_t * set, sigset_t * oldset);
int real_pthread_mutex_init(pthread_mutex_t * mutex,
			    const pthread_mutexattr_t * attr);
int real_pthread_mutex_destroy(pthread_mutex_t * mutex);
int real_pthread_mutex_lock(pthread_mutex_t * mutex);
int real_pthread_mutex_trylock(pthread_mutex_t * mutex);
int real_pthread_mutex_unlock(pthread_mutex_t * mutex);
int real_pthread_barrier_init(pthread_barrier_t * barrier,
			      const pthread_barrierattr_t * attr,
			      unsigned int count);
int real_pthread_barrier_destroy(pthread_barrier_t * barrier);
int real_pthread_barrier_wait(pthread_barrier_t * barrier);
int real_pthread_setaffinity_np(pthread_t thread,
				size_t cpusetsize, const cpu_set_t * cpuset);
pthread_t real_pthread_self(void);
int real_pthread_spin_init(pthread_spinlock_t * lock, int pshared);
int real_pthread_spin_destroy(pthread_spinlock_t * lock);
int real_pthread_spin_lock(pthread_spinlock_t * lock);
int real_pthread_spin_trylock(pthread_spinlock_t * lock);
int real_pthread_spin_unlock(pthread_spinlock_t * lock);
int real_sched_yield(void);
int real_sleep(unsigned int seconds);
int real_usleep(useconds_t usec);
int real_nanosleep(const struct timespec *req, struct timespec *rem);

void * real_calloc(size_t nmemb, size_t size);
void *real_malloc(size_t size);
void real_free(void * ptr);
void * real_realloc(void * ptr, size_t size);
int real_posix_memalign(void ** memptr, size_t alignment, size_t size);
void * real_aligned_alloc(size_t alignment, size_t size);
void * real_valloc(size_t size);
void * real_memalign(size_t alignment, size_t size);
void * real_pvalloc(size_t size);

int real_socket(int domain, int type, int protocol);
int real_connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen);
int real_accept(int sockfd, struct sockaddr * addr, socklen_t * addrlen);
int real_bind(int sockfd, const struct sockaddr * addr, socklen_t addrlen);
int real_listen(int sockfd, int backlog);
ssize_t real_send(int sockfd, const void * buf, size_t len, int flags);
ssize_t real_recv(int sockfd, void * buf, size_t len, int flags);
int real_close(int fd);
int real_fcntl(int fd, int cmd, ...);
int real_select(int nfds, fd_set * readfds, fd_set * writefds,
		fd_set * exceptfds, struct timeval * timeout);
ssize_t real_sendto(int sockfd, const void * buf, size_t len, int flags,
		    const struct sockaddr * dest_addr, socklen_t addrlen);
ssize_t real_recvfrom(int sockfd, void * buf, size_t len, int flags,
		      struct sockaddr * src_addr, socklen_t * addrlen);

#endif	/* MYTH_REAL_FUN_H_ */
