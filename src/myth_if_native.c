/*
 * myth_if_native.c
 */

#include "myth/myth.h"
#include "myth_config.h"
#include "myth_init.h"
#include "myth_misc.h"
#include "myth_sched.h"

#include "myth_init_func.h"
#include "myth_sync_func.h"
#include "myth_sched_func.h"
#include "myth_worker_func.h"
#include "myth_wls_func.h"
#include "myth_adws_sched_func.h"

/* --------------------------------
   --- global initialization functions
   -------------------------------- */

int myth_init(void) {
  return myth_init_ex_body(0);
}

int myth_init_ex(myth_globalattr_t* attr) {
  return myth_init_ex_body(attr);
}

void myth_fini(void) {
  myth_fini_body();
}

int myth_globalattr_init(myth_globalattr_t* attr) {
  return myth_globalattr_init_body(attr);
}

int myth_globalattr_destroy(myth_globalattr_t* attr) {
  return myth_globalattr_destroy_body(attr);
}

int myth_globalattr_get_stacksize(myth_globalattr_t* attr, size_t* stacksize) {
  return myth_globalattr_get_stacksize_body(attr, stacksize);
}

int myth_globalattr_set_stacksize(myth_globalattr_t* attr, size_t stacksize) {
  return myth_globalattr_set_stacksize_body(attr, stacksize);
}

int myth_globalattr_get_guardsize(myth_globalattr_t* attr, size_t* guardsize) {
  return myth_globalattr_get_guardsize_body(attr, guardsize);
}

int myth_globalattr_set_guardsize(myth_globalattr_t* attr, size_t guardsize) {
  return myth_globalattr_set_guardsize_body(attr, guardsize);
}

int myth_globalattr_get_n_workers(myth_globalattr_t* attr, size_t* n_workers) {
  return myth_globalattr_get_n_workers_body(attr, n_workers);
}

int myth_globalattr_set_n_workers(myth_globalattr_t* attr, size_t n_workers) {
  return myth_globalattr_set_n_workers_body(attr, n_workers);
}

int myth_globalattr_get_bind_workers(myth_globalattr_t* attr, int* bind_workers) {
  return myth_globalattr_get_bind_workers_body(attr, bind_workers);
}

int myth_globalattr_set_bind_workers(myth_globalattr_t* attr, int bind_workers) {
  return myth_globalattr_set_bind_workers_body(attr, bind_workers);
}

int myth_globalattr_get_child_first(myth_globalattr_t* attr, int* child_first) {
  return myth_globalattr_get_child_first_body(attr, child_first);
}

int myth_globalattr_set_child_first(myth_globalattr_t* attr, int child_first) {
  return myth_globalattr_set_child_first_body(attr, child_first);
}

/* --------------------------------------------------
   --- basic thread functions (myth_create, etc.)
   -------------------------------------------------- */

myth_thread_t myth_create(myth_func_t func, void* arg) {
  myth_thread_t id = 0;
  /* TODO: how to signal an error? */
  myth_create_ex_body(&id, 0, func, arg);
  return id;
}

int myth_create_ex(myth_thread_t*      id,
                   myth_thread_attr_t* attr,
		               myth_func_t         func,
                   void*               arg) {
  return myth_create_ex_body(id, attr, func, arg);
}

int myth_join(myth_thread_t th, void** result) {
  return myth_join_body(th,result);
}

int myth_tryjoin(myth_thread_t th, void** result) {
  return myth_tryjoin_body(th, result);
}

int myth_timedjoin(myth_thread_t th, void** result, const struct timespec *abstime) {
  return myth_timedjoin_body(th, result, abstime);
}

int myth_create_join_many_ex(myth_thread_t * ids,
			     myth_thread_attr_t * attrs,
			     myth_func_t func,
			     void * args,
			     void * results,
			     size_t id_stride,
			     size_t attr_stride,
			     size_t arg_stride,
			     size_t result_stride,
			     long nthreads) {
  return myth_create_join_many_ex_body(ids, attrs, func, args, results,
				       id_stride, attr_stride, arg_stride, result_stride,
				       nthreads);
}

