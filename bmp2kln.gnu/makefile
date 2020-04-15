#############################################################################
#
# BMP2KLN MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

OBJS=bmp2kln.o

.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@

all: bmp2kln.exe

bmp2kln.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $@

bmp2kln.o: bmp2kln.c
