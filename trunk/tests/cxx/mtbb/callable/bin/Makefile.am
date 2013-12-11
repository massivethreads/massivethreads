#noinst_PROGRAMS = bin_serial bin_mth_native bin_mth_compat
check_PROGRAMS = bin_serial bin_mth_native bin_mth_compat

bin_serial_SOURCES     = bin.cc
bin_mth_native_SOURCES = bin.cc
bin_mth_compat_SOURCES = bin.cc

bin_serial_CXXFLAGS     = -I../../../../../mtbb -DTO_SERIAL
bin_mth_native_CXXFLAGS = -I../../../../../mtbb -DTO_MTHREAD_NATIVE
bin_mth_compat_CXXFLAGS = -I../../../../../mtbb -DTO_MTHREAD

bin_mth_native_LDADD = ../../../../../libmyth.la 
bin_mth_compat_LDADD = ../../../../../libmyth.la 

TESTS = $(check_PROGRAMS)
