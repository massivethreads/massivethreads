CC=gcc
CFLAGS=-O3
LDFLAGS=@../../src/myth-ld.opts -Wl,-R../../src/.libs -L../../src/.libs -lmyth-ld

.PHONY: debug clean

all: fib fib2

fib: fib.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

fib2: fib2.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

debug:
	$(CC) -o fib fib.c -pthread -ggdb $(CFLAGS)
	$(CC) -o fib2 fib2.c -pthread -ggdb $(CFLAGS)

clean: 
	rm -f fib fib2
