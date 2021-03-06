OBJS=watcher.o config.o util.o
SRC=watcher.c watcher.h config.c config.h util.c util.h
CC=gcc
CFLAGS=-Wall -Werror -pedantic -pedantic-errors -std=c99 -g
LDFLAGS=

.SUFFIXES: .c .o .h

all: tags watcher

util.o: util.c util.h

config.o: config.c config.h util.h

watcher.o: watcher.c watcher.h config.h util.h

watcher: $(OBJS)

clean:
	rm -rf watcher
	rm -f *.o
	rm -f tags

tags: $(SRC)
	ctags $^

.PHONY: all clean
