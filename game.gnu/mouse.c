/*
#############################################################################
#
# mouse.c
#
#
# Mouse pointer stuff for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# INT 33h Function AX=0000h --- Reset Driver amd Get Status
# INT 33h Function AX=0003h --- Get Mouse Position and Button Status
# INT 33h Function AX=000Bh --- Determine Motion Distance
#
#############################################################################
*/


/* includes */

#include <stdio.h>

//#include <dos.h>

#include <dpmi.h>

#include "mytypes.h"
#include "game.h"
#include "vesa.h"
#include "mouse.h"
#include "protos.h"


/* mouse pointer - 1=white 0=black -1=transparent */

#define POINTER_XSIZE 11
#define POINTER_YSIZE 19

BYTE pointer[POINTER_YSIZE][POINTER_XSIZE]={
  { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1},
  { 0, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1},
  { 0, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1},
  { 0, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1},
  { 0, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1},
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1},
  { 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
  { 0, 1, 1, 0, 1, 1, 0,-1,-1,-1,-1},
  { 0, 1, 0,-1, 0, 1, 1, 0,-1,-1,-1},
  { 0, 0,-1,-1, 0, 1, 1, 0,-1,-1,-1},
  { 0,-1,-1,-1,-1, 0, 1, 1, 0,-1,-1},
  {-1,-1,-1,-1,-1, 0, 1, 1, 0,-1,-1},
  {-1,-1,-1,-1,-1,-1, 0, 1, 1, 0,-1},
  {-1,-1,-1,-1,-1,-1, 0, 1, 1, 0,-1},
  {-1,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1}
  };

UWORD buffer[POINTER_YSIZE][POINTER_XSIZE];

WORD mousex,mousey;

extern WORD quitflag;


/*
#############################################################################
#
# Error messages
#
#############################################################################
*/

char MOUSE_FAILED_MSG[]="No mouse driver or no mouse hardware";

char *MouseErrorMessages[] = {
  MOUSE_FAILED_MSG
  };


/*
#############################################################################
#
# MouseInit()
#
# Initialises mouse driver, makes initial read of motion counters and sets
# where the mouse pointer starts.
#
#############################################################################
*/

WORD MouseInit(void) {

  __dpmi_regs regs;

  regs.x.ax=0x0000;
  __dpmi_int(0x33,&regs);
  if(regs.x.ax!=0xffff)
    return(MOUSE_FAILED);

  regs.x.ax=0x000b;
  __dpmi_int(0x33,&regs);

  mousex=X_RESOLUTION>>1;
  mousey=Y_RESOLUTION>>1;

  return(MOUSE_OKAY);
  }


/*
#############################################################################
#
# MouseDraw() --- PRIVATE!
#
# Used by MouseWait() to draw/undraw mouse pointer.
#
#############################################################################
*/

void MouseDraw(UBYTE draw) {

  UWORD x,y,ptr0,ptr1;

  ptr0=VesaColour(0,0,0);
  ptr1=VesaColour(255,255,255);

  for(y=0;y<POINTER_YSIZE;y++) {

    for(x=0;x<POINTER_XSIZE;x++) {

      if(draw) {
	VesaPoint (
	  x+mousex,y+mousey,
	  &buffer[y][x]
	  );
	if(pointer[y][x]==0)
	  VesaPlot(x+mousex,y+mousey,ptr0);
	if(pointer[y][x]==1)
	  VesaPlot(x+mousex,y+mousey,ptr1);
	}
      else {
	VesaPlot (
	  x+mousex,y+mousey,
	  buffer[y][x]
	  );
	}

      } /* for() */

    } /* for() */

  } /* MouseDraw() */


/*
#############################################################################
#
# MouseCheck()
#
# Allows caller to get left mouse button status.
#
#############################################################################
*/

UWORD MouseCheck() {
  __dpmi_regs regs;
  regs.x.ax=0x0003;
  __dpmi_int(0x33,&regs);
  return(regs.x.bx&1);
  }


/*
#############################################################################
#
# MouseWait()
#
# Allows caller to wait for left mouse button click while showing a mouse
# pointer. In most video modes, most of this would not be needed, but the
# mouse driver can't seem to handle our chosen "HiColor" VESA modes, so we
# have to handle it ourselves.
#
# Also, since we're here most of the time, this is where we'll handle the
# fact we're Close-Aware for Windows 95.
#
#############################################################################
*/

void MouseWait(UWORD *clickx,UWORD *clicky,UWORD immed) {

  __dpmi_regs regs;
  UWORD button=1,clicked=0;

  /* ensure left mouse button is released */
  while(button==1) {
    regs.x.ax=0x0003;
    __dpmi_int(0x33,&regs);
    button=regs.x.bx&1;
    }

  /* read motion counters so main loop has first values */
  regs.x.ax=0x000b;
  __dpmi_int(0x33,&regs);

  /* draw pointer in initial position */
  MouseDraw(1);

  /* main loop - ends when left mouse button clicked and released */
  while((clicked==0)||(button==1)) {

    /* keep polling mouse until mouse moved or left button changed */
    while((button==clicked)&&(regs.x.cx==0)&&(regs.x.dx==0)) {
      MusicContinue();
      if(CheckWin95()) {
        if(QueryClose()) {
          quitflag=QUIT_EXIT;
          return;
          }
        }
      regs.x.ax=0x0003;
      __dpmi_int(0x33,&regs);
      button=regs.x.bx&1;
      regs.x.ax=0x000b;
      __dpmi_int(0x33,&regs);
      }

    /* if left button pressed, flag it */
    if((clicked==0)&&(button==1)) {
      clicked=1;
      /* if immediate exit required, fudge/fake button release */
      if(immed)
	button=0;
      }

    /* else update pointer position */
    else {

      /* undraw pointer */
      MouseDraw(0);

      /* calculate new position */
      mousex+=(int)regs.x.cx;
      mousey+=(int)regs.x.dx;

      /* ensure new position is valid */
      if(mousex<0) mousex=0;
      if(mousex>799) mousex=799;
      if(mousey<0) mousey=0;
      if(mousey>599) mousey=599;

      /* draw pointer in new position */
      MouseDraw(1);

      /* re-read motion counters to ensure back to zero */
      regs.x.ax=0x000b;
      __dpmi_int(0x33,&regs);

      } /* if() */

    } /* while() */

  /* undraw mouse */
  MouseDraw(0);

  /* return position of mouse click */
  *clickx=mousex;
  *clicky=mousey;

  } /* MouseWait() */
