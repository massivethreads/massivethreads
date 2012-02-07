APP = nbody

# add C flags and C++ flags
CC_CXX_FLAGS = -g -O3
CFLAGS = $(CC_CXX_FLAGS)
CXXFLAGS = $(CC_CXX_FLAGS)

# add basename of object files
BASES = $(APP) def force genpar gentree
OBJS = $(addsuffix .o, $(BASES))

$(APP): $(OBJS)
	$(CXX) -o $(APP) $(OBJS)

clean: 
	rm -f $(OBJS) 

all_clean: clean
	rm -f $(APP)
