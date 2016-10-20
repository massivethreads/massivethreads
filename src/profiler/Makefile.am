SUBDIRS = . dag2any drview

# required when you use libtool (for building shared libs)
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libdr.la
include_HEADERS = dag_recorder.h dag_recorder_inl.h dag_recorder_impl.h papi_counters.h

libdr_la_SOURCES = dag_recorder.c dag_recorder_no_inl.c chronological.c dr_dump.c gen_stat.c gen_dot.c gen_gpl.c gen_text.c read_dag.c options.c interpolate_counters.c papi_counters.c
libdr_la_CFLAGS = $(CFLAGS)
libdr_la_LDFLAGS = $(LDFLAGS)
if BUILD_DAG_RECORDER_WITH_PAPI
libdr_la_CFLAGS += -DDAG_RECORDER_ENABLE_PAPI=1
libdr_la_LDFLAGS += -lpapi
endif
