AC_INIT([massivethreads], [0.21], [massivethreads@eidos.ic.i.u-tokyo.ac.jp], [massivethreads], [http://code.google.com/p/massivethreads/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
# required when you build shared libraries, relying on libtool
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
# use -O3 optimization instead of -O2
if test "$ac_test_CFLAGS" = set; then
  CFLAGS=$ac_save_CFLAGS
elif test $ac_cv_prog_cc_g = yes; then
  if test "$GCC" = yes; then
    CFLAGS="-g -O3 -Wall -ftls-model=initial-exec"
  else
    CFLAGS="-g"
  fi
else
  if test "$GCC" = yes; then
    CFLAGS="-O3 -Wall -ftls-model=initial-exec"
  else
    CFLAGS=
  fi
fi
AC_PROG_AWK
AM_PROG_AS
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR(Test for dlopen failed.)])
AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR(Test for libpthread failed.)])

# Check size
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([void*])

# Check functions
# AC_CHECK_FUNCS(__builtin_unreachable __builtin_clz)

#
# --enable-malloc-wrapper
#   flag controlling the behavior of malloc
#

AC_ARG_ENABLE([malloc_wrapper],
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
			      malloc implementation.])])

if test "x$enable_malloc_wrapper" = "xyes" ; then
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


# initialize libtool
LT_INIT
AC_CONFIG_FILES([Makefile tests/Makefile tests/fib/Makefile])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
