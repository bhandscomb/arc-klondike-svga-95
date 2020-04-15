/*
#############################################################################
#
# win95.c
#
#
# Windows 95 support module for Klondike 95
#
#
# DOS INTERRUPTS USED
#
# INT 2Fh, AX=168Eh, DX=0000h --- VM Title, Set Application Title
# INT 2Fh, AX=168Fh, DH=00h --- VM Close, Enable/Disable Close Command
# INT 2Fh, AX=168Fh, DH=01h --- VM Close, Query Close
#
#############################################################################
*/


/* includes */

//#include <dos.h>

#include <dpmi.h>
#include <go32.h>
#include <sys\movedata.h>
#include <sys\segments.h>


/*
#############################################################################
#
# SetAppTitle()
#
# Sets the application title
#
#############################################################################
*/

void SetAppTitle(char *AppTitle) {
  __dpmi_regs regs;
  int DOSSEG,DOSSEL,rc;
  DOSSEG=__dpmi_allocate_dos_memory((strlen(AppTitle)+16)>>4,&DOSSEL);
  if(DOSSEG==-1)
    return;
  movedata(_my_ds(),(int)AppTitle,DOSSEL,0,strlen(AppTitle)+1);
  regs.x.ax=0x168e;
  regs.x.dx=0;
  regs.x.es=DOSSEG;
  regs.x.di=0;
  __dpmi_int(0x2f,&regs);
  rc=__dpmi_free_dos_memory(DOSSEL);
  }

/*
#############################################################################
#
# SetupCloseAware()
#
# Tells Windows 95 we're close aware
#
#############################################################################
*/

void SetupCloseAware(void) {
  __dpmi_regs regs;
  regs.x.ax=0x168f;
  regs.x.dx=0x0001;
  __dpmi_int(0x2f,&regs);
  }

/*
#############################################################################
#
# ClearCloseAware()
#
# Tells Windows 95 we're not close aware
#
#############################################################################
*/

void ClearCloseAware(void) {
  __dpmi_regs regs;
  regs.x.ax=0x168f;
  regs.x.dx=0x0000;
  __dpmi_int(0x2f,&regs);
  }

/*
#############################################################################
#
# QueryClose()
#
# Asks Windows if user wants us to quit
#
#############################################################################
*/

int QueryClose(void) {
  __dpmi_regs regs;
  regs.x.ax=0x168f;
  regs.x.dx=0x0100;
  __dpmi_int(0x2f,&regs);
  return((regs.x.ax==0x168f)?0:-1);
  }
