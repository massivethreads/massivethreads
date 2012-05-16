/*
 * treecode.c
 * A treecode implementation to achive best scalability on shared memory machines.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

#ifndef	PARALLELIZE
#define PARALLELIZE	0
#endif

#define	N_CHILD		8

typedef double real_t;
typedef long idx_t;
typedef unsigned long mot_t;

typedef struct vec {
  double x, y, z;  
} vec_t;

typedef struct mol {
  idx_t id; 
  mot_t mid;
  double mass;
  vec_t acc;
  vec_t pos;
  vec_t vel;
} mol_t;

/* tree */
typedef struct node {
  struct node *child[N_CHILD];
  mol_t v_mol;  /* value of node */
  idx_t n_mol;  /* # of mols in tree */
  idx_t molarr_l, molarr_h; /* array range */
  idx_t morton_l, morton_h; /* morton range */
} node_t;

/* simulation data structure */
typedef struct sim {  // simulation instance
  unsigned long n_mol;
  int steps;
  mol_t *mols;
  struct node *tree;
  long int t_init, t_treebuild, t_treefree;
} sim_t;

/* uitilities */
inline int curr_time_micro(void)
{
  struct timeval tp[1];
  struct timezone tzp[1];
  gettimeofday(tp, tzp);
  return tp->tv_sec * 1000000 + tp->tv_usec;
}

inline void * xmalloc(size_t size)
{
  void *p = malloc(size);
  if (p == NULL) {
    perror("malloc failed");
    abort();
  }
  return p;
}

/* tree routines */
inline int bin_search(mol_t *mols, int key, int imin, int imax, int low)
{
  int imid;
  
  while (imax >= imin) {
    imid = (imin + imax) / 2;
    if (mols[imid].mid < key)
      imin = imid + 1;
    else if (mols[imid].mid > key)
      imax = imid - 1;
    else
      return imid;
  }
  return low ? imin : imax;
}

void select_subarr(mol_t *mols, node_t *p, idx_t *idx_l, idx_t *idx_h)
{
  idx_t _low, _high;
  if (mols[*idx_h].mid < p->morton_l || mols[*idx_h].mid > p->morton_h) {
    *idx_l = *idx_h = -1;
    return;
  }
  _low = bin_search(mols, p->morton_l, *idx_l, *idx_h, 1);
  _high = bin_search(mols, p->morton_h, *idx_l, *idx_h, 0);
  if (_low == -1 || _high == -1 || _low > _high) _low = _high = -1;
  *idx_l = _low;
  *idx_h = _high;
}

void tree_build_serial(node_t * node, mol_t * mols)
{
  idx_t size = 0;
  double stride;
  idx_t low, high;
  int i;

  if (node->molarr_l >= 0 && node->molarr_h >= 0)
    size = node->molarr_h - node->molarr_l + 1;

  if (size == 1) {
    node->v_mol = mols[node->molarr_l];
  } else if (size > 1) {
    for (i = 0; i < N_CHILD; i++)
      node->child[i] = xmalloc(sizeof(node_t));
    stride = (node->morton_h - node->morton_l + 1) / N_CHILD;
    node->child[0]->morton_l = node->morton_l;
    node->child[0]->morton_h = node->morton_l + stride;
    for (i = 1; i < N_CHILD; i++) {
      node->child[i]->morton_l = node->morton_l + stride * i + 1;
      node->child[i]->morton_h = node->morton_l + stride * (i + 1);
    }
    low = node->molarr_l;
    high = node->molarr_h;
    for (i = 0; i < N_CHILD; i++) {
      select_subarr(mols, node, &low, &high);
      node->child[i]->molarr_l = low;
      node->child[i]->molarr_h = high;
      if (low == -1 || high == -1) low = node->molarr_l;
      else low = high;
      high = node->molarr_h;
    }
  }
}

void tree_build_parallel(node_t *node)
{
}

void tree_build(sim_t * sim)
{
  node_t * root;

  sim->t_treebuild = curr_time_micro();
  root = (node_t *) xmalloc(sizeof(node_t));
  root->n_mol = sim->n_mol;
  root->molarr_l = 0;
  root->molarr_h = sim->n_mol - 1;
  root->morton_l = sim->mols[root->molarr_l].mid;
  root->morton_h = sim->mols[root->molarr_h].mid;
  sim->tree = root;

#if PARALLELIZE
  tree_build_parallel(sim->tree);
#else
  tree_build_serial(sim->tree, sim->mols);
#endif

  sim->t_treebuild = curr_time_micro() - sim->t_treebuild;
}

void tree_free_serial(node_t * node)
{
  int i;

  if (node == NULL) return;

  for (i=0; i < N_CHILD; i++)
    tree_free_serial(node->child[i]);

  free(node);
}

void * tree_free_parallel(void * args)
{
  pthread_t ths[N_CHILD];
  node_t *p = (node_t *) args;
  int i;
  void * ret;

  if (p == NULL) return;

  for (i = 0; i < N_CHILD; i++) {
    if (i < N_CHILD - 1)
      pthread_create(&ths[i], NULL, tree_free_parallel, p->child[i]);
    else
      tree_free_parallel(p->child[i]);
  }

  for (i = 0; i < N_CHILD - 1; i++)
    pthread_join(ths[i], (void *) ret);

  free(p);
}

void tree_free(sim_t * sim)
{

#if PARALLELIZE
  tree_free_parallel(sim->tree);
#else
  tree_free_serial(sim->tree);
#endif
  
}

/* simulation routines */
void init(sim_t *sim)
{
  idx_t i;
  curr_time_micro();  /* dummy call to make first timing accurate, Darwin only? */
  sim->n_mol = 64; 
  sim->steps = 0;
  sim->t_treebuild = 0;
  sim->mols = xmalloc(sizeof(mol_t) * sim->n_mol);
  for (i = 0; i < sim->n_mol; i++) {
    sim->mols[i].id = i;
    sim->mols[i].mid = i;
  }
}

void finalize(sim_t *sim)
{
  free(sim->mols);
}

void status(sim_t *sim)
{
  fprintf(stdout, "  Tree build: %ld\n", sim->t_treebuild);
}

int main(int argc, char *argv[])
{
  sim_t sim;
  
  init(&sim);

  tree_build(&sim);
  status(&sim);

  finalize(&sim);

  return 0;
}
