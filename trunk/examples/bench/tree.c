#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#ifndef USE_THREADS
#define USE_THREADS 1
#endif

#define N_CHILDREN  8
#define N_DEPTH     8
#define N_ITERATION 8

struct node;
struct node {
  int depth;
  int free;
  struct node* children[N_CHILDREN];
};

inline int curr_time_micro(void)
{
  struct timeval tp[1];
  struct timezone tzp[1];
  gettimeofday (tp, tzp);
  return tp->tv_sec * 1000000 + tp->tv_usec;
}

inline struct node * new_node(int depth) {
  int i;
  struct node *n = (struct node *) malloc(sizeof(struct node));
  if (!n) {
    perror("malloc failed");
    exit(1);
  }
  memset(n, 0x0, sizeof(struct node));
  n->depth = depth;
  return n;
}

void tree_build(struct node * node, int depth)
{
  int i;

  if (node->depth >= depth)
    return;

  for (i = 0; i < N_CHILDREN; i++) {
    node->children[i]  = new_node(node->depth + 1);
    tree_build(node->children[i], depth);
  }
}

#if USE_THREADS
struct thd_dat {
  struct node * node;
  int depth;
};

void * tree_build_thd(void * args)
{
  pthread_t ths[N_CHILDREN];
  struct thd_dat dat[N_CHILDREN];
  struct thd_dat *p = (struct thd_dat *) args;
  int i;
  void * ret;

  if (p->node->depth >= p->depth)
    return (void *) 0;

  for (i = 0; i < N_CHILDREN; i++) {
    p->node->children[i] = new_node(p->node->depth + 1); 
    dat[i].node = p->node->children[i];
    dat[i].depth = p->depth;
    if (i < N_CHILDREN - 1)
      pthread_create(&ths[i], NULL, tree_build_thd, &dat[i]);
    else
      tree_build_thd(&dat[i]);
  }

  for (i = 0; i < N_CHILDREN - 1; i++)
    pthread_join(ths[i], (void **) ret);
}
#endif /* USE_THREADS */

void tree_traversal(struct node *node, int *n, int *n_free)
{
  int i;
  
  if (node == NULL)
    return;

  *n++;
  for (i = 0; i < N_CHILDREN; i++)
    tree_traversal(node->children[i], n, n_free);
}

#if USE_THREADS
void * tree_free_thd(void * args)
{
  pthread_t ths[N_CHILDREN];
  struct node * p = (struct node *) args;
  int i;
  void * ret;

  if (p == NULL)
    return (void *) 0;

  for (i = 0; i < N_CHILDREN; i++) {
    if (i < N_CHILDREN - 1)
      pthread_create(&ths[i], NULL, tree_free_thd, p->children[i]);
    else
      tree_free_thd(p->children[i]);
  }
  for (i = 0; i < N_CHILDREN - 1; i++)
    pthread_join(ths[i], (void **) ret);
  
  free(p);
}
#endif

void tree_free(struct node *node)
{
  int i;

  if (node == NULL)
    return;

  for (i=0; i < N_CHILDREN; i++)
    tree_free(node->children[i]);
  free(node);
}

int main(int argc, char *argv[])
{
  struct node *root;
  int depth, total, iter, n, n_free;
  int i, t0, t1, t2, t3;

  if (argc < 3) {
    printf("Arguments not sufficient, use default value\n");
    depth = N_DEPTH;
    iter = N_ITERATION;
  } else {
    depth = atoi(argv[1]);
    iter = atoi(argv[2]);
  }
  total = (1 - pow(N_CHILDREN, depth + 1)) / (1 - N_CHILDREN);
  printf("Iter: %d, Depth: %d, Nodes: %d\n", iter, depth, total);
 
  for (i = 0; i < iter; i++) {
    root = new_node(0);
    root->depth = 0;
    
    t0 = curr_time_micro();
#if USE_THREADS
    struct thd_dat dat;
    dat.node = root;
    dat.depth = depth;
    tree_build_thd(&dat);
#else
    tree_build(root, depth);
#endif
    t1 = curr_time_micro();
    tree_traversal(root, &n, &n_free);
    t2 = curr_time_micro();
#if USE_THREADS
    tree_free_thd(root);
#else
    tree_free(root);
#endif
    t3 = curr_time_micro();
    printf("[%2d] Build: %d, Traversal: %d, Free: %d\n", i, t1-t0, t2-t1, t3-t2);
  }
  
  return 0;
}
