#ifndef MYTH_SYNC_FUNC_H_
#define MYTH_SYNC_FUNC_H_

#include "myth_sync.h"
#include "myth_sleep_queue_func.h"

//#define ALIGN_MUTEX

/* ----------- mutex ----------- */

MYTH_CTX_CALLBACK void myth_mutex_lock_1(void *arg1,void *arg2,void *arg3) {
  myth_sleep_queue_t * q = arg1;
  myth_thread_t cur = arg2;
  myth_sleep_queue_enq(q, cur);
}

static inline void myth_mutex_block_on_queue(myth_sleep_queue_t * q) {
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t cur = env->this_thread;
  /* pop next thread to run */
  myth_thread_t next = myth_queue_pop(&env->runnable_q);
  myth_context_t next_ctx;
  env->this_thread = next;
  if (next){
    next->env = env;
    next_ctx = &next->context;
  } else{
    next_ctx = &env->sched.context;
  }
  myth_swap_context_withcall(&cur->context, next_ctx,
			     myth_mutex_lock_1, 
			     (void *)q, (void *)cur, 0);
}

static inline int myth_mutex_wake_one_from_queue(myth_mutex_t * mutex) {
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t next = 0;
  while (!next) {
    next = myth_sleep_queue_deq(mutex->sleep_q);
  }
  next->env = env;
  __sync_fetch_and_sub(&mutex->state, 1);
  myth_queue_push(&env->runnable_q, next);
}

static inline int myth_mutex_init_body(myth_mutex_t * mutex, 
				       const myth_mutexattr_t * attr) {
  (void)attr;
  myth_sleep_queue_init(mutex->sleep_q);
  mutex->state = 0;
  return 0;
}

static inline int myth_mutex_destroy_body(myth_mutex_t * mutex)
{
  myth_sleep_queue_destroy(mutex->sleep_q);
  return 0;
}

static inline int myth_mutex_lock_body(myth_mutex_t * mutex) {
  while (1) {
    long s = mutex->state;
    assert(s >= 0);
    if ((s & 1) == 0) {
      if (__sync_bool_compare_and_swap(&mutex->state, s, s + 1)) {
	break;
      }
    } else {
      if (__sync_bool_compare_and_swap(&mutex->state, s, s + 2)) {
	myth_mutex_block_on_queue(mutex->sleep_q);
      }
    }
  }
  return 0;
}
  
static inline int myth_mutex_unlock_body(myth_mutex_t * mutex) {
  myth_running_env_t env = myth_get_current_env();
  while (1) {
    long s = mutex->state;
    assert(s > 0);
    assert(s & 1);
    if (s > 1) {
      if (__sync_bool_compare_and_swap(&mutex->state, s, s - 2)) {
	myth_mutex_wake_one_from_queue(mutex);
	break;
      }
    } else {
      assert(s == 1);
      if (__sync_bool_compare_and_swap(&mutex->state, 1, 0)) {
	break;
      }
    }
  }
}

static inline int myth_mutex_trylock_body(myth_mutex_t * mutex) {
  while (1) {
    long s = mutex->state;
    if (s & 1) {
      return EBUSY;
    } else if (__sync_bool_compare_and_swap(&mutex->state, s, s + 1)) {
      return 0;
    } else {
      continue;
    }
  }
}

/* ----------- condition variable ----------- */

static inline int myth_cond_init_body(myth_cond_t * cond, 
				      const myth_condattr_t * attr) {
  (void)attr;
  cond->wsize = DEFAULT_FESIZE;
  cond->nw = 0;
  cond->wl = cond->def_wl;
  return 0;
}

static inline int myth_cond_destroy_body(myth_cond_t * cond) {
  myth_running_env_t env = myth_get_current_env();
  if (cond->wl != cond->def_wl){
    myth_flfree(env->rank, sizeof(myth_thread_t) * cond->wsize, cond->wl);
  }
  return 0;
}

static inline int myth_cond_signal_body(myth_cond_t * cond) {
  myth_thread_t next;
  myth_running_env_t env = myth_get_current_env();
  if (cond->nw){
    cond->nw--;
    next = cond->wl[cond->nw];
    next->env = env;
    myth_queue_push(&env->runnable_q, next);
  }
  return 0;
}

static inline int myth_cond_broadcast_body(myth_cond_t * cond)
{
  myth_running_env_t env = myth_get_current_env();
  int i;
  for (i=0; i<cond->nw; i++){
    cond->wl[i]->env = env;
    myth_queue_push(&env->runnable_q, cond->wl[i]);
  }
  cond->nw = 0;
  return 0;
}

