/*
#############################################################################
#
# reko.c
#
#
# Cardset manipulation for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# --- none ---
#
#############################################################################
*/


#include <stdio.h>

#include "mytypes.h"
#include "vesa.h"
#include "game.h"
#include "reko.h"
#include "files.h"
#include "protos.h"


#define HAM                0x00000800

#define PALETTE_ENTRIES    256
#define PALETTE_ENTRY_SIZE   3

#define MAX_CARDS           68

#define CARD_ROWS          130
#define CARD_MAX_PLANES      8
#define CARD_ROW_BYTES      11


#define REKO_TAG           0x52454B4F
#define REK0_TAG           0x52454B30


struct REKOhdr {
  ULONG RekoTag;
  ULONG CardDataSize;
  ULONG BytesPerCard;
  UWORD Height;
  UWORD Width;
  ULONG ViewMode;
  UBYTE Planes;
  UBYTE NumCards;
  } __attribute__ ((packed));


struct REKOhdr RekoHeader[2];

UBYTE          Palette[2][PALETTE_ENTRIES][PALETTE_ENTRY_SIZE];
UWORD          SVGACard[MAX_CARDS][CARD_ROWS][CARD_ROW_BYTES*8];
UWORD          BackgroundColour;


/*
#############################################################################
#
# Error messages
#
#############################################################################
*/

char REKOERR_READ_MSG[]="Error reading card file";
char REKOERR_BADCARD_MSG[]="Card not in deck";
char REKOERR_BADDECK_MSG[]="Error in deck header - deck unusable";
char REKOERR_NODECK_MSG[]="Error opening card file";
char REKOERR_NOPREFS_MSG[]="Unable to get any prefs cards";
char REKOERR_DUMMY_MSG[]="";

char *RekoErrorMessages[] = {
  REKOERR_READ_MSG,
  REKOERR_BADCARD_MSG,
  REKOERR_DUMMY_MSG,
  REKOERR_DUMMY_MSG,
  REKOERR_BADDECK_MSG,
  REKOERR_NODECK_MSG,
  REKOERR_NOPREFS_MSG
  };


/*
#############################################################################
#
# swapword() & swaplong()
#
# Since the data format originated on an Amiga (Motorola), we need to fiddle
# with all multi-byte values.
#
#############################################################################
*/

void swapword(UWORD *w) {
  *w=(*w<<8)|(*w>>8);
  }

void swaplong(ULONG *l) {
  *l=(*l<<24)|((*l&0xff00)<<8)|((*l&0xff0000)>>8)|(*l>>24);
  }


/*
#############################################################################
#
# RekoGetDeck()
#
# Reads named file as a REKO cardset.
#
# If deckbase is non-zero, we're reading the supplemental deck.
#
#############################################################################
*/

