AC_INIT([massivethreads], [0.97], [massivethreads@eidos.ic.i.u-tokyo.ac.jp], [massivethreads], [https://github.com/massivethreads/massivethreads/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
# required when you build shared libraries, relying on libtool
AC_CONFIG_MACRO_DIR([m4])
# avoid executing autotools on the user side
AM_MAINTAINER_MODE([disable])

# -----------------------------
# basics
# -----------------------------

AC_PROG_CC
# recommended CFLAGS
# -O3
# -ftls-model=initial-exec
AC_PROG_CC_C_O
AM_PROG_CC_C_O
AM_PROG_AS
AC_PROG_AWK

# on new systems, this seems required
# (without it it errs "linking libtool libraries using a 
# non-POSIX archiver requires 'AM_PROG_AR' in 'configure.ac");
# on old systems, this results in an error
# macro `AM_PROG_AR' not found in library
# error: possibly undefined macro: AM_PROG_AR
# If this token and others are legitimate, please use m4_pattern_allow.
# See the Autoconf documentation.
AM_PROG_AR

# =============================
# process user options
# =============================

# -----------------------
# whether or not build libmyth-ld
# -----------------------

AC_ARG_ENABLE([myth-ld],
              [AS_HELP_STRING([--enable-myth-ld=yes|no],
	                      [build libmyth-ld or not, a library that wraps some system functions with GNU linker options --wrap=function_name (default=yes)])],
              [enable_myth_ld="${enableval}"],
              [enable_myth_ld="yes"])

# only record --enable-myth-ld for now
# we will later check if the linker supports --wrap and decide if
# we really build libmyth-ld

# -----------------------
# whether or not build libmyth-dl
# -----------------------

AC_ARG_ENABLE([myth-dl],
              [AS_HELP_STRING([--enable-myth-dl=yes|no],
	                      [build libmyth-dl or not, a library that wraps some system functions with dlopen/dlsym])],
              [enable_myth_dl="${enableval}"],
              [enable_myth_dl="yes"])

# only record --enable-myth-dl for now
# we will later check if dlopen/dlsym are available and decide if
# we really build libmyth-dl

# -----------------------
# bind workers or not
# -----------------------

AC_ARG_ENABLE([bind-workers],
            [AS_HELP_STRING([--enable-default-bind-workers=yes/no],
                            [Set default value for bind-workers.
                             Default: yes.  You can change it at runtime
			     by setting MYTH_BIND_WORKERS.])],
            [default_bind_workers="${enableval}"],
            [default_bind_workers="yes"])

if test "x$default_bind_workers" != "xno" ; then
  AC_DEFINE_UNQUOTED([MYTH_DEFAULT_BIND_WORKERS],[1],[if 1, bind workers by default])
else						 
  AC_DEFINE_UNQUOTED([MYTH_DEFAULT_BIND_WORKERS],[0],[if 1, bind workers by default])
fi

# -----------------------
# --enable-eco-mode
#   flag controlling the behavior of eco-mode scheduler
# -----------------------

AC_ARG_ENABLE([eco-mode],
              [AS_HELP_STRING([--enable-eco-mode=yes|no],
	                      [Enable or disable eco-mode scheduler. "no" completely 
			      disables it when compiling MassiveThreads. "yes" 
			      enables it, but it is not activated by default.
			      To use it you have to activate by setting environment
			      variable MYTH_ECO_MODE=1. Default is "no".
			      When eco-mode scheduler is enabled, thread creation
			      overhead might be slightly increased, whichever it is activated or not])],
			      ,
			     [enable_eco_mode="no"])

if test "x$enable_eco_mode" = "xyes" ; then
  AC_DEFINE_UNQUOTED([MYTH_ECO_MODE],[1],[if 1, enable eco-mode])
  AC_DEFINE_UNQUOTED([MYTH_ECO_TEIAN_STEAL],[1],[if 1, enable eco-mode])
else
  AC_DEFINE_UNQUOTED([MYTH_ECO_MODE],[0],[if 1, enable eco-mode])
  AC_DEFINE_UNQUOTED([MYTH_ECO_TEIAN_STEAL],[0],[if 1, enable eco-mode])
fi

# -----------------------
# use ucontext or native context
# -----------------------

AC_ARG_ENABLE([native-context],
            [AS_HELP_STRING([--enable-native-context=yes|no],
                            [use optimized assembly language context switch (default=yes)])],
            [enable_native_context="${enableval}"],
            [enable_native_context="yes"])

if test x"$enable_native_context" = "xno" ; then
   enable_native_context=1
else
   enable_native_context=0
fi
AC_DEFINE_UNQUOTED([MYTH_FORCE_UCONTEXT],
	[$enable_native_context],
	[use ucontext if set, otherwise assembly context])


# -----------------------
# set user threads' default stack size
# -----------------------

AC_ARG_WITH([default-stack-size],
            [AS_HELP_STRING([--with-default-stack-size=S],
                            [Set default stack size of threads to S bytes.
                             Default: 131072.  You can change it at runtime
			     by setting MYTH_DEF_STKSIZE.])],
            [default_stack_size="$withval"],
            [default_stack_size=131072])

AC_DEFINE_UNQUOTED([MYTH_DEF_STACK_SIZE],
	[$default_stack_size],[Default stack size])

# -----------------------
# set the scheduler thread's stack size
# -----------------------

AC_ARG_WITH([sched-stack-size],
            [AS_HELP_STRING([--with-sched-stack-size=S],
                            [Set default stack size of the scheduler thread to S bytes.
                             Default: 1048576.])],
            [sched_stack_size="$withval"],
            [sched_stack_size=1048576])

AC_DEFINE_UNQUOTED([MYTH_SCHED_STACK_SIZE],
	[$sched_stack_size],[Scheduler stack size])

# -----------------------
# set user threads' default guard size
# -----------------------

AC_ARG_WITH([default-guard-size],
            [AS_HELP_STRING([--with-default-guard-size=S],
                            [Set default guard size of threads to S bytes.
                             Default: 4096.  You can change it at runtime
			     by setting MYTH_DEF_GUARDSIZE.])],
            [default_guard_size="$withval"],
            [default_guard_size=4096])

AC_DEFINE_UNQUOTED([MYTH_DEF_GUARD_SIZE],
	[$default_guard_size],[Default guard size])

#
# child first or not
#

AC_ARG_WITH([child-first],
            [AS_HELP_STRING([--with-child-first=yes/no],
                            [Set default value for child-first.
                             Default: 1.  You can change it at runtime
			     by setting MYTH_CHILD_FIRST.])],
            [default_child_first="$withval"],
            [default_child_first="yes"])

if test "x$default_child_first" != "xno" ; then
   AC_DEFINE_UNQUOTED([MYTH_CHILD_FIRST],[1],[if 1, child first by default])
else						 
   AC_DEFINE_UNQUOTED([MYTH_CHILD_FIRST],[0],[if 1, child first by default])
fi

# -----------------------------
# sizes
# -----------------------------

# Check sizes
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([void*])

# =============================
# libraries
# =============================

# ---------------------
# mandatory libraries
# ---------------------

AC_CHECK_LIB(pthread, pthread_create,
		      [],
		      [AC_MSG_ERROR(libpthread not available.)])

# ---------------------
# libraries affecting build targets depending on availability
# ---------------------

#
# check if --wrap=xxxx is supported
#
AX_CHECK_LINK_FLAG([-Wl,--wrap=malloc],
	[linker_accepts_wrap="yes"],
	[linker_accepts_wrap="no"])

AM_CONDITIONAL([BUILD_MYTH_LD],
	[test x"$enable_myth_ld" = "xyes" -a x"$linker_accepts_wrap" = "xyes"])


# ---------------------
# optional libraries
# ---------------------

# ------
# -ldl dlopen
# ------
AC_CHECK_LIB(dl, dlsym,
		 [have_dlsym="yes"],
		 [have_dlsym="no"])

AC_CHECK_HEADERS([dlfcn.h])

AC_CHECK_DECL(RTLD_NEXT,
	[have_rtld_next="yes"],
	[have_rtld_next="no"],
[#define _GNU_SOURCE
#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif
])

AM_CONDITIONAL([BUILD_MYTH_DL],
	[test x"$enable_myth_dl" = "xyes" -a x"$have_rtld_next" = "xyes" ])


# -------------
# -lrt clock_gettime
# -------------
AC_CHECK_LIB(rt, clock_gettime)

# =============================
# headers
# =============================

AC_CHECK_HEADERS([malloc.h])


# -----------------------------
# declarations
# -----------------------------

# 
# aligned_alloc
# 

AC_MSG_CHECKING([aligned_alloc])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#define _ISOC11_SOURCE 1
#include <stdlib.h>
]],
	[[ return (aligned_alloc(1, 1) ? 0 : 1); ]])],
	[have_aligned_alloc="yes"],
	[have_aligned_alloc="no"])
