#program name: fb_arc_set
#author: Florian Fuerst (12122096)

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -pthread

OBJECTS = main.o

.PHONY: all clean
all: generator supervisor

generator: generator.o 
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: supervisor.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o generator supervisor