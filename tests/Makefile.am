#SUBDIRS = c native
#if CXX_AVAILABLE
#SUBDIRS += cxx
#endif

#

check_PROGRAMS = \
	myth_malloc \
	myth_calloc \
	myth_posix_memalign \
	myth_aligned_alloc \
	myth_valloc \
	myth_memalign \
	myth_pvalloc \
	myth_free \
	myth_realloc \
	myth_create_0 \
	myth_create_1 \
	myth_create_2 \
	myth_create_join_many \
	myth_yield_0 \
	myth_yield_1 \
	myth_yield_2 \
	myth_sleep_queue \
	myth_lock \
	myth_trylock \
	myth_mixlock \
	myth_cond_signal \
	myth_cond_broadcast_0 \
	myth_cond_broadcast_1 \
	myth_barrier \
	myth_join_counter \
	myth_felock \
	myth_dag_1d \
	myth_dag_2d \
	myth_dag_random \
	pth_create_0 \
	pth_create_1 \
	pth_create_2 \
	pth_yield \
	pth_lock \
	pth_trylock \
	pth_mixlock \
	pth_barrier \
	measure_create \
	measure_latency \
	measure_wakeup_latency \
	myth_create_0_cc \
	myth_create_1_cc \
	myth_create_2_cc \
	myth_create_join_many_cc \
	myth_yield_0_cc \
	myth_yield_1_cc \
	myth_yield_2_cc \
	myth_sleep_queue_cc \
	myth_lock_cc \
	myth_trylock_cc \
	myth_mixlock_cc \
	myth_cond_signal_cc \
	myth_cond_broadcast_0_cc \
	myth_cond_broadcast_1_cc \
	myth_barrier_cc \
	myth_join_counter_cc \
	myth_felock_cc \
	myth_dag_1d_cc \
	myth_dag_2d_cc \
	myth_dag_random_cc \
	pth_create_0_cc \
	pth_create_1_cc \
	pth_create_2_cc \
	pth_yield_cc \
	pth_lock_cc \
	pth_trylock_cc \
	pth_mixlock_cc \
	pth_barrier_cc \
	measure_create_cc \
	measure_latency_cc \
	measure_wakeup_latency_cc


#	new_test
# three programs that fail for now
#	pth_cond_signal
#	pth_cond_broadcast_0
#	pth_cond_broadcast_1
# don't know why, but they succeed (examine later)
#	pth_cond_signal_cc
#	pth_cond_broadcast_0_cc
#	pth_cond_broadcast_1_cc
#

# check_PROGRAMS = myth_lock myth_trylock myth_mixlock

#  -I$(abs_top_srcdir)/src
common_cflags = -I$(abs_top_srcdir)/include
common_cxxflags = $(common_cflags)
common_ldadd = $(abs_top_srcdir)/src/libmyth.la -lrt

myth_malloc_SOURCES = myth_malloc.c
myth_malloc_CFLAGS = $(common_cflags)
myth_malloc_LDADD  = $(common_ldadd)

myth_calloc_SOURCES = myth_calloc.c
myth_calloc_CFLAGS = $(common_cflags)
myth_calloc_LDADD  = $(common_ldadd)

myth_posix_memalign_SOURCES = myth_posix_memalign.c
myth_posix_memalign_CFLAGS = $(common_cflags)
myth_posix_memalign_LDADD  = $(common_ldadd)

myth_aligned_alloc_SOURCES = myth_aligned_alloc.c
myth_aligned_alloc_CFLAGS = $(common_cflags)
myth_aligned_alloc_LDADD  = $(common_ldadd)

myth_valloc_SOURCES = myth_valloc.c
myth_valloc_CFLAGS = $(common_cflags)
myth_valloc_LDADD  = $(common_ldadd)

myth_memalign_SOURCES = myth_memalign.c
myth_memalign_CFLAGS = $(common_cflags)
myth_memalign_LDADD  = $(common_ldadd)

myth_pvalloc_SOURCES = myth_pvalloc.c
myth_pvalloc_CFLAGS = $(common_cflags)
myth_pvalloc_LDADD  = $(common_ldadd)

myth_free_SOURCES = myth_free.c
myth_free_CFLAGS = $(common_cflags)
myth_free_LDADD  = $(common_ldadd)

myth_realloc_SOURCES = myth_realloc.c
myth_realloc_CFLAGS = $(common_cflags)
myth_realloc_LDADD  = $(common_ldadd)

myth_create_0_SOURCES = myth_create_0.c
myth_create_0_CFLAGS = $(common_cflags)
myth_create_0_LDADD  = $(common_ldadd)

