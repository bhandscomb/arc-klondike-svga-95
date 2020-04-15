/*
#############################################################################
#
# main.c
#
#
# Main module for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# --- none ---
#
#############################################################################
*/


/* includes */

#include <dos.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "mytypes.h"
#include "game.h"
#include "reko.h"
#include "vesa.h"
#include "text.h"
#include "errmsg.h"
#include "music.h"
#include "files.h"
#include "version.h"
#include "protos.h"

#include "birthday.h"


/* strings for thank you message */

#define BETATEST     "beta-testing"
#define REALGAME     "playing"


/* define to 1 to enable DOSemu check, else 0 */
/* crashes plain DOS, dunno why :-( */
#define DOSEMUCHECK  0


/* thank you message */

#define THANKYOU \
  "Thank you for " RELEASETYPE " Klondike 95 v" VERNUMB " (" VERDATE ")"


/* sign off messages */

#define NUMSIGNOFFMSGS 16

char SIGNOFFMSGS[] =

  /*** ADs (3) ***/
  "eTlly uo rrfeidn sbauo thtsig ma!e\0"
  "xErt aacdressts ohlu deba avlibaelf or mhwre eoy uog thtsig ma.e\0"
  "hTsig ma esib sadeu op nlKnoidekD lexu eGA A.3 0of rht emAgi.a\0"

  /*** Mini fortune style entries (6) ***/
  "iWnnni gsi'n tveretyihgn.. .\0"
  " Aepnn yasev dsia p neyny uoh vanet's eptny te .\0"
  "hT erttu hsio tut eher.. .\0"
  ".. .oy'rera W molb!e  : )-\0"
  "tAl aetso en1 89 7owdrp orecssroc uodlr nuf or m aisgnel3 06 klfpoyp !\0"
  "OD S.302 ? 163k0f olpp,yu aslb.eD SO6 2.?23 1 4.M4f olppei soti snatll !\0"

  /*** Monty Python (3) ***/
  "A\"wlya solkoo  nht erbgiths di efol fi.e.. \"\0"
  "T\"ih saprrtoi  sedda !tIi  sonm ro.e.. \"\0"
  "nA don wof rosemhtni gocpmeletyld fiefertn.. .\0"

  /*** Pink Floyd (2) ***/
  "hTre'e sosemno enim  yehdab tui 't son tem.. .\0"
  "oCemo  noy uimen rof rrttu hna dedulisno ,na dhsni!e\0"

  /*** Misc Lyrics (2) ***/
  "I\"'t smioptrna thttaw  eekpet  ocsehudel ,htre eumtsb  eond leya\".\0"
  "D\"not's ohtos ohgtnu ,oD'n thsoo,tS ohto\"!\0"
  ;


/* birthday checking */
struct birthday {
  int day,mon,year;
  char *name,*desc;
  };

struct birthday birthdaylist[] = {
  {  7, 3,1977, "aSamtnah", "rBai'n sistsre , aDCt ihfe !" },
  {  5, 4,1971, "iDna e", "eh rnoh reb rihtad y" },
  { 21, 4,1971, "rBai n", "aAdravkr ,ht eoced rfoK oldnki e59 !" },
  { 24, 6,1973, "nAid", "ht eno ena dnoylP worelsva!e" },
  {  4,12,1979, "ePet r", "rBai'n srbtoeh,ra t tolan tuet!r" },
  { -1,-1,-1,"","" }
  };


/* dfp is debug file pointer, quitflag controls game flow */

//FILE *dfp=NULL;
WORD cards,quitflag;


/*
#############################################################################
#
# kill unwanted DJGPP startup features
#
#############################################################################
*/

char **__crt0_glob_function(char *_argument) {
  return(0);
  };
void __crt0_load_environment_file(char *appname) {
  };
void __crt0_setup_arguments(void) {
  };


/*
#############################################################################
#
# err()
#
# Subroutine called when 'bombing out' of program
#
#############################################################################
*/

void err(int where,WORD rc,char *msg) {
  //if(dfp) fclose(dfp);
  RekoDiscardDeck();
  printf("%d %s\n",rc,msg);
  WhatError(where,rc);
  ConfigFlush();
  exit(1);
  }


/*
#############################################################################
#
# DOSemuCheck()
#
# Having tried a recent(ish) version of DOSemu, I can testify that it
# doesn't work, probably the VESA BIOS extension fiddling around...
#    Linux:  1.3.20
#    DOSemu: 0.60.1
#
#############################################################################
*/

#if (DOSEMUCHECK==1)

