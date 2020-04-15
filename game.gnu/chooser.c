/*
#############################################################################
#
# chooser.c
#
#
# Cardset selector code for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# INT 2Fh, AX=1600h --- Windows Enhanced Mode Installation Check
# INT 2Fh, AX=4680h --- Windows 3.0 Installation Check (undoc)
# INT 2Fh, AX=4A33h --- DOS 7.0 Check
#
#############################################################################
*/


/* includes */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <dir.h>
#include <string.h>
#include <ctype.h>

#include <dpmi.h>

#include "mytypes.h"
#include "game.h"
#include "vesa.h"
#include "text.h"
#include "errmsg.h"
#include "files.h"
#include "protos.h"

#include "icons.h"


/* styles of 3d box */

#define THIN              1
#define THICK             2

#define RAISED_THIN       THIN
#define RAISED_THICK      THICK
#define INDENTED_THIN    -THIN
#define INDENTED_THICK   -THICK

#define ISRAISED          (type>0)
#define ISINDENTED        (type<0)
#define THICKNESS         (ISRAISED?type:-type)


/* text specifications */

#define LIST_WIDTH        80
#define LIST_HEIGHT       12
#define LIST_FORMAT       "%-80s"

#define INFO_WIDTH        80
#define INFO_HEIGHT       11
#define INFO_FORMAT       "%-80s"


/* graphic info */

#define G_SCROLL_WIDTH    16

#define G_LIST_WIDTH      (LIST_WIDTH*8+G_SCROLL_WIDTH+8)
#define G_LIST_HEIGHT     (LIST_HEIGHT*16+8)
#define G_LIST_X_GAP      ((X_RESOLUTION-G_LIST_WIDTH)>>1)

#define G_INFO_WIDTH      (INFO_WIDTH*8+G_SCROLL_WIDTH+8)
#define G_INFO_HEIGHT     (INFO_HEIGHT*16+8)
#define G_INFO_X_GAP      ((X_RESOLUTION-G_INFO_WIDTH)>>1)

#define G_BOTH_HEIGHT     (G_LIST_HEIGHT+G_INFO_HEIGHT)

#define G_BOX_Y_GAP       ((Y_RESOLUTION-(G_LIST_HEIGHT+G_INFO_HEIGHT))/3)


/* text positions */

#define LIST_X(x)         (G_LIST_X_GAP+4+(x)*8)
#define LIST_Y(y)         (G_BOX_Y_GAP+4+(y)*16)
#define INFO_X(x)         (G_INFO_X_GAP+4+(x)*8)
#define INFO_Y(y)         ((G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+4+(y)*16)


/* deck list and info structures */

struct ListNode {
  struct ListNode *next;
  char FileName[1];   /* variable length to save valuable heap space */
  };

struct InfoNode {
  struct InfoNode *next;
  char Text[1];   /* variable length to save valuable heap space */
  };


/* click region definition */

typedef struct {
  WORD x1,y1;
  WORD x2,y2;
  void (*clickfunc)(WORD);
  WORD funcparam;
  } ChooserClickRegion;


/* click regions */