myth_create_1_SOURCES = myth_create_1.c
myth_create_1_CFLAGS = $(common_cflags)
myth_create_1_LDADD  = $(common_ldadd)

myth_create_2_SOURCES = myth_create_2.c
myth_create_2_CFLAGS = $(common_cflags)
myth_create_2_LDADD  = $(common_ldadd)

myth_create_join_many_SOURCES = myth_create_join_many.c
myth_create_join_many_CFLAGS = $(common_cflags)
myth_create_join_many_LDADD  = $(common_ldadd)

myth_yield_0_SOURCES = myth_yield_0.c
myth_yield_0_CFLAGS = $(common_cflags)
myth_yield_0_LDADD  = $(common_ldadd)

myth_yield_1_SOURCES = myth_yield_1.c
myth_yield_1_CFLAGS = $(common_cflags)
myth_yield_1_LDADD  = $(common_ldadd)

myth_yield_2_SOURCES = myth_yield_2.c
myth_yield_2_CFLAGS = $(common_cflags)
myth_yield_2_LDADD  = $(common_ldadd)

myth_sleep_queue_SOURCES = myth_sleep_queue.c
myth_sleep_queue_CFLAGS = $(common_cflags)
myth_sleep_queue_LDADD  = $(common_ldadd)

myth_lock_SOURCES = myth_lock.c
myth_lock_CFLAGS = $(common_cflags)
myth_lock_LDADD  = $(common_ldadd)

myth_trylock_SOURCES = myth_trylock.c
myth_trylock_CFLAGS = $(common_cflags)
myth_trylock_LDADD  = $(common_ldadd)

myth_mixlock_SOURCES = myth_mixlock.c
myth_mixlock_CFLAGS = $(common_cflags)
myth_mixlock_LDADD  = $(common_ldadd)

myth_cond_signal_SOURCES = myth_cond_signal.c
myth_cond_signal_CFLAGS = $(common_cflags)
myth_cond_signal_LDADD  = $(common_ldadd)

myth_cond_broadcast_0_SOURCES = myth_cond_broadcast_0.c
myth_cond_broadcast_0_CFLAGS = $(common_cflags)
myth_cond_broadcast_0_LDADD  = $(common_ldadd)

myth_cond_broadcast_1_SOURCES = myth_cond_broadcast_1.c
myth_cond_broadcast_1_CFLAGS = $(common_cflags)
myth_cond_broadcast_1_LDADD  = $(common_ldadd)

myth_barrier_SOURCES = myth_barrier.c
myth_barrier_CFLAGS = $(common_cflags)
myth_barrier_LDADD  = $(common_ldadd)

myth_join_counter_SOURCES = myth_join_counter.c
myth_join_counter_CFLAGS = $(common_cflags)
myth_join_counter_LDADD  = $(common_ldadd)

myth_felock_SOURCES = myth_felock.c
myth_felock_CFLAGS = $(common_cflags)
myth_felock_LDADD  = $(common_ldadd)

myth_dag_1d_SOURCES = myth_dag_1d.c
myth_dag_1d_CFLAGS = $(common_cflags)
myth_dag_1d_LDADD  = $(common_ldadd)

myth_dag_2d_SOURCES = myth_dag_2d.c
myth_dag_2d_CFLAGS = $(common_cflags)
myth_dag_2d_LDADD  = $(common_ldadd)

myth_dag_random_SOURCES = myth_dag_random.c
myth_dag_random_CFLAGS = $(common_cflags)
myth_dag_random_LDADD  = $(common_ldadd)

pth_create_0_SOURCES = pth_create_0.c
pth_create_0_CFLAGS = $(common_cflags)
pth_create_0_LDADD  = $(common_ldadd)

pth_create_1_SOURCES = pth_create_1.c
pth_create_1_CFLAGS = $(common_cflags)
pth_create_1_LDADD  = $(common_ldadd)

pth_create_2_SOURCES = pth_create_2.c
pth_create_2_CFLAGS = $(common_cflags)
pth_create_2_LDADD  = $(common_ldadd)

pth_yield_SOURCES = pth_yield.c
pth_yield_CFLAGS = $(common_cflags)
pth_yield_LDADD  = $(common_ldadd)

pth_lock_SOURCES = pth_lock.c
pth_lock_CFLAGS = $(common_cflags)
pth_lock_LDADD  = $(common_ldadd)

pth_trylock_SOURCES = pth_trylock.c
pth_trylock_CFLAGS = $(common_cflags)
pth_trylock_LDADD  = $(common_ldadd)

pth_mixlock_SOURCES = pth_mixlock.c
pth_mixlock_CFLAGS = $(common_cflags)
pth_mixlock_LDADD  = $(common_ldadd)

