OBJS=watcher.o config.o
SRC=watcher.c config.h
CC=gcc
CFLAGS=-Wall -Werror -pedantic -pedantic-errors -std=c99 -g
LDFLAGS=

.SUFFIXES: .c .o .h

all: tags watcher

watcher.o: config.h

watcher: $(OBJS)

clean:
	rm -rf watcher
	rm -f *.o
	rm -f tags

tags: $(SRC)
	ctags $^

.PHONY: all clean
