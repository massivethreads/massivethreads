AC_INIT([massivethreads], [0.97], [massivethreads@eidos.ic.i.u-tokyo.ac.jp], [massivethreads], [https://github.com/massivethreads/massivethreads/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
# required when you build shared libraries, relying on libtool
AC_CONFIG_MACRO_DIR([m4])
# avoid executing autotools on the user side
AM_MAINTAINER_MODE([disable])

AC_PROG_CC
# use -O3 optimization instead of -O2
if test "x$ac_test_CFLAGS" = "xset"; then
  CFLAGS=$ac_save_CFLAGS
elif test "x$ac_cv_prog_cc_g" = "xyes"; then
  if test "x$GCC" = "xyes"; then
    CFLAGS="-g -O3 -Wall -ftls-model=initial-exec"
  else
    CFLAGS="-g"
  fi
else
  if test "x$GCC" = "xyes"; then
    CFLAGS="-O3 -Wall -ftls-model=initial-exec"
  else
    CFLAGS=
  fi
fi
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

# Essential libraries
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR(Test for dlopen failed.)])
AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR(Test for libpthread failed.)])
AC_CHECK_LIB(rt, clock_gettime , [], [AC_MSG_ERROR(Test for clock_gettime failed.)])

# Check sizes
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([void*])

# Check functions
# AC_CHECK_FUNCS(__builtin_unreachable __builtin_clz)

#
# check declarations
#

AC_MSG_CHECKING([declaration of aligned_alloc])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#define _ISOC11_SOURCE 1
#include <stdlib.h>
]],
					     [[ aligned_alloc; ]])], 
                  [AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_DECL_ALIGNED_ALLOC], [1], [if set, aligned_alloc is declared in stdlib.h])],
		  [AC_MSG_RESULT([no]);  AC_DEFINE([HAVE_DECL_ALIGNED_ALLOC], [0], [if set, aligned_alloc is declared in stdlib.h])])


dnl AC_CHECK_DECL([aligned_alloc])
dnl if test "x$ac_cv_have_decl_aligned_alloc" = xyes; then
dnl    AC_DEFINE([HAVE_DECL_ALIGNED_ALLOC], [1], [If set, aligned_alloc is declared]) 
dnl else
dnl    AC_DEFINE([HAVE_DECL_ALIGNED_ALLOC], [0], [If set, aligned_alloc is declared]) 
dnl fi

#
# --enable-malloc-wrapper
#   flag controlling the behavior of malloc
#

AC_ARG_ENABLE([malloc-wrapper],
              [AS_HELP_STRING([--enable-malloc-wrapper=yes|no],
	                      [Enable or disable malloc wrapper. "no" completely 
			      disables it when compiling MassiveThreads. "yes" 
			      enables it, but you can still disable it at runtime 
			      by setting environment variable MYTH_DONT_WRAP_MALLOC=1.  
			      Default is "yes".  If you experience a segfault for 
			      programs which don't without MassiveThreads, it may be a 
			      MassiveThreads bug around malloc wapper; chances are
			      that MYTH_DONT_WRAP_MALLOC=0 at runtime or --disable-malloc 
			      will avoid it at the cost of resorting to less scalable 
			      malloc implementation.])],
			     ,[enable_malloc_wrapper="yes"])

if test "x$enable_malloc_wrapper" != "xno" ; then
  AC_DEFINE_UNQUOTED([MYTH_WRAP_MALLOC],[],[If unset, we never wrap malloc])
  AC_DEFINE_UNQUOTED([MYTH_WRAP_MALLOC_RUNTIME],[],[If set, you can choose wrap malloc or not at runtime. default is to wrap.  set environment variable MYTH_DONT_WRAP_MALLOC=1 not to wrap it])
fi

#
# set user threads' default stack size
#

AC_ARG_WITH([default-stack-size],
            [AS_HELP_STRING([--with-default-stack-size=S],
                            [Set default stack size of threads to S bytes.
                             Default: 131072.  You can change it at runtime
			     by setting MYTH_DEF_STKSIZE.])],
            [default_stack_size="$withval"],
            [default_stack_size=131072])

AC_DEFINE_UNQUOTED([MYTH_DEF_STACK_SIZE],[$default_stack_size],[Default stack size])

#
# set the scheduler thread's stack size
#

AC_ARG_WITH([sched-stack-size],
            [AS_HELP_STRING([--with-sched-stack-size=S],
                            [Set default stack size of the scheduler thread to S bytes.
                             Default: 1048576.])],
            [sched_stack_size="$withval"],
            [sched_stack_size=1048576])

AC_DEFINE_UNQUOTED([MYTH_SCHED_STACK_SIZE],[$sched_stack_size],[Scheduler stack size])

#
# --enable-eco-mode
#   flag controlling the behavior of eco-mode scheduler
#

AC_ARG_ENABLE([eco-mode],
              [AS_HELP_STRING([--enable-eco-mode=yes|no],
	                      [Enable or disable eco-mode scheduler. "no" completely 
			      disables it when compiling MassiveThreads. "yes" 
			      enables it, but it is not activated by default.
			      To use it you have to activate by setting environment
			      variable MYTH_ECO_MODE=1. Default is "yes".
			      When eco-mode scheduler is enabled, thread creation
			      overhead might be slightly increased, whichever it is activated or not])],
			      ,
			     [enable_eco_mode="yes"])

if test "x$enable_eco_mode" != "xno" ; then
  AC_DEFINE_UNQUOTED([MYTH_ECO_MODE],[],[If unset, eco-mode is disabled])
  AC_DEFINE_UNQUOTED([MYTH_ECO_TEIAN_STEAL],[],[If unset, eco-mode is disabled])
fi

#
# --enable-bind-workers
#   flag controlling the binding of worker thread 
#

AC_ARG_ENABLE([bind-workers],
              [AS_HELP_STRING([--enable-bind-workers=yes|no],
	                      [Change the default behavior of worker thread binding. 
			      "yes" enables it and each worker thread is bound to each CPU core. 
			      This setting can be overridden by setting environmental variable
			      MYTH_BIND_WORKERS=[1|0]])],
			      ,
			     [enable_bind_workers="yes"])

if test "x$enable_bind_workers" != "xno" ; then
  AC_DEFINE_UNQUOTED([MYTH_BIND_WORKERS],[],[If unset, worker thread binding is disabled]) 
fi

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
	tests/native/Makefile 
	tests/c/Makefile 
	tests/c/fib/Makefile 
	tests/c/profiler/Makefile 
	tests/c/profiler/minimum/Makefile 
	tests/cxx/Makefile 
	tests/cxx/mtbb/Makefile 
	tests/cxx/mtbb/callable/Makefile
	tests/cxx/mtbb/callable/bin/Makefile
	tests/cxx/mtbb/callable/profiler/Makefile
	tests/cxx/mtbb/lambda/Makefile
	tests/cxx/mtbb/lambda/bin/Makefile
	tests/cxx/mtbb/lambda/profiler/Makefile
])
# AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_HEADERS([include/myth/config.h])
AC_OUTPUT