AC_MSG_RESULT([$have_aligned_alloc])
if test x"$have_aligned_alloc" = "xyes" ; then
   AC_DEFINE_UNQUOTED([HAVE_ALIGNED_ALLOC],[1],
		[if set, aligned_alloc is declared in stdlib.h])
fi
AM_CONDITIONAL([BUILD_TEST_MYTH_ALIGNED_ALLOC],
	[test x"$have_aligned_alloc" = "xyes"])

# 
# memalign
# 

AC_MSG_CHECKING([memalign])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#define _ISOC11_SOURCE 1
#include <stdlib.h>
]],
	[[ return (memalign(1, 1) ? 0 : 1); ]])],
	[have_memalign="yes"],
	[have_memalign="no"])
AC_MSG_RESULT([$have_memalign])
if test x"$have_memalign" = "xyes" ; then
   AC_DEFINE_UNQUOTED([HAVE_MEMALIGN],[1],
		[if set, memalign is declared in stdlib.h])
fi
AM_CONDITIONAL([BUILD_TEST_MYTH_MEMALIGN],
	[ test x = y ])

# [ test x"$have_memalign" = "xyes" ]

# 
# pvalloc
# 

AC_MSG_CHECKING([pvalloc])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#define _ISOC11_SOURCE 1
#include <stdlib.h>
]],
	[[ return (pvalloc(1, 1) ? 0 : 1); ]])],
	[have_pvalloc="yes"],
	[have_pvalloc="no"])
