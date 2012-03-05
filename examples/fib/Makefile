CC=gcc
APP=fib

# add C flags and C++ flags
CC_CXX_FLAGS = -O3 
CFLAGS = $(CC_CXX_FLAGS)
CXXFLAGS = $(CC_CXX_FLAGS)

SRCS = $(APP).c
.PHONY: debug clean

$(APP): $(SRCS)
	$(CC) -o $(APP) $(SRCS) $(CC_CXX_FLAGS) -L../../ -lmyth-compat

debug:
	$(CC) -o $(APP) $(SRCS) -pthread -ggdb $(CC_CXX_FLAGS)

clean: 
	rm -f $(APP) 
