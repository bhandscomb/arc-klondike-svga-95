/*
############################################################################
#
# showkln.c
#
# Show Klondike 95 "KLN" picture
#
#############################################################################
*/


/* includes */

#include <stdio.h>
#include <stdlib.h>

#include <dpmi.h>
#include <go32.h>
#include <sys\movedata.h>
#include <sys\segments.h>

// #include "mytypes.h"
typedef long LONG;
typedef short WORD;
typedef char BYTE;
typedef unsigned long ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;

// #include "vesa.h"
#define X_RESOLUTION 800
#define Y_RESOLUTION 600

#define VESA_OKAY 0
#define VESA_FAIL -1
#define VESA_NOMODE -2
#define VESA_NOMEM -3
#define VESA_BADMODE -4
#define VESA_BADPICT -5
#define VESA_NODOSMEM -6
#define VESA_PRE_1_2 -7
#define VESA_2_NOLIN -8

#define VesaColour(r,g,b) \
  ( (((r)&0xf8)<<8) | (((g)&0xfc)<<3) | (((b)&0xf8)>>3) )


// #include "protos.h"
void VesaPlotRow(UWORD,UWORD,UWORD,UWORD *); 


/* video mode definitions (internals) */

#define VIDEO_MODE      0x114
#define VIDEO_PAGES64      15
#define BYTES_PER_PIXEL     2
#define BPP_MULT(x)     ((x)+(x))


/* KLN buffer size (must be greater than BMP2KLN reported value) */

#define KLNBUFFSIZE     1000


/* rowaddr[] stores video memory offsets for start of rows */

ULONG rowaddr[Y_RESOLUTION];


/* prevpage[] used to eliminate as much "Window" swapping as possible */

UWORD oldmode,vbe=0;
UWORD wingranshift,winseg[2],prevpage[2]={99,99},readwin=0,writewin=0;
ULONG wingranmask;

/* linear frame buffer stuff */

#define LFBSIZE ((X_RESOLUTION*Y_RESOLUTION)<<1)
int lfb_linearaddress,lfb_selector;


/*
#############################################################################
#
# Error messages
#
#############################################################################
*/

char VESA_FAIL_MSG[]="VESA call failed - no vesa driver?";
char VESA_NOMODE_MSG[]="Required VESA mode not supported";
char VESA_NOMEM_MSG[]="Not enough memory on video card - 1MB min";
char VESA_BADMODE_MSG[]="VESA mode unusable by gfx code";
char VESA_BADPICT_MSG[]="Corrupted KLN graphic file";
char VESA_NODOSMEM_MSG[]="Unable to allocate enough conventional memory";
char VESA_PRE_1_2_MSG[]="Require at least VESA BIOS Extension v1.2";
char VESA_2_NOLIN_MSG[]="Require linear framebuffer with VBE 2.0 or above";

char *VesaErrorMessages[] = {
  VESA_FAIL_MSG,
  VESA_NOMODE_MSG,
  VESA_NOMEM_MSG,
  VESA_BADMODE_MSG,
  VESA_BADPICT_MSG,
  VESA_NODOSMEM_MSG,
  VESA_PRE_1_2_MSG,
  VESA_2_NOLIN_MSG
  };


/*
#############################################################################
#
# peek()
# poke()
#
# DJGPP support
#
#############################################################################
*/

UWORD peek(UWORD seg,UWORD off) {
  UWORD dat;
  movedata(_dos_ds,(seg<<4)+off,_my_ds(),(int)&dat,2);
  return(dat);
  }

void poke(UWORD seg,UWORD off,UWORD dat) {
  movedata(_my_ds(),(int)&dat,_dos_ds,(seg<<4)+off,2);
  }


/*
#############################################################################
#
# VesaSetPage()
#
# Selects a video memory page for use.
#
#############################################################################
*/

void VesaSetPage(UWORD win,UWORD page) {
  __dpmi_regs regs;
  regs.x.bx=win;
  regs.x.dx=page;
  regs.x.ax=0x4f05;
  __dpmi_int(0x10,&regs);
  /* update previous page */
  prevpage[win]=page;
  }


