mtbb_cc    := $(wildcard *_mtbb.cc)
mtbb_dr_cc := $(wildcard *_mtbb_dr.cc)
omp_cc     := $(wildcard *_omp.cc)
omp_dr_cc  := $(wildcard *_omp_dr.cc)

mtbb_exe    := $(patsubst %.cc,%,$(mtbb_cc))
mtbb_dr_exe := $(patsubst %.cc,%,$(mtbb_dr_cc))
omp_exe     := $(patsubst %.cc,%,$(omp_cc))
omp_dr_exe  := $(patsubst %.cc,%,$(omp_dr_cc))

prefix:=$(HOME)/parallel2/sys/inst/g

all : $(mtbb_exe) $(mtbb_dr_exe) $(omp_exe) $(omp_dr_exe)

CXXFLAGS := -std=c++0x -I$(prefix)/include -O0 -g
LDFLAGS := -L$(prefix)/lib -Wl,-R$(prefix)/lib 

$(mtbb_exe) : % : %.cc
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^ -lmyth-native

$(mtbb_dr_exe) : % : %.cc
#	$(CXX) -o $@ $(CXXFLAGS) -DTO_SERIAL -DDAG_RECORDER=2 $(LDFLAGS) $^ -ldr -lpthread
	$(CXX) -o $@ $(CXXFLAGS) -DDAG_RECORDER=2 $(LDFLAGS) $^ -lmyth-native -ldr -lpthread

$(omp_exe) : % : %.cc
	$(CXX) -o $@ -fopenmp $(CXXFLAGS) $(LDFLAGS) $^

$(omp_dr_exe) : % : %.cc
	$(CXX) -o $@ -fopenmp $(CXXFLAGS) -DDAG_RECORDER=2 $(LDFLAGS) $^ -ldr
