#############################################################################
#
# SHOWKLN MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=showkln.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: showkln.exe

showkln.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

showkln.o: showkln.c
