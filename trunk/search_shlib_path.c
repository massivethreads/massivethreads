#include <pthread.h>

void * f(void *x) { return x; }
int main()
{
  pthread_t t;
  pthread_create(&t,NULL,f,NULL);
	return 0;
}