MYTH_CTX_CALLBACK void myth_cond_wait_1(void *arg1,void *arg2,void *arg3)
{
  myth_running_env_t env = (myth_running_env_t)arg1;
  myth_thread_t next = (myth_thread_t)arg2;
  myth_mutex_t * mtx = (myth_mutex_t *)arg3;
  env->this_thread=next;
  if (next){
    next->env=env;
  }
  myth_mutex_unlock_body(mtx);
}

static inline int myth_cond_wait_body(myth_cond_t * cond, myth_mutex_t * mutex)
{
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t this_thread = env->this_thread;
  //Re-allocate if waiting list is full
  if (cond->wsize <= cond->nw){
    if (cond->wsize == DEFAULT_FESIZE){
      cond->wl = myth_flmalloc(env->rank, sizeof(myth_thread_t) * cond->wsize*2);
      memcpy(cond->wl, cond->def_wl, sizeof(myth_thread_t) * cond->wsize);
    } else {
      cond->wl = myth_flrealloc(env->rank,
				sizeof(myth_thread_t) * cond->wsize,
				cond->wl,
				sizeof(myth_thread_t) * cond->wsize*2);
    }
    cond->wsize *= 2;
  }
  //add
  cond->wl[cond->nw] = this_thread;
  cond->nw++;
  myth_thread_t next;
  myth_context_t next_ctx;
  next = myth_queue_pop(&env->runnable_q);
  if (next){
    next->env = env;
    next_ctx = &next->context;
  } else {
    next_ctx = &env->sched.context;
  }
  //Switch context
  myth_swap_context_withcall(&this_thread->context, next_ctx, 
			     myth_cond_wait_1, 
			     (void*)env, (void*)next, (void*)mutex);
  //Lock again
  myth_mutex_lock_body(mutex);
  return 0;
}


/* ----------- barrier ----------- */

static inline int myth_barrier_init_body(myth_barrier_t * barrier, 
					 const myth_barrierattr_t * attr, 
					 int nthreads)
{
  (void)attr;
  myth_running_env_t env;
  env=myth_get_current_env();
  barrier->nthreads=nthreads;
  barrier->rest=nthreads;
  myth_internal_lock_init(&barrier->lock);
  if (nthreads>1){
    barrier->th=myth_flmalloc(env->rank,sizeof(myth_thread_t)*(nthreads-1));
  }
  myth_wbarrier();
  return 0;
}

MYTH_CTX_CALLBACK void myth_barrier_wait_1(void *arg1,void *arg2,void *arg3)
{
  myth_running_env_t env;
  myth_thread_t next;
  myth_internal_lock_t *plock;
  env=(myth_running_env_t)arg1;
  next=(myth_thread_t)arg2;
  plock=(myth_internal_lock_t*)arg3;
  myth_assert(next);
  env->this_thread=next;
  myth_internal_lock_unlock(plock);
}

MYTH_CTX_CALLBACK void myth_barrier_wait_2(void *arg1,void *arg2,void *arg3)
{
  (void)arg3;
  myth_running_env_t env;
  myth_internal_lock_t *plock;
  env=(myth_running_env_t)arg1;
  plock=(myth_internal_lock_t*)arg2;
  env->this_thread=NULL;
  myth_internal_lock_unlock(plock);
}

static inline int myth_barrier_destroy_body(myth_barrier_t * bar)
{
  myth_running_env_t env;
  env=myth_get_current_env();
  myth_flfree(env->rank,sizeof(myth_thread_t)*((bar)->nthreads-1),(bar)->th);
  return 0;
}

static inline int myth_barrier_wait_body(myth_barrier_t * bar)
{
  if (bar->nthreads<2)return MYTH_BARRIER_SERIAL_THREAD;
  myth_barrier_t * b;
  b=bar;
  int newval;
  myth_running_env_t env;
  myth_thread_t this_thread;
  env=myth_get_current_env();
  this_thread=env->this_thread;
  myth_internal_lock_lock(&bar->lock);
  newval=b->rest;
  newval--;
  b->rest=newval;
  if (newval!=0){
    //This thread is not last, will be blocked
    b->th[newval-1]=this_thread;
    //Try to get a runnable thread
    myth_thread_t next;
    next=myth_queue_pop(&env->runnable_q);
    if (next){
      next->env=env;
      env->this_thread=next;
      //Swap the contexts
      myth_swap_context_withcall(&this_thread->context,&next->context,myth_barrier_wait_1,(void*)env,(void*)next,(void*)&bar->lock);
    }
    else{
      //Switch to the scheduler and do work-stealing
      myth_swap_context_withcall(&this_thread->context,&env->sched.context,myth_barrier_wait_2,(void*)env,(void*)&bar->lock,NULL);
    }
    return 0;
  }
  else{
    //This thread is last, continue all other threads
    //Restore barrier status
    b->rest=b->nthreads;
    //Enqueue all the threads in the barrier
    int i;
    for (i=0;i<b->nthreads-1;i++){
      //set env
      b->th[i]->env=env;
      myth_queue_push(&env->runnable_q,b->th[i]);
    }
    myth_internal_lock_unlock(&bar->lock);
    return MYTH_BARRIER_SERIAL_THREAD;
  }
  myth_unreachable();
  return 0;
}

