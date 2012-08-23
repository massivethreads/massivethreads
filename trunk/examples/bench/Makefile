CC=gcc

# add C flags and C++ flags
CC_CXX_FLAGS = -O3 
CFLAGS = $(CC_CXX_FLAGS)
CXXFLAGS = $(CC_CXX_FLAGS)

.PHONY: debug clean

all: mm tree hybrid_tree tmfunc

mm: mm.c
	$(CC) -o $@ $< $(CC_CXX_FLAGS) -L../../.libs -lmyth-compat

tree: tree.c
	$(CC) -o $@ $< $(CC_CXX_FLAGS) -L../../.libs -lmyth-compat -lm

hybrid_tree: hybrid_tree.c
	$(CC) -o $@ $< $(CC_CXX_FLAGS) -L../../.libs -lmyth-compat -lm

tmfunc: tmfunc.c
	$(CC) -o $@ $< $(CC_CXX_FLAGS) -L../../.libs -lmyth-compat -lm

debug: mm_debug tree_debug hybrid_tree_debug

mm_debug: mm.c
	$(CC) -o $@ $< -pthread -ggdb $(CC_CXX_FLAGS)

tree_debug: tree.c
	$(CC) -o $@ $< -pthread -ggdb -lm $(CC_CXX_FLAGS)

hybrid_tree_debug: hybrid_tree.c
	$(CC) -o $@ $< -pthread -ggdb -lm $(CC_CXX_FLAGS)

clean: 
	rm -f mm tree hybrid_tree tmfunc
