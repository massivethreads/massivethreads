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
	myth_yield_body();
	return 0;
}

pthread_t pthread_self(void)
{
	return (pthread_t)myth_self_body();
}

int pthread_create(pthread_t *pth,const pthread_attr_t * attr,void *(*func)(void*),void *args)
{
	myth_thread_t mt;
	mt=myth_create_body(func,args);
	*pth=(pthread_t)mt;
	return 0;
}

int pthread_join(pthread_t th,void**ret)
{
	myth_join_body((myth_thread_t)th,ret);
	return 0;
}

int pthread_detach (pthread_t th)
{
	myth_detach_body((myth_thread_t)th);
	return 0;
}

int pthread_setcancelstate (int __state, int *__oldstate)
{
	return myth_setcancelstate_body(__state,__oldstate);
}

int pthread_setcanceltype (int __type, int *__oldtype)
{
	return myth_setcanceltype_body(__type,__oldtype);
}

int pthread_cancel (pthread_t __th)
{
	return myth_cancel_body((myth_thread_t)__th);
}

void pthread_testcancel(void)
{
	myth_testcancel_body();
}

/*
void __pthread_register_cancel (__pthread_unwind_buf_t *__buf)
{
	myth_thread_t th=myth_self_body();
	//TODO:Push this buffer
}

void __pthread_unregister_cancel (__pthread_unwind_buf_t *__buf)
{
	myth_thread_t th=myth_self_body();
	//TODO:Pop this buffer
}

void __pthread_unwind_next(__pthread_unwind_buf_t *__buf)
{
	__pthread_unwind_buf_t *__next_buf;
	//TODO:get next unwind buffer from __buf
	siglongjmp(__next_buf->__cancel_jmp_buf,1);
	myth_unreachable();
}*/

int pthread_key_create (pthread_key_t *__key,void (*__destr_function) (void *))
{
	return myth_key_create_body((myth_key_t*)__key,__destr_function);
}

int pthread_key_delete (pthread_key_t __key)
{
	return myth_key_delete_body((myth_key_t)__key);
}

void *pthread_getspecific (pthread_key_t __key)
{
	return myth_getspecific_body((myth_key_t)__key);
}

int pthread_setspecific (pthread_key_t __key,const void *__pointer)
{
	return myth_setspecific_body((myth_key_t)__key,(void*)__pointer);
}

int pthread_attr_setstacksize (pthread_attr_t *__attr,
				      size_t __stacksize)
{
	return 0;
}

int pthread_barrier_init (pthread_barrier_t * __barrier,__const pthread_barrierattr_t *	 __attr, unsigned int __count)
{
	*((myth_barrier_t*)__barrier)=myth_barrier_create_body(__count);
	return 0;
}

/* Destroy a previously dynamically initialized barrier BARRIER.  */
int pthread_barrier_destroy (pthread_barrier_t *__barrier)
{
	return myth_barrier_destroy_body(*((myth_barrier_t*)__barrier));
}

/* Wait on barrier BARRIER.  */
int pthread_barrier_wait (pthread_barrier_t *__barrier)
{
	return myth_barrier_wait_body(*((myth_barrier_t*)__barrier));
}

static inline void handle_mutex_initializer(pthread_mutex_t *mtx)
{
#ifdef MYTH_SUPPORT_MUTEX_INITIALIZER
#ifdef MYTH_UNSAFE_MUTEX_INITIALIZER
	myth_mutex_t *m=(myth_mutex_t*)&(mtx->__size[0]);
	if (*m)return;
#else
	static const pthread_mutex_t s_mtx_init=PTHREAD_MUTEX_INITIALIZER;
	if (mtx->__data.__lock!=s_mtx_init.__data.__lock)return;
	if (mtx->__data.__count!=s_mtx_init.__data.__count)return;
	if (mtx->__data.__owner!=s_mtx_init.__data.__owner)return;
	if (mtx->__data.__nusers!=s_mtx_init.__data.__nusers)return;
	//if (mtx->__data.__kind!=s_mtx_init.__data.__kind)return;
	if (mtx->__data.__spins!=s_mtx_init.__data.__spins)return;
	if (mtx->__data.__list.__next!=s_mtx_init.__data.__list.__next)return;
	if (mtx->__data.__list.__prev!=s_mtx_init.__data.__list.__prev)return;
#endif
	real_pthread_mutex_init(mtx,NULL);
#endif
}

