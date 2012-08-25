CC=gcc
CFLAGS=-O3
TARGETS=uctx sjmp

all: $(TARGETS)

uctx: uctx.c
	$(CC) -o $@ $? $(CFLAGS)

sjmp: sjmp.c
	$(CC) -o $@ $? $(CFLAGS)

.phony:
clean:
	rm -rf $(TARGETS)
