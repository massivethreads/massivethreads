/* 
 * myth_real_fun.c
 */
#include <assert.h>
#include <stdarg.h>

#include "myth/myth_config.h"
#include "myth_real_fun.h"
#include "myth_misc.h"
#include "myth_real_fun.h"

//#if MYTH_WRAP_PTHREAD || MYTH_WRAP_MALLOC || MYTH_WRAP_IO
#if MYTH_WRAP_PTHREAD || MYTH_WRAP_IO
#include <dlfcn.h>
#include <link.h>
#endif

/* this is how function-wrapping works.

   let's say we want to wrap a function (e.g., pthread_create).
   it means when user program calls the function, it calls the version
   defined in massivethreads library. this is simply done by
   having a .c file defining these functions. for example,
   myth_pthread.c defines pthread_create that calls an 
   myth_create_body.
   a complication arises when massivethreads internally uses
   the original version of pthread_create, defined in libpthread.
   to ensure massivethreads calls the original version, 
   massivethreads never calls these function directly. instead,
   it calls real_pthread_create function (defined in this file)
   whenever it wants to call the original pthread_create.

   depending on compile time flags real_pthread_create behaves
   differently. when MYTH_WRAP_PTHREAD is zero, then it simply
   calls pthread_create.  this version does not expose the wrapped
   version of pthread_create, so thigs are straightforward.
   otherwise it first dynamically obtains the address of pthread_create
   defined in libpthread. 

   a difficulty here is how to know from which file massivethreads
   can find the right symbol.  in the previous implementation it
   used to use dlsym(RTLD_NEXT, "pthread_create"), which means
   the symbol is searched for from the "next" shared library in the
   link command line.  this works as long as -lpthread does not come
   before -lmyth in the link command line.  that is, it works if the link
   command line looks like gcc ... -lmyth -lpthread; if you, for 
   whatever reasons, do something like gcc ... -lpthread -lmyth, however, 
   dlsym fails to load pthread_create.
   
 */


