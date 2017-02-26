
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <myth/myth.h>


int main(int argc, char ** argv) {
  myth_key_t key;
  myth_key_create(&key, 0);
  myth_setspecific(key, 0);
  return 0;
}

