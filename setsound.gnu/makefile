#############################################################################
#
# SETSOUND MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=config.o main.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: setsound.exe

setsound.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

config.o: config.c files.h

main.o: main.c mytypes.h protos.h
