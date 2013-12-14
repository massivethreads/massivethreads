check_PROGRAMS = \
	minimum_tbblike_standalone \
	minimum_tbblike_mth \
	minimum_omplike_standalone \
	minimum_omplike_mth

top = ../../../..

minimum_tbblike_standalone_SOURCES = minimum_tbblike.c
minimum_tbblike_mth_SOURCES        = minimum_tbblike.c
minimum_omplike_standalone_SOURCES = minimum_omplike.c
minimum_omplike_mth_SOURCES        = minimum_omplike.c

minimum_tbblike_standalone_CFLAGS = -I$(top) -I$(top)/profiler -DSTANDALONE_TEST
minimum_tbblike_mth_CFLAGS        = -I$(top) -I$(top)/profiler 
minimum_omplike_standalone_CFLAGS = -I$(top) -I$(top)/profiler -DSTANDALONE_TEST
minimum_omplike_mth_CFLAGS        = -I$(top) -I$(top)/profiler 

minimum_tbblike_standalone_LDADD = $(top)/profiler/libdr.la
minimum_tbblike_mth_LDADD        = $(top)/profiler/libdr.la $(top)/libmyth.la
minimum_omplike_standalone_LDADD = $(top)/profiler/libdr.la
minimum_omplike_mth_LDADD        = $(top)/profiler/libdr.la $(top)/libmyth.la

TESTS = $(check_PROGRAMS)