/*
#############################################################################
#
# VesaLine()
#
# Draws a horizontal line of a specified colour.
#
#############################################################################
*/

void VesaLine(UWORD x,UWORD y,UWORD xlen,UWORD colour) {
  UWORD linedata[X_RESOLUTION],xloop;
  for(xloop=0;xloop<xlen;xloop++)
    linedata[xloop]=colour;
  VesaPlotRow(x,y,xlen,linedata);
  }


/*
#############################################################################
#
# VesaPlotRow()
#
# Draws a row of a graphic.
#
#############################################################################
*/

void VesaPlotRow(UWORD x,UWORD y,UWORD xlen,UWORD *colourarray) {
  ULONG pixaddr,pixoffset;
  UWORD page;
  if(((x+xlen)>X_RESOLUTION)||(y>=Y_RESOLUTION))
    return;
  pixaddr=rowaddr[y]+BPP_MULT(x);
  if(vbe==1) {
    if((page=pixaddr>>wingranshift)!=prevpage[writewin]) {
      VesaSetPage(writewin,page);
      }
    pixoffset=pixaddr&wingranmask;
    if((pixoffset+BPP_MULT(xlen))>wingranmask) {
      movedata (
        _my_ds(),(int)colourarray,
        _dos_ds,(winseg[writewin]<<4)+pixoffset,
        (ULONG)wingranmask+1-pixoffset
        );
      VesaSetPage(writewin,++page);
      movedata (
        _my_ds(),((int)colourarray)+((ULONG)wingranmask+1-pixoffset),
        _dos_ds,winseg[writewin]<<4,
        BPP_MULT(xlen)-((ULONG)wingranmask+1-pixoffset)
        );
      }
    else {
      movedata (
        _my_ds(),(int)colourarray,
        _dos_ds,(winseg[writewin]<<4)+pixoffset,
        BPP_MULT(xlen)
        );
      }
    }
  else {
    movedata (
      _my_ds(),(int)colourarray,
      lfb_selector,pixaddr,
      BPP_MULT(xlen)
      );
    }
  }


/*
############################################################################
#
# VesaShowPic()
#
# Show a picture file in the center of the screen.
#
#############################################################################
*/

WORD VesaShowPic(char *name) {
  ULONG size[2];
  UWORD xpos,ypos,x,y,len,klnbuff[KLNBUFFSIZE],buff[X_RESOLUTION],*dat;
  FILE *fp;
  if((fp=fopen(name,"rb"))==NULL)
    return(-1);
  /* check file type */
  fread(klnbuff,2,8,fp);
  if (
    (klnbuff[0]!=0x6c4b) || (klnbuff[1]!=0x6e6f) ||
    (klnbuff[2]!=0x6964) || (klnbuff[3]!=0x656b) ||
    (klnbuff[4]!=0x4720) || (klnbuff[5]!=0x7866) ||
    (klnbuff[6]!=0x0d21) || (klnbuff[7]!=0x1a0a)
    ) {
    fclose(fp);
    return(-1);
    }
  /* get picture size */
  fread(size,4,2,fp);
  xpos=(X_RESOLUTION-size[0])>>1;
  ypos=(Y_RESOLUTION-size[1])>>1;
  for(y=0;y<size[1];y++) {
    /* get KLN line data length then KLN line data */
    fread(&len,2,1,fp);
    /* can we do this? */
    if(len>KLNBUFFSIZE)
      fseek(fp,len<<1,SEEK_CUR);
    else {
      fread(klnbuff,2,len,fp);
      /* decompress KLN line data */
      dat=klnbuff;
      for(x=0;x<size[0];) {
	len=*dat++;
	if(len&0x8000) {
	  for(len&=0x7fff;len>0;len--)
	    buff[x++]=*dat;
	  dat++;
	  }
	else {
	  for(;len>0;len--)
	    buff[x++]=*dat++;
	  }
	}
      VesaPlotRow(xpos,ypos++,size[0],buff);
      }
    }
  fclose(fp);
  return(0);
  }


