#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#ifndef PARALLELIZE
#define PARALLELIZE 1
#endif

#ifndef USE_MEMPOOL
#define USE_MEMPOOL 1
#endif

#define N_CHILD  8

int G_MAX_DEPTH = 7;
int G_ITERATION = 50;
int G_TRACEITER = 40;

typedef struct node {
  int depth;
  struct node* child[N_CHILD];
  int ith, memp_idx;
} node_t;
node_t **G_MEMPOOL = NULL;

inline int curr_time_micro(void)
{
  struct timeval tp[1];
  struct timezone tzp[1];
  gettimeofday (tp, tzp);
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

inline node_t * memp_get(int idx)
{
  node_t *ptr = G_MEMPOOL[idx];
  if (ptr) {
    G_MEMPOOL[idx] = NULL;
  } else {
    ptr = xmalloc(sizeof(node_t));
    memset(ptr, 0x0, sizeof(node_t));
    ptr->memp_idx = idx;
  }
  return ptr;
}

inline void memp_put(node_t * ptr)
{
  int idx = ptr->memp_idx;
  if (G_MEMPOOL[idx]) {
    printf("warning: memory pool put failed for %d\n", idx);
  } else {
    G_MEMPOOL[idx] = ptr;
  }
}

inline node_t * new_node(int depth) {
  node_t *n = (node_t *) xmalloc(sizeof(struct node));
  memset(n, 0x0, sizeof(node_t));
  n->depth = depth;
  return n;
}

void tree_build_serial(node_t * node)
{
  int i;

  if (node->depth >= G_MAX_DEPTH) return;

  for (i = 0; i < N_CHILD; i++) {
    node->child[i]  = new_node(node->depth + 1);
    tree_build_serial(node->child[i]);
  }
}


void * tree_build_parallel(void * args)
{
  pthread_t ths[N_CHILD];
  node_t *p = (node_t *) args;
  int i, child_depth, base, offset, ith;

  if (p->depth >= G_MAX_DEPTH) return (void *) 0;
  
  child_depth = p->depth + 1;
#if USE_MEMPOOL
  base = (pow(N_CHILD, child_depth) - 1) / (N_CHILD - 1);  /* base of level */
  offset = p->ith * N_CHILD;
#endif

  for (i = 0; i < N_CHILD; i++) {
#if USE_MEMPOOL
    ith = offset + i;
    p->child[i] = memp_get(base + ith);
    p->child[i]->ith = ith;
    p->child[i]->depth = child_depth;
#else
    p->child[i] = new_node(child_depth); 
#endif
    if (i < N_CHILD - 1)
      pthread_create(&ths[i], NULL, tree_build_parallel, p->child[i]);
    else
      tree_build_parallel(p->child[i]);
  }

  for (i = 0; i < N_CHILD - 1; i++)
    pthread_join(ths[i], NULL);
}

void tree_build(node_t * tree)
{
#if PARALLELIZE
  tree_build_parallel(tree);
#else
  tree_build_serial(tree);
#endif /* PARALLELIZE */
}

void tree_traversal(node_t *node, int *n)
{
  int i;
  
  if (node == NULL) return;

  (*n)++;
  for (i = 0; i < N_CHILD; i++)
    tree_traversal(node->child[i], n);
}

void * tree_free_parallel(void * args)
{
  pthread_t ths[N_CHILD];
  struct node * p = (struct node *) args;
  int i;

  if (p == NULL) return (void *) 0;

  for (i = 0; i < N_CHILD; i++) {
    if (i < N_CHILD - 1)
      pthread_create(&ths[i], NULL, tree_free_parallel, p->child[i]);
    else
      tree_free_parallel(p->child[i]);
  }
  for (i = 0; i < N_CHILD - 1; i++)
    pthread_join(ths[i], NULL);
  
#if USE_MEMPOOL
  memp_put(p);
#else
  free(p);
#endif
}

void tree_free_serial(node_t *node)
{
  int i;

  if (node == NULL) return;

  for (i = 0; i < N_CHILD; i++)
    tree_free_serial(node->child[i]);
  free(node);
}

void tree_free(node_t *tree)
{
#if PARALLELIZE
  tree_free_parallel(tree);
#else
  tree_free_serial(tree);
#endif
}

int main(int argc, char *argv[])
{
  struct node *root;
  int total, n;
  int i, t0, t1, t2, t3;

  if (argc < 2)
    printf("Arguments not sufficient, use default value\n");
  else {
    G_MAX_DEPTH = atoi(argv[1]);
    if (argc >= 3)
      G_ITERATION = atoi(argv[2]);
    if (argc >= 4)
      G_TRACEITER = atoi(argv[3]);
    else
      G_TRACEITER = 0.8 * G_ITERATION;
  }

  total = (pow(N_CHILD, G_MAX_DEPTH + 1) - 1) / (N_CHILD - 1);
#if USE_MEMPOOL
  G_MEMPOOL = xmalloc(sizeof(node_t *) * total);
  memset(G_MEMPOOL, 0x0, sizeof(node_t *) * total);
#endif

  printf("Depth: %d, Iter: %d (mtrace the %dth), Nodes: %d\n", 
    G_MAX_DEPTH, G_ITERATION, G_TRACEITER, total);

  for (i = 0; i < G_ITERATION; i++) {
#if USE_MEMPOOL
    root = memp_get(0);
    root->ith = 0;
    root->memp_idx = 0;
#else
    root = new_node(0);
#endif
    root->depth = 0;
    
    t0 = curr_time_micro();
    if (i == G_TRACEITER) mtrace();
    tree_build(root);
    if (i == G_TRACEITER) muntrace();
    t1 = curr_time_micro();

    n = 0;
    tree_traversal(root, &n);
    if (n != total)
      printf("Tree check failed! (%d vs. %d)\n", total, n);
    
    t2 = curr_time_micro();
    tree_free(root);
    t3 = curr_time_micro();
    printf("[%2d] Build: %d, Traversal: %d, Free: %d\n", i, t1-t0, t2-t1, t3-t2);
  }

#if USE_MEMPOOL
  for (i = 0; i < total; i++)
    free(G_MEMPOOL[i]);;
  free(G_MEMPOOL);
#endif
  
  return 0;
}
