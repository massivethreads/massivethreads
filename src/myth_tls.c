/*
 * myth_tls.c
 *
 *  Created on: 2011/05/17
 *      Author: jnakashima
 */

#include "myth/myth.h"
#include "myth/myth_internal_lock.h"

#include "myth_tls_func.h"

#if 1

myth_tls_key_allocator_t g_myth_tls_key_allocator[1];

#else
//Global variable declarations

myth_internal_lock_t g_myth_tls_lock;
myth_tls_entry *g_myth_tls_list;
int g_myth_tls_list_size;
int g_myth_tls_key_status[MYTH_TLS_KEY_SIZE];
#endif