/* ----------- join counter ----------- */

//Join counter implementation is incomplete
//Do not use them!

static inline int myth_join_counter_init_body(myth_join_counter_t * jc, 
					      const myth_join_counterattr_t * attr, 
					      int val) {
  //Create a join counter with initial value val
  //val must be positive
  (void)attr;
  assert(val>0);
  jc->initial=val;
  jc->count=val;
  jc->th=NULL;
  myth_internal_lock_init(&jc->lock);
  return 0;
}

MYTH_CTX_CALLBACK void myth_jc_wait_1(void *arg1,void *arg2,void *arg3)
{
	myth_running_env_t env;
	myth_thread_t next;
	myth_internal_lock_t *plock;
	env=(myth_running_env_t)arg1;
	next=(myth_thread_t)arg2;
	plock=(myth_internal_lock_t*)arg3;
	myth_assert(next);
	env->this_thread=next;
	myth_internal_lock_unlock(plock);
}

MYTH_CTX_CALLBACK void myth_jc_wait_2(void *arg1,void *arg2,void *arg3)
{
  (void)arg3;
	myth_running_env_t env;
	myth_internal_lock_t *plock;
	env=(myth_running_env_t)arg1;
	plock=(myth_internal_lock_t*)arg2;
	env->this_thread=NULL;
	myth_internal_lock_unlock(plock);
}

static inline int myth_join_counter_wait_body(myth_join_counter_t * jc)
{
  if (jc->count<=0){
    //reset is join counter reaches 0
    myth_internal_lock_lock(&jc->lock);
    assert(jc->th==NULL);
    jc->count+=jc->initial;
    myth_internal_lock_unlock(&jc->lock);
    return 0;
  } else {
    assert(jc->th==NULL);
    //Wait
    myth_running_env_t env;
    env=myth_get_current_env();
    myth_thread_t this_thread;
    this_thread=env->this_thread;
    myth_internal_lock_lock(&jc->lock);
    jc->th=this_thread;
    //Deque a thread
    myth_thread_t next;
    next=myth_queue_pop(&env->runnable_q);
    if (next){
      next->env=env;
      env->this_thread=next;
      //Swap context
      myth_swap_context_withcall(&this_thread->context,&next->context,myth_jc_wait_1,(void*)env,(void*)next,(void*)&jc->lock);
    } else{
      //Switch to the scheduler
      myth_swap_context_withcall(&this_thread->context,&env->sched.context,myth_jc_wait_2,(void*)env,(void*)&jc->lock,NULL);
    }
    assert(jc->th==NULL);
    myth_internal_lock_lock(&jc->lock);
    jc->count+=jc->initial;
    myth_internal_lock_unlock(&jc->lock);
    return 0;
  }
}

static inline int myth_join_counter_dec_body(myth_join_counter_t * jc)
{
  int dec_ret;
  myth_internal_lock_lock(&jc->lock);
  dec_ret=jc->count;
  //decrement join counter
  dec_ret=--jc->count;
  //assert(dec_ret>=0);
  if (dec_ret<=0){
    //Now the join counter is 0
    //Enqueue waiters
    if (jc->th){
      myth_thread_t th=jc->th;
      jc->th=NULL;
      myth_running_env_t env=myth_get_current_env();
      th->env=env;
      myth_queue_push(&env->runnable_q,th);
    }
  }
  myth_internal_lock_unlock(&jc->lock);
  return 0;
}


static inline int myth_felock_init_body(myth_felock_t * felock,
					const myth_felockattr_t * attr)
{
  (void)attr;
  myth_mutex_init_body(felock->lock, 0);
  felock->fe=0;
  felock->fsize = felock->esize = DEFAULT_FESIZE;
  felock->ne = felock->nf = 0;
  felock->el = felock->def_el;
  felock->fl = felock->def_fl;
  return 0;
}

