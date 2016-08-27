# build this directory first and then tests
#SUBDIRS = . profiler
SUBDIRS = .

# sources (.c and .S) and flags common across the three libraries
# COMMON_SRCS = myth_log.c myth_sched.c myth_worker.c myth_malloc_wrapper.c myth_sync.c myth_init.c myth_misc.c myth_io.c myth_original_lib.c myth_tls.c myth_context.S myth_context_gvar.c myth_eco.c
COMMON_SRCS = \
	myth_log.c \
	myth_sched.c \
	myth_worker.c \
	myth_sync.c \
	myth_init.c \
	myth_misc.c \
	myth_tls.c \
	myth_thread.c \
	myth_context.c \
	myth_context_switch.S \
	myth_eco.c \
	myth_if_native.c

COMMON_FLAGS = \
	-D_GNU_SOURCE \
	-I$(abs_top_srcdir)/include

# required when you use libtool (for building shared libs)
ACLOCAL_AMFLAGS = -I m4


#lib_LTLIBRARIES = libmyth.la libmyth-compat.la libmyth-native.la
lib_LTLIBRARIES = libmyth.la libmyth-vanilla.la
# include_HEADERS = myth.h myth_if_native.h 
nobase_include_HEADERS = \
	mtbb/task_group.h \
	mtbb/parallel_for.h \
	mtbb/parallel_reduce.h \
	tpswitch/omp_dr.h \
	tpswitch/tbb_dr.h \
	tpswitch/cilkplus_dr.h \
	tpswitch/cilk_dr.cilkh \
	tpswitch/serial_dr.h \
	tpswitch/tpswitch.h

libmyth_vanilla_la_SOURCES = $(COMMON_SRCS) myth_real_fun_vanilla.c
libmyth_la_SOURCES = $(COMMON_SRCS) myth_real_fun.c myth_wrap_pthread.c myth_wrap_malloc.c myth_wrap_socket.c 

# libmyth_wrap_la_SOURCES = $(COMMON_SRCS) myth_if_native.c myth_wrap_malloc.c myth_wrap_pthread.c myth_wrap_io.c 

#libmyth_la_SOURCES        = $(COMMON_SRCS) myth_if_native.c myth_if_pthread.c myth_constructor.c
#libmyth_native_la_SOURCES = $(COMMON_SRCS) myth_if_native.c                   myth_constructor.c
#libmyth_compat_la_SOURCES = $(COMMON_SRCS)                  myth_if_pthread.c myth_constructor.c

# C compiler flags
libmyth_vanilla_la_CFLAGS         = $(COMMON_FLAGS)
libmyth_la_CFLAGS         = $(COMMON_FLAGS)
#libmyth_native_la_CFLAGS  = $(COMMON_FLAGS)
#libmyth_compat_la_CFLAGS  = $(COMMON_FLAGS)

# this is for assembly (.S) file = myth_context.S
libmyth_vanilla_la_CCASFLAGS         = $(COMMON_FLAGS)
libmyth_la_CCASFLAGS         = $(COMMON_FLAGS)
#libmyth_native_la_CCASFLAGS  = $(COMMON_FLAGS)
#libmyth_compat_la_CCASFLAGS  = $(COMMON_FLAGS)