int pthread_mutex_init (pthread_mutex_t *__mutex,
			       __const pthread_mutexattr_t *__mutexattr)
{
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	*mtx=myth_mutex_create_body();
	return 0;
}

int pthread_mutex_destroy (pthread_mutex_t *__mutex)
{
	handle_mutex_initializer(__mutex);
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	myth_mutex_destroy_body(*mtx);
	return 0;
}

int pthread_mutex_trylock (pthread_mutex_t *__mutex)
{
	handle_mutex_initializer(__mutex);
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	return myth_mutex_trylock_body(*mtx)?0:EBUSY;
}

int pthread_mutex_lock (pthread_mutex_t *__mutex)
{
	handle_mutex_initializer(__mutex);
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	myth_mutex_lock_body(*mtx);
	return 0;
}

int pthread_mutex_unlock (pthread_mutex_t *__mutex)
{
	handle_mutex_initializer(__mutex);
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	myth_mutex_unlock_body(*mtx);
	return 0;
}

int pthread_felock_init (pthread_mutex_t *__mutex,
			       __const pthread_mutexattr_t *__mutexattr)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	*mtx=myth_felock_create_body();
	return 0;
}

int pthread_felock_destroy (pthread_mutex_t *__mutex)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	myth_felock_destroy_body(*mtx);
	return 0;
}

int pthread_felock_lock (pthread_mutex_t *__mutex)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	myth_felock_lock_body(*mtx);
	return 0;
}

int pthread_felock_wait_lock (pthread_mutex_t *__mutex,int val)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	myth_felock_wait_lock_body(*mtx,val);
	return 0;
}

int pthread_felock_unlock (pthread_mutex_t *__mutex)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	myth_felock_unlock_body(*mtx);
	return 0;
}

int pthread_felock_set_unlock (pthread_mutex_t *__mutex,int val)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	myth_felock_set_unlock_body(*mtx,val);
	return 0;
}

int pthread_felock_status (pthread_mutex_t *__mutex)
{
	myth_felock_t *mtx=(myth_felock_t*)&(__mutex->__size[0]);
	return myth_felock_status_body(*mtx);
}

int pthread_cond_init (pthread_cond_t * __cond,
			      __const pthread_condattr_t *__cond_attr)
{
	myth_cond_t *cond=(myth_cond_t*)&(__cond->__size[0]);
	*cond=myth_cond_create_body();
	return 0;
}


int pthread_cond_destroy (pthread_cond_t *__cond)
{
	myth_cond_t *cond=(myth_cond_t*)&(__cond->__size[0]);
	myth_cond_destroy_body(*cond);
	return 0;
}

int pthread_cond_signal (pthread_cond_t *__cond)
{
	myth_cond_t *cond=(myth_cond_t*)&(__cond->__size[0]);
	myth_cond_signal_body(*cond);
	return 0;
}

int pthread_cond_broadcast (pthread_cond_t *__cond)
{
	myth_cond_t *cond=(myth_cond_t*)&(__cond->__size[0]);
	myth_cond_broadcast_body(*cond);
	return 0;
}

int pthread_cond_wait (pthread_cond_t * __cond,
			      pthread_mutex_t * __mutex)
{
	myth_cond_t *cond=(myth_cond_t*)&(__cond->__size[0]);
	myth_mutex_t *mtx=(myth_mutex_t*)&(__mutex->__size[0]);
	myth_cond_wait_body(*cond,*mtx);
	return 0;
}
