/*
 * treecode.c
 * A treecode implementation to achive best scalability on shared memory machines.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#ifndef	PARALLELIZE
#define PARALLELIZE	1
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
  int n_step;
  int steps;
  mol_t *mols;
  node_t *tree;
  long int t_init, t_treebuild, t_treefree;
} sim_t;
sim_t sim_instance;
sim_t *sim = &sim_instance;
  

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

inline void * xcmalloc(size_t size)
{
  void *p = malloc(size);
  if (p == NULL) {
    perror("malloc failed");
    abort();
  }
  memset(p, 0x0, size);
  return p;
}

/* tree routines */
inline int bin_search(mol_t *mols, mot_t key, 
  idx_t imin, idx_t imax, int low_bound)
{
  idx_t imid;
  
  while (imax >= imin) {
    imid = (imin + imax) / 2;
    if (mols[imid].mid < key)
      imin = imid + 1;
    else if (mols[imid].mid > key)
      imax = imid - 1;
    else
      return imid;
  }
  return low_bound ? imin : imax;
}

// Given ml and mh, find a subrange [il, ih] from original [il, ih]
int select_subarr(mol_t *mols, mot_t ml, mot_t mh, idx_t *il, idx_t *ih)
{
  idx_t _low, _high;

  if (mols[*ih].mid < ml || mols[*il].mid > mh) return 1;
  if ((_low = bin_search(mols, ml, *il, *ih, 1)) == -1) return 1;
  if ((_high = bin_search(mols, mh, *il, *ih, 0)) == -1) return 1;
  if (_low > _high) return 1;
  *il = _low;
  *ih = _high;
  return 0;
}

void tree_build_serial(node_t * node)
{
  idx_t curr_il, curr_ih;
  mot_t curr_ml, curr_mh;
  real_t stride;
  mol_t * mols;
  int i;
  
  mols = sim->mols;
  if (node->n_mol == 1) {
    node->v_mol = mols[node->molarr_l];
  } else if (node->n_mol > 1) {
    stride = (node->morton_h - node->morton_l) / N_CHILD;
    curr_il = node->molarr_l;
    curr_ih = node->molarr_h;
    for (i = 0; i < N_CHILD; i++) {
      curr_ml = node->morton_l + stride * i + i;
      curr_mh = curr_ml + stride;
      if (select_subarr(mols, curr_ml, curr_mh, &curr_il, &curr_ih) == 0) {
        /* subspaces is non-empty */
        node->child[i] = xcmalloc(sizeof(node_t));
        node->child[i]->n_mol = curr_ih - curr_il + 1; /* assert(n_mol>0) */
        node->child[i]->molarr_l = curr_il;
        node->child[i]->molarr_h = curr_ih;
        node->child[i]->morton_l = curr_ml;
        node->child[i]->morton_h = curr_mh;
        curr_il = curr_ih;
        
        tree_build_serial(node->child[i]);
      } else {
        curr_il = node->molarr_l;
      }
      curr_ih = node->molarr_h;
    }
  }
}

void * tree_build_parallel(void *args)
{
  pthread_t ths[N_CHILD];
  idx_t curr_il, curr_ih;
  mot_t curr_ml, curr_mh;
  real_t stride;
  mol_t * mols;
  node_t * node;
  int i, n_ths;
  void * ret;
  
  n_ths = 0;
  mols = sim->mols;
  node = (node_t *) args;
  if (node->n_mol == 1) {
    node->v_mol = mols[node->molarr_l];
  } else if (node->n_mol > 1) {
    stride = (node->morton_h - node->morton_l) / N_CHILD;
    curr_il = node->molarr_l;
    curr_ih = node->molarr_h;
    for (i = 0; i < N_CHILD; i++) {
      curr_ml = node->morton_l + stride * i + i;
      curr_mh = curr_ml + stride;
      if (select_subarr(mols, curr_ml, curr_mh, &curr_il, &curr_ih) == 0) {
        /* subspaces is non-empty */
        node->child[i] = xcmalloc(sizeof(node_t));
        node->child[i]->n_mol = curr_ih - curr_il + 1; /* assert(n_mol>0) */
        node->child[i]->molarr_l = curr_il;
        node->child[i]->molarr_h = curr_ih;
        node->child[i]->morton_l = curr_ml;
        node->child[i]->morton_h = curr_mh;
        curr_il = curr_ih;
        pthread_create(&ths[n_ths], NULL, tree_build_parallel, node->child[i]);
        n_ths++;
      } else {
        curr_il = node->molarr_l;
      }
      curr_ih = node->molarr_h;
    }
  }
  for (i = 0; i < n_ths; i++)
    pthread_join(ths[i], (void *) ret);
}

void tree_build(void)
{
  node_t * root;

  sim->t_treebuild = curr_time_micro();
  root = (node_t *) xcmalloc(sizeof(node_t));
  root->n_mol = sim->n_mol;
  root->molarr_l = 0;
  root->molarr_h = sim->n_mol - 1;
  root->morton_l = sim->mols[root->molarr_l].mid;
  root->morton_h = sim->mols[root->molarr_h].mid;
  sim->tree = root;

#if PARALLELIZE
  tree_build_parallel(sim->tree);
#else
  tree_build_serial(sim->tree);
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

void tree_free(void)
{

  sim->t_treefree = curr_time_micro();

#if PARALLELIZE
  tree_free_parallel(sim->tree);
#else
  tree_free_serial(sim->tree);
#endif
  
  sim->t_treefree = curr_time_micro() - sim->t_treefree;
}

void tree_check_walk(node_t *node, int *n, int depth) {
  int i;
  
  if (node == NULL) return;
  
  (*n)++;
  for (i = 0; i < N_CHILD; i++) {
    tree_check_walk(node->child[i], n, depth + 1);
  }
}

void tree_valid_check(void)
{
  int n_total = 0;
  tree_check_walk(sim->tree, &n_total, 0); 
  printf("Valid check: walked %d nodes\n", n_total);
}

/* simulation routines */
void init(int argc, char *argv[])
{
  idx_t i;
  curr_time_micro();  /* dummy call to make first timing accurate, Darwin only? */
  sim->n_mol = pow(N_CHILD, 7); 
  sim->n_step = atoi(argv[1]);
  sim->steps = 0;
  sim->t_treebuild = 0;
  sim->mols = xmalloc(sizeof(mol_t) * sim->n_mol);
  for (i = 0; i < sim->n_mol; i++) {
    sim->mols[i].id = i;
    sim->mols[i].mid = i;
  }
  printf("N=%ld, STEPS=%d\n", sim->n_mol, sim->n_step);
}

void finalize(void)
{
  free(sim->mols);
}

void status(void)
{
  fprintf(stdout, 
    "  Tree build: %ld\n"
    "  Tree free:  %ld\n", 
    sim->t_treebuild, sim->t_treefree);
}

int main(int argc, char *argv[])
{
  init(argc, argv);
  
  while (sim->steps <= sim->n_step) {
    tree_build();
    tree_valid_check();
    tree_free();
    status();
    sim->steps++;
  }

  finalize();

  return 0;
}