int myth_create_join_various_ex(myth_thread_t * ids,
				myth_thread_attr_t * attrs,
				myth_func_t * funcs,
				void * args,
				void * results,
				size_t id_stride,
				size_t attr_stride,
				size_t func_stride,
				size_t arg_stride,
				size_t result_stride,
				long nthreads) {
  return myth_create_join_various_ex_body(ids, attrs, funcs, args, results,
					  id_stride, attr_stride, func_stride,
					  arg_stride, result_stride,
					  nthreads);
}

void myth_exit(void* ret) {
  myth_exit_body(ret);
}

int myth_detach(myth_thread_t th) {
  return myth_detach_body(th);
}

void myth_cleanup_thread(myth_thread_t th) {
  myth_cleanup_thread_body(th);
}

/* --------------------------------------------------
   --- worker functions
   -------------------------------------------------- */

int myth_get_worker_num(void) {
  return myth_get_worker_num_body();
}

int myth_get_num_workers(void) {
  return myth_get_num_workers_body();
}

int myth_is_myth_worker(void) {
  return myth_is_myth_worker_body();
}

/* -----------------------------
   --- thread attributes
   ----------------------------- */

int myth_thread_attr_init(myth_thread_attr_t * attr) {
  return myth_thread_attr_init_body(attr);
}

int myth_thread_attr_getdetachstate(const myth_thread_attr_t *attr,
				    int *detachstate) {
  return myth_thread_attr_getdetachstate_body(attr, detachstate);
}

int myth_thread_attr_setdetachstate(myth_thread_attr_t *attr,
				    int detachstate) {
  return myth_thread_attr_setdetachstate_body(attr, detachstate);
}

int myth_thread_attr_getguardsize(const myth_thread_attr_t *attr,
				  size_t *guardsize) {
  return myth_thread_attr_getguardsize_body(attr, guardsize);
}

int myth_thread_attr_setguardsize(myth_thread_attr_t *attr, size_t guardsize) {
  return myth_thread_attr_setguardsize_body(attr, guardsize);
}

int myth_thread_attr_getstacksize(const myth_thread_attr_t *attr, size_t *stacksize) {
  return myth_thread_attr_getstacksize_body(attr, stacksize);
}

int myth_thread_attr_setstacksize(myth_thread_attr_t *attr, size_t stacksize) {
  return myth_thread_attr_setstacksize_body(attr, stacksize);
}

int myth_thread_attr_getstack(const myth_thread_attr_t *attr,
			  void **stackaddr, size_t *stacksize) {
  return myth_thread_attr_getstack_body(attr, stackaddr, stacksize);
}

int myth_thread_attr_setstack(myth_thread_attr_t *attr,
			      void *stackaddr, size_t stacksize) {
  return myth_thread_attr_setstack_body(attr, stackaddr, stacksize);
}

int myth_getconcurrency(void) {
  return myth_getconcurrency_body();
}

/* ------------------------------
   --- yield
   ------------------------------ */

void myth_yield_ex(int yield_opt) {
  myth_yield_ex_body(yield_opt);
}

void myth_yield(void) {
  myth_yield_body();
}

int myth_sched_yield(void) {
  return myth_yield_body();
}

/* ------------------------------
   --- cancel
   ------------------------------ */

int myth_setcancelstate(int state, int *oldstate) {
  return myth_setcancelstate_body(state,oldstate);
}

int myth_setcanceltype(int type, int *oldtype) {
  return myth_setcanceltype_body(type,oldtype);
}

int myth_cancel(myth_thread_t th) {
  return myth_cancel_body(th);
}

void myth_testcancel(void) {
  myth_testcancel_body();
}


/* ----------------------------------
   --- once
   ---------------------------------- */

int myth_once(myth_once_t * once_control, void (*init_routine)(void)) {
  return myth_once_body(once_control, init_routine);
}

/* -------------------------
   --- mutex
   ------------------------- */

int myth_mutex_init(myth_mutex_t * mutex, const myth_mutexattr_t * attr) {
  return myth_mutex_init_body(mutex, attr);
}

int myth_mutex_destroy(myth_mutex_t * mutex) {
  return myth_mutex_destroy_body(mutex);
}

int myth_mutex_trylock(myth_mutex_t * mutex) {
  return myth_mutex_trylock_body(mutex);
}

int myth_mutex_lock(myth_mutex_t * mutex) {
  return myth_mutex_lock_body(mutex);
}

