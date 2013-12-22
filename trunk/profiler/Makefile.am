
# required when you use libtool (for building shared libs)
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libdr.la
include_HEADERS = dag_recorder.h dag_recorder_inl.h

libdr_la_SOURCES = record_dag.c chronological.c gen_dot.c gen_gpl.c
libdr_la_CFLAGS = $(CFLAGS)
