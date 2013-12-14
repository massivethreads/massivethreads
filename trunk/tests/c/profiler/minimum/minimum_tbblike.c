/* 
 * minimum function test for dag_recorder
 */

#if STANDALONE_TEST
int dr_get_worker() { return 0; }
int dr_get_cpu() { return 0; }
int dr_get_num_workers() { return 1; }
#else  /* with MassiveThreads */
#include <myth.h>
int dr_get_worker() { return myth_get_worker_num(); }
int dr_get_cpu() { return sched_getcpu(); }
int dr_get_num_workers() { return myth_get_num_workers(); }
#endif

#include <dag_recorder.h>

static int bin(int n) {
  if (n == 0) {
    return 1;
  } else {
    /* sandwich task_group */
    dr_dag_node * t = dr_enter_task_group();
    dr_return_from_task_group(t);
    /* sandwich create_task */
    dr_dag_node * c;
    t = dr_enter_create_task(&c);
    int x;
    { 
      /* inside task. sandwich entire task */
      dr_start_task(c);
      x = bin(n - 1);
      dr_end_task();
    }
    dr_return_from_create_task(t);
    int y = bin(n - 1);
    /* sandwich wait_tasks */
    t = dr_enter_wait_tasks();
    dr_return_from_wait_tasks(t);
    return x + y;
  }
}


int main() {
  int n = 10;
  dr_start(0);
  long x = bin(n);
  dr_stop();
  printf("bin(%d) = %ld\n", n, x);
  return 0;
}
