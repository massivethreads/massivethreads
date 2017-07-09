#include <assert.h>
#include <stdio.h>
#include <myth/myth.h>

enum {
  status_full = 1,
  status_sleeping = 2,
};

void sleep_exit(long sec) {
  sleep(sec);
  exit(1);
}

/* -------------------------------------------- */

/* multiple producer, single consumer bounded buffer */

/* an item in a bounded buffer.
   not very important in this program (just for checking
   if the result is correct) */
typedef struct {
  volatile long producer;	/* producer rank */
  volatile long payload;	/* payload */
} bb_item;

typedef struct {
  volatile long h;	    /* number of items that have been gotten | consumer_waiting_bit */
  volatile long t;	    /* number of items that have been put */
  long sz;		    /* length of a */
  volatile bb_item * a;	    /* items array */
  myth_uncond_t u[1];
} bounded_buffer_t;

int bb_init(bounded_buffer_t * bb, long sz) {
  bb->h = 0;
  bb->t = 0;
  bb->sz = sz;
  bb->a = malloc(sizeof(bb_item) * sz);
  for (long i = 0; i < sz; i++) {
    bb->a[i].producer = -1;
    bb->a[i].payload = -1;
  }
  myth_uncond_init(bb->u);
  return 1;			/* OK */
}

#ifndef DBG
#define DBG 0
#endif
const int dbg = DBG;

/* tail pointer is <index>|<consumer_waiting_bit> */
long mk_tail(long t, int consumer_waiting) {
  return (t << 1) | consumer_waiting;
}

int is_consumer_waiting(long t_) {
  return t_ & 1;
}


/* put an item x in bounded buffer bb.
   it bails out after n_tires trials
   (assuming that it is caused by a bug, not
   an inherint scheduling delay)
 */
int bb_put(bounded_buffer_t * bb, bb_item x, long n_tries) {
  long sz = bb->sz;
  for (long i = 0; i < n_tries; i++) {
    long t_ = bb->t;		/* tail + sleep bits */
    long h  = bb->h;		/* head */
    long t = t_ >> 1;		/* t = number of items that have been put */
    int cw = is_consumer_waiting(t_);  /* consumer waiting */
    if (dbg>=2) {
      printf(" put [%ld]: h=%ld, t=(%ld,%d)\n", i, h, t, cw);
    }
    /* try to get the room to which to put the item */
    if (t - h < sz) {
      if (dbg>=2) {
	printf(" put [%ld]: not full."
	       " cas (%ld,%d) -> (%ld,%d)\n",
	       i, t, cw, t + 1, 0);
      }
      if (__sync_bool_compare_and_swap(&bb->t, t_, mk_tail(t + 1, 0))) {
	/* I won. I am the one who put t-th item */
	if (dbg>=2) {
	  printf(" put [%ld]: won\n", i);
	}
	assert(bb->a[t % sz].payload == -1);
	bb->a[t % sz].producer = x.producer;
	bb->a[t % sz].payload = x.payload;
	if (cw) {
	  if (dbg>=2) {
	    printf(" put [%ld]: wake up consumer\n", i);
	  }
	  myth_uncond_signal(bb->u);
	}
	if (dbg>=2) {
	  printf(" put [%ld]: -> OK\n", i);
	}
	return 1;			/* done */
      } else {
	if (dbg>=2) {
	  printf(" put [%ld]: lost\n", i);
	}
      }
    } else {
      /* queue is full. yield */
      myth_yield();
    }
  }
  fprintf(stderr,
	  "could not put after %ld trials."
	  " I bet something went wrong (h = %ld, t = %ld)\n",
	  n_tries, bb->h, bb->t);
  sleep_exit(1);
  return 0;			/* probably something went wrong */
}


