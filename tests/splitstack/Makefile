# Specify the directory of MassiveThraeds with split stack
MTH_S_PREFIX=
# Specify normal MassiveThreads directory
MTH_N_PREFIX=

CFLAGS = -g -O3 -Wall
LIBS = -lmyth-native

S_CFLAGS = -fsplit-stack $(CFLAGS)
N_CFLAGS = $(CFLAGS)

N_CPFLAGS = -L$(MTH_N_PREFIX)/lib -I$(MTH_N_PREFIX)/include -Wl,-rpath=$(MTH_N_PREFIX)/lib 

S_CPFLAGS = -L$(MTH_S_PREFIX)/lib -I$(MTH_S_PREFIX)/include -Wl,-rpath=$(MTH_S_PREFIX)/lib

#all: sstest-nn sstest-ns sstest-sn sstest-ss
all: sstest-nn sstest-ss

sstest-nn: sstest-n.c
	gcc -o $@ $(N_CFLAGS) $(N_CPFLAGS) $^ $(LIBS)

sstest-ns: sstest-n.c
	gcc -o $@ $(N_CFLAGS) $(S_CPFLAGS) $^ $(LIBS)

sstest-sn: sstest-n.c
	gcc -o $@ $(S_CFLAGS) $(N_CPFLAGS) $^ $(LIBS)

sstest-ss: sstest-n.c
	gcc -o $@ $(S_CFLAGS) $(S_CPFLAGS) $^ $(LIBS)