int myth_mutex_timedlock(myth_mutex_t *restrict mutex,
			 const struct timespec *restrict abstime) {
  return myth_mutex_timedlock_body(mutex, abstime);
}

int myth_mutex_unlock(myth_mutex_t * mutex) {
  return myth_mutex_unlock_body(mutex);
}

int myth_mutexattr_init(myth_mutexattr_t *attr) {
  return myth_mutexattr_init_body(attr);
}

int myth_mutexattr_destroy(myth_mutexattr_t *attr) {
  return myth_mutexattr_destroy_body(attr);
}

int myth_mutexattr_gettype(const myth_mutexattr_t *restrict attr,
			   int *restrict type) {
  return myth_mutexattr_gettype_body(attr, type);
}

int myth_mutexattr_settype(myth_mutexattr_t *attr, int type) {
  return myth_mutexattr_settype_body(attr, type);
}

/* ---------------------------
   --- reader-writer lock
   --------------------------- */

int myth_rwlock_init(myth_rwlock_t *restrict rwlock,
		     const myth_rwlockattr_t *restrict attr) {
  return myth_rwlock_init_body(rwlock, attr);
}
// myth_rwlock_t rwlock = MYTH_RWLOCK_INITIALIZER;

int myth_rwlock_destroy(myth_rwlock_t *rwlock) {
  return myth_rwlock_destroy_body(rwlock);
}

int myth_rwlock_rdlock(myth_rwlock_t *rwlock) {
  return myth_rwlock_rdlock_body(rwlock);
}

int myth_rwlock_tryrdlock(myth_rwlock_t *rwlock) {
  return myth_rwlock_tryrdlock_body(rwlock);
}

int myth_rwlock_timedrdlock(myth_rwlock_t *restrict rwlock,
			    const struct timespec *restrict abstime) {
  return myth_rwlock_timedrdlock_body(rwlock, abstime);
}

int myth_rwlock_wrlock(myth_rwlock_t *rwlock) {
  return myth_rwlock_wrlock_body(rwlock);
}

int myth_rwlock_trywrlock(myth_rwlock_t *rwlock) {
  return myth_rwlock_trywrlock_body(rwlock);
}

int myth_rwlock_timedwrlock(myth_rwlock_t *restrict rwlock,
			    const struct timespec *restrict abstime) {
  return myth_rwlock_timedwrlock_body(rwlock, abstime);
}

int myth_rwlock_unlock(myth_rwlock_t *rwlock) {
  return myth_rwlock_unlock_body(rwlock);
}

int myth_rwlockattr_init(myth_rwlockattr_t *attr) {
  return myth_rwlockattr_init_body(attr);
}

int myth_rwlockattr_destroy(myth_rwlockattr_t *attr) {
  return myth_rwlockattr_destroy_body(attr);
}

int myth_rwlockattr_getkind(const myth_rwlockattr_t *attr,
			    int *pref) {
  return myth_rwlockattr_getkind_body(attr, pref);
}

int myth_rwlockattr_setkind(myth_rwlockattr_t *attr,
			    int pref) {
  return myth_rwlockattr_setkind_body(attr, pref);
}

/* ------------------------------
   --- condition variables
   ------------------------------ */

int myth_cond_init(myth_cond_t * cond, const myth_condattr_t * attr) {
  return myth_cond_init_body(cond, attr);
}
// myth_cond_t cond = MYTH_COND_INITIALIZER;

int myth_cond_destroy(myth_cond_t * cond) {
  return myth_cond_destroy_body(cond);
}

int myth_cond_signal(myth_cond_t * cond) {
  return myth_cond_signal_body(cond);
}

int myth_cond_broadcast(myth_cond_t * cond) {
  return myth_cond_broadcast_body(cond);
}

int myth_cond_wait(myth_cond_t * cond, myth_mutex_t * mutex) {
  return myth_cond_wait_body(cond, mutex);
}

int myth_cond_timedwait(myth_cond_t *restrict cond,
			myth_mutex_t *restrict mutex,
			const struct timespec *restrict abstime) {
  return myth_cond_timedwait_body(cond, mutex, abstime);
}

int myth_condattr_init(myth_condattr_t *attr) {
  return myth_condattr_init_body(attr);
}

