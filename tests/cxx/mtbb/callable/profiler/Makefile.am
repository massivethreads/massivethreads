check_PROGRAMS = callable_profiler_serial callable_profiler_mth

top = ../../../../..

callable_profiler_serial_SOURCES = callable_profiler.cc
callable_profiler_mth_SOURCES    = callable_profiler.cc

callable_profiler_serial_CXXFLAGS = -I$(top)/profiler -DTO_SERIAL
callable_profiler_mth_CXXFLAGS    = -I$(top)/profiler -I$(top)/mtbb -DTO_MTHREAD_NATIVE

callable_profiler_serial_LDADD = $(top)/profiler/libdr.la
callable_profiler_mth_LDADD    = $(top)/profiler/libdr.la $(top)/libmyth.la 

TESTS = $(check_PROGRAMS)
