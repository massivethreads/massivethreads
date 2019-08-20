#pragma once
#ifndef MYTH_WORKERS_RANGE_FUNC_H_
#define MYTH_WORKERS_RANGE_FUNC_H_

#include <math.h>

#include "myth/myth.h"
#include "myth_config.h"

#if MYTH_ENABLE_ADWS

#include "myth_misc.h"

static inline void myth_init_workers_range(myth_workers_range_t* workers_range, double left, double right) {
  workers_range->left         = left;
  workers_range->right        = right;
  workers_range->left_worker  = (int)left;
  workers_range->right_worker = (int)right;
}

static inline int myth_workers_range_at_left_boundary(const myth_workers_range_t workers_range) {
  return workers_range.left == (double)workers_range.left_worker;
}

static inline double myth_size_of_workers_range(const myth_workers_range_t workers_range) {
  return workers_range.left - workers_range.right;
}

static inline double myth_other_workers_amount_of_work(const myth_workers_range_t workers_range) {
  return workers_range.left - (double)(workers_range.right_worker + 1);
}

static inline void myth_assert_workers_range(const myth_workers_range_t workers_range) {
  (void)workers_range;
  myth_assert(workers_range.left >= workers_range.right);
  myth_assert(0.0 <= workers_range.left  && workers_range.left  <= g_attr.n_workers);
  myth_assert(0.0 <= workers_range.right && workers_range.right <  g_attr.n_workers);
  myth_assert(workers_range.left_worker == (int)workers_range.left);
  myth_assert(workers_range.right_worker == (int)workers_range.right);
}

static inline int myth_is_same_workers_range(const myth_workers_range_t range1, const myth_workers_range_t range2) {
  return (range1.left == range2.left && range1.right == range2.right);
}

static inline void myth_divide_workers_range(const myth_workers_range_t workers_range,
    myth_workers_range_t* left_range, myth_workers_range_t* right_range, double amount_from_left) {
  if (amount_from_left == 0.0) amount_from_left = 0.00001;
  double mid = workers_range.left - amount_from_left;
  myth_init_workers_range(left_range, workers_range.left, mid);
  myth_init_workers_range(right_range, mid, workers_range.right);
  myth_assert_workers_range(*left_range);
  myth_assert_workers_range(*right_range);
}

#endif

#endif	/* MYTH_WORKERS_RANGE_FUNC_H_ */