int myth_condattr_destroy(myth_condattr_t *attr) {
  return myth_condattr_destroy_body(attr);
}

/* ----------------------
   --- spin locks
   ---------------------- */

int myth_spin_init(myth_spinlock_t *lock) {
  return myth_spin_init_body(lock);
}

int myth_spin_destroy(myth_spinlock_t *lock) {
  return myth_spin_destroy_body(lock);
}

int myth_spin_lock(myth_spinlock_t *lock) {
  return myth_spin_lock_body(lock);
}

int myth_spin_trylock(myth_spinlock_t *lock) {
  return myth_spin_trylock_body(lock);
}

int myth_spin_unlock(myth_spinlock_t *lock) {
  return myth_spin_unlock_body(lock);
}

/* ----------------------------
   --- barrier
   ---------------------------- */

int myth_barrier_init(myth_barrier_t * barrier,
		      const myth_barrierattr_t * attr,
		      unsigned int nthreads) {
  return myth_barrier_init_body(barrier, attr, nthreads);
}

int myth_barrier_destroy(myth_barrier_t * barrier) {
  return myth_barrier_destroy_body(barrier);
}

int myth_barrier_wait(myth_barrier_t * barrier) {
  return myth_barrier_wait_body(barrier);
}

int myth_barrierattr_init(myth_barrierattr_t *attr) {
  return myth_barrierattr_init_body(attr);
}

int myth_barrierattr_destroy(myth_barrierattr_t *attr) {
  return myth_barrierattr_destroy_body(attr);
}

/* ----------------------------
   --- join counter
   ---------------------------- */

int myth_join_counter_init(myth_join_counter_t * jc,
			   const myth_join_counterattr_t * attr, int val) {
  return myth_join_counter_init_body(jc, attr, val);
}

int myth_join_counter_wait(myth_join_counter_t * jc) {
  return myth_join_counter_wait_body(jc);
}

int myth_join_counter_dec(myth_join_counter_t * jc) {
  return myth_join_counter_dec_body(jc);
}

int myth_join_counterattr_init(myth_join_counterattr_t * attr) {
  return myth_join_counterattr_init_body(attr);
}

int myth_join_counterattr_destroy(myth_join_counterattr_t * attr) {
  return myth_join_counterattr_destroy_body(attr);
}

/* ----------------------------
   --- full empty lock
   ---------------------------- */

int myth_felock_init(myth_felock_t * fe, const myth_felockattr_t * attr) {
  return myth_felock_init_body(fe, attr);
}

int myth_felock_destroy(myth_felock_t * fe) {
  return myth_felock_destroy_body(fe);
}

int myth_felock_lock(myth_felock_t * fe) {
  return myth_felock_lock_body(fe);
}

int myth_felock_unlock(myth_felock_t * fe) {
  return myth_felock_unlock_body(fe);
}

int myth_felock_wait_and_lock(myth_felock_t * fe, int status_to_wait) {
  return myth_felock_wait_and_lock_body(fe, status_to_wait);
}

int myth_felock_mark_and_signal(myth_felock_t * fe, int status_to_signal) {
  return myth_felock_mark_and_signal_body(fe, status_to_signal);
}

int myth_felock_status(myth_felock_t * fe) {
  return myth_felock_status_body(fe);
}

int myth_felockattr_init(myth_felockattr_t * attr) {
  return myth_felockattr_init_body(attr);
}

int myth_felockattr_destroy(myth_felockattr_t * attr) {
  return myth_felockattr_destroy_body(attr);
}

/* ------------------------------
   --- uncondition variables
   ------------------------------ */

int myth_uncond_init(myth_uncond_t * u) {
  return myth_uncond_init_body(u);
}

int myth_uncond_destroy(myth_uncond_t * u) {
  return myth_uncond_destroy_body(u);
}

int myth_uncond_wait(myth_uncond_t * u) {
  return myth_uncond_wait_body(u);
}

int myth_uncond_signal(myth_uncond_t * u) {
  return myth_uncond_signal_body(u);
}

/* --------------------------------
   --- thread local storage
   -------------------------------- */

int myth_key_create(myth_key_t *key, void (*destructor)(void *)) {
  return myth_key_create_body(key, destructor);
}

int myth_key_delete(myth_key_t key) {
  return myth_key_delete_body(key);
}