AC_MSG_RESULT([$have_pvalloc])
if test x"$have_pvalloc" = "xyes" ; then
   AC_DEFINE_UNQUOTED([HAVE_PVALLOC],[1],
		[if set, pvalloc is declared in stdlib.h])
fi
AM_CONDITIONAL([BUILD_TEST_MYTH_PVALLOC],
	[test x"$have_pvalloc" = "xyes" ])


# 
# pthread_barrier
# 

AC_MSG_CHECKING([pthread_barrier_init])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
]],
	[[ pthread_barrier_init(0, 0, 0); ]])],
	[have_pthread_barrier_init="yes"],
	[have_pthread_barrier_init="no"])
AC_MSG_RESULT([$have_pthread_barrier_init])
if test x"$have_pthread_barrier_init = "xyes" ; then
   AC_DEFINE_UNQUOTED([HAVE_PTHREAD_BARRIER],[1],
		[if set, pthread_barrier is declared in pthread.h])
fi
AM_CONDITIONAL([BUILD_TEST_PTH_BARRIER],
	[test x"$have_pthread_barrier_init = "xyes" ])


# 
# pthread_yield
# 

AC_MSG_CHECKING([pthread_yield])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#define _GNU_SOURCE
#include <pthread.h>
]],
	[[ pthread_yield(); ]])],
	[have_pthread_yield="yes"],
	[have_pthread_yield="no"])
AC_MSG_RESULT([$have_pthread_yield])
if test x"$have_pthread_yield = "xyes" ; then
   AC_DEFINE_UNQUOTED([HAVE_PTHREAD_YIELD],[1],
		[if set, pthread_yield is declared in pthread.h])
fi
AM_CONDITIONAL([BUILD_TEST_PTH_BARRIER],
	[test x"$have_pthread_yield = "xyes" ])


#
# for C program tests that follow, we treat warnings as error,
# as some tests work by checking if the compiler issues a
# warning
#

AC_LANG_WERROR

#
# check if sysv_abi attribute is available
#

AC_MSG_CHECKING([sysv_abi attribute])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[ __attribute__((sysv_abi)) void fun() { } ]])], 
                  [AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_ATTR_SYSV_ABI], [1], [if set, sysv_abi attribute is available])],
		  [AC_MSG_RESULT([no]);  AC_DEFINE([HAVE_ATTR_SYSV_ABI], [0], [if set, sysv_abi attribute is available])])

#
# check if what kind of symbols are defined for functions in 
# shared libraries. from foo, foo? _foo? foo@PLT? _foo@PLT? 
#

AC_MSG_CHECKING([global symbol modifier])
AC_LINK_IFELSE(
[AC_LANG_PROGRAM(
[[ void fun() { asm volatile("call _malloc@PLT"); } ]],
[[ fun(); ]])],
[with_underscore_with_PLT=yes],
[with_underscore_with_PLT=no])

