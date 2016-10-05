/* 
 * myth_wls_func.h --- worker local storage
 */
#pragma once
#ifndef MYTH_WLS_FUNC_H_
#define MYTH_WLS_FUNC_H_

#include <pthread.h>
#include "myth/myth.h"
#include "myth_config.h"

static int myth_wls_key_create_body(myth_wls_key_t *key, void (*destr_function)(void *)) {
  return real_pthread_key_create((pthread_key_t*)key, destr_function);
}

static int myth_wls_key_delete_body(myth_wls_key_t key) {
  return real_pthread_key_delete((pthread_key_t)key);
}

static int myth_wls_setspecific_body(myth_wls_key_t key, const void *data) {
  return real_pthread_setspecific((pthread_key_t)key, data);
}

static void * myth_wls_getspecific_body(myth_wls_key_t key) {
  return real_pthread_getspecific((pthread_key_t)key);
}
#endif	/* MYTH_WLS_FUNC_H_ */
