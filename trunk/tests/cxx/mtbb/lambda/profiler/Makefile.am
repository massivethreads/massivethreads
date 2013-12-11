check_PROGRAMS = lambda_profiler_serial lambda_profiler_mth

top = ../../../../..

lambda_profiler_serial_SOURCES = lambda_profiler.cc
lambda_profiler_mth_SOURCES    = lambda_profiler.cc

lambda_profiler_serial_CXXFLAGS = -I$(top)/mtbb -I$(top)/profiler -std=c++0x -DTO_SERIAL
lambda_profiler_mth_CXXFLAGS    = -I$(top)/mtbb -I$(top)/profiler -std=c++0x -DTO_MTHREAD_NATIVE

lambda_profiler_serial_LDADD = $(top)/profiler/libdr.la 
lambda_profiler_mth_LDADD    = $(top)/profiler/libdr.la $(top)/libmyth.la

TESTS = $(check_PROGRAMS)
