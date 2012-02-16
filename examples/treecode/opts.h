/* opts.h */

#ifndef OPTS_H
#define OPTS_H

// Default values
#define DEFAULT_MOL     100
#define DEFAULT_STEPS   100

struct arguments {
  int mol;
  int steps;

  arguments(int _mol = DEFAULT_MOL, int _steps = DEFAULT_STEPS);
};

arguments * parse_cmdline(int, char* []);

#endif /* OPTS_H */