typedef struct {
  int (*pthread_key_create)(pthread_key_t *, void (*)(void *));
  int (*pthread_key_delete)(pthread_key_t);
  void * (*pthread_getspecific)(pthread_key_t);
  int (*pthread_setspecific)(pthread_key_t, const void *);
  int (*pthread_create)(pthread_t * , const pthread_attr_t *, void *(*)(void *), void *);
  int (*pthread_join)(pthread_t, void **);
  int (*pthread_kill)(pthread_t, int);
  int (*pthread_sigmask)(int, const sigset_t *, sigset_t *);
  int (*pthread_mutex_init)(pthread_mutex_t *, const pthread_mutexattr_t *);
  int (*pthread_mutex_destroy)(pthread_mutex_t *);
  int (*pthread_mutex_lock)(pthread_mutex_t *);
  int (*pthread_mutex_trylock)(pthread_mutex_t *);
  int (*pthread_mutex_unlock)(pthread_mutex_t *);
  int (*pthread_barrier_init)(pthread_barrier_t *, const pthread_barrierattr_t *, unsigned int);
  int (*pthread_barrier_destroy)(pthread_barrier_t *);
  int (*pthread_barrier_wait)(pthread_barrier_t *);
  int (*pthread_setaffinity_np)(pthread_t, size_t, const cpu_set_t *);
  pthread_t (*pthread_self)(void);
  int (*pthread_spin_init)(pthread_spinlock_t *, int);
  int (*pthread_spin_destroy)(pthread_spinlock_t *);
  int (*pthread_spin_lock)(pthread_spinlock_t *);
  int (*pthread_spin_trylock)(pthread_spinlock_t *);
  int (*pthread_spin_unlock)(pthread_spinlock_t *);

  int (*sched_yield)(void);
  unsigned int (*sleep)(unsigned int);
  int (*usleep)(useconds_t usec);
  int (*nanosleep)(const struct timespec *req, struct timespec *rem);
  void * (*calloc)(size_t,size_t);
  void * (*malloc)(size_t);
  void (*free)(void *);
  void * (*realloc)(void *,size_t);
  int (*posix_memalign)(void **, size_t, size_t);
  void * (*aligned_alloc)(size_t, size_t);
  void * (*valloc)(size_t);
  void * (*memalign)(size_t, size_t);
  void * (*pvalloc)(size_t);

  int (*socket)(int, int, int);
  int (*connect)(int, const struct sockaddr *, socklen_t);
  int (*accept)(int , struct sockaddr *,socklen_t *);
  int (*bind)(int , const struct sockaddr *, socklen_t);
  int (*listen)(int , int);
  ssize_t (*send)(int , const void *, size_t , int );
  ssize_t (*recv)(int , void *, size_t , int );
  int (*close)(int );
  int (*fcntl)(int , int , ...);
  int (*select)(int, fd_set*, fd_set*,fd_set *, struct timeval *);
  ssize_t (*sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
  ssize_t (*recvfrom)(int , void *, size_t, int , struct sockaddr *, socklen_t *);

} real_function_table_t;

static real_function_table_t real_function_table = {
  //#if MYTH_WRAP_PTHREAD || MYTH_WRAP_MALLOC || MYTH_WRAP_IO
#if MYTH_WRAP_PTHREAD || MYTH_WRAP_IO
#else
  .pthread_key_create = pthread_key_create,
  .pthread_key_delete = pthread_key_delete,
  .pthread_getspecific = pthread_getspecific,
  .pthread_setspecific = pthread_setspecific,
  .pthread_create = pthread_create,
  .pthread_join = pthread_join,
  .pthread_kill = pthread_kill,
  .pthread_sigmask = pthread_sigmask,
  .pthread_mutex_init = pthread_mutex_init,
  .pthread_mutex_destroy = pthread_mutex_destroy,
  .pthread_mutex_lock = pthread_mutex_lock,
  .pthread_mutex_trylock = pthread_mutex_trylock,
  .pthread_mutex_unlock = pthread_mutex_unlock,
  .pthread_barrier_init = pthread_barrier_init,
  .pthread_barrier_destroy = pthread_barrier_destroy,
  .pthread_barrier_wait = pthread_barrier_wait,
  .pthread_setaffinity_np = pthread_setaffinity_np,
  .pthread_self = pthread_self,
  .pthread_spin_init = pthread_spin_init,
  .pthread_spin_destroy = pthread_spin_destroy,
  .pthread_spin_lock = pthread_spin_lock,
  .pthread_spin_trylock = pthread_spin_trylock,
  .pthread_spin_unlock = pthread_spin_unlock,

  .sched_yield = sched_yield,
  .sleep = sleep,
  .usleep = usleep,
  .nanosleep = nanosleep,

  .calloc = calloc,
  .malloc = malloc,
  .free = free,
  .realloc = realloc,
  .posix_memalign = posix_memalign,
  .aligned_alloc = aligned_alloc,
  .valloc = valloc,
  .memalign = memalign,
  .pvalloc = pvalloc,

  .socket = socket,
  .connect = connect,
  .accept = accept,
  .bind = bind,
  .listen = listen,
  .send = send,
  .recv = recv,
  .close = close,
  .fcntl = fcntl,
  .select = select,
  .sendto = sendto,
  .recvfrom = recvfrom,
#endif
};

static void ensure_real_functions() {
  assert(0);
}

int real_pthread_key_create(pthread_key_t * key, void (*destructor)(void *)) {
  if (!real_function_table.pthread_key_create) ensure_real_functions();
  return real_function_table.pthread_key_create(key, destructor);
}

int real_pthread_key_delete(pthread_key_t key) {
  if (!real_function_table.pthread_key_delete) ensure_real_functions();
  return real_pthread_key_delete(key);
}

void * real_pthread_getspecific(pthread_key_t key) {
  if (!real_function_table.pthread_getspecific) ensure_real_functions();
  return real_function_table.pthread_getspecific(key);
}

int real_pthread_setspecific(pthread_key_t key, const void * val) {
  if (!real_function_table.pthread_setspecific) ensure_real_functions();
  return real_function_table.pthread_setspecific(key, val);
}

int real_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
			void *(*start_routine)(void *), void * arg) {
  if (!real_function_table.pthread_create) ensure_real_functions();
  return real_function_table.pthread_create(thread, attr, start_routine, arg);
}

