/*
#############################################################################
#
# text.c
#
#
# Text routines for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# INT 10h Function AX=1130h --- Get Font Information
#
#############################################################################
*/


#include <stdio.h>
//#include <dos.h>

#include <dpmi.h>
#include <go32.h>
#include <sys\movedata.h>
#include <sys\segments.h>

#include "mytypes.h"
#include "vesa.h"
#include "text.h"
#include "protos.h"

#define FONTPTR_8_16 0x06
#define FONTPTR_8_8  0x03

UBYTE BigFont[256][16];
UBYTE SmallFont[256][8];


/*
#############################################################################
#
# TextInit()
#
# Grabs the ROM fonts using INT 10h AX=1130h - need to access BP reg so we
# are using REGPACK rather than REGS/SREGS combo.
#
#############################################################################
*/

void TextInit() {
  __dpmi_regs regs;
  regs.x.ax=0x1130;
  regs.x.bx=FONTPTR_8_16<<8;
  __dpmi_int(0x10,&regs);
  movedata (
    _dos_ds,(regs.x.es<<4)+regs.x.bp,
    _my_ds(),(int)BigFont,
    256*16
    );
  regs.x.ax=0x1130;
  regs.x.bx=FONTPTR_8_8<<8;
  __dpmi_int(0x10,&regs);
  movedata (
    _dos_ds,(regs.x.es<<4)+regs.x.bp,
    _my_ds(),(int)SmallFont,
    256*8
    );
  }


/*
#############################################################################
#
# TextWrite()
#
# Write text to the screen at a specified position using a specified font.
#
#############################################################################
*/

void TextWrite(char *msg,WORD x,WORD y,UWORD font,UWORD fg,UWORD bg) {
  UWORD VesaTextRow[X_RESOLUTION];
  UWORD row,xpos,ypos,charlen,gfxlen,c,p,*clr;
  UBYTE b;
  charlen=strlen(msg);
  if((charlen==0)||(y>=Y_RESOLUTION))
    return;
  gfxlen=charlen<<3;
  ypos=y;
  if(x==-1)
    xpos=(X_RESOLUTION-gfxlen)>>1;
  else
    xpos=x;
  switch(font) {
    case SMALL_FONT:
      for(row=0;row<8;row++) {
	clr=VesaTextRow;
	for(c=0;c<charlen;c++) {
	  b=SmallFont[(int)msg[c]][row];
	  for(p=0;p<8;p++) {
	    if(b&0x80)
	      *clr++=fg;
	    else
	      *clr++=bg;
	    b<<=1;
	    }
	  }
	VesaPlotRow(xpos,ypos,gfxlen,VesaTextRow);
	ypos++;
	if(ypos==Y_RESOLUTION)
	  return;
	}
      break;
    case BIG_FONT:
      for(row=0;row<16;row++) {
	clr=VesaTextRow;
	for(c=0;c<charlen;c++) {
	  b=BigFont[(int)msg[c]][row];
	  for(p=0;p<8;p++) {
	    if(b&0x80)
	      *clr++=fg;
	    else
	      *clr++=bg;
	    b<<=1;
	    }
	  }
	VesaPlotRow(xpos,ypos,gfxlen,VesaTextRow);
	ypos++;
	if(ypos==Y_RESOLUTION)
	  return;
	}
      break;
    }
  }