/*
#############################################################################
#
# VesaPlot()
#
# Plots a point on a 65536 colour mode screen. Uses VESA BIOS extension for
# "window" swapping.
#
#############################################################################
*/

void VesaPlot(UWORD x,UWORD y,UWORD colour) {

  ULONG pixaddr;
  UWORD page;

  /* reject out of range co-ords */
  if((x>=X_RESOLUTION)||(y>=Y_RESOLUTION))
    return;

  /* pixel offset to whole of video ram */
  pixaddr=rowaddr[y]+BPP_MULT(x);

  if(vbe==1) {

    /* perform BIOS window swapping only if required */
    if((page=pixaddr>>wingranshift)!=prevpage[writewin])
      VesaSetPage(writewin,page);

    /* poke the pixel */
    poke(winseg[writewin],pixaddr&wingranmask,colour);

    }

  else

    movedata(_my_ds(),(int)&colour,lfb_selector,pixaddr,2);

  } /* VesaPlot() */


/*
#############################################################################
#
# VesaPoint()
#
# Returns the colour of a point 65536 colour mode screen. Uses VESA BIOS
# extension for "window" swapping.
#
#############################################################################
*/

void VesaPoint(UWORD x,UWORD y,UWORD *colour) {

  ULONG pixaddr;
  UWORD page;

  /* reject out of range co-ords - return */
  if((x>=X_RESOLUTION)||(y>=Y_RESOLUTION))
    return;

  /* pixel offset to whole of video ram */
  pixaddr=rowaddr[y]+BPP_MULT(x);


  if(vbe==1) {

    /* perform BIOS window swapping only if required */
    if((page=pixaddr>>wingranshift)!=prevpage[readwin])
      VesaSetPage(readwin,page);

    /* peek the pixel */
    *colour=peek(winseg[readwin],pixaddr&wingranmask);

    }

  else

    movedata(lfb_selector,pixaddr,_my_ds(),(int)colour,2);

  }


/*
#############################################################################
#
# VesaInit() and VesaCleanUp()
#
# VesaInit() sets up our required video mode using VESA BIOS extensions and
# also calculates a table of pixel addresses for the start of rows so that
# plotting speed may be increased. It also saves the current video mode.
#
# VesaCleanUp() sets the video mode back as per when VesaInit() was called.
#
#############################################################################
*/