int real_pthread_join(pthread_t thread, void ** status) {
  if (!real_function_table.pthread_join) ensure_real_functions();
  return real_function_table.pthread_join(thread, status);
}

int real_pthread_kill(pthread_t thread, int sig) {
  if (!real_function_table.pthread_kill) ensure_real_functions();
  return real_function_table.pthread_kill(thread, sig);
}

int real_pthread_sigmask(int how, const sigset_t * set, sigset_t * oldset) {
  if (!real_function_table.pthread_sigmask) ensure_real_functions();
  return real_function_table.pthread_sigmask(how, set, oldset);
}

int real_pthread_mutex_init(pthread_mutex_t * mutex,
			    const pthread_mutexattr_t * attr) {
  if (!real_function_table.pthread_mutex_init) ensure_real_functions();
  return real_function_table.pthread_mutex_init(mutex, attr);
}

int real_pthread_mutex_destroy(pthread_mutex_t * mutex) {
  if (!real_function_table.pthread_mutex_destroy) ensure_real_functions();
  return real_function_table.pthread_mutex_destroy(mutex);
}

int real_pthread_mutex_lock(pthread_mutex_t * mutex) {
  if (!real_function_table.pthread_mutex_lock) ensure_real_functions();
  return real_function_table.pthread_mutex_lock(mutex);
}

int real_pthread_mutex_trylock(pthread_mutex_t * mutex) {
  if (!real_function_table.pthread_mutex_trylock) ensure_real_functions();
  return real_function_table.pthread_mutex_trylock(mutex);
}

int real_pthread_mutex_unlock(pthread_mutex_t * mutex) {
  if (!real_function_table.pthread_mutex_unlock) ensure_real_functions();
  return real_function_table.pthread_mutex_unlock(mutex);
}

int real_pthread_barrier_init(pthread_barrier_t * barrier,
			      const pthread_barrierattr_t * attr,
			      unsigned int count) {
  if (!real_function_table.pthread_barrier_init) ensure_real_functions();
  return real_function_table.pthread_barrier_init(barrier, attr, count);
}


int real_pthread_barrier_destroy(pthread_barrier_t * barrier) {
  if (!real_function_table.pthread_barrier_destroy) ensure_real_functions();
  return real_function_table.pthread_barrier_destroy(barrier);
}

int real_pthread_barrier_wait(pthread_barrier_t * barrier) {
  if (!real_function_table.pthread_barrier_wait) ensure_real_functions();
  return real_function_table.pthread_barrier_wait(barrier);
}


int real_pthread_setaffinity_np(pthread_t thread,
				size_t cpusetsize, const cpu_set_t * cpuset) {
  if (!real_function_table.pthread_setaffinity_np) ensure_real_functions();
  return real_function_table.pthread_setaffinity_np(thread, cpusetsize, cpuset);
}

pthread_t real_pthread_self() {
  if (!real_function_table.pthread_self) ensure_real_functions();
  return real_function_table.pthread_self();
}

int real_pthread_spin_init(pthread_spinlock_t * lock, int pshared) {
  if (!real_function_table.pthread_spin_init) ensure_real_functions();
  return real_function_table.pthread_spin_init(lock, pshared);
}


int real_pthread_spin_destroy(pthread_spinlock_t * lock) {
  if (!real_function_table.pthread_spin_destroy) ensure_real_functions();
  return real_function_table.pthread_spin_destroy(lock);
}

