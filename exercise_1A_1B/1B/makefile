# @file makefile
# @author Florian Fuerst (12122096)
# @brief The makefile for the task fb_arc_set which compiles and links files
# @date 2022-11-13

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
# LDFLAGS = -lrt -pthread

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