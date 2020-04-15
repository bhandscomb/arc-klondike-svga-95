#############################################################################
#
# CARDINFO MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=cardinfo.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: cardinfo.exe

cardinfo.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

cardinfo.o: cardinfo.c
