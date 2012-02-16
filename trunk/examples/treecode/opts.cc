#include <cstdio>
#include <iostream>
#include "defs.h"
#include "optionparser.h"
#include "opts.h"

#define _STR(i) #i
#define STR(i) _STR(i)

using namespace option;

enum optionIndex {
  UNKNOWN,
  HELP,
  VERSION,
  MOL,
  STEPS,
};

const Descriptor usage[] =
{
  {UNKNOWN, 0, "", "", Arg::None,
    "Usage: "PROG_NAME" [options]\n"},
  {MOL, 0, "n", "nmol", Arg::None,
    "  -n, --nmol   number of particles (default: "STR(DEFAULT_MOL)")."},
  {STEPS, 0, "s", "steps", Arg::None,
    "  -s, --steps  steps of simulation (default: "STR(DEFAULT_STEPS)")."},
  {HELP, 0, "h", "help", Arg::None, 
    "  -h, --help   display this help and exit." },
  {VERSION, 0, "", "version", Arg::None, 
    "  --version    display version information and exit." },
  {0,0,0,0,0,0} /* boundary guard */
};

arguments::arguments(int _mol, int _steps)
{
  mol = _mol;
  steps = _steps;
}

arguments * parse_cmdline(int argc, char *argv[])
{
  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  Stats stats(usage, argc, argv);
  Option options[stats.options_max], buffer[stats.buffer_max];
  Parser parse(usage, argc, argv, options, buffer);

  if (parse.error()) {
    std::cout << PROG_NAME << ": failed to parse command line\n";
    return NULL;
  }

  if (options[HELP] || options[UNKNOWN]) {
    if (options[UNKNOWN])
      std::cout << PROG_NAME << ": unknown options '" << options[UNKNOWN].name << "'\n";
    printUsage(std::cout, usage);
    return NULL;
  }

  arguments * args = new arguments();
  
  if (argc == 0) return args; // return with default arguments

  return args;
}