bb_item bb_get(bounded_buffer_t * bb, long n_tries) {
  long h = bb->h;		/* head of the queue */
  long sz = bb->sz;
  for (long i = 0; i < n_tries; i++) {
    long t_ = bb->t;		/* tail of the queue */
    long t = t_ >> 1;		/* t = number of items that have been put */
    int cw = is_consumer_waiting(t_);  /* consumer waiting */
    if (dbg>=2) {
      printf(" get [%ld]: h=%ld, (%ld,%d)\n", i, h, t, cw);
    }
    assert(h <= t);
    if (h < t) {		/* the queue has an item (not empty). get it */
      assert(cw == 0);
      if (dbg>=2) {
	printf(" get [%ld]: not empty."
	       " cas (%ld,%d) -> (%ld,%d)\n",
	       i, t, cw, t, cw);
      }
      /* the element may not have been populated yet; loop until it has */
      for (long k = 0; k < n_tries; k++) {
	bb_item x = bb->a[h % sz];
	/* if x.payload == -1 => 
	   not populated yet. someone must be writing to it just now */
	if (x.payload != -1 && x.producer != -1) {
	  bb->a[h % sz].producer = -1;
	  bb->a[h % sz].payload = -1;
	  bb->h = h + 1;	/* advance head pointer */
	  if (dbg>=2) {
	    printf(" get [%ld]: -> %ld\n", i, x.payload);
	  }
	  return x;
	}
      }
      fprintf(stderr,
	      "a buffer element did not become valid in %ld trials."
	      " I bet something went wrong\n", n_tries);
      sleep_exit(1);
    } else {
      /* the queue is empty. indicate I am going to sleep by flipping
	 the bits of t.  as a producer may be about to insert an item, 
	 we need to check if I am ahead of any such producer. 
	 use cas for that */
      if (dbg>=2) {
	printf(" get [%ld]: empty. cas (%ld,%d) -> (%ld,%d)\n",
	       i, t, cw, t, 1);
      }
      if (__sync_bool_compare_and_swap(&bb->t, t_, mk_tail(t, 1))) {
	/* I won. any producer inserting an item should see t bit-flipped
	   (and wake me up). I will sleep */
	if (dbg>=2) {
	  printf(" get [%ld]: will sleep\n", i);
	}
	myth_uncond_wait(bb->u);
      }
    }
  }  
  fprintf(stderr,
	  "could not get after %ld trials."
	  " I bet something went wrong (h = %ld, t = %ld)\n",
	  n_tries, bb->h, bb->t);
  sleep_exit(1);
  bb_item dummy = { -1, -1 };
  return dummy;
}

long producer(bounded_buffer_t * bb, long n, long n_tries, int rank) {
  for (long i = 0; i < n; i++) {
    bb_item x = { rank, i };
    bb_put(bb, x, n_tries);
  }
  return 1;			/* OK */
}

long consumer(bounded_buffer_t * bb, long n, long n_tries, int n_producers) {
  long progress[n_producers];
  for (long i = 0; i < n_producers; i++) {
    progress[i] = 0;
  }
  for (long i = 0; i < n * n_producers; i++) {
    bb_item x = bb_get(bb, n_tries);
    assert(0 <= x.producer);
    assert(x.producer < n_producers);
    assert(x.payload == progress[x.producer]);
    progress[x.producer]++;
  }
  return 1;			/* OK */
}

typedef struct {
  long n;
  long n_tries;
  bounded_buffer_t * bb;
  int rank;
  int n_producers;
  myth_thread_t tid;
  void * ret;
} thread_arg_t;

void * producer_or_consumer(void * arg_) {
  thread_arg_t * a = arg_;
  int rank = a->rank;
  int n_producers = a->n_producers;
  long n = a->n;
  long n_tries = a->n_tries;
  bounded_buffer_t * bb = a->bb;
  if (rank < n_producers) {
    return (void *)producer(bb, n, n_tries, rank);
  } else {
    return (void *)consumer(bb, n, n_tries, n_producers);
  }
}

int main(int argc, char ** argv) {
  long n        = (argc > 1 ? atol(argv[1]) : 100000);
  long bb_size  = (argc > 2 ? atol(argv[2]) : 10);
  long n_producers = (argc > 3 ? atol(argv[3]) : 100);
  long n_tries  = (argc > 4 ? atol(argv[4]) : 1000 * 1000 * 1000);
  
  bounded_buffer_t bb[1];
  bb_init(bb, bb_size);
  
  thread_arg_t args[n_producers + 1];
  for (int rank = 0; rank < n_producers + 1; rank++) {
    thread_arg_t * a = &args[rank];
    a->bb = bb;
    a->n = n;
    a->n_tries = n_tries;
    a->n_producers = n_producers;
    a->rank = rank;
    a->tid = 0;
  }
  int r = myth_create_join_many_ex(&args[0].tid,
				   0,
				   producer_or_consumer,
				   args,
				   &args[0].ret,
				   sizeof(thread_arg_t),
				   0, sizeof(thread_arg_t),
				   sizeof(thread_arg_t),
				   n_producers + 1);
  assert(r == 0);
  for (int i = 0; i < n_producers; i++) {
    assert(args[i].ret == (void *)1);
  }
  printf("OK\n");
  return 0;
}