void *myth_getspecific(myth_key_t key) {
  return myth_getspecific_body(key);
}

int myth_setspecific(myth_key_t key, const void * pointer) {
  return myth_setspecific_body(key, pointer);
}

/* --------------------------------
   --- worker local storage
   -------------------------------- */

int myth_wls_key_create(myth_wls_key_t *key, void (*destructor)(void *)) {
  return myth_wls_key_create_body(key, destructor);
}

int myth_wls_key_delete(myth_wls_key_t key) {
  return myth_wls_key_delete_body(key);
}

void *myth_wls_getspecific(myth_wls_key_t key) {
  return myth_wls_getspecific_body(key);
}

int myth_wls_setspecific(myth_wls_key_t key, const void * pointer) {
  return myth_wls_setspecific_body(key, pointer);
}

/* --------------------------------
   --- thread-related functions
   -------------------------------- */

myth_thread_t myth_self(void) {
  return myth_self_body();
}

int myth_equal(myth_thread_t t1, myth_thread_t t2) {
  return myth_equal_body(t1, t2);
}

unsigned int myth_sleep(unsigned int s) {
  return myth_sleep_body(s);
}

int myth_usleep(useconds_t usec) {
  return myth_usleep_body(usec);
}

int myth_nanosleep(const struct timespec *req, struct timespec *rem) {
  return myth_nanosleep_body(req, rem);
}

/* --------------------------------
   --- logging and profiling functions
   -------------------------------- */

void myth_log_start(void) {
  myth_log_start_body();
}

void myth_log_pause(void) {
  myth_log_pause_body();
}

void myth_log_flush(void) {
  myth_log_flush_body();
}

void myth_log_reset(void) {
  myth_log_reset_body();
}

void myth_log_annotate_thread(myth_thread_t th,char *name) {
  myth_log_annotate_thread_body(th,name);
}

void myth_sched_prof_start(void) {
  myth_sched_prof_start_body();
}

void myth_sched_prof_pause(void) {
  myth_sched_prof_pause_body();
}

/* -----------------------------------------
   --- Almost Deterministic Work Stealing
   ----------------------------------------- */

myth_thread_t myth_adws_create_first(myth_func_t           func,
                                     void*                 arg,
                                     double                work,
                                     double                w_total,
                                     myth_workers_range_t* workers_range) {
#if MYTH_ENABLE_ADWS
  myth_thread_t id = 0;
  myth_adws_create_ex_first_body(&id, 0, func, arg, work, w_total, workers_range);
  return id;
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

myth_thread_t myth_adws_create(myth_func_t func,
                               void*       arg,
                               double      work) {
#if MYTH_ENABLE_ADWS
  myth_thread_t id = 0;
  myth_adws_create_ex_body(&id, 0, func, arg, work);
  return id;
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

int myth_adws_create_ex_first(myth_thread_t*        id,
                              myth_thread_attr_t*   attr,
                              myth_func_t           func,
                              void*                 arg,
                              double                work,
                              double                w_total,
                              myth_workers_range_t* workers_range) {
#if MYTH_ENABLE_ADWS
  return myth_adws_create_ex_first_body(id, attr, func, arg, work, w_total, workers_range);
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

int myth_adws_create_ex(myth_thread_t*      id,
                        myth_thread_attr_t* attr,
                        myth_func_t         func,
                        void*               arg,
                        double              work) {
#if MYTH_ENABLE_ADWS
  return myth_adws_create_ex_body(id, attr, func, arg, work);
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

int myth_adws_join(myth_thread_t th, void** result) {
#if MYTH_ENABLE_ADWS
  return myth_adws_join_body(th, result);
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

int myth_adws_join_last(myth_thread_t th, void** result, myth_workers_range_t workers_range) {
#if MYTH_ENABLE_ADWS
  return myth_adws_join_last_body(th, result, workers_range);
#else
  fprintf(stderr, "ADWS is disabled!\n");
  exit(0);
#endif
}

int myth_adws_get_stealable() {
#if MYTH_ENABLE_ADWS
  return myth_adws_get_stealable_body();
#else
  return -1;
#endif
}

void myth_adws_set_stealable(int flag) {
#if MYTH_ENABLE_ADWS
  return myth_adws_set_stealable_body(flag);
#else
  (void)flag;
#endif
}