WORD RekoGetDeck(char *name,UWORD deckbase) {

  FILE *ifp;
  UWORD PBbord,PBfill,PBx=0,PBy;
  UWORD Deck,StdCards,PaletteEntries,RowBytes;
  UWORD card,row,byte,bit,pix;
  WORD plane;
  UBYTE HAMmode,red=0,grn=0,blu=0,colour,bitmask,hamdat;
  UBYTE CardRow[CARD_MAX_PLANES][CARD_ROW_BYTES];

  PBbord=WHITE;
  PBfill=VesaColour(0xe0,0x08,0x08);

  Deck=(deckbase!=0)?1:0;

  if((ifp=fopen(name,"rb"))==NULL)
    return(REKOERR_NODECK);

  /* get header */
  if(fread(&RekoHeader[Deck],sizeof(struct REKOhdr),1,ifp)!=1)
    return(REKOERR_READ);

  /* swap byte order in header */
  swaplong(&RekoHeader[Deck].RekoTag);
  swaplong(&RekoHeader[Deck].CardDataSize);
  swaplong(&RekoHeader[Deck].BytesPerCard);
  swapword(&RekoHeader[Deck].Height);
  swapword(&RekoHeader[Deck].Width);
  swaplong(&RekoHeader[Deck].ViewMode);

  /* if deckbase!=0, allow "REK0" */
  if((deckbase!=0)&&(RekoHeader[Deck].RekoTag==REK0_TAG))
    RekoHeader[Deck].RekoTag=REKO_TAG;

  /* check for valid header values */
  if (
    (RekoHeader[Deck].RekoTag!=REKO_TAG) ||
    (RekoHeader[Deck].NumCards>MAX_CARDS) ||
    (RekoHeader[Deck].Height!=CARD_ROWS) ||
    (RekoHeader[Deck].Width!=CARD_ROW_BYTES*8) ||
    (
      CARD_ROWS*CARD_ROW_BYTES*RekoHeader[Deck].Planes !=
      RekoHeader[Deck].BytesPerCard
      )
    )
    return(REKOERR_BADDECK);

  /* HAM decode? */
  HAMmode=((RekoHeader[Deck].ViewMode&HAM)?1:0);

  /* get palette size */
  PaletteEntries =
    1<<(RekoHeader[Deck].Planes-(HAMmode<<1));

  /* get size of row data */
  RowBytes=RekoHeader[Deck].Planes*CARD_ROW_BYTES;

  /* read the palette */
  if(fread(&Palette[Deck],3,PaletteEntries,ifp)!=PaletteEntries)
    return(REKOERR_READ);

  /* set BackgroundColour for future use (if main deck) */
  if(deckbase==0)
    BackgroundColour=VesaColour (
      Palette[0][0][0],
      Palette[0][0][1],
      Palette[0][0][2]
      );

  /* only init progress bar if deckbase==0 */
  if(deckbase==0) {

    PBx=(X_RESOLUTION>>1)-(RekoHeader[Deck].NumCards<<1);
    VesaLine(PBx-1,Y_RESOLUTION-21,(RekoHeader[Deck].NumCards<<2)+2,PBbord);
    for(PBy=Y_RESOLUTION-20;PBy<Y_RESOLUTION-2;PBy++) {
      VesaPlot(PBx-1,PBy,PBbord);
      VesaPlot(PBx+(RekoHeader[Deck].NumCards<<2),PBy,PBbord);
      }
    VesaLine(PBx-1,Y_RESOLUTION-2,(RekoHeader[Deck].NumCards<<2)+2,PBbord);

    }

  for(card=deckbase;card<(deckbase+RekoHeader[Deck].NumCards);card++) {

    MusicContinue();

    for(row=0;row<CARD_ROWS;row++) {

      /* read card row */
      if(fread(CardRow,RowBytes,1,ifp)!=1)
	return(REKOERR_READ);

      /* start row convert - HAM support */
      if(HAMmode) {
        red=Palette[Deck][0][0];
        grn=Palette[Deck][0][1];
        blu=Palette[Deck][0][2];
        }

      pix=0;

      for(byte=0;byte<CARD_ROW_BYTES;byte++) {

        bitmask=0x80;

        for(bit=0;bit<8;bit++) {

	  /* form colour value */
	  colour=0;
	  for(plane=RekoHeader[Deck].Planes-1;plane>-1;plane--) {
            //colour=(colour<<1)+((CardRow[plane][byte]&bitmask)?1:0);
            colour<<=1;
            if(CardRow[plane][byte]&bitmask) colour++;
	    }
          bitmask>>=1;

	  /* HAM decode? */
          if(HAMmode) {

	    /* HAM6 or HAM8? */
	    switch(RekoHeader[Deck].Planes) {
	      case 8:
		hamdat=colour&0x3f;
                switch(colour&0xc0) {
                  case 0x00:
                    red=Palette[Deck][hamdat][0];
                    grn=Palette[Deck][hamdat][1];
                    blu=Palette[Deck][hamdat][2];
                    break;
                  case 0x40:
                    blu=(blu&0x03)|(hamdat<<2);
                    break;
                  case 0x80:
                    red=(red&0x03)|(hamdat<<2);
                    break;
                  case 0xc0:
                    grn=(grn&0x03)|(hamdat<<2);
                    break;
                  } /* switch(hamsw) */
                break;
	      case 6:
                hamdat=colour&0x0f;
                switch(colour&0x30) {
                  case 0x00:
                    red=Palette[Deck][hamdat][0];
                    grn=Palette[Deck][hamdat][1];
                    blu=Palette[Deck][hamdat][2];
                    break;
                  case 0x10:
                    blu=(blu&0x0f)|(hamdat<<4);
                    break;
                  case 0x20:
                    red=(red&0x0f)|(hamdat<<4);
                    break;
                  case 0x30:
                    grn=(grn&0x0f)|(hamdat<<4);
                    break;
                  } /* switch(hamsw) */
		break;
	      } /* switch(RekoHeader[Deck].Planes) */

            /* store 16-bit colour data */
            SVGACard[card][row][pix++]=VesaColour(red,grn,blu);

	    } /* HoldAndModify */

	  else {

	    /* not HAM mode, just a plain colour index */
            /* store 16-bit colour data */
            SVGACard[card][row][pix++]=VesaColour (
              red=Palette[Deck][colour][0],
              grn=Palette[Deck][colour][1],
              blu=Palette[Deck][colour][2]
              );

	    } /* !HoldAndModify */

	  } /* for(bit) */

	} /* for(byte) */

      } /* for(row) */

    /* progress bar (if deckbase==0) */
    if(deckbase==0) {
      for(PBy=Y_RESOLUTION-20;PBy<Y_RESOLUTION-2;PBy++)
	VesaLine(PBx,PBy,4,PBfill);
      PBx+=4;
      }

    } /* for(card) */

  fclose(ifp);

  if(deckbase==0) {

    /* number of cards in  main deck */
    StdCards=RekoHeader[Deck].NumCards;

    /* read supplemental (prefs) deck if needed */
    if(StdCards<MAX_CARDS) {
      /* ensure we get it okay - yep recursion... */
      if(RekoGetDeck(PREFSDECK,CARD_PREF)!=9)
	return(REKOERR_NOPREFS);
      }

    /* tell caller how many cards we read from main deck */
    return(StdCards);

    }

  else {

    return(RekoHeader[Deck].NumCards);

    }

  } /* RekoGetDeck() */