WORD VesaInit(void) {

  __dpmi_regs regs;
  __dpmi_meminfo linearmeminfo;

  UWORD row;
  UWORD Mode,ModePtr;
  ULONG wingran=0;

  int DOSSEL,DOSSEG,rc;

  #define unaligned __attribute__ ((packed))

  struct SVGAINFO {
    ULONG      VESASignature;
    UWORD      VESAVersion;
    ULONG      OEMStringPtr;
    ULONG      Capabilities;
    UWORD      VideoModePtr[2];
    UWORD      MemoryAmount;
    /*---*/
    UWORD      OemSoftwareRev;
    ULONG      OemVendorNamePtr;
    ULONG      OemProductNamePtr;
    ULONG      OemProductRevPtr;
    UBYTE      pad[222];
    UBYTE      OemData[256];
    } unaligned SVGAinfo;

  struct MODEINFO {
    UWORD     ModeAttributes;
    UBYTE     WinAAttributes;
    UBYTE     WinBAttributes;
    UWORD     WinGranularity;
    UWORD     WinSize;
    UWORD     WinASegment;
    UWORD     WinBSegment;
    ULONG     WinFuncPtr;
    UWORD     BytesPerScnLine;
    /*---*/
    UWORD     XResolution;
    UWORD     YResolution;
    UBYTE     XCharSize;
    UBYTE     YCharSize;
    UBYTE     NumberOfPlanes;
    UBYTE     BitsPerPixel;
    UBYTE     NumberOfBanks;
    UBYTE     MemoryModel;
    UBYTE     BankSize;
    UBYTE     NumberOfPages;
    UBYTE     reserved;
    /*---*/
    UBYTE     RedMaskSize;
    UBYTE     RedFieldPosition;
    UBYTE     GreenMaskSize;
    UBYTE     GreenFieldPosition;
    UBYTE     BlueMaskSize;
    UBYTE     BlueFieldPosition;
    UBYTE     RsvdMaskSize;
    UBYTE     RsvdFieldPosition;
    UBYTE     DirectColorModeInfo;
    ULONG     PhysBasePtr;
    ULONG     OffScreenMemOffset;
    UWORD     OffScreenMemSize;
    /*---*/
    UBYTE     pad[206];
    } unaligned ModeInfo;

  typedef union {
    struct SVGAINFO si;
    struct MODEINFO mi;
    } DOSMEMBLOCK;

  #define FREEDOSMEM \
    rc=__dpmi_free_dos_memory(DOSSEL)

  #define GETMODE \
    movedata ( \
      _dos_ds,(SVGAinfo.VideoModePtr[1]<<4)+ModePtr, \
      _my_ds(),(int)&Mode, \
      2 \
      )

  /* get dos mem */
  DOSSEG=__dpmi_allocate_dos_memory((sizeof(DOSMEMBLOCK)+15)>>4,&DOSSEL);
  if(DOSSEG==-1)
    return(VESA_NODOSMEM);

  /* get SVGA information */
  SVGAinfo.VESASignature=0x32454256;
  movedata(_my_ds(),(int)&SVGAinfo,DOSSEL,0,4);
  regs.x.ax=0x4f00;
  regs.x.es=DOSSEG;
  regs.x.di=0;
  __dpmi_int(0x10,&regs);
  if((regs.h.al!=0x4f)||(regs.h.ah!=0x00)) {
    FREEDOSMEM;
    return(VESA_FAIL);
    }
  movedata(DOSSEL,0,_my_ds(),(int)&SVGAinfo,sizeof(struct SVGAINFO));

  /* check VBE version */
  vbe=0;
  if((SVGAinfo.VESAVersion>>8)==1) {
    if((SVGAinfo.VESAVersion&0xff)>=2)
      vbe=1;
    }
  else {
    if((SVGAinfo.VESAVersion>>8)>=2)
      vbe=2;
    }
  if(vbe==0) {
    FREEDOSMEM;
    return(VESA_PRE_1_2);
    }

  /* check if our mode is supported */
  ModePtr=SVGAinfo.VideoModePtr[0];
  GETMODE;
  while((Mode!=0xffff)&&(Mode!=VIDEO_MODE)) {
    ModePtr+=2;
    GETMODE;
    }
  if(Mode==0xffff) {
    FREEDOSMEM;
    return(VESA_NOMODE);
    }

  /* make sure card has enough memory */
  if(SVGAinfo.MemoryAmount<VIDEO_PAGES64) {
    FREEDOSMEM;
    return(VESA_NOMEM);
    }

  /* get some vital details */
  regs.x.ax=0x4f01;
  regs.x.es=DOSSEG;
  regs.x.di=0;
  regs.x.cx=VIDEO_MODE;
  __dpmi_int(0x10,&regs);
  movedata(DOSSEL,0,_my_ds(),(int)&ModeInfo,sizeof(struct MODEINFO));

  if(vbe==1) {
    wingran=(ULONG)ModeInfo.WinGranularity*1024;
    winseg[0]=ModeInfo.WinASegment;
    winseg[1]=ModeInfo.WinBSegment;
    /* for speed, assume window granularity is a power of 2 */
    wingranshift=0;
    wingranmask=wingran-1;
    while(wingran!=1) {
      wingranshift++;
      wingran>>=1;
      }
    }
  else {
    linearmeminfo.address=ModeInfo.PhysBasePtr;
    linearmeminfo.size=LFBSIZE;
    lfb_linearaddress=__dpmi_physical_address_mapping(&linearmeminfo);
    lfb_selector=__dpmi_allocate_ldt_descriptors(1);
    __dpmi_set_segment_base_address(lfb_selector,lfb_linearaddress);
    __dpmi_set_segment_limit(lfb_selector,LFBSIZE-1);
    }


  /* make sure the hardware supports the mode */
  if((ModeInfo.ModeAttributes&0x01)!=0x01) {
    FREEDOSMEM;
    puts("!Attr");
    return(VESA_BADMODE);
    }

  /* superfluous, but I'll check anyway... */
  if(vbe==1)
    if((ULONG)ModeInfo.WinSize*1024<wingran) {
      FREEDOSMEM;
      puts("!WinSize");
      return(VESA_BADMODE);
      }

  /* if vbe 2, ensure we have linear frame buffer support */
  if(vbe==2) {
    if((ModeInfo.ModeAttributes&0x80)!=0x80) {
      FREEDOSMEM;
      return(VESA_2_NOLIN);
      }
    }

  if(vbe==1) {

    /* If window exist bits not set, ensure read & write bits are clear  */
    if((ModeInfo.WinAAttributes&0x01)!=0x01)
      ModeInfo.WinAAttributes=0x00;
    if((ModeInfo.WinBAttributes&0x01)!=0x01)
      ModeInfo.WinBAttributes=0x00;

    /* Check if win A readable, if not, win B must be! */
    if((ModeInfo.WinAAttributes&0x02)!=0x02) {
      if((ModeInfo.WinBAttributes&0x02)!=0x02) {
        FREEDOSMEM;
        puts("!ReadWin");
        return(VESA_BADMODE);
        }
      else
        readwin=1;
      }

    /* Check if win A writeable, if not, win B must be! */
    if((ModeInfo.WinAAttributes&0x04)!=0x04) {
      if((ModeInfo.WinBAttributes&0x04)!=0x04) {
        FREEDOSMEM;
        puts("!WriteWin");
        return(VESA_BADMODE);
        }
      else
        writewin=1;
      }

    }

  /* get and store current video mode */
  regs.x.ax=0x4f03;
  __dpmi_int(0x10,&regs);
  if((regs.h.al!=0x4f)||(regs.h.ah!=0x00)) {
    FREEDOSMEM;
    return(VESA_FAIL);
    }
  oldmode=regs.x.bx;

  /* select our chosen video mode */
  regs.x.ax=0x4f02;
  regs.x.bx=VIDEO_MODE;
  __dpmi_int(0x10,&regs);
  if((regs.h.al!=0x4f)||(regs.h.ah!=0x00)) {
    FREEDOSMEM;
    return(VESA_FAIL);
    }

  /* calculate plotting "speed up" table */
  for(row=0;row<Y_RESOLUTION;row++)
    rowaddr[row]=(ULONG)X_RESOLUTION*BYTES_PER_PIXEL*row;

  FREEDOSMEM;
  return(VESA_OKAY);

  } /* VesaInit() */


