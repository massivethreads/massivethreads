#SUBDIRS = c native
#if CXX_AVAILABLE
#SUBDIRS += cxx
#endif
check_PROGRAMS = \
	myth_create_0 \
	myth_create_1 \
	myth_create_2 \
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
	measure_create \
	measure_latency \
	measure_wakeup_latency

#	new_test

# check_PROGRAMS = myth_lock myth_trylock myth_mixlock

common_cflags = -I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/src
common_ldadd = $(abs_top_srcdir)/src/libmyth.la 

myth_create_0_SOURCES = myth_create_0.c
myth_create_0_CFLAGS = $(common_cflags)
myth_create_0_LDADD  = $(common_ldadd)

myth_create_1_SOURCES = myth_create_1.c
myth_create_1_CFLAGS = $(common_cflags)
myth_create_1_LDADD  = $(common_ldadd)

myth_create_2_SOURCES = myth_create_2.c
myth_create_2_CFLAGS = $(common_cflags)
myth_create_2_LDADD  = $(common_ldadd)

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

measure_create_SOURCES = measure_create.c
measure_create_CFLAGS = $(common_cflags)
measure_create_LDADD  = $(common_ldadd)

measure_latency_SOURCES = measure_latency.c
measure_latency_CFLAGS = $(common_cflags)
measure_latency_LDADD  = $(common_ldadd)

measure_wakeup_latency_SOURCES = measure_wakeup_latency.c
measure_wakeup_latency_CFLAGS = $(common_cflags)
measure_wakeup_latency_LDADD  = $(common_ldadd)

# new_test_SOURCES = new_test.c
# new_test_CFLAGS = $(common_cflags)
# new_test_LDADD  = $(common_ldadd)

TESTS = $(check_PROGRAMS)