#pth_cond_signal_SOURCES = pth_cond_signal.c
#pth_cond_signal_CFLAGS = $(common_cflags)
#pth_cond_signal_LDADD  = $(common_ldadd)

#pth_cond_broadcast_0_SOURCES = pth_cond_broadcast_0.c
#pth_cond_broadcast_0_CFLAGS = $(common_cflags)
#pth_cond_broadcast_0_LDADD  = $(common_ldadd)

#pth_cond_broadcast_1_SOURCES = pth_cond_broadcast_1.c
#pth_cond_broadcast_1_CFLAGS = $(common_cflags)
#pth_cond_broadcast_1_LDADD  = $(common_ldadd)

pth_barrier_SOURCES = pth_barrier.c
pth_barrier_CFLAGS = $(common_cflags)
pth_barrier_LDADD  = $(common_ldadd)

measure_create_SOURCES = measure_create.c
measure_create_CFLAGS = $(common_cflags)
measure_create_LDADD  = $(common_ldadd)

measure_latency_SOURCES = measure_latency.c
measure_latency_CFLAGS = $(common_cflags)
measure_latency_LDADD  = $(common_ldadd)

measure_wakeup_latency_SOURCES = measure_wakeup_latency.c
measure_wakeup_latency_CFLAGS = $(common_cflags)
measure_wakeup_latency_LDADD  = $(common_ldadd)

#
# C++
#
myth_create_0_cc_SOURCES = myth_create_0_cc.cc
myth_create_0_cc_CXXFLAGS = $(common_cflags)
myth_create_0_cc_LDADD  = $(common_ldadd)

myth_create_1_cc_SOURCES = myth_create_1_cc.cc
myth_create_1_cc_CXXFLAGS = $(common_cflags)
myth_create_1_cc_LDADD  = $(common_ldadd)

myth_create_2_cc_SOURCES = myth_create_2_cc.cc
myth_create_2_cc_CXXFLAGS = $(common_cxxflags)
myth_create_2_cc_LDADD  = $(common_ldadd)

myth_create_join_many_cc_SOURCES = myth_create_join_many_cc.cc
myth_create_join_many_cc_CXXFLAGS = $(common_cxxflags)
myth_create_join_many_cc_LDADD  = $(common_ldadd)

myth_yield_0_cc_SOURCES = myth_yield_0_cc.cc
myth_yield_0_cc_CXXFLAGS = $(common_cxxflags)
myth_yield_0_cc_LDADD  = $(common_ldadd)

myth_yield_1_cc_SOURCES = myth_yield_1_cc.cc
myth_yield_1_cc_CXXFLAGS = $(common_cxxflags)
myth_yield_1_cc_LDADD  = $(common_ldadd)

myth_yield_2_cc_SOURCES = myth_yield_2_cc.cc
myth_yield_2_cc_CXXFLAGS = $(common_cxxflags)
myth_yield_2_cc_LDADD  = $(common_ldadd)

myth_sleep_queue_cc_SOURCES = myth_sleep_queue_cc.cc
myth_sleep_queue_cc_CXXFLAGS = $(common_cxxflags)
myth_sleep_queue_cc_LDADD  = $(common_ldadd)

myth_lock_cc_SOURCES = myth_lock_cc.cc
myth_lock_cc_CXXFLAGS = $(common_cxxflags)
myth_lock_cc_LDADD  = $(common_ldadd)

myth_trylock_cc_SOURCES = myth_trylock_cc.cc
myth_trylock_cc_CXXFLAGS = $(common_cxxflags)
myth_trylock_cc_LDADD  = $(common_ldadd)

myth_mixlock_cc_SOURCES = myth_mixlock_cc.cc
myth_mixlock_cc_CXXFLAGS = $(common_cxxflags)
myth_mixlock_cc_LDADD  = $(common_ldadd)

myth_cond_signal_cc_SOURCES = myth_cond_signal_cc.cc
myth_cond_signal_cc_CXXFLAGS = $(common_cxxflags)
myth_cond_signal_cc_LDADD  = $(common_ldadd)

myth_cond_broadcast_0_cc_SOURCES = myth_cond_broadcast_0_cc.cc
myth_cond_broadcast_0_cc_CXXFLAGS = $(common_cxxflags)
myth_cond_broadcast_0_cc_LDADD  = $(common_ldadd)

myth_cond_broadcast_1_cc_SOURCES = myth_cond_broadcast_1_cc.cc
myth_cond_broadcast_1_cc_CXXFLAGS = $(common_cxxflags)
myth_cond_broadcast_1_cc_LDADD  = $(common_ldadd)

