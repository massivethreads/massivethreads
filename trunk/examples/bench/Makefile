CC=gcc

# add C flags and C++ flags
CC_CXX_FLAGS = -O3 
CFLAGS = $(CC_CXX_FLAGS)
CXXFLAGS = $(CC_CXX_FLAGS)

.PHONY: debug clean

all: mm

mm: mm.c
	$(CC) -o $@ $< $(CC_CXX_FLAGS) -L../../ -lmyth-compat

debug:
	$(CC) -o mm mm.c -pthread -ggdb $(CC_CXX_FLAGS)

clean: 
	rm -f mm