int real_pthread_spin_lock(pthread_spinlock_t * lock) {
  if (!real_function_table.pthread_spin_lock) ensure_real_functions();
  return real_function_table.pthread_spin_lock(lock);
}

int real_pthread_spin_trylock(pthread_spinlock_t * lock) {
  if (!real_function_table.pthread_spin_trylock) ensure_real_functions();
  return real_function_table.pthread_spin_trylock(lock);
}

int real_pthread_spin_unlock(pthread_spinlock_t * lock) {
  if (!real_function_table.pthread_spin_unlock) ensure_real_functions();
  return real_function_table.pthread_spin_unlock(lock);
}

int real_sched_yield() {
  if (!real_function_table.sched_yield) ensure_real_functions();
  return real_function_table.sched_yield();
}

int real_sleep(unsigned int seconds) {
  if (!real_function_table.sleep) ensure_real_functions();
  return real_function_table.sleep(seconds);
}

int real_usleep(useconds_t usec) {
  if (!real_function_table.usleep) ensure_real_functions();
  return real_function_table.usleep(usec);
}

int real_nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!real_function_table.nanosleep) ensure_real_functions();
  return real_function_table.nanosleep(req, rem);
}

void * real_calloc(size_t nmemb, size_t size) {
  if (!real_function_table.calloc) ensure_real_functions();
  return real_function_table.calloc(nmemb, size);
}

void *real_malloc(size_t size) {
  if (!real_function_table.malloc) ensure_real_functions();
  return real_function_table.malloc(size);
}

void real_free(void * ptr) {
  if (!real_function_table.free) ensure_real_functions();
  return real_function_table.free(ptr);
}

void * real_realloc(void * ptr, size_t size) {
  if (!real_function_table.realloc) ensure_real_functions();
  return real_function_table.realloc(ptr, size);
}

int real_posix_memalign(void ** memptr, size_t alignment, size_t size) {
  if (!real_function_table.posix_memalign) ensure_real_functions();
  return real_function_table.posix_memalign(memptr, alignment, size);

}

void * real_aligned_alloc(size_t alignment, size_t size) {
  if (!real_function_table.aligned_alloc) ensure_real_functions();
  return real_function_table.aligned_alloc(alignment, size);
}

void * real_valloc(size_t size) {
  if (!real_function_table.valloc) ensure_real_functions();
  return real_function_table.valloc(size);
}

void * real_memalign(size_t alignment, size_t size) {
  if (!real_function_table.memalign) ensure_real_functions();
  return real_function_table.memalign(alignment, size);
}

void * real_pvalloc(size_t size) {
  if (!real_function_table.pvalloc) ensure_real_functions();
  return real_function_table.pvalloc(size);
}

int real_socket(int domain, int type, int protocol) {
  if (!real_function_table.socket) ensure_real_functions();
  return real_function_table.socket(domain, type, protocol);
}

int real_connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen) {
  if (!real_function_table.connect) ensure_real_functions();
  return real_function_table.connect(sockfd, addr, addrlen);
}

int real_accept(int sockfd, struct sockaddr * addr, socklen_t * addrlen) {
  if (!real_function_table.accept) ensure_real_functions();
  return real_function_table.accept(sockfd, addr, addrlen);
}

int real_bind(int sockfd, const struct sockaddr * addr, socklen_t addrlen) {
  if (!real_function_table.bind) ensure_real_functions();
  return real_function_table.bind(sockfd, addr, addrlen);
}

int real_listen(int sockfd, int backlog) {
  if (!real_function_table.listen) ensure_real_functions();
  return real_function_table.listen(sockfd, backlog);
}

ssize_t real_send(int sockfd, const void * buf, size_t len, int flags) {
  if (!real_function_table.send) ensure_real_functions();
  return real_function_table.send(sockfd, buf, len, flags);
}