AC_LINK_IFELSE(
[AC_LANG_PROGRAM(
[[ void fun() { asm volatile("call malloc@PLT"); } ]],
[[ fun(); ]])],
[no_underscore_with_PLT=yes],
[no_underscore_with_PLT=no])

AC_LINK_IFELSE(
[AC_LANG_PROGRAM(
[[ void fun() { asm volatile("call _malloc"); } ]],
[[ fun(); ]])],
[with_underscore_no_PLT=yes],
[with_underscore_no_PLT=no])

AC_LINK_IFELSE(
[AC_LANG_PROGRAM(
[[ void fun() { asm volatile("call malloc"); } ]],
[[ fun(); ]])],
[no_underscore_no_PLT=yes],
[no_underscore_no_PLT=yes])

if test "x$with_underscore_with_PLT" = "xyes" ; then
   AC_MSG_RESULT([with underscore, with PLT])
   AC_DEFINE([GLOBAL_SYM_MODIFIER], [GLOBAL_SYM_MODIFIER_WITH_UNDERSCORE_WITH_PLT], [global symbol modifier (_foo@PLT)])
elif test "x$no_underscore_with_PLT" = "xyes" ; then
   AC_MSG_RESULT([no underscore, with PLT])
   AC_DEFINE([GLOBAL_SYM_MODIFIER], [GLOBAL_SYM_MODIFIER_NO_UNDERSCORE_WITH_PLT], [global symbol modifier (foo@PLT)])
elif test "x$with_underscore_no_PLT" = "xyes" ; then
   AC_MSG_RESULT([with underscore, no PLT])
   AC_DEFINE([GLOBAL_SYM_MODIFIER], [GLOBAL_SYM_MODIFIER_WITH_UNDERSCORE_NO_PLT], [global symbol modifier (_foo)])
elif test "x$no_underscore_no_PLT" = "xyes"  ; then
   AC_MSG_RESULT([no underscore, no PLT])
   AC_DEFINE([GLOBAL_SYM_MODIFIER], [GLOBAL_SYM_MODIFIER_NO_UNDERSCORE_NO_PLT], [global symbol modifier (foo)])
else
   AC_MSG_ERROR([could not get how to call functions in a shared library from assembly])
fi

#
# conditionally build test programs
# we compile C++ test programs only when
# C++ compiler is available.
# we compile C++ test programs relying on C++ lambdas
# only when C++ compiler supports them
#
AC_PROG_CXX
AC_LANG_PUSH([C++])
AC_LANG_CPLUSPLUS
AC_MSG_CHECKING([C++ lambda expression support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ [=] { }(); ]])], 
                  [AC_MSG_RESULT([yes]); cxx_lambda_available=yes],
		  [AC_MSG_RESULT([no]); cxx_lambda_available=no])
AM_CONDITIONAL([CXX_AVAILABLE], [test "x$ac_cv_prog_cxx_g" = "xyes"])
AM_CONDITIONAL([CXX_LAMBDA_AVAILABLE], [test "x$cxx_lambda_available" = "xyes"])
AC_LANG_POP([C++])

AC_CHECK_HEADERS([sqlite3.h], [AC_SUBST(LIBSQLITE3, [-lsqlite3])])

# initialize libtool
LT_INIT
AC_CONFIG_FILES([
	Makefile 
	include/Makefile
	src/Makefile
	src/profiler/Makefile
	src/profiler/dag2any/Makefile
	src/profiler/drview/Makefile
	tests/Makefile 
])
dnl tests/native/Makefile 
dnl tests/c/Makefile 
dnl tests/c/fib/Makefile 
dnl tests/c/profiler/Makefile 
dnl tests/c/profiler/minimum/Makefile 
dnl tests/cxx/Makefile 
dnl tests/cxx/mtbb/Makefile 
dnl tests/cxx/mtbb/callable/Makefile
dnl tests/cxx/mtbb/callable/bin/Makefile
dnl tests/cxx/mtbb/callable/profiler/Makefile
dnl tests/cxx/mtbb/lambda/Makefile
dnl tests/cxx/mtbb/lambda/bin/Makefile
dnl tests/cxx/mtbb/lambda/profiler/Makefile
AC_CONFIG_HEADERS([src/config.h])
AC_OUTPUT
