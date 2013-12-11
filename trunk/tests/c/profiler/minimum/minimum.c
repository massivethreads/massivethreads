/* 
 * minimum function test for dag_recorder
 */

#include <dag_recorder.h>

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
  dr_options opts[1];
  dr_options_default(opts);
  opts->collapse = 0;
  dr_start(opts);
  bin(5);
  dr_stop();
  //dr_print_task_graph(0);	/* stdout */
  //dr_gen_dot_task_graph(0);
  return 0;
}
