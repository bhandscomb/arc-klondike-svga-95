#############################################################################
#
# KLONDIKE MAKEFILE
#
#############################################################################


CC=gcc
FLAGS=-s

#CC=gcc -pg
#FLAGS=


STDHDRS=mytypes.h protos.h vesa.h

OBJS= \
	chooser.o \
	config.o \
	errmsg.o \
	klongame.o \
	main.o \
	mouse.o \
	reko.o \
	text.o \
	vesa.o \
	win95.o \
	music.o \
	libsb.a


.c.o:
	$(CC) -c -O2 -Wall -m486 $< -o $@


all: klondike.exe

klondike.exe: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -lemu -o $@


chooser.o: chooser.c $(STDHDRS) game.h text.h errmsg.h files.h icons.h

config.o: config.c files.h

errmsg.o: errmsg.c mytypes.h vesa.h reko.h mouse.h errmsg.h

klongame.o: klongame.c $(STDHDRS) game.h text.h

main.o: main.c $(STDHDRS) game.h reko.h text.h errmsg.h music.h files.h version.h

mouse.o: mouse.c $(STDHDRS) game.h mouse.h

reko.o: reko.c $(STDHDRS) game.h reko.h files.h

text.o: text.c $(STDHDRS) text.h

vesa.o: vesa.c $(STDHDRS)

win95.o: win95.c

music.o: music.c mytypes.h protos.h music.h