static inline int myth_felock_destroy_body(myth_felock_t * felock)
{
  myth_running_env_t env;
  env = myth_get_current_env();
  myth_mutex_destroy_body(felock->lock);
  if (felock->el != felock->def_el){
    assert(felock->esize>DEFAULT_FESIZE);
    myth_flfree(env->rank, sizeof(myth_thread_t) * felock->esize, felock->el);
  }
  if (felock->fl != felock->def_fl){
    assert(felock->fsize > DEFAULT_FESIZE);
    myth_flfree(env->rank, sizeof(myth_thread_t) * felock->fsize, felock->fl);
  }
  return 0;
}

static inline int myth_felock_lock_body(myth_felock_t * felock)
{
  return myth_mutex_lock_body(felock->lock);
}

MYTH_CTX_CALLBACK void myth_felock_wait_lock_1(void *arg1,void *arg2,void *arg3)
{
  myth_running_env_t env = (myth_running_env_t)arg1;;
  myth_thread_t next = (myth_thread_t)arg2;
  myth_felock_t * fe = (myth_felock_t *)arg3;
  env->this_thread=next;
  if (next){
    next->env=env;
  }
  myth_mutex_unlock_body(fe->lock);
}

static inline int myth_felock_wait_lock_body(myth_felock_t * felock, int val)
{
  myth_running_env_t e = myth_get_current_env();
  myth_thread_t this_thread = e->this_thread;
  while (1) {
    myth_mutex_lock_body(felock->lock);
    volatile myth_running_env_t ev;
    ev = myth_get_current_env();
    myth_running_env_t env;
    env = ev;
    assert(this_thread == env->this_thread);
    if ((!!val) == (!!felock->fe)){
      return 0;
    }
    //add to queue
#if 1
    if (val){
      //wait for fe to be !0
      //check size and reallocate
      if (felock->fsize <= felock->nf){
	if (felock->fsize == DEFAULT_FESIZE){
	  felock->fl = myth_flmalloc(env->rank, 
				     sizeof(myth_thread_t) * felock->fsize * 2);
	  memcpy(felock->fl, felock->def_fl, 
		 sizeof(myth_thread_t) * felock->fsize);
	} else {
	  felock->fl = myth_flrealloc(env->rank,
				      sizeof(myth_thread_t) * felock->fsize,
				      felock->fl,
				      sizeof(myth_thread_t) * felock->fsize * 2);
	}
	felock->fsize *= 2;
      }
      //add
      felock->fl[felock->nf] = this_thread;
      felock->nf++;
    } else {
      //wait for fe to be 0
      //check size and reallocate
      if (felock->esize <= felock->ne){
	if (felock->esize == DEFAULT_FESIZE){
	  felock->el = myth_flmalloc(env->rank, 
				     sizeof(myth_thread_t) * felock->esize * 2);
	  memcpy(felock->el, felock->def_el, 
		 sizeof(myth_thread_t) * felock->esize);
	} else {
	  felock->el = myth_flrealloc(env->rank,
				      sizeof(myth_thread_t) * felock->esize,
				      felock->el,
				      sizeof(myth_thread_t) * felock->esize * 2);
	}
	felock->esize *= 2;
      }
      //add
      felock->el[felock->ne] = this_thread;
      felock->ne++;
    }
    myth_thread_t next;
    myth_context_t next_ctx;
    next = myth_queue_pop(&env->runnable_q);
    if (next){
      next->env = env;
      next_ctx = &next->context;
    }
    else{
      next_ctx = &env->sched.context;
    }
    myth_swap_context_withcall(&this_thread->context, next_ctx, 
			       myth_felock_wait_lock_1,
			       (void*)env, (void*)next, (void*)felock);
#else
    myth_mutex_unlock_body(&felock->lock);
    myth_yield_body();
#endif
  }
}

static inline int myth_felock_unlock_body(myth_felock_t * felock)
{
  myth_running_env_t env = myth_get_current_env();
  myth_thread_t next;
  if (felock->fe){
    if (felock->nf){
      felock->nf--;
      next = felock->fl[felock->nf];
      next->env = env;
      myth_queue_push(&env->runnable_q, next);
    }
  } else {
    if (felock->ne){
      felock->ne--;
      next = felock->el[felock->ne];
      next->env = env;
      myth_queue_push(&env->runnable_q, next);
    }
  }
  return myth_mutex_unlock_body(felock->lock);
}

static inline int myth_felock_set_unlock_body(myth_felock_t * felock, int val)
{
  felock->fe = val;
  return myth_felock_unlock_body(felock);
}

static inline int myth_felock_status_body(myth_felock_t * felock)
{
  return felock->fe;
}

#endif /* MYTH_SYNC_FUNC_H_ */
