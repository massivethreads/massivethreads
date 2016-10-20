namespace mtbb {
  typedef size_t stack_size_type;
     
  class task_scheduler_init {
  public:
    static const int automatic = -1;
    static const int deferred = -2;
    task_scheduler_init(int max_threads=automatic, 
			stack_size_type thread_stack_size=0) {
      if (max_threads != automatic && max_threads != deferred) {
	myth_globalattr_set_n_workers(0, max_threads);
      }
      if (thread_stack_size != 0) {
	myth_globalattr_set_stacksize(0, thread_stack_size);
      }
      if (max_threads != deferred) {
	myth_init();
      }
    }
    ~task_scheduler_init() {
      /* do nothing in this implementation */
    }
    void initialize( int max_threads=automatic ) {
      if (max_threads != automatic && max_threads != deferred) {
	myth_globalattr_set_n_workers(0, max_threads);
      }
    }
    void terminate() {
      /* do nothing in this implementation */
    }
    static int default_num_threads() {
      size_t n = 0;
      myth_globalattr_get_n_workers(0, &n);
      return n;
    }
    bool is_active() const {
      return 1;
    }
  };
}
