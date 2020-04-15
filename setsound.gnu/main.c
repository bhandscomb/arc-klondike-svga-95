/*
#############################################################################
#
# main.c - setsound
#
#############################################################################
*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pc.h>
#include <keys.h>

#include "mytypes.h"
#include "protos.h"


char **__crt0_glob_function(char *_argument) {
  return(0);
  };
void __crt0_load_environment_file(char *appname) {
  };
void __crt0_setup_arguments(void) {
  };


int confirm(void) {
  int key='\0';
  fflush(stdout);
  while((key!='Y')&&(key!='N'))
    key=toupper(getxkey());
  printf("%c\n",key);
  return(key);
  }

int getnumber(void) {
  int key='\0';
  char numberstr[80],*p;
  p=numberstr;
  fflush(stdout);
  while((key!=K_Return)||(p==numberstr)) {
    key=getxkey();
    if(isdigit(key)) {
      *p++=key;
      putchar(key);
      fflush(stdout);
      }
    }
  *p='\0';
  putchar('\n');
  return(atoi(numberstr));
  }


int gettrack(char *where) {
  int track=-1,key;
  while(track<0) {
    while(track<0) {
      printf("\nEnter track to play at %s screen:  ",where);
      track=getnumber();
      }
    if(track)
      printf("\nPlay CD track %d at %s screen. Okay? (Y/N)  ",track,where);
    else
      printf("\nDon't play CD at %s screen. Okay? (Y/N)  ",where);
    key=confirm();
    if(key=='N')
      track=-1;
    }
  return(track);
  }


void SetAmiga() {
  int key;
  char cfgstr[80];

  ConfigPut("MUSIC","MOD");

  ConfigGet("MIXRATE",cfgstr);
  if(cfgstr[0]!='\0') {
    printf("\nUse previous MOD music settings? (Y/N)  ");
    key=confirm();
    if(key=='Y')
      return;
    }

  printf (
    "\nPlease select one of the following mix rates:\n\n"
    "   1   11000 Hz (e.g. 386)\n"
    "   2   22050 Hz (e.g. 486)\n"
    "   4   44100 Hz (e.g. 486DX2 or above)\n\n"
    ">>---> "
    );
  fflush(stdout);
  key='\0';
  while((key!='1')&&(key!='2')&&(key!='4'))
    key=getxkey();
  printf("%c\n",key);

  switch(key) {
    case '1': ConfigPut("MIXRATE","11000"); break;
    case '2': ConfigPut("MIXRATE","22050"); break;
    case '4': ConfigPut("MIXRATE","44100"); break;
    }

  }


void SetCD() {
  int key,title,game,win;
  char cfgstr[80];

  ConfigPut("MUSIC","CD");

  ConfigGet("CDTITLE",cfgstr);
  if(cfgstr[0]!='\0') {
    printf("\nUse previous CD music settings? (Y/N)  ");
    key=confirm();
    if(key=='Y')
      return;
    }

  puts("\nSelect CD tracks to play. Entering 0 means \"don't play\"");

  title=gettrack("title");
  game=gettrack("game");
  win=gettrack("win");

  sprintf(cfgstr,"%d",title);
  ConfigPut("CDTITLE",cfgstr);
  sprintf(cfgstr,"%d",game);
  ConfigPut("CDGAME",cfgstr);
  sprintf(cfgstr,"%d",win);
  ConfigPut("CDWIN",cfgstr);

  }


void SetMIDI() {
  int midiport=0,key;
  char *blaster,cfgstr[80];

  ConfigPut("MUSIC","MIDI");

  ConfigGet("MIDIPORT",cfgstr);
  if(cfgstr[0]!='\0') {
    printf("\nUse previous MIDI music settings? (Y/N)  ");
    key=confirm();
    if(key=='Y')
      return;
    }

  blaster=getenv("BLASTER");
  if(blaster) {
    while(*blaster)
      if(toupper(*blaster++)=='P') {
        sscanf(blaster,"%x",&midiport);
        break;
        }
    }

  if(midiport) {
    printf (
      "\n\"SET BLASTER=\" says MIDI is at port %xh. Use this? (Y/N)  ",
      midiport
      );
    key=confirm();
    if(key=='N')
      midiport=0;
    }

  if(!midiport) {
    printf("\nUse default MIDI port 330h? (Y/N)  ");
    key=confirm();
    if(key=='Y')
      midiport=0x330;
    }

  while(!midiport) {
    printf("\nEnter MIDI port address:  ");
    fflush(stdout);
    scanf("%x",&midiport);
    printf("\nPort %xh. Okay? (Y/N)  ",midiport);
    key=confirm();
    if(key=='N')
      midiport=0;
    }

  sprintf(cfgstr,"%x",midiport);
  ConfigPut("MIDIPORT",cfgstr);

  }


void SetNoMusic() {
  puts("\nNo Music!");
  ConfigPut("MUSIC","OFF");
  }


void main(void) {
  int key;

  puts("\nSETSOUND v1.0 - Klondike 95 - (c) 1995 Aardvark Productions\n");

  ConfigRead();

  printf (
    "Please select one of the following:\n\n"
    "   A   Amiga Original (converted to MOD format)\n"
    "   C   CD Music (Not Yet Implemented)\n"
    "   M   MIDI Music (*NOT* AWE-32 WaveTable compatible)\n"
    "   N   No Music\n\n"
    ">>---> "
    );
  fflush(stdout);
  key='\0';
  while((key!='A')&&(key!='C')&&(key!='M')&&(key!='N'))
    key=toupper(getxkey());
  printf("%c\n",key);

  switch(key) {
    case 'A': SetAmiga(); break;
    case 'C': SetCD(); break;
    case 'M': SetMIDI(); break;
    case 'N': SetNoMusic(); break;
    }

  printf("\nUpdating config...");
  fflush(stdout);

  ConfigWrite();
  ConfigFlush();

  puts("Done. Bye.");

  }
