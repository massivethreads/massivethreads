/*
 * papi_counters.h
 */
#ifndef __PAPI_COUNTERS_H__
#define __PAPI_COUNTERS_H__
#pragma once

#if __cplusplus
extern "C" {
#endif

  enum { dr_static_max_papi_events = 10 };

  typedef struct dr_papi_gdata dr_papi_gdata_t;
  typedef struct dr_papi_tdata dr_papi_tdata_t;

  struct dr_papi_gdata {
    int initialized;
    char * event_names[dr_static_max_papi_events];
    int g_event_codes[dr_static_max_papi_events];
    int n_events;
  };

  struct dr_papi_tdata {
    union {
      struct {
        int initialized;
        int t_event_codes[dr_static_max_papi_events];
        int n_events;
        int eventset;
        unsigned long long sampling_interval;
        unsigned long long next_sampling_clock;
      };
      char __pad__[64][2];
    };  
  };

  dr_papi_gdata_t * dr_papi_make_gdata();
  int dr_papi_init(dr_papi_gdata_t *);
  dr_papi_tdata_t * dr_papi_make_tdata(dr_papi_gdata_t *);
  int dr_papi_init_thread(dr_papi_gdata_t *, dr_papi_tdata_t *);
  int dr_papi_read(dr_papi_gdata_t *, dr_papi_tdata_t *, long long *);
  
#if __cplusplus
};
#endif

#endif