myth_barrier_cc_SOURCES = myth_barrier_cc.cc
myth_barrier_cc_CXXFLAGS = $(common_cxxflags)
myth_barrier_cc_LDADD  = $(common_ldadd)

myth_join_counter_cc_SOURCES = myth_join_counter_cc.cc
myth_join_counter_cc_CXXFLAGS = $(common_cxxflags)
myth_join_counter_cc_LDADD  = $(common_ldadd)

myth_felock_cc_SOURCES = myth_felock_cc.cc
myth_felock_cc_CXXFLAGS = $(common_cxxflags)
myth_felock_cc_LDADD  = $(common_ldadd)

myth_dag_1d_cc_SOURCES = myth_dag_1d_cc.cc
myth_dag_1d_cc_CXXFLAGS = $(common_cxxflags)
myth_dag_1d_cc_LDADD  = $(common_ldadd)

myth_dag_2d_cc_SOURCES = myth_dag_2d_cc.cc
myth_dag_2d_cc_CXXFLAGS = $(common_cxxflags)
myth_dag_2d_cc_LDADD  = $(common_ldadd)

myth_dag_random_cc_SOURCES = myth_dag_random_cc.cc
myth_dag_random_cc_CXXFLAGS = $(common_cxxflags)
myth_dag_random_cc_LDADD  = $(common_ldadd)

pth_create_0_cc_SOURCES = pth_create_0_cc.cc
pth_create_0_cc_CXXFLAGS = $(common_cxxflags)
pth_create_0_cc_LDADD  = $(common_ldadd)

pth_create_1_cc_SOURCES = pth_create_1_cc.cc
pth_create_1_cc_CXXFLAGS = $(common_cxxflags)
pth_create_1_cc_LDADD  = $(common_ldadd)

pth_create_2_cc_SOURCES = pth_create_2_cc.cc
pth_create_2_cc_CXXFLAGS = $(common_cxxflags)
pth_create_2_cc_LDADD  = $(common_ldadd)

pth_yield_cc_SOURCES = pth_yield_cc.cc
pth_yield_cc_CXXFLAGS = $(common_cxxflags)
pth_yield_cc_LDADD  = $(common_ldadd)

pth_lock_cc_SOURCES = pth_lock_cc.cc
pth_lock_cc_CXXFLAGS = $(common_cxxflags)
pth_lock_cc_LDADD  = $(common_ldadd)

pth_trylock_cc_SOURCES = pth_trylock_cc.cc
pth_trylock_cc_CXXFLAGS = $(common_cxxflags)
pth_trylock_cc_LDADD  = $(common_ldadd)

pth_mixlock_cc_SOURCES = pth_mixlock_cc.cc
pth_mixlock_cc_CXXFLAGS = $(common_cxxflags)
pth_mixlock_cc_LDADD  = $(common_ldadd)

# pth_cond_signal_cc_SOURCES = pth_cond_signal_cc.cc
# pth_cond_signal_cc_CXXFLAGS = $(common_cxxflags)
# pth_cond_signal_cc_LDADD  = $(common_ldadd)

# pth_cond_broadcast_0_cc_SOURCES = pth_cond_broadcast_0_cc.cc
# pth_cond_broadcast_0_cc_CXXFLAGS = $(common_cxxflags)
# pth_cond_broadcast_0_cc_LDADD  = $(common_ldadd)

# pth_cond_broadcast_1_cc_SOURCES = pth_cond_broadcast_1_cc.cc
# pth_cond_broadcast_1_cc_CXXFLAGS = $(common_cxxflags)
# pth_cond_broadcast_1_cc_LDADD  = $(common_ldadd)

pth_barrier_cc_SOURCES = pth_barrier_cc.cc
pth_barrier_cc_CXXFLAGS = $(common_cxxflags)
pth_barrier_cc_LDADD  = $(common_ldadd)

measure_create_cc_SOURCES = measure_create_cc.cc
measure_create_cc_CXXFLAGS = $(common_cxxflags)
measure_create_cc_LDADD  = $(common_ldadd)

measure_latency_cc_SOURCES = measure_latency_cc.cc
measure_latency_cc_CXXFLAGS = $(common_cxxflags)
measure_latency_cc_LDADD  = $(common_ldadd)

measure_wakeup_latency_cc_SOURCES = measure_wakeup_latency_cc.cc
measure_wakeup_latency_cc_CXXFLAGS = $(common_cxxflags)
measure_wakeup_latency_cc_LDADD  = $(common_ldadd)

# new_test_SOURCES = new_test.c
# new_test_CXXFLAGS = $(common_cflags)
# new_test_LDADD  = $(common_ldadd)

TESTS = $(check_PROGRAMS)
