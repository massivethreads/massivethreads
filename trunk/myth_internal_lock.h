#ifndef MYTH_INTERNAL_LOCK_H_
#define MYTH_INTERNAL_LOCK_H_

#include "myth_config.h"
#include "myth_mem_barrier.h"
#include "myth_original_lib.h"

//Internal locks
#ifdef MYTH_INTERNAL_LOCK_MUTEX
//pthread_mutex
typedef pthread_mutex_t myth_internal_lock_t;
static inline void myth_internal_lock_init(myth_internal_lock_t *ptr){
	assert(real_pthread_mutex_init(ptr,NULL)==0);
}
static inline void myth_internal_lock_destroy(myth_internal_lock_t *ptr){
	assert(real_pthread_mutex_destroy(ptr)==0);
}
static inline void myth_internal_lock_lock(myth_internal_lock_t *ptr){
	assert(real_pthread_mutex_lock(ptr)==0);
}
static inline void myth_internal_lock_unlock(myth_internal_lock_t *ptr){
	assert(real_pthread_mutex_unlock(ptr)==0);
}
static inline int myth_internal_lock_trylock(myth_internal_lock_t *lock)
{
	return real_pthread_mutex_trylock(lock)==0;
}
#elif defined MYTH_INTERNAL_LOCK_SPINLOCK1
//pthread_spin
typedef pthread_spinlock_t myth_internal_lock_t;
static inline void myth_internal_lock_init(myth_internal_lock_t *ptr){
	assert(real_pthread_spin_init(ptr,PTHREAD_PROCESS_PRIVATE)==0);
}
static inline void myth_internal_lock_destroy(myth_internal_lock_t *ptr){
	assert(real_pthread_spin_destroy(ptr)==0);
}
static inline void myth_internal_lock_lock(myth_internal_lock_t *ptr){
	assert(real_pthread_spin_lock(ptr)==0);
}
static inline void myth_internal_lock_unlock(myth_internal_lock_t *ptr){
	assert(real_pthread_spin_unlock(ptr)==0);
}
static inline int myth_internal_lock_trylock(myth_internal_lock_t *lock)
{
	return real_pthread_spin_trylock(lock)==0;
}
#elif defined MYTH_INTERNAL_LOCK_SPINLOCK2
//spinlock implemented by myself
typedef volatile int myth_internal_lock_t;
static inline void myth_internal_lock_init(myth_internal_lock_t *ptr)
{
	*ptr=0;
}
static inline void myth_internal_lock_destroy(myth_internal_lock_t *ptr){}
static inline void myth_internal_lock_lock(myth_internal_lock_t *ptr)
{
	//error check
	volatile int k=*ptr;
	if (k!=0 && k!=1){
		myth_rwbarrier();
		k=*ptr;
		if (k!=0 && k!=1){
			//fprintf(stderr,"*ptr=%d\n",k);
			assert(k==0 ||k==1);
			//asm volatile ("":::"memory");
		}
	}
	asm volatile(
			"1:cmp $0,%0\n"
			"je 2f\n"
			"rep;nop\n"
			"jmp 1b\n"
			"2:xor %%eax,%%eax\n"//eax=0
			"lock cmpxchg %2,%0\n"//if (*ptr==0)*ptr=1
			"jne 1b\n"
			:"=m"(*ptr):"m"(*ptr),"r"(1):"%eax","cc","memory");
}
static inline void myth_internal_lock_unlock(myth_internal_lock_t *ptr)
{
	//Serialize write instructions
	myth_wbarrier();
	//Reset value
	*ptr=0;
}
static inline int myth_internal_lock_trylock(myth_internal_lock_t *ptr)
{
	int ret;
	asm volatile(
			"lock cmpxchg %4,%0\n"//if (*ptr==0)*ptr=1
			:"=m"(*ptr),"=a"(ret):"1"(0),"m"(*ptr),"r"(1):"cc","memory");
	return ret==0;
}
#else
#error "Please choose internal locking method"
#endif

#endif /* MYTH_LOCK_H_ */
