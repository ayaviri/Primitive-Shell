CC=gcc
CFLAGS=-g -std=c11 -D_DEFAULT_SOURCE

SHELL_OBJS=$(patsubst %.c,%.o,$(filter-out tokenize.c,$(wildcard *.c)))

ifeq ($(shell uname), Darwin)
	LEAKTEST ?= leaks --atExit --
else
	LEAKTEST ?= valgrind --leak-check=full
endif

.PHONY: all valgrind clean

valgrind: shell
	$(LEAKTEST) ./shell

clean: 
	rm -rf *.o
	rm -f shell

shell: $(SHELL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

