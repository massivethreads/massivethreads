/* treecode.cc */

#include <iostream>
#include "treecode.h"

int main(void)
{
  vector v1, v2, v3;
  v1.set(1, 1, 1);
  v2.set(2, 2, 2);
  v3 = v2;
  v3 += v1;
  std::cout << v3.x << "\n";
}