void DOSemuCheck(void) {
  __dpmi_regs regs;
  /* interrupt info from "isemu.c" from DOSemu 0.60.1 */
  regs.h.al=0x00;
  __dpmi_int(0xe6,&regs);
  if(regs.x.ax==0xaa55) {
    /* AX contained "signature", so version of DOSemu is in BX/CX */
    printf (
      "Sorry, try native DOS instead of DOSemu %d.%d.%d\n",
      regs.h.bh,regs.h.bl,regs.x.cx
      );
    exit(1);
    }
  }

#endif


/*
#############################################################################
#
# main()
#
# The 'main' function - the root of all evil doings, maybe...
#
#############################################################################
*/

int main(void) {

  WORD rc,pic;
  UWORD mX,mY,msgno;
  char *msg=SIGNOFFMSGS;
  char deckname[260],deckfile[260],signoff[80];
  char bdname[20],bddesc[70],bdstr[102];
  char MusicType[80];
  int bdnamelen,bddesclen;
  struct date d1,d2;
  struct time t;
  struct birthday *bdptr=birthdaylist;
  int age;

  /* GNU-ables */
  _get_dos_version(1);
  getdate(&d1);
  gettime(&t);
  srandom((360000*t.ti_hour)+(6000*t.ti_min)+(100*t.ti_sec)+t.ti_hund);

#if (DOSEMUCHECK==1)
  /* DOSemu can't run this game (because of VESA?) */
  DOSemuCheck();
#endif

  /* read configuration file */
  ConfigRead();

  /* birthday checking */
  bdstr[0]='\0';
  while(bdptr->day!=-1) {
    if((bdptr->day==d1.da_day)&&(bdptr->mon==d1.da_mon)) {
      age=d1.da_year-bdptr->year;
      bdnamelen=strlen(bdptr->name);
      bddesclen=strlen(bdptr->desc);
      swab(bdptr->name,bdname,bdnamelen);
      swab(bdptr->desc,bddesc,bddesclen);
      if(bdname[bdnamelen-1]==' ')
        bdname[bdnamelen-1]='\0';
      else
        bdname[bdnamelen]='\0';
      if(bddesc[bddesclen-1]==' ')
        bddesc[bddesclen-1]='\0';
      else
        bddesc[bddesclen]='\0';
      sprintf (
        bdstr,
        " It's %s's %d%s birthday! Many happy returns to %s ",
        bdname,
        age,(age%10==1)?"st":(age%10==2)?"nd":(age%10==3)?"rd":"th",
        bddesc
        );
      break;
      }
    bdptr++;
    }

  /* init music and play intro music (with birthday prefix if reqd & MIDI) */
  ConfigGet("MUSIC",MusicType);
  if(strcmp(MusicType,"MIDI")==0) {
    if(MidiMusicInit()==MUSIC_OKAY)
      if(MidiMusicLoad(INTROMUSNAME)==MUSIC_OKAY) {
        if(bdstr[0]!='\0')
          MidiMusicOneShot(BirthdayMPX2);
        else
          rc=MidiMusicPlay();
        }
    }
  if(strcmp(MusicType,"MOD")==0) {
    if(ModMusicInit()==MUSIC_OKAY)
      if(ModMusicLoad(INTROMODNAME)==MUSIC_OKAY)
        rc=ModMusicPlay();
    }
  if(strcmp(MusicType,"CD")==0) {
    if(CdMusicInit()==MUSIC_OKAY)
      CdMusicPlay("TITLE");
    }

  /* Windows 95 application title and Close-Aware status */
  if(CheckWin95()) {
    SetAppTitle("Klondike 95 v" VERNUMB);
    SetupCloseAware();
    }

  /* get deck name */
  ConfigGet("DECK",deckname);
  if(deckname[0]=='\0') {
    strcpy(deckname,DEFAULTDECK);
    ConfigPut("DECK",deckname);
    }

  /* open debug output */
  //dfp=fopen("COM2","w");

  /* this is how debug messages is set out in the code */
  //if(dfp)
    //fprintf(dfp,"\014 Klondike 95 v" VERNUMB "\n~~~~~~~~~~~~~~~~~~\n");

  /* first, set up the various bits and pieces we need */
  if((rc=VesaInit())<0) {
    err(ERR_IN_VESA,rc,"Error setting up screen");
    }
  TextInit();
  if((rc=MouseInit())<0) {
    VesaCleanUp();
    err(ERR_IN_MOUSE,rc,"Error setting up mouse");
    }
  pic=VesaShowPic(TITLEPICNAME);
  if(bdstr[0]!='\0')
    TextWrite (
      bdstr,
      -1,9,
      SMALL_FONT,
      VesaColour(240,150,20),VesaColour(60,60,60)
      );
  sprintf(deckfile,"%s%s",DECKDIR,deckname);
  if((cards=RekoGetDeck(deckfile,0))<0) {
    VesaCleanUp();
    err(ERR_IN_REKO,cards,"Error reading deck");
    }
  if(pic==0) {
    TextWrite (
      " Click mouse to play ",
      -1,Y_RESOLUTION-19,
      BIG_FONT,
      VesaColour(0,0,0),VesaColour(255,255,255)
      );
    MouseWait(&mX,&mY,0);
    }

  /* game music */
  if(strcmp(MusicType,"MIDI")==0) {
    MidiMusicStop();
    MidiMusicFree();
    if(MidiMusicLoad(GAMEMUSNAME)==MUSIC_OKAY)
      rc=MidiMusicPlay();
    }
  if(strcmp(MusicType,"MOD")==0) {
    ModMusicStop();
    ModMusicFree();
    if(ModMusicLoad(GAMEMODNAME)==MUSIC_OKAY)
      rc=ModMusicPlay();
    }
  if(strcmp(MusicType,"CD")==0) {
    CdMusicStop();
    CdMusicPlay("GAME");
    }

  /* the PROGRAM loop, once per game */
  while(quitflag!=QUIT_EXIT) {

    /* set up a new game */
    RekoClearTable();
    Shuffle();
    InitDeal(cards);
    quitflag=0;

    /* the GAME loop, once per mouse click */
    while(!quitflag) {
      MouseWait(&mX,&mY,0);
      if((mX==0)&&(mY==0))
	quitflag=QUIT_EXIT;
      else
	HandleClick(mX,mY);
      }

    /* if exit game loop because of winning, display a picture */
    if(quitflag==QUIT_WIN) {
      if(VesaShowPic(WINPICNAME)==0) {
        /* win music */
        if(strcmp(MusicType,"MIDI")==0) {
          MidiMusicStop();
          MidiMusicFree();
          if(MidiMusicLoad(WINMUSNAME)==MUSIC_OKAY)
            rc=MidiMusicPlay();
          }
        if(strcmp(MusicType,"MOD")==0) {
          ModMusicStop();
          ModMusicFree();
          if(ModMusicLoad(WINMODNAME)==MUSIC_OKAY)
            rc=ModMusicPlay();
          }
        if(strcmp(MusicType,"CD")==0) {
          CdMusicStop();
          CdMusicPlay("WIN");
          }
	MouseWait(&mX,&mY,0);
        /* game music */
        if(strcmp(MusicType,"MIDI")==0) {
          MidiMusicStop();
          MidiMusicFree();
          if(MidiMusicLoad(GAMEMUSNAME)==MUSIC_OKAY)
            rc=MidiMusicPlay();
          }
        if(strcmp(MusicType,"MOD")==0) {
          ModMusicStop();
          ModMusicFree();
          if(ModMusicLoad(GAMEMODNAME)==MUSIC_OKAY)
            rc=ModMusicPlay();
          }
        if(strcmp(MusicType,"CD")==0) {
          CdMusicStop();
          CdMusicPlay("GAME");
          }
        }
      }

    } /* end of program loop */


  /* free up resources */
  VesaCleanUp();
  RekoDiscardDeck();

  /* nice little message if day changes ;-) */
  getdate(&d2);
  if (
    (d1.da_day!=d2.da_day) ||
    (d1.da_mon!=d2.da_mon) ||
    (d1.da_year!=d2.da_year)
    )
    puts("So you've finally found the quit feature...  ;-)\n");

  /* pick a 'sign off' message at random and decode it */
  msgno=random()%NUMSIGNOFFMSGS;
  while(msgno) {
    while(*msg++)
      /* nothing */ ;
    msgno--;
    }
  swab(msg,signoff,strlen(msg));
  signoff[strlen(msg)]='\0';

  /* output the 'thank you' and the 'sign off' messages */
  printf("%s\n\nRemember...\n\n%s\n",THANKYOU,signoff);

  /* update configuration file if necessary */
  ConfigWrite();
  ConfigFlush();

  /* Disable Close-Aware status (for when we're run in a DOS box) */
  if(CheckWin95())
    ClearCloseAware();

  /* music */
  if(strcmp(MusicType,"MIDI")==0)
    MidiMusicCleanUp();
  if(strcmp(MusicType,"MOD")==0)
    ModMusicCleanUp();
  if(strcmp(MusicType,"CD")==0)
    CdMusicCleanUp();

  /* that's it! */
  return(0);

  }
