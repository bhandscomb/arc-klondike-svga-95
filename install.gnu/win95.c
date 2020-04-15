/*
#############################################################################
#
# win95.c
#
#
# Windows 95 module for Klondike installer
#
#
#############################################################################
*/


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#include <dpmi.h>
#include <go32.h>
#include <sys\movedata.h>
#include <sys\segments.h>

#include "mytypes.h"


/*
#############################################################################
#
# CheckWinRunning()
# CheckWin95()
# CheckVolumeLFN()
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
  _get_dos_version(1);
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

int CheckVolumeLFN(char *DriveRoot) {
  __dpmi_regs regs;
  int DOSSEL,DOSSEG,rc;
  DOSSEG=__dpmi_allocate_dos_memory(17,&DOSSEL);
  if(DOSSEG==-1)
    return(0);
  movedata(_my_ds(),(int)DriveRoot,DOSSEL,0,strlen(DriveRoot)+1);
  /* INT 21h, AX=71A0h, Get Volume Information */
  regs.x.ax=0x71a0;
  regs.x.es=DOSSEG;
  regs.x.di=16;
  regs.x.cx=255;
  regs.x.ds=DOSSEG;
  regs.x.dx=0;
  __dpmi_int(0x21,&regs);
  puts("EEE");
  rc=__dpmi_free_dos_memory(DOSSEL);
  if(!(regs.x.flags&1)) {
    if(regs.x.bx&0x4000)
      return(1);
    }
  return(0);
  }


/*
#############################################################################
#
# LFNrename()
#
# Long file name file package
#
#############################################################################
*/

int LFNrename(char *oldname,char *newname) {
  __dpmi_regs regs;
  int DOSSEL,DOSSEG,rc;
  DOSSEG=__dpmi_allocate_dos_memory (
    (strlen(oldname)+strlen(newname)+17)>>4,
    &DOSSEL
    );
  if(DOSSEG==-1)
    return(1);
  movedata(_my_ds(),(int)oldname,DOSSEL,0,strlen(oldname)+1);
  movedata(_my_ds(),(int)newname,DOSSEL,strlen(oldname)+1,strlen(newname)+1);
  /* INT 21h, AX=7156h, Rename File */
  regs.x.ax=0x7156;
  regs.x.ds=DOSSEG;
  regs.x.dx=0;
  regs.x.es=DOSSEG;
  regs.x.di=strlen(oldname)+1;
  __dpmi_int(0x21,&regs);
  rc=__dpmi_free_dos_memory(DOSSEL);
  return((regs.x.flags&1)?0:1);
  }
