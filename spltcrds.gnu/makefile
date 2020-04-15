#############################################################################
#
# SPLTCRDS MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=spltcrds.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: spltcrds.exe

spltcrds.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

spltcrds.o: spltcrds.c
