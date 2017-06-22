/*
 * papi_counters.c
 */

#include "papi_counters.h"

#define DAG_RECORDER 2
#include "dag_recorder_impl.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* if DAG_RECORDER_ENABLE_PAPI is false,
   you should be able to compile this
   file without papi and all 
   functions become noop */
#if DAG_RECORDER_ENABLE_PAPI
#include <papi.h>
#endif

#if DAG_RECORDER_ENABLE_PAPI
/* show PAPI error message */
static void 
dr_papi_show_error_(int retval, const char * file, int line) {
  fprintf(stderr, "error:%s:%d papi %d: %s\n", 
          file, line, retval, 
#if DAG_RECORDER_ENABLE_PAPI
          PAPI_strerror(retval)
#else
          "????"
#endif
          );
}

/* parse event name event using PAPI */
static int 
dr_papi_parse_event_papi(char * event_name, int * event_code, int * papi_retval) {
#if DAG_RECORDER_ENABLE_PAPI
  *papi_retval = PAPI_event_name_to_code(event_name, event_code);
  if (*papi_retval == PAPI_OK) {
    return 1;
  } else {
    return 0;
  }
#else
  /* always succeeed */
  (void) event_name; (void) event_code; (void) papi_retval;
  return 1;
#endif
}

/* parse raw event like r1234 (I thought it works,
   but it didn't) */
static int 
dr_papi_parse_event_raw(const char * event_name, int * event_code) {
  if (strlen(event_name) == 0) return 0;
  else if (event_name[0] != 'r') return 0;
  else {
    char * endptr = 0;
    long ec = strtol(event_name + 1, &endptr, 16);
    if (*endptr == 0) {
      * event_code = (int)ec;
      return 1;
    } else {
      return 0;
    }
  }
}

/* parse event name like 0x1234. it did not work either */
static int 
dr_papi_parse_event_hex(const char * event_name, int * event_code) {
  if (strlen(event_name) <= 1) return 0;
  else if (event_name[0] != '0' || event_name[1] != 'x') return 0;
  else {
    char * endptr = 0;
    long ec = strtol(event_name + 2, &endptr, 16);
    if (*endptr == 0) {
      * event_code = ec;
      return 1;
    } else {
      return 0;
    }
  }
}

/* parse PAPI event name */
static int 
dr_papi_parse_event(char * event, int * event_code, int * papi_retval) {
  if (dr_papi_parse_event_papi(event, event_code, papi_retval)) return 1;
  if (dr_papi_parse_event_raw(event, event_code)) return 1;
  if (dr_papi_parse_event_hex(event, event_code)) return 1;
  return 0;
}

/* events : a string like "L1_DCM,L2_DCM,L3_DCM"
   event_codes : an array that can have max_events
   parse events into an array of event codes.
   return the number of events successfully parsed */
static int 
dr_papi_parse_events(const char * events, 
                   char ** event_names, 
                   int * event_codes, int max_events) {
  int i = 0;
  char event_name[128];
  const char * p = events;

  int k;
  for (k = 0; k < max_events; k++) {
    event_names[k] = NULL;
    event_codes[k] = 0;
  }

  if (p) 
    p = strdup(p);
  /* p is like "PAPI_TOT_INS,PAPI_FP_INS,PAPI_LD_INS.
     split it by ',', convert each of them to event_code,
     and put them into code_names array */
  while (p) {
    const char * fmt = "%s";
    char * q = strchr((char *)p, ',');
    /* n = length of the string before , */
    int n = (q == NULL ? (int)strlen(p) : q - p);
    unsigned int to_write = n + 1;
    if (to_write > sizeof(event_name)) {
      fprintf(stderr, "warning: event name too long %s (ignored)\n", p);
    } else {
      /* first check if PAPI recognizes it */
      int event_code = 0;
      int papi_retval = 0; 
      snprintf(event_name, to_write, fmt, p);
      if (dr_papi_parse_event(event_name, &event_code, &papi_retval)) {
        if (i < max_events) {
          event_names[i] = strdup(event_name);
          event_codes[i] = event_code;
          //fprintf(stderr, "%s (%x)\n", event_name, event_code);
          i++;
        } else {
          /* but the user specified too many events */
          fprintf(stderr, 
                  "warning: too many events specified%s (ignored)\n", 
                  event_name);
        }
      } else {
        fprintf(stderr, 
                "warning: could not find event %s; "
                "you made typo or forgot to init_runtime? the papi error follows:\n", event_name);
        dr_papi_show_error_(papi_retval, __FILE__, __LINE__);
      }
    }
    if (q == NULL) break;
    else p = q + 1;
  };
  return i;                        /* event_count */
}

