/* 
 * read_dag.c
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dag_recorder_impl.h"

static dr_pi_dag * 
dr_read_dag(const char * filename) {
  int fd = open(filename, O_RDONLY);
  long n, m;
  void * a;
  dr_pi_dag * G;
  if (fd == -1) {
    fprintf(stderr, "open: %s (%s)\n", 
	    strerror(errno), filename);
    return 0;
  }
  if (read(fd, &n, sizeof(n)) != sizeof(n)
      || read(fd, &m, sizeof(m)) != sizeof(m)) {
    const char * err = strerror(errno);
    fprintf(stderr, "read: %s (%s) offset %ld\n", 
	    err, filename, lseek(fd, 0, SEEK_CUR));
    close(fd);
    return 0;
  }
  off_t real_sz = lseek(fd, 0, SEEK_END);
  off_t expected_sz = sizeof(n) + sizeof(m) 
    + sizeof(dr_pi_dag_node) * n
    + sizeof(dr_pi_dag_edge) * m;
  if (expected_sz != real_sz) {
    fprintf(stderr, "error: file %s may be corrupted; it has %ld nodes and %ld edges; we expected %ld bytes, but the file is %ld bytes\n", 
	    filename, n, m, expected_sz, real_sz);
    close(fd);
    return 0;
  }
  a = mmap(NULL, 
	   sizeof(dr_pi_dag_node) * n
	   + sizeof(dr_pi_dag_edge) * m, PROT_READ | PROT_WRITE,
	   MAP_PRIVATE, fd, 0);
  if (a == MAP_FAILED) {
    fprintf(stderr, "mmap: %s (%s)\n", strerror(errno), filename);
    close(fd);
    return 0;
  }
  G = dr_malloc(sizeof(dr_pi_dag));
  G->n = n;
  G->m = m;
  G->T = (dr_pi_dag_node *)(a + sizeof(n) + sizeof(m));
  G->E = (dr_pi_dag_edge *)&G->T[n];
  close(fd);
  return G;
}

/* record_dag.c */
int dr_gen_dot(dr_pi_dag * G);
int dr_gen_gpl(dr_pi_dag * G);
void dr_opts_init(dr_options * opts);

int dr_read_and_analyze_dag(const char * filename) {
  dr_pi_dag * G = dr_read_dag(filename);
  if (G) {
    dr_opts_init(0);
    if (dr_gen_dot(G) == 0) return 0;
    if (dr_gen_gpl(G) == 0) return 0;
    return 1;
  } else {
    return 0;
  }
}