ssize_t real_recv(int sockfd, void * buf, size_t len, int flags) {
  if (!real_function_table.recv) ensure_real_functions();
  return real_function_table.recv(sockfd, buf, len, flags);
}

int real_close(int fd) {
  if (!real_function_table.close) ensure_real_functions();
  return real_function_table.close(fd);
}

int real_fcntl(int fd, int cmd, ...) {
  if (!real_function_table.fcntl) ensure_real_functions();
  switch (cmd) {
    /* cmd not taking any arg */
  case F_GETFD:
  case F_GETFL:
  case F_GETOWN:
  case F_GETSIG:
  case F_GETLEASE:
#if defined(F_GETPIPE_SZ)
  case F_GETPIPE_SZ:		/*  (void; since Linux 2.6.35) */
#endif
#if defined(F_ADD_SEALS)
  case F_GET_SEALS:		/*  (void; since Linux 3.17) */
#endif
    return real_function_table.fcntl(fd, cmd);
    /* cmd taking int */
  case F_DUPFD:
#if defined(F_DUPFD_CLOEXEC)
  case F_DUPFD_CLOEXEC:		/* (int; since Linux 2.6.24) */
#endif
  case F_SETFD:
  case F_SETFL:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
#if defined(F_SETPIPE_SZ)
  case F_SETPIPE_SZ:		/*  (int; since Linux 2.6.35) */
#endif
#if defined(F_ADD_SEALS)
  case F_ADD_SEALS:		/*  (int; since Linux 3.17) */
#endif
    /* cmd taking an int */
    {
      va_list ap;
      va_start(ap, cmd);
      int arg = va_arg(ap, int);
      va_end(ap);
      return real_function_table.fcntl(fd, cmd, arg);
    }
  case F_SETLK:
  case F_SETLKW:
  case F_GETLK:
  case F_OFD_SETLK:
  case F_OFD_SETLKW:
  case F_OFD_GETLK:
#if defined(F_GETOWN_EX)
  case F_GETOWN_EX:	 /* (since Linux 2.6.32) */
#endif
#if defined(F_SETOWN_EX)
  case F_SETOWN_EX: /* (since Linux 2.6.32)  */
#endif
    /* cmd taking a pointer */
    {
      va_list ap;
      va_start(ap, cmd);
      void * arg = va_arg(ap, void*);
      va_end(ap);
      return real_function_table.fcntl(fd, cmd, arg);
    }
  default:
    /* punt; assume int-taking */
    /* cmd taking a pointer */
    {
      va_list ap;
      va_start(ap, cmd);
      void * arg = va_arg(ap, void*);
      va_end(ap);
      return real_function_table.fcntl(fd, cmd, arg);
    }
  }
}

int real_select(int nfds, fd_set * readfds, fd_set * writefds,
		fd_set * exceptfds, struct timeval * timeout) {
  if (!real_function_table.select) ensure_real_functions();
  return real_function_table.select(nfds, readfds, writefds, exceptfds, timeout);
}

ssize_t real_sendto(int sockfd, const void * buf, size_t len, int flags,
		    const struct sockaddr * dest_addr, socklen_t addrlen) {
  if (!real_function_table.sendto) ensure_real_functions();
  return real_function_table.sendto(sockfd, buf, len,
				    flags, dest_addr, addrlen);
}

ssize_t real_recvfrom(int sockfd, void * buf, size_t len, int flags,
		      struct sockaddr * src_addr, socklen_t * addrlen) {
  if (!real_function_table.recvfrom) ensure_real_functions();
  return real_function_table.recvfrom(sockfd, buf, len, flags,
				      src_addr, addrlen);
}

#if 0
void __attribute__((constructor)) myth_get_original_funcs(void)
{
  myth_get_pthread_funcs();
  myth_get_io_funcs();
}

void __attribute__((destructor)) myth_free_original_funcs(void)
{
  myth_free_pthread_funcs();
  myth_free_io_funcs();
}
#endif
