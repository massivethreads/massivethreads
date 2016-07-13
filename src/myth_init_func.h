/* 
 * myth_init_func.h
 */
#pragma once
#ifndef MYTH_INIT_FUNC_H
#define MYTH_INIT_FUNC_H

#include "myth_init.h"

static inline int myth_ensure_init_ex(myth_attr_t * attr) {
  if (g_myth_init_state == myth_init_state_initialized) {
    return 1;
  } else {
    return myth_init_ex_body(attr);
  }
}

static inline int myth_ensure_init(void) {
  return myth_ensure_init_ex(0);
}

#endif
