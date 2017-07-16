### set MASSIVE_INST_DIR so it points to where
### you installed MassiveThreads (i.e., what you gave
### to --prefix)
# MASSIVE_INST_DIR?=../../src/

CXXFLAGS:=-std=c++0x -g -pthread -ggdb -Wall -I../../src/ -I../../include
CXX:=g++
LIBS=-Wl,-R../../src/.libs -L../../src/.libs -lmyth -ldl

fib : fib.o
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

clean :
	rm -f fib fib.o