static void 
mem_barrier() {
  __sync_synchronize();
}
#endif /* DAG_RECORDER_ENABLE_PAPI */

dr_papi_gdata_t *
dr_papi_make_gdata() {
#if DAG_RECORDER_ENABLE_PAPI
  dr_papi_gdata_t * gd = (dr_papi_gdata_t *) malloc(sizeof(dr_papi_gdata_t));
  memset(gd, 0, sizeof(dr_papi_gdata_t));
  return gd;
#else
  return NULL;
#endif /* DAG_RECORDER_ENABLE_PAPI */
}

/* initialize the PAPI measurement in DAR Recorder.
   This function is not thread-safe, should be executed
   one time by one thread at the beginning. */
int
dr_papi_init(dr_papi_gdata_t * gd) {
#if DAG_RECORDER_ENABLE_PAPI
  {
    int ver = PAPI_library_init(PAPI_VER_CURRENT);
    if (ver != PAPI_VER_CURRENT) { 
      fprintf(stderr, "error:%s:%d: could not init papi. PAPI_library_init(version=%d.%d.%d.%d) returned version=%d.%d.%d.%d (header version and library version differ; check if you are using papi.h and libpapi.so of the same version)\n",
              __FILE__, __LINE__, 
              PAPI_VERSION_MAJOR(PAPI_VER_CURRENT),
              PAPI_VERSION_MINOR(PAPI_VER_CURRENT),
              PAPI_VERSION_REVISION(PAPI_VER_CURRENT),
              PAPI_VERSION_INCREMENT(PAPI_VER_CURRENT),
              PAPI_VERSION_MAJOR(ver),
              PAPI_VERSION_MINOR(ver),
              PAPI_VERSION_REVISION(ver),
              PAPI_VERSION_INCREMENT(ver));
      return 0;
    }
  }
  if (PAPI_thread_init(pthread_self) != PAPI_OK) {
    fprintf(stderr, "error:%s:%d: could not init papi\n",
            __FILE__, __LINE__);
    return 0;
  }
  {
    /* impose user-specified max on events */
    int max_events = (GS.opts.papi_max_events < dr_static_max_papi_events) ? GS.opts.papi_max_events : dr_static_max_papi_events;
    gd->n_events = dr_papi_parse_events(GS.opts.papi_events, gd->event_names, gd->g_event_codes, max_events);
    gd->initialized = 1;
    mem_barrier();
  }
  if (DAG_RECORDER_VERBOSE_LEVEL >= 1) {
    printf("dr_papi_init(): successful.\n");
  }
  return 1;
#else
  (void) gd;
  return 1;
#endif /* DAG_RECORDER_ENABLE_PAPI */  
}


dr_papi_tdata_t *
dr_papi_make_tdata(dr_papi_gdata_t * gd) {
#if DAG_RECORDER_ENABLE_PAPI
  dr_papi_tdata_t * td = (dr_papi_tdata_t *) malloc(sizeof(dr_papi_tdata_t));
  memset(td, 0, sizeof(dr_papi_tdata_t));
  return td;
#else
  return NULL;
#endif /* DAG_RECORDER_ENABLE_PAPI */
}

#if DAG_RECORDER_ENABLE_PAPI
/* create an event set from an array of event_codes,
   set it to thread-local data td. 
   return the number of events successfully put in the set */
