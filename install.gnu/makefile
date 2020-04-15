#############################################################################
#
# INSTALL MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=config.o decode.o main.o win95.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: install.exe

install.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

config.o: config.c files.h

decode.o: decode.c

main.o: main.c mytypes.h keycodes.h colours.h protos.h

win95.o: win95.c mytypes.h
