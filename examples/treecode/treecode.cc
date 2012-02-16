/* treecode.cc */

#include <cstdlib>
#include <iostream>
#include "treecode.h"

int main(int argc, char *argv[])
{
  parse_cmdline(argc, argv);
  exit(0);
  vector v1, v2, v3;
  v1.set(1, 1, 1);
  v2.set(2, 2, 2);
  v3 = v2;
  v3 += v1;
  std::cout << v3.x << "\n";

  tree t;
  t.root = newcell();
  t.root->split();
  t.root->subs[0] = newbody();
  t.print();
}
