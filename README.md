# arc-klondike-svga-95
Archive, DOS SVGA Klondike game, for historical interest

---

Lots of little mini projects making up the whole, a DOS based (but Windows 95 aware) Klondike solitaire game written to use "Reko" cardsets as seen on the Commodore Amiga computer for a game called Klondike Deluxe AGA.

Various compilers used originally but over time everything was "ported" to DJGPP, a DOS based version of the GNU Compiler Collection, hence the ".gnu" folder suffixes.

I thought to preserve this as a demonstration how software could use 16-bit "HiColor" SVGA from DOS complete with mouse support. The classic DOS mouse drivers (MOUSE.COM) only handled text and basic graphic modes but did not support HiColor so the game code effectively includes a "sprite" routine for the mouse pointer. It also is interesting in the use of DOS calls in a "32-bit" extended DOS environment.

There are a range of "Amiga-isms" in the code, particularly relating to typedefs to mimic those used on the Amiga platform.

The code presented here is in an "unrecalled" state of development, as lots of experiments were being done. In effect the last "snapshot". The game did actually work but the experimental sound support was probably very flaky.


### CODE FOLDERS (name.gnu)

* bmp2kln - converted a BMP file to a custom (slightly) compressed file. The game used 16-bit "HiColor" modes and this format made for easy display of title/win screens.

* cardinfo - just a (development) tool to "describe" a Reko cardset file.

* cmpr - compression tool, based on "SIXPACK" code submitted to Dr Dobbs Journal competition in 1991. Used to compress files ready for distribution.

* game - the actual game.

* install - custom installer.

* mq - tool that was used to collate compressed file (see cmpr) into a file "queue" that was used in the installer (see install)

* setsound - game configuration tool. At this stage of development some testing was being done to add background and spot music to the game. Many games of the time had a "setsound" or equivalent to configure sound for their system.

* showkln - simple (development) tool used to show a "kln" format graphic file (see bmp2kln) outside of the game.

* spltcrds - simple (development) tool to take a Reko format cardset file and split it into usable (on PC) graphic files. This was the first thing created to develop the Reko reader code.

### ALSO INCLUDED

* docs - folder of selected interesting development and "user" documents.

* sixpack.c - original version of compression code (see cmpr) before adapted for use in this project.

### 2020 BRIEF CODE REVIEW COMMENTS

Just a quick scroll through some of the code files lead to a few observations...

Code formatting style leaves a lot to be desired. On the other hand it is fairly consistent at least within each mini project. Consistently bad. Spaces are mostly used only within text and for indenting, almost never for readability.

Game code is almost certainly not buildable in current form. Have discovered the makefile refers to a missing binary blob of library code (libsb.a) so even if you got DJGPP up and running some code would have to be de-activated. No idea what it is now, but have seen reference to an object library of this name and similarly named header files as per the game code on GitHub for a Linux game.

Some changes made for DJGPP and in particular running in the DOS protected mode environment left some source comments incorrect. Similarly a define once used to control how a certain VESA related task was performed probably does nothing as the code was stripped and turned into a pre-processor macro.

Related note, there is the odd empty function dotted around, left over from the move from using Extended Memory (XMS) directly to having 32-bit code and DPMI services. P.S. 16-bit code using XMS was a pain requiring lots of little blobs of inline assembly code, as if making BIOS calls wasn't bad enough. We're so much better off now.

In general as it stands the game code is very much "beta" quality in places. Indeed in a version history document comment was made that there would be no "release" version 1.1 and that the code was due for an overhaul, which never happened.
