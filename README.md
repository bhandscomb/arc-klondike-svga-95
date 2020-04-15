# arc-klondike-svga-95
Archive, DOS SVGA Klondike game, for historical interest

Lots of little mini projects making up the whole, a DOS based (but Windows 95 aware) Klondike solitaire game written to use "Reko" cardsets as seen on the Commodore Amiga computer for a game called Klondike Deluxe AGA.

Various compilers used originally but over time everything was "ported" to DJGPP, a DOS based version of the GNU Compiler Collection, hence the ".gnu" folder suffixes.

I thought to preserve this as a demonstration how software could use 16-bit "HiColor" SVGA from DOS complete with mouse support. The classic DOS mouse drivers (MOUSE.COM) only handled text and basic graphic modes but did not support HiColor so the game code effectively includes a "sprite" routine for the mouse pointer. It also is interesting in the use of DOS calls in a "32-bit" extended DOS environment.

There are a range of "Amiga-isms" in the code, particularly relating to typedefs to mimic those used on the Amiga platform.

The code presented here is in an "unrecalled" state of development, as lots of experiments were being done. In effect the last "snapshot". The game did actually work but the experimental sound support was probably very flaky.

* bmp2kln - converted a BMP file to a custom (slightly) compressed file. The game used 16-bit "HiColor" modes and this format made for easy display of title/win screens.

* cardinfo - just a (development) tool to "describe" a Reko cardset file.

* cmpr - compression tool, based on "SIXPACK" code submitted to Dr Dobbs Journal competition in 1991. Used to compress files ready for distribution.

* game - the actual game.

* install - custom installer.

* mq - tool that was used to collate compressed file (see cmpr) into a file "queue" that was used in the installer (see install)

* setsound - game configuration tool. At this stage of development some testing was being done to add background and spot music to the game. Many games of the time had a "setsound" or equivalent to configure sound for their system.

* showkln - simple (development) tool used to show a "kln" format graphic file (see bmp2kln) outside of the game.

* spltcrds - simple (development) tool to take a Reko format cardset file and split it into usable (on PC) graphic files. This was the first thing created to develop the Reko reader code.

* sixpack.c - original version of compression code (see cmpr) before adapted for use in this project.
