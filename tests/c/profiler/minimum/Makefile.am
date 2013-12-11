check_PROGRAMS = minimum_standalone minimum_mth

minimum_standalone_SOURCES = minimum.c
minimum_mth_SOURCES        = minimum.c

minimum_standalone_CFLAGS = -I../../../.. -I../../../../profiler -DSTANDALONE_TEST
minimum_mth_CFLAGS        = -I../../../.. -I../../../../profiler 

minimum_standalone_LDADD  = ../../../../profiler/libdr.la 
minimum_mth_LDADD         = ../../../../profiler/libdr.la ../../../../libmyth.la 

TESTS = $(check_PROGRAMS)