ChooserClickRegion ChooserRegions[] = {

  {X_RESOLUTION-40,2,X_RESOLUTION-3,39,Back2System,0},

  {LIST_X(0),LIST_Y(0),LIST_X(LIST_WIDTH)-1,LIST_Y(1)-1,ListClick,0},
  {LIST_X(0),LIST_Y(1),LIST_X(LIST_WIDTH)-1,LIST_Y(2)-1,ListClick,1},
  {LIST_X(0),LIST_Y(2),LIST_X(LIST_WIDTH)-1,LIST_Y(3)-1,ListClick,2},
  {LIST_X(0),LIST_Y(3),LIST_X(LIST_WIDTH)-1,LIST_Y(4)-1,ListClick,3},
  {LIST_X(0),LIST_Y(4),LIST_X(LIST_WIDTH)-1,LIST_Y(5)-1,ListClick,4},
  {LIST_X(0),LIST_Y(5),LIST_X(LIST_WIDTH)-1,LIST_Y(6)-1,ListClick,5},
  {LIST_X(0),LIST_Y(6),LIST_X(LIST_WIDTH)-1,LIST_Y(7)-1,ListClick,6},
  {LIST_X(0),LIST_Y(7),LIST_X(LIST_WIDTH)-1,LIST_Y(8)-1,ListClick,7},
  {LIST_X(0),LIST_Y(8),LIST_X(LIST_WIDTH)-1,LIST_Y(9)-1,ListClick,8},
  {LIST_X(0),LIST_Y(9),LIST_X(LIST_WIDTH)-1,LIST_Y(10)-1,ListClick,9},
  {LIST_X(0),LIST_Y(10),LIST_X(LIST_WIDTH)-1,LIST_Y(11)-1,ListClick,10},
  {LIST_X(0),LIST_Y(11),LIST_X(LIST_WIDTH)-1,LIST_Y(12)-1,ListClick,11},

  {(X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+2,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+17,
    ListScroll,-1},

/*
  {(X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+18,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+G_LIST_HEIGHT-19,
    ListSelect,0},
*/

  {(X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+G_LIST_HEIGHT-18,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+G_LIST_HEIGHT-3,
    ListScroll,1},

  {(X_RESOLUTION-G_INFO_X_GAP)-18,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+2,
    (X_RESOLUTION-G_INFO_X_GAP)-3,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+17,
    InfoScroll,-1},

  {(X_RESOLUTION-G_INFO_X_GAP)-18,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-18,
    (X_RESOLUTION-G_INFO_X_GAP)-3,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-3,
    InfoScroll,1},

  {-1,-1,-1,-1,NULL,-1}

  };


/* deck list head node, number of list entries, and length of info */

struct ListNode *DeckList=NULL;
struct InfoNode *InfoList=NULL;
int ListEntries=0,InfoLines=0;


/* list & info globals */

int listtop,chosen,highlight,infotop;
char Blanks[82];
UWORD ListScrollLeft,ListScrollRight;
UWORD ListScrollTop,ListScrollBottom,ListScrollHeight;
UWORD InfoScrollLeft,InfoScrollRight;
UWORD InfoScrollTop,InfoScrollBottom,InfoScrollHeight;


/* from main module */

extern WORD cards,quitflag;


/*
#############################################################################
#
# CheckWinRunning()
# CheckWin95()
#
# Various host platform checks
#
#############################################################################
*/

int CheckWinRunning(void) {
  __dpmi_regs regs;
  /* INT 2Fh, AX=1600h, Windows Enhanced Mode Installation Check */
  regs.x.ax=0x1600;
  __dpmi_int(0x2f,&regs);
  switch(regs.h.al) {
    case 0x00: break;   /* neither Win 3.x enhanced nor Win/386 2.x */
    case 0x80: break;   /* neither Win 3.x enhanced nor Win/386 2.x */
    case 0x01: return(2);   /* Windows/386 2.x */
    case 0xff: return(2);   /* Windows/386 2.x */
    default: return(regs.h.al);   /* Windows version: AL.AH */
    }
  /* INT 2Fh, AX=4680h, Windows 3.0 Installation Check (undoc) */
  regs.x.ax=0x4680;
  __dpmi_int(0x2f,&regs);
  /* conflict:
	       Ralf Brown's interrupt list says AX is 0000h if Windows
	       3.0 is in real or standard mode or DOS 5 DOSSHELL active.

	       Other source (PC Intern) said AX is ??80h if Windows
	       3.0 is NOT running, else it is.

	       Since PC Intern value of 80h in AL comes from the caller,
	       it is probably okay to use this...
  */
  return((regs.h.al==0x80)?0:3);
  }

int CheckWin95(void) {
  __dpmi_regs regs;
  if(_osmajor>=7) {
    /* INT 2Fh, AX=4A33h, DOS 7.0 Check */
    regs.x.ax=0x4a33;
    __dpmi_int(0x2f,&regs);
    if(regs.x.ax==0) {
      return(1);
      }
    }
  return(0);
  }


/*
#############################################################################
#
# AddDeckList()
#
# Add decks to the deck list, based on supplied wildcard pattern
#
#############################################################################
*/

void AddDeckList(char *pattern) {
  struct ffblk ffblk;
  int findhandle;
  struct ListNode *ListEntry,*NewEntry;
  char *thename;

  /* find first file and set 'thename' pointer */
  findhandle=!findfirst(pattern,&ffblk,0);
  thename=ffblk.ff_name;
  /* continue if we got it okay */
  if(findhandle) {
    for(;;) {
      /* don't add prefs decks */
      if(stricmp(thename,PREFSDECK)==0)
	goto skip;
      /* allocate memory for new deck list entry and copy filename */
      if((NewEntry=malloc(sizeof(struct ListNode)+strlen(thename)))==NULL)
	break;
      strcpy(NewEntry->FileName,ffblk.ff_name);
      /* if no previous entries, simply set 'head' node and we're finished */
      if(DeckList==NULL) {
	NewEntry->next=NULL;
	DeckList=NewEntry;
	}
      else {
	/* find position to insert entry, start at 'head' */
	ListEntry=DeckList;
	/* continue while the 'next' entry exists */
	while(ListEntry->next) {
	  /* stop if 'next' is greater than new entry */
	  if(stricmp(ListEntry->next->FileName,NewEntry->FileName)>0)
	    break;
	  /* point to 'next' */
	  ListEntry=ListEntry->next;
	  }
	/* insert new entry after current entry */
	NewEntry->next=ListEntry->next;
	ListEntry->next=NewEntry;
	}
      ListEntries++;
skip:
      /* find next file - stop if we fail */
      if(findnext(&ffblk))
        break;
      }
    }
  }


/*
#############################################################################
#
# FreeDeckList()
#
# Removes deck list from memory
#
#############################################################################
*/

void FreeDeckList(void) {
  struct ListNode *NextToFree;
  NextToFree=DeckList;
  while(NextToFree) {
    NextToFree=DeckList->next;
    free(DeckList);
    DeckList=NextToFree;
    }
  ListEntries=0;
  }


/*
#############################################################################
#
# MakeDeckList()
#
# Forms a deck list using all supported deck wildcard patterns
#
#############################################################################
*/

void MakeDeckList(void) {
  FreeDeckList();
  AddDeckList(DECKDIR "*.rek");
#if 0
  /* because of the way Win 95 works, the above finds *.reko as well */
  AddDeckList(DECKDIR "*.reko");
#endif
  }


/*
#############################################################################
#
# ThreeD()
#
# Draw 3D box
#
#############################################################################
*/

void ThreeD(UWORD x1,UWORD y1,UWORD x2,UWORD y2,WORD type) {
  UWORD ys,ye,y,tlclr,brclr;
  ys=y1+THICKNESS;
  ye=y2-THICKNESS;
  tlclr=ISRAISED?WHITE:BLACK;
  brclr=ISRAISED?BLACK:WHITE;
  VesaLine(x1,y1,x2-x1,tlclr);
  VesaPlot(x2,y1,DGREY);
  if(THICKNESS==THICK) {
    VesaLine(x1,y1+1,(x2-x1)-1,tlclr);
    VesaPlot(x2-1,y1+1,DGREY);
    VesaPlot(x2,y1+2,brclr);
    }
  for(y=ys;y<=ye;y++) {
    if(THICKNESS==THICK) {
      VesaLine(x1,y,2,tlclr);
      VesaLine(x2-1,y,2,brclr);
      }
    else {
      VesaPlot(x1,y,tlclr);
      VesaPlot(x2,y,brclr);
      }
    }
  if(THICKNESS==THICK) {
    VesaPlot(x1,y2-1,tlclr);
    VesaPlot(x1+1,y2-1,DGREY);
    VesaLine(x1+2,y2-1,(x2-x1)-1,brclr);
    }
  VesaPlot(x1,y2,DGREY);
  VesaLine(x1+1,y2,x2-x1,brclr);
  }


/*
#############################################################################
#
# DisplayList()
#
# Writes into list box
#
#############################################################################
*/

void DisplayList() {
  struct ListNode *ListEntry;
  int n;
  char EntryName[LIST_WIDTH+1];
  if(DeckList==NULL)
    return;
  ListEntry=DeckList;
  for(n=0;n<listtop;n++) {
    if(ListEntry==NULL)
      return;
    ListEntry=ListEntry->next;
    }
  for(n=0;n<LIST_HEIGHT;n++,ListEntry=ListEntry->next) {
    if(ListEntry==NULL)
      break;
    sprintf(EntryName,LIST_FORMAT,ListEntry->FileName);
    TextWrite (
      EntryName,
      LIST_X(0),LIST_Y(n),
      BIG_FONT,
      WHITE,(listtop+n==highlight)?DGREY:LGREY
      );
    }
  /* erase old scroll indicator */
  for(n=ListScrollTop;n<=ListScrollBottom;n++)
    VesaLine(ListScrollLeft,n,ListScrollRight-ListScrollLeft+1,LGREY);
  /* draw new scroll indicator */
  if(ListEntries<=LIST_HEIGHT)
    ThreeD (
      ListScrollLeft,ListScrollTop,
      ListScrollRight,ListScrollBottom,
      RAISED_THIN
      );
  else
    ThreeD (
      ListScrollLeft,
      ListScrollTop+((ListScrollHeight*listtop)/ListEntries),
      ListScrollRight,
      ListScrollTop+((ListScrollHeight*(listtop+LIST_HEIGHT))/ListEntries),
      RAISED_THIN
      );
  }


/*
#############################################################################
#
# GetDeckName()
#
# Get file name from deck list
#
#############################################################################
*/

void GetDeckName(int number,char **name) {
  struct ListNode *DeckNode;
  int n;
  DeckNode=DeckList;
  *name=NULL;
  for(n=0;n<number;n++) {
    if(DeckNode)
      DeckNode=DeckNode->next;
    else
      return;
    }
  if(DeckNode==NULL)
    return;
  *name=DeckNode->FileName;
  }


/*
#############################################################################
#
# DrawChooser()
#
# Draw the cardset selector screen
#
#############################################################################
*/

void DrawChooser(int win) {

  UWORD y;
/*
  char selstr[]="SELECT",tmpstr[]="#";
*/

  /* clear screen (light grey) */
  VesaClear(LGREY);

  /* system icon on a button for exit */
  ThreeD(X_RESOLUTION-40,2,X_RESOLUTION-3,39,RAISED_THICK);
  for(y=0;y<32;y++) {
    switch(win) {
      case 0:
	VesaPlotRow(X_RESOLUTION-37,y+5,32,MsdosIcon[y]);
	break;
      case 95:
	VesaPlotRow(X_RESOLUTION-37,y+5,32,Windows95Icon[y]);
	break;
      default:
	VesaPlotRow(X_RESOLUTION-37,y+5,32,WindowsIcon[y]);
      }
    }

  /* cardset list */
  ThreeD (
    G_LIST_X_GAP,G_BOX_Y_GAP,
    (X_RESOLUTION-G_LIST_X_GAP)-1,G_BOX_Y_GAP+G_LIST_HEIGHT-1,
    INDENTED_THICK
    );
  /* list scroll bar */
  ThreeD (
    (X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+2,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+17,
    RAISED_THIN
    );
  TextWrite (
    "\030",
    (X_RESOLUTION-G_LIST_X_GAP)-14,G_BOX_Y_GAP+6,
    SMALL_FONT,
    BLACK,LGREY
    );
/*
  ThreeD (
    (X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+18,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+G_LIST_HEIGHT-19,
    RAISED_THIN
    );
  for(y=0;y<6;y++) {
    tmpstr[0]=selstr[y];
    TextWrite (
      tmpstr,
      (X_RESOLUTION-G_LIST_X_GAP)-14,G_BOX_Y_GAP+(G_LIST_HEIGHT>>1)-54+y*20,
      SMALL_FONT,
      BLACK,LGREY
      );
    }
*/
  ListScrollLeft=(X_RESOLUTION-G_INFO_X_GAP)-16;
  ListScrollTop=G_BOX_Y_GAP+20;
  ListScrollRight=(X_RESOLUTION-G_INFO_X_GAP)-5;
  ListScrollBottom=G_BOX_Y_GAP+G_LIST_HEIGHT-21;
  ListScrollHeight=ListScrollBottom-ListScrollTop;
  ThreeD (
    ListScrollLeft-2,ListScrollTop-2,
    ListScrollRight+2,ListScrollBottom+2,
    INDENTED_THIN
    );
  ThreeD (
    ListScrollLeft,ListScrollTop,
    ListScrollRight,ListScrollBottom,
    RAISED_THIN
    );
  ThreeD (
    (X_RESOLUTION-G_LIST_X_GAP)-18,G_BOX_Y_GAP+G_LIST_HEIGHT-18,
    (X_RESOLUTION-G_LIST_X_GAP)-3,G_BOX_Y_GAP+G_LIST_HEIGHT-3,
    RAISED_THIN
    );
  TextWrite (
    "\031",
    (X_RESOLUTION-G_LIST_X_GAP)-14,G_BOX_Y_GAP+G_LIST_HEIGHT-14,
    SMALL_FONT,
    BLACK,LGREY
    );

  /* cardset info */
  ThreeD (
    G_INFO_X_GAP,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT,
    (X_RESOLUTION-G_INFO_X_GAP)-1,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-1,
    INDENTED_THICK
    );
  /* info scroll bar */
  ThreeD (
    (X_RESOLUTION-G_INFO_X_GAP)-18,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+2,
    (X_RESOLUTION-G_INFO_X_GAP)-3,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+17,
    RAISED_THIN
    );
  TextWrite (
    "\030",
    (X_RESOLUTION-G_INFO_X_GAP)-14,(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+6,
    SMALL_FONT,
    BLACK,LGREY
    );
  InfoScrollLeft=(X_RESOLUTION-G_INFO_X_GAP)-16;
  InfoScrollTop=(G_BOX_Y_GAP<<1)+G_LIST_HEIGHT+20;
  InfoScrollRight=(X_RESOLUTION-G_INFO_X_GAP)-5;
  InfoScrollBottom=((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-21;
  InfoScrollHeight=InfoScrollBottom-InfoScrollTop;
  ThreeD (
    InfoScrollLeft-2,InfoScrollTop-2,
    InfoScrollRight+2,InfoScrollBottom+2,
    INDENTED_THIN
    );
  ThreeD (
    InfoScrollLeft,InfoScrollTop,
    InfoScrollRight,InfoScrollBottom,
    RAISED_THIN
    );
  ThreeD (
    (X_RESOLUTION-G_INFO_X_GAP)-18,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-18,
    (X_RESOLUTION-G_INFO_X_GAP)-3,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-3,
    RAISED_THIN
    );
  TextWrite (
    "\031",
    (X_RESOLUTION-G_INFO_X_GAP)-14,((G_BOX_Y_GAP<<1)+G_BOTH_HEIGHT)-14,
    SMALL_FONT,
    BLACK,LGREY
    );

  }


/*
#############################################################################
#
# ChooseDeck()
#
# Friendly cardset selector
#
#############################################################################
*/

void ChooseDeck(void) {
  int win,n;
  UWORD mX,mY;
  char *DeckName,DeckFile[260];
  ChooserClickRegion *Region;

  /* initialise Blanks[] for ClearInfo() */
  for(n=0;n<80;n++)
    Blanks[n]=' ';
  Blanks[n]='\0';

  /* default action is to 'restart' a game */
  quitflag=QUIT_RESTART;

  /* system check */
  if(CheckWin95()!=0)
    win=95;
  else
    win=CheckWinRunning();

  DrawChooser(win);

#if 0
  TextWrite (
    " Cardset information display not yet implemented. ",
    INFO_X(17),INFO_Y(5),
    BIG_FONT,
    BLACK,WHITE
    );
#endif

#if 0
  {
  char xxxxx[70];
  sprintf(xxxxx,"_stklen  = 0x%04x   stackavail = 0x%04x   SP = 0x%04x",_stklen,stackavail(),_SP);
  TextWrite(xxxxx,INFO_X(5),INFO_Y(1),SMALL_FONT,BLACK,LGREY);
  sprintf(xxxxx,"_heaplen = 0x%04x   coreleft   = 0x%04x",_heaplen,coreleft());
  TextWrite(xxxxx,INFO_X(5),INFO_Y(2),SMALL_FONT,BLACK,LGREY);
  }
#endif

  listtop=0;
  chosen=highlight=-1;

  MakeDeckList();
  DisplayList();

#if 0
  {
  char xxxxx[70];
  sprintf(xxxxx,"_stklen  = 0x%04x   stackavail = 0x%04x   SP = 0x%04x",_stklen,stackavail(),_SP);
  TextWrite(xxxxx,INFO_X(5),INFO_Y(9),SMALL_FONT,BLACK,LGREY);
  sprintf(xxxxx,"_heaplen = 0x%04x   coreleft   = 0x%04x",_heaplen,coreleft());
  TextWrite(xxxxx,INFO_X(5),INFO_Y(10),SMALL_FONT,BLACK,LGREY);
  }
#endif

  while((chosen==-1)&&(quitflag!=QUIT_EXIT)) {
    while(MouseCheck())
      /* nothing */;
    MouseWait(&mX,&mY,1);
    if(quitflag==QUIT_EXIT)
      return;
    Region=ChooserRegions;
    while(Region->x1!=-1) {
      if (
	(mX>=Region->x1) &&
	(mY>=Region->y1) &&
	(mX<=Region->x2) &&
	(mY<=Region->y2)
	) {
	(*(Region->clickfunc)) (
	  Region->funcparam
	  );
	break;
	}
      Region++;
      }
    }

  if(quitflag==QUIT_EXIT)
    return;

  GetDeckName(chosen,&DeckName);
  if(DeckName==NULL)
    return;

  ConfigPut("DECK",DeckName);

  RekoDiscardDeck();
  sprintf(DeckFile,"%s%s",DECKDIR,DeckName);
  if((cards=RekoGetDeck(DeckFile,0))<0) {
    VesaCleanUp();
    printf("\"%s\"\n",DeckName);
    err(ERR_IN_REKO,cards,"Error reading deck");
    }

  }


/*
#############################################################################
#
# DisplayInfo()
#
# Writes into info box
#
#############################################################################
*/

void DisplayInfo() {
  struct InfoNode *InfoEntry;
  int n;
  char EntryText[INFO_WIDTH+1];
  if(InfoList==NULL)
    return;
  InfoEntry=InfoList;
  for(n=0;n<infotop;n++) {
    if(InfoEntry==NULL)
      return;
    InfoEntry=InfoEntry->next;
    }
  for(n=0;n<INFO_HEIGHT;n++,InfoEntry=InfoEntry->next) {
    if(InfoEntry==NULL)
      break;
    sprintf(EntryText,INFO_FORMAT,InfoEntry->Text);
    TextWrite(EntryText,INFO_X(0),INFO_Y(n),BIG_FONT,DGREY,LGREY);
    }
  /* erase old scroll indicator */
  for(n=InfoScrollTop;n<=InfoScrollBottom;n++)
    VesaLine(InfoScrollLeft,n,InfoScrollRight-InfoScrollLeft+1,LGREY);
  /* draw new scroll indicator */
  if(InfoLines<=INFO_HEIGHT)
    ThreeD (
      InfoScrollLeft,InfoScrollTop,
      InfoScrollRight,InfoScrollBottom,
      RAISED_THIN
      );
  else
    ThreeD (
      InfoScrollLeft,
      InfoScrollTop+((InfoScrollHeight*infotop)/InfoLines),
      InfoScrollRight,
      InfoScrollTop+((InfoScrollHeight*(infotop+INFO_HEIGHT))/InfoLines),
      RAISED_THIN
      );
  }


/*
#############################################################################
#
# ClearInfo()
#
# Removes info text from memory and screen
#
#############################################################################
*/

void ClearInfo(void) {
  struct InfoNode *NextToFree;
  int n;
  NextToFree=InfoList;
  while(NextToFree) {
    NextToFree=InfoList->next;
    free(InfoList);
    InfoList=NextToFree;
    }
  InfoLines=0;
  for(n=0;n<INFO_HEIGHT;n++)
    TextWrite(Blanks,INFO_X(0),INFO_Y(n),BIG_FONT,WHITE,LGREY);
  }


/*
#############################################################################
#
# LoadInfo()
#
# Loads info text for a deck into memory
#
#############################################################################
*/

void LoadInfo(char *REKname) {
  int n;
  char CSIname[260];
  char thetext[82];
  FILE *infofp;
  struct InfoNode *InfoEntry,*NewEntry=NULL;

  /* clear existing info (if any) */
  ClearInfo();
  InfoEntry=NULL;

  /* form filename of CardSet Info file */
  strcpy(CSIname,DATADIR);
  strcat(CSIname,REKname);
  if(toupper(CSIname[strlen(CSIname)-1])=='O')
    CSIname[strlen(CSIname)-1]='\0';
  strcpy(&CSIname[strlen(CSIname)-3],"CSI");

  /* open file CSI file */
  if((infofp=fopen(CSIname,"r"))==NULL) {
    /* if we can't, say so! */
    TextWrite (
      " Unable to find information about this cardset. ",
      INFO_X(17),INFO_Y(5),
      BIG_FONT,
      BLACK,WHITE
      );
    /* erase old scroll indicator */
    for(n=InfoScrollTop;n<=InfoScrollBottom;n++)
      VesaLine(InfoScrollLeft,n,InfoScrollRight-InfoScrollLeft+1,LGREY);
    /* draw new (full) scroll indicator */
    ThreeD (
      InfoScrollLeft,InfoScrollTop,
      InfoScrollRight,InfoScrollBottom,
      RAISED_THIN
      );
    return;
    }

  /* read file */
  for(;;) {
    /* try to get a line */
    fgets(thetext,82,infofp);
    /* if found end of file, we're done */
    if(feof(infofp))
      break;
    /* strip newline character */
    if(strlen(thetext))
      thetext[strlen(thetext)-1]='\0';
    /* allocate memory for new info line and copy text */
    if((NewEntry=malloc(sizeof(struct InfoNode)+strlen(thetext)))==NULL)
      break;
    strcpy(NewEntry->Text,thetext);
    /* if no previous entries, simply set 'head' node and we're finished */
    /* otherwise, set the next of the previous to the current :-) */
    if(InfoList==NULL)
      InfoList=NewEntry;
    else
      InfoEntry->next=NewEntry;
    /* increase line count */
    InfoLines++;
    /* set the previous to the current before going round again */
    InfoEntry=NewEntry;
    }

  /* set the next of the last to NULL to terminate list */
  NewEntry->next=NULL;

  /* close file, we're done with it */
  fclose(infofp);

  /* initial info display */
  infotop=0;
  DisplayInfo();

  }


/*
#############################################################################
#
# Click Functions
#
#############################################################################
*/

void ListClick(WORD scrnpos) {
  char *DeckName;
  if(highlight==listtop+scrnpos) {
    chosen=highlight;
    return;
    }
  highlight=listtop+scrnpos;
  DisplayList();
  GetDeckName(highlight,&DeckName);
  if(DeckName==NULL)
    return;
  LoadInfo(DeckName);
  }

void ListScroll(WORD direction) {
  do {
    if((direction==-1)&&(listtop>0))
      listtop--;
    if((direction==1)&&(listtop<ListEntries-LIST_HEIGHT))
      listtop++;
    DisplayList();
    } while(MouseCheck());
  }

/*
void ListSelect(WORD dummy) {
  chosen=highlight;
  }
*/

void InfoScroll(WORD direction) {
  if((direction==-1)&&(infotop>0))
    infotop--;
  if((direction==1)&&(infotop<InfoLines-INFO_HEIGHT))
    infotop++;
  DisplayInfo();
  }

void Back2System(WORD dummy) {
  quitflag=QUIT_EXIT;
  }