/*
#############################################################################
#
# RekoDiscardDeck()
#
# Not required since we've moved to DJGPP, but we'll leave it here in case
# we need it again...
#
#############################################################################
*/

void RekoDiscardDeck(void) {

  /* do nothing */

  } /* RekoDiscardDeck() */


/*
#############################################################################
#
# RekoClearTable() & RekoClean()
#
# To remain as visually compatible to the original Amiga version, the
# background colour is set to the first colour in the cardset's palette.
#
#############################################################################
*/

void RekoClearTable(void) {

  VesaClear(BackgroundColour);

  } /* RekoClearTable() */


void RekoClean(UWORD x1,UWORD y1,UWORD x2,UWORD y2) {

  UWORD y,xlen;

  xlen=x2-x1;

  /* NOTE: includes x1/y1, but NOT x2/y2 - intentional */
  for(y=y1;y<y2;y++)
    VesaLine(x1,y,xlen,BackgroundColour);

  } /* RekoClean() */


/*
#############################################################################
#
# RekoDrawCard()
#
# Draws a card onto the screen at the specified position.
#
#############################################################################
*/

WORD RekoDrawCard(UWORD card,UWORD xposn,UWORD yposn) {

  UWORD row;

  if(card>MAX_CARDS)
    return(REKOERR_BADCARD);

  for(row=0;row<130;row++) {

    VesaPlotRow(xposn,yposn,88,SVGACard[card][row]);
    yposn++;

    } /* for(row) */

  return(REKOERR_OKAY);

  } /* RekoDrawCard() */
