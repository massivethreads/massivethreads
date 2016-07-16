/* 
 * myth_thread.c
 */
#include "myth/myth.h"
#include "myth_thread.h"

#include "myth_tls_func.h"

size_t myth_get_stack_size(void) {
  myth_thread_t th = myth_self_body();
  size_t sz = th->stack_size;
  return (sz ? sz : g_attr.default_stack_size);
}
