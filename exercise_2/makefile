# @file makefile
# @author Florian Fuerst (12122096)
# @brief The makefile for the exercise intmul which compiles and links files
# @date 2022-12-11
 
 CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

OBJECTS = intmul

.PHONY : all clean

all: $(OBJECTS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
    
clean:
	rm -rf $(OBJECTS) *.o

intmul.o: intmul.c