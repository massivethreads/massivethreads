APP = nbody

# add C flags and C++ flags
CC_CXX_FLAGS = -O3 
CFLAGS = $(CC_CXX_FLAGS)
CXXFLAGS = $(CC_CXX_FLAGS)

SRCS = $(APP).cc def.cc force.cc genpar.cc gentree.cc def.h
.PHONY: debug clean

$(APP): $(SRCS)
	$(CXX) -o $(APP) $(SRCS) $(CC_CXX_FLAGS) -L../../ -lmyth-compat

debug:
	$(CXX) -o $(APP) $(SRCS) -pthread -ggdb

clean: 
	rm -f $(APP) 