void VesaCleanUp(void) {

  __dpmi_regs regs;
  __dpmi_meminfo linearmeminfo;

  /* set video mode back to how we found it */
  regs.x.ax=0x4f02;
  regs.x.bx=oldmode;
  __dpmi_int(0x10,&regs);

  __dpmi_free_ldt_descriptor(lfb_selector);
  linearmeminfo.address=lfb_linearaddress;
  __dpmi_free_physical_address_mapping(&linearmeminfo);

  } /* VesaCleanUp() */


/*
#############################################################################
#
# VesaClear()
#
# Simply 'clears' the screen with the selected colour.
#
#############################################################################
*/

void VesaClear(UWORD colour) {


  UWORD loop,BlankLine[X_RESOLUTION];

  /* make a clear screen row (local) */
  for(loop=0;loop<X_RESOLUTION;loop++)
    BlankLine[loop]=colour;

  /* write to every screen row */
  for(loop=0;loop<Y_RESOLUTION;loop++)
    VesaPlotRow(0,loop,X_RESOLUTION,BlankLine);

  }


/*****/


int main(int argc,char *argv[]) {
  WORD rc,pic;
  if(argc!=2) {
    puts("Usage: showkln <klnfile>");
    exit(1);
    }
  if((rc=VesaInit())<0) {
    printf("%d Error setting up screen\n",rc);
    exit(1);
    }
  if((pic=VesaShowPic(argv[1]))==0) {
    getchar();
    }
  VesaCleanUp();
  return(0);
  }
