//myth_pthread_if.c : pthread-like interface

#include "myth_init.h"
#include "myth_sched.h"
#include "myth_worker.h"
#include "myth_io.h"
#include "myth_tls.h"

#include "myth_sched_proto.h"
#include "myth_io_proto.h"
#include "myth_tls_proto.h"

#include "myth_worker_func.h"
#include "myth_io_func.h"
#include "myth_sync_func.h"
#include "myth_sched_func.h"
#include "myth_tls_func.h"

#include "myth_if_pthread.h"

int sched_yield(void)
{

	real_sched_yield();
	myth_yield_body(1);
	return 0;
}

pthread_t pthread_self(void)
{
	return (pthread_t)myth_self_body();
}

int pthread_create(pthread_t *pth, const pthread_attr_t * attr, 
		   void *(*func)(void*),void *args)
{
  (void)attr;
	myth_thread_t mt;
	mt=myth_create_body(func,args,0);
	*pth=(pthread_t)mt;
	return 0;
}

int pthread_join(pthread_t th,void**ret)
{
	myth_join_body((myth_thread_t)th,ret);
	return 0;
}

void pthread_exit(void *ret)
{
	myth_exit_body(ret);
	//To avoid warning, this code is unreachable
	while (1) { }
}

int pthread_detach (pthread_t th)
{
	myth_detach_body((myth_thread_t)th);
	return 0;
}

int pthread_setcancelstate (int state, int *oldstate)
{
	return myth_setcancelstate_body(state,oldstate);
}

int pthread_setcanceltype (int type, int *oldtype)
{
	return myth_setcanceltype_body(type,oldtype);
}

int pthread_cancel (pthread_t th)
{
	return myth_cancel_body((myth_thread_t)th);
}

void pthread_testcancel(void)
{
	myth_testcancel_body();
}

int pthread_key_create (pthread_key_t *key,void (*destructor) (void *))
{
	return myth_key_create_body((myth_key_t*)key,destructor);
}

int pthread_key_delete (pthread_key_t key)
{
	return myth_key_delete_body((myth_key_t)key);
}

void *pthread_getspecific (pthread_key_t key)
{
	return myth_getspecific_body((myth_key_t)key);
}

int pthread_setspecific (pthread_key_t key,const void *ptr)
{
	return myth_setspecific_body((myth_key_t)key,(void*)ptr);
}

/* ---------- pthread_mutex ---------- */

static inline void handle_mutex_initializer(pthread_mutex_t *mtx)
{
#ifdef MYTH_SUPPORT_MUTEX_INITIALIZER
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  if (memcmp(&m, mtx, sizeof(pthread_mutex_t)) == 0) {
    /* mtx appears to have been initialized by PTHREAD_MUTEX_INITIALIZER */
    pthread_mutex_init(mtx, 0);
  }
#endif
}

int pthread_mutex_init(pthread_mutex_t *mutex,
		       const pthread_mutexattr_t *attr) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  assert(sizeof(myth_mutexattr_t) <= sizeof(pthread_mutexattr_t));
  return myth_mutex_init_body((myth_mutex_t *)mutex, 
			      (const myth_mutexattr_t *)attr);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  handle_mutex_initializer(mutex);
  return myth_mutex_destroy_body((myth_mutex_t *)mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  handle_mutex_initializer(mutex);
  return myth_mutex_trylock_body((myth_mutex_t *)mutex);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  handle_mutex_initializer(mutex);
  return myth_mutex_lock_body((myth_mutex_t *)mutex);
}

int pthread_mutex_unlock (pthread_mutex_t *mutex) {
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  handle_mutex_initializer(mutex);
  return myth_mutex_unlock_body((myth_mutex_t *)mutex);
}


int pthread_cond_init (pthread_cond_t * cond,
		       const pthread_condattr_t *cond_attr) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  assert(sizeof(myth_condattr_t) <= sizeof(pthread_condattr_t));
  return myth_cond_init_body((myth_cond_t *)cond, 
			     (const myth_condattr_t *)cond_attr);
}

int pthread_cond_destroy(pthread_cond_t *cond) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  return myth_cond_destroy_body((myth_cond_t *)cond);
}

int pthread_cond_signal(pthread_cond_t * cond)
{
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  return myth_cond_signal_body((myth_cond_t *)cond);
}

int pthread_cond_broadcast(pthread_cond_t * cond) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  return myth_cond_broadcast_body((myth_cond_t *)cond);
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
  assert(sizeof(myth_cond_t) <= sizeof(pthread_cond_t));
  assert(sizeof(myth_mutex_t) <= sizeof(pthread_mutex_t));
  return myth_cond_wait_body((myth_cond_t *)cond, (myth_mutex_t *)mutex);
}

int pthread_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, 
			 unsigned int count) {
  assert(sizeof(myth_barrier_t) <= sizeof(pthread_barrier_t));
  assert(sizeof(myth_barrierattr_t) <= sizeof(pthread_barrierattr_t));
  return myth_barrier_init_body((myth_barrier_t *)barrier,
				(myth_barrierattr_t *)attr,
				count);
}

/* Destroy a previously dynamically initialized barrier BARRIER.  */
int pthread_barrier_destroy(pthread_barrier_t *barrier) {
  assert(sizeof(myth_barrier_t) <= sizeof(pthread_barrier_t));
  return myth_barrier_destroy_body((myth_barrier_t *)barrier);
}

/* Wait on barrier BARRIER.  */
int pthread_barrier_wait (pthread_barrier_t *barrier) {
  return myth_barrier_wait_body((myth_barrier_t*)barrier);
}