static int 
dr_papi_make_thread_eventset(char ** event_names, int * event_codes, int n_events, dr_papi_tdata_t * td) {
#if DAG_RECORDER_ENABLE_PAPI
  int n_ok = 0;
  int es = PAPI_NULL;
  int retval0 = PAPI_create_eventset(&es);
  if (retval0 != PAPI_OK) {
    dr_papi_show_error_(retval0, __FILE__, __LINE__);
    td->n_events = n_ok;
    td->eventset = es;
    return 0;
  }
  int i;
  for (i = 0; i < n_events; i++) {
    int retval1 = PAPI_add_event(es, event_codes[i]);
    if (retval1 == PAPI_OK) {
      td->t_event_codes[n_ok++] = event_codes[i];
    } else {
      fprintf(stderr, "couldn't add event %s (code=%x)\n", 
              event_names[i], event_codes[i]);
      dr_papi_show_error_(retval1, __FILE__, __LINE__);
    }
  }
  td->n_events = n_ok;
  td->eventset = es;
  return n_ok;
#else
  (void) event_names; 
  (void) event_codes;
  (void) n_events;
  (void) td;
  return 0;                        /* no events */
#endif
}
#endif /* DAG_RECORDER_ENABLE_PAPI */

/* initialize thread-specific PAPI measurement,
   should be called by every thread */
int
dr_papi_init_thread(dr_papi_gdata_t * gd, dr_papi_tdata_t * td) {
#if DAG_RECORDER_ENABLE_PAPI
  if (!gd->initialized) {
    return 0;
  }
  {
    int pe = PAPI_register_thread();
    if (pe != PAPI_OK) {
      dr_papi_show_error_(pe, __FILE__, __LINE__);
      return 0;
    }
  }
  {
    if (!dr_papi_make_thread_eventset(gd->event_names, gd->g_event_codes, gd->n_events, td)) {
      if (gd->n_events) {
        return 0;
      }
    }
  }
  {
    int pe = PAPI_start(td->eventset);
    if (pe != PAPI_OK) {
      dr_papi_show_error_(pe, __FILE__, __LINE__);
      return 0;
    }
  }
  td->sampling_interval = GS.opts.papi_sampling_interval;
  td->next_sampling_clock = 0;
  td->initialized = 1;
  if (DAG_RECORDER_VERBOSE_LEVEL >= 1) {
    printf("dr_papi_init_thread(): successful.\n");
  }
 return 1;
#else
  (void) gd;
  (void) td;
  return 1;
#endif /* DAG_RECORDER_ENABLE_PAPI */  
};

#if DAG_RECORDER_ENABLE_PAPI
static long long
dr_papi_interpolate(unsigned long long t0, long long v0, unsigned long long t1, long long v1, unsigned long long t) {
  (void) dr_check(t0 < t1);
  double a = ((double)t - (double)t0) / ((double)t1 - (double)t0);
  double b = ((double)t1 - (double)t) / ((double)t1 - (double)t0);
  return (long long)(a * v1 + b * v0);  
}
#endif /* DAG_RECORDER_ENABLE_PAPI */

int
dr_papi_read(dr_papi_gdata_t * gd, dr_papi_tdata_t * td, long long * values) {
#if DAG_RECORDER_ENABLE_PAPI
  if (!gd || !td || !values) {
    if (DAG_RECORDER_VERBOSE_LEVEL >= 2) {
      fprintf(stderr, "dr_papi_read(): abort due to NULL gd or td or values.\n");
    }
    return 0;
  }
  unsigned long long now = dr_get_tsc();
  if (td->n_events) {
    if (now < td->next_sampling_clock) {
      /* skip due to too short interval, leave for post-makeup by interpolation */
#if 0
      fprintf(stderr, "skipped now = %llu, next = %llu, interval = %llu\n", now, td->next_sampling_clock, td->sampling_interval);
#endif
      int c;        
      for (c = 0; c < td->n_events; c++) {
        values[c] = dr_papi_interpolate(td->t0, td->values0[c], td->t1, td->values1[c], now);
      }
    } else {
      /* read from PAPI */
      td->next_sampling_clock = now + td->sampling_interval;
      int pe = PAPI_read(td->eventset, values);
      if (pe != PAPI_OK) {
        dr_papi_show_error_(pe, __FILE__, __LINE__);
        return 0;
      }
      /* for interpolation */
      td->t0 = td->t1;
      td->t1 = now;
      int c;
      for (c = 0; c < td->n_events; c++) {
        td->values0[c] = td->values1[c];
        td->values1[c] = values[c];
      }
    }
  }
  return 1;
#else
  (void) gd;
  (void) td;
  return 1;
#endif /* DAG_RECORDER_ENABLE_PAPI */  
};
