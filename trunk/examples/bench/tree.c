#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#ifndef PARALLELIZE
#define PARALLELIZE 1
#endif

#define N_CHILDREN  8

int G_MAX_DEPTH = 7;
int G_ITERATION = 50;
int G_TRACEITER = 40;

typedef struct node {
  int depth;
  struct node* children[N_CHILDREN];
} node_t;

inline int curr_time_micro(void)
{
  struct timeval tp[1];
  struct timezone tzp[1];
  gettimeofday (tp, tzp);
  return tp->tv_sec * 1000000 + tp->tv_usec;
}

inline node_t * new_node(int depth) {
  node_t *n = (node_t *) malloc(sizeof(struct node));
  if (!n) {
    perror("malloc failed");
    exit(1);
  }
  memset(n, 0x0, sizeof(node_t));
  n->depth = depth;
  return n;
}

void tree_build_serial(node_t * node)
{
  int i;

  if (node->depth >= G_MAX_DEPTH) return;

  for (i = 0; i < N_CHILDREN; i++) {
    node->children[i]  = new_node(node->depth + 1);
    tree_build_serial(node->children[i]);
  }
}

void * tree_build_parallel(void * args)
{
  pthread_t ths[N_CHILDREN];
  node_t *p = (node_t *) args;
  void * ret;
  int i;

  if (p->depth >= G_MAX_DEPTH) return (void *) 0;

  for (i = 0; i < N_CHILDREN; i++) {
    p->children[i] = new_node(p->depth + 1); 
    if (i < N_CHILDREN - 1)
      pthread_create(&ths[i], NULL, tree_build_parallel, p->children[i]);
    else
      tree_build_parallel(p->children[i]);
  }

  for (i = 0; i < N_CHILDREN - 1; i++)
    pthread_join(ths[i], (void **) ret);
}

void tree_build(node_t * tree)
{
#if PARALLELIZE
  tree_build_parallel(tree);
#else
  tree_build_serial(tree);
#endif /* PARALLELIZE */
}

void tree_traversal(struct node *node, int *n)
{
  int i;
  
  if (node == NULL)
    return;

  (*n)++;
  for (i = 0; i < N_CHILDREN; i++)
    tree_traversal(node->children[i], n);
}

void * tree_free_parallel(void * args)
{
  pthread_t ths[N_CHILDREN];
  struct node * p = (struct node *) args;
  int i;
  void * ret;

  if (p == NULL) return (void *) 0;

  for (i = 0; i < N_CHILDREN; i++) {
    if (i < N_CHILDREN - 1)
      pthread_create(&ths[i], NULL, tree_free_parallel, p->children[i]);
    else
      tree_free_parallel(p->children[i]);
  }
  for (i = 0; i < N_CHILDREN - 1; i++)
    pthread_join(ths[i], (void **) ret);
  
  free(p);
}

void tree_free_serial(node_t *node)
{
  int i;

  if (node == NULL) return;

  for (i = 0; i < N_CHILDREN; i++)
    tree_free_serial(node->children[i]);
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
  total = (1 - pow(N_CHILDREN, G_MAX_DEPTH + 1)) / (1 - N_CHILDREN);
  printf("Depth: %d, Iter: %d (mtrace the %dth), Nodes: %d\n", 
    G_MAX_DEPTH, G_ITERATION, G_TRACEITER, total);

  for (i = 0; i < G_ITERATION; i++) {
    root = new_node(0);
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
  
  return 0;
}
