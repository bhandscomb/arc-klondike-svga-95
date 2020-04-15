/*
#############################################################################
#
# spltcrds.c
#
# write cards in either IFF ILBM format (colour depth as per cardset),
# BMP format (24-bit BGR) or RAW format (24-bit RGB).
#
# splitcards
# v1.0 - 05/06/94      Initial Version
# v1.1 - 07/06/94      Writes "cmpByteRun1" (cheats, no compression)
#
# splitcards2
# v2.0 - 27/02/95      Initial Adaptation
#
# splitcards3
# v3.0 - 15/04/95      Final(?) Adaptation
#
# spltcrds [PC]
# v1.0 - 12/09/95      First PC Version (quick-and-dirty)
# v1.1 - 13/09/95      Added ".LBM" suffix, implementation bug fixes
# v1.2 - 04/11/95      More fixes
# v2.0 - 05/11/95      Now based upon REKO module of Klondike 95
# v2.1 - 29/12/95      Added raw mode, change xxxMODE slightly
# v2.2 - 25/03/96      Added some output stats (cards, colours, HAM)
# v2.2a- 13/06/96      Moved to DJGPP
# v2.2b- 28/08/96      Killed a couple of compiler warnings, added fflush's
# v2.3 - 12/11/97      Added RKP formats (256DYN/32K)
# v2.4 - 30/12/97      Fixed HAM6 colour modify decode bug
#
#############################################################################
*/


/*
#############################################################################
#
# includes
#
#############################################################################
*/

#include <stdio.h>
#include <stdlib.h>


/*
#############################################################################
#
# my types
#
#############################################################################
*/

typedef long               LONG;
typedef unsigned long      ULONG;
typedef short              WORD;
typedef unsigned short     UWORD;
typedef char               BYTE;
typedef unsigned char      UBYTE;


/*
#############################################################################
#
# REKO/RKP handling (REKO structs are Motorola format)
#
#############################################################################
*/

#define HAM                0x00000800

#define PALETTE_ENTRIES    256
#define PALETTE_ENTRY_SIZE   3

#define MAX_CARDS           69

#define CARD_ROWS          130
#define CARD_WIDTH          88
#define CARD_MAX_PLANES      8
#define CARD_ROW_BYTES      11

#define CARD_MAX_SIZE      ((ULONG)CARD_ROWS*CARD_ROW_BYTES*8*3)

#define REKO_TAG           0x4F4B4552
#define PCREKO_TAG0        0x4350
#define PCREKO_TAG1        0x4552
#define PCREKO_TAG2        0x4F4B
#define PCREKO_TYPE32K     0x0000
#define PCREKO_TYPEDYN     0x2044

typedef struct reko {
  ULONG RekoTag;
  ULONG CardDataSize;
  ULONG BytesPerCard;
  UWORD Height;
  UWORD Width;
  ULONG ViewMode;
  UBYTE Planes;
  UBYTE NumCards;
  } __attribute__ ((packed)) REKO;

typedef struct pcreko {
  UWORD PCRekoTag[3];
  UWORD CardsetType;
  ULONG CardsetLength;
  ULONG BytesPerCard;
  UWORD Width;
  UWORD Height;
  UBYTE BitsPerPixel;
  UBYTE NumCards;
  } __attribute__ ((packed)) PCREKO;

REKO   RekoHeader;
PCREKO PCRekoHeader;
UBYTE  Palette[PALETTE_ENTRIES][PALETTE_ENTRY_SIZE];
UWORD  PCPalette[PALETTE_ENTRIES];
UBYTE  Card[CARD_ROWS][CARD_MAX_PLANES][CARD_ROW_BYTES];
UBYTE  PCDynCard[CARD_ROWS][CARD_WIDTH];
UWORD  PC32kCard[CARD_ROWS][CARD_WIDTH];
UBYTE  TwFourBitCard[CARD_ROWS][CARD_ROW_BYTES*8][3];


/*
#############################################################################
#
# ILBM handling (structs are Motorola format)
#
#############################################################################
*/

#define mkid(a,b,c,d)   (((ULONG)(a)<<24)|((ULONG)(b)<<16)|((c)<<8)|(d))

#define idFORM  mkid('F','O','R','M')
#define idILBM  mkid('I','L','B','M')
#define idBMHD  mkid('B','M','H','D')
#define idCMAP  mkid('C','M','A','P')
#define idCAMG  mkid('C','A','M','G')
#define idBODY  mkid('B','O','D','Y')

typedef struct form {
  ULONG id;
  ULONG size;
  ULONG type;
  } FORM;

typedef struct chunkhdr {
  ULONG id;
  ULONG size;
  } CHUNKHEADER;

typedef struct bmhd {
  UWORD w;
  UWORD h;
  WORD  x;
  WORD  y;
  UBYTE nPlanes;
  UBYTE masking;
  UBYTE compression;
  UBYTE pad1;
  UWORD transparentColor;
  UBYTE xAspect,yAspect;
  UWORD pageWidth,pageHeight;
  } __attribute__ ((packed)) BMHD;

#define BODY_SIZE \
  ( \
  (1+((CARD_ROW_BYTES+1)&0xfffffffe)) \
  * CARD_ROWS \
  * RekoHeader.Planes \
  )


/*
#############################################################################
#
# BMP handling (structs are Intel format)
#
#############################################################################
*/

#define BMP_TAG 0x4D42

typedef struct {
  UWORD bfType;
  ULONG bfSize;
  UWORD bfReserved1;
  UWORD bfReserved2;
  ULONG bfOffBits;
  } __attribute__ ((packed)) BITMAPFILEHEADER;


typedef struct {
  ULONG biSize;
  ULONG biWidth;
  ULONG biHeight;
  UWORD biPlanes;
  UWORD biBitCount;
  ULONG biCompression;
  ULONG biSizeImage;
  ULONG biXPelsPerMeter;
  ULONG biYPelsPerMeter;
  ULONG biClrUsed;
  ULONG biClrImportant;
  } __attribute__ ((packed)) BITMAPINFOHEADER;


/*
#############################################################################
#
# xxxMODE flags
#
#############################################################################
*/

#define nulMODE 0
#define IFFMODE 1
#define BMPMODE 2
#define RAWMODE 3


/*
#############################################################################
#
# Version and usage strings
#
#############################################################################
*/

char VersionString[]="$VER: spltcrds 2.4 (30.12.97)";
char UsageString[]="spltcrds <cardfile> [/I | /B | /R]";


/*
#############################################################################
#
# General purpose globals
#
#############################################################################
*/

FILE * ifp=NULL,*ofp=NULL;
char   outfilename[14];


/*
#############################################################################
#
# Function prototypes for card writers
#
#############################################################################
*/

void WriteIFF(int);
void WriteBMP(int);
void WriteRAW(int);


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
# Error handler
#
#############################################################################
*/

void error(char *when) {
  if(ifp) fclose(ifp);
  if(ofp) fclose(ofp);
  printf("\n*** error while %s ***\n",when);
  exit(1);
  }


/*
#############################################################################
#
# The Main Program
#
#############################################################################
*/

int main(int argc,char *argv[]) {

  UWORD DeckPaletteEntries,card,row,byte,pixel;
  WORD  bit,plane;
  UBYTE red,grn,blu,colour,bitmask,hamsw=0,hamdat=0,XXXMODE=nulMODE;
  UBYTE hamkeepmask,hammodshift;
  UBYTE RKP=0,NumCards;

  /* default mode */
  if(argc==2)
    XXXMODE=IFFMODE;

  /* parse option if present */
  if(argc==3) {
    if(argv[2][0]=='/') {
      switch(argv[2][1]) {
	case 'i':
	case 'I':
	  XXXMODE=IFFMODE;
	  break;
	case 'b':
	case 'B':
	  XXXMODE=BMPMODE;
	  break;
	case 'r':
	case 'R':
	  XXXMODE=RAWMODE;
	  break;
	}
      }
    }

  /* make sure usage is okay */
  if(XXXMODE==nulMODE) {
    puts(UsageString);
    exit(1);
    }

  /* open cardset file */
  if((ifp=fopen(argv[1],"rb"))==NULL)
    error("opening cardset file");

  /* get header */
  if(fread(&RekoHeader,sizeof(REKO),1,ifp)!=1)
    error("reading header");

  /* check if "REKO" */
  if(RekoHeader.RekoTag==REKO_TAG) {

    /* swap byte order in header */
    swaplong(&RekoHeader.CardDataSize);
    swaplong(&RekoHeader.BytesPerCard);
    swapword(&RekoHeader.Height);
    swapword(&RekoHeader.Width);
    swaplong(&RekoHeader.ViewMode);

    /* check for valid header values */
    if (
      (RekoHeader.Height!=CARD_ROWS) ||
      (RekoHeader.Width!=CARD_ROW_BYTES*8) ||
      (CARD_ROWS*CARD_ROW_BYTES*RekoHeader.Planes!=RekoHeader.BytesPerCard)
      )
      error("checking header");

    /* get palette size */
    DeckPaletteEntries=1<<(RekoHeader.Planes-((RekoHeader.ViewMode&HAM)?2:0));

    /* report some stats */
    printf (
      "%1d Cards, %1d Palette Entries%s\n",
      RekoHeader.NumCards,
      DeckPaletteEntries,
      (RekoHeader.ViewMode&HAM)?", HAM":""
      );

    /* read the palette */
    if(fread(&Palette,3,DeckPaletteEntries,ifp)!=DeckPaletteEntries)
      error("reading palette");

    NumCards=RekoHeader.NumCards;
    if(RekoHeader.ViewMode&HAM) {
      switch(RekoHeader.Planes) {
        case 8:
          hamkeepmask=0x03;
          hammodshift=2;
          break;
        case 6:
          hamkeepmask=0x0f;
          hammodshift=4;
          break;
        }
      }
    }

  else {

    /* not "REKO", try for "PCREKO" */

    /* get header */
    rewind(ifp);
    if(fread(&PCRekoHeader,sizeof(PCREKO),1,ifp)!=1)
      error("reading header");

    /* check if "PCREKO" */
    if((PCRekoHeader.PCRekoTag[0]==PCREKO_TAG0)&&
      (PCRekoHeader.PCRekoTag[1]==PCREKO_TAG1)&&
      (PCRekoHeader.PCRekoTag[2]==PCREKO_TAG2)) {

      /* check for valid header values */
      if (
        (PCRekoHeader.Height!=CARD_ROWS) ||
        (PCRekoHeader.Width!=CARD_WIDTH) ||
        (
        CARD_ROWS*CARD_WIDTH*(PCRekoHeader.BitsPerPixel>>3) !=
        PCRekoHeader.BytesPerCard
        )
        )
        error("checking header");

      /* report some stats */
      printf (
        "%1d Cards, %s\n",
        PCRekoHeader.NumCards,
        (PCRekoHeader.BitsPerPixel==8)?"256 Dynamic":"32k"
        );

      NumCards=PCRekoHeader.NumCards;
      RKP=PCRekoHeader.BitsPerPixel>>3;

      }

    }

  /* progress bar */
  for(card=0;card<NumCards;card++)
    putchar('\372');
  putchar('\r');
  fflush(stdout);

  for(card=0;card<NumCards;card++) {

    putchar('r');
    fflush(stdout);

    if(RKP>0)
      fseek(ifp,4,SEEK_CUR);

    if(RKP==1)
      if(fread(&PCPalette,PALETTE_ENTRIES*2,1,ifp)!=1)
        error("reading card palette");

    for(row=0;row<CARD_ROWS;row++) {

      /* read card row */
      switch(RKP) {
        case 0:
          if(fread(Card[row],RekoHeader.Planes*CARD_ROW_BYTES,1,ifp)!=1)
            error("reading card");
          break;
        case 1:
          if(fread(PCDynCard[row],CARD_WIDTH,1,ifp)!=1)
            error("reading card");
          break;
        case 2:
          if(fread(PC32kCard[row],CARD_WIDTH*2,1,ifp)!=1)
            error("reading card");
          break;
        }

      switch(RKP) {

        case 0:

          /* start row convert - HAM support */
          red=Palette[0][0];
          grn=Palette[0][1];
          blu=Palette[0][2];

          for(byte=0;byte<CARD_ROW_BYTES;byte++) {

            for(bit=7;bit>-1;bit--) {

              /* form colour value */
              bitmask=1<<bit;
              colour=0;
              for(plane=RekoHeader.Planes-1;plane>-1;plane--) {
                colour=(colour<<1)+((Card[row][plane][byte]&bitmask)?1:0);
                }
            
              /* HAM decode? */
              if(RekoHeader.ViewMode&HAM) {
          
                /* HAM6 or HAM8? */
                switch(RekoHeader.Planes) {
                  case 8:
                    hamsw=(colour&0xc0)>>6;
                    hamdat=colour&0x3f;
                    break;
                  case 6:
                    hamsw=(colour&0x30)>>4;
                    hamdat=colour&0x0f;
                    break;
                  } /* switch(RekoHeader.Planes) */

                /* HAM action? */
                switch(hamsw) {
                  case 0x00:
                    red=Palette[hamdat][0];
                    grn=Palette[hamdat][1];
                    blu=Palette[hamdat][2];
                    break;
                  case 0x01:
                    blu=(blu&hamkeepmask)|(hamdat<<hammodshift);
                    break;
                  case 0x02:
                    red=(red&hamkeepmask)|(hamdat<<hammodshift);
                    break;
                  case 0x03:
                    grn=(grn&hamkeepmask)|(hamdat<<hammodshift);
                    break;
                  } /* switch(hamsw) */

                } /* HoldAndModify */
            
              else {
            
                /* not HAM mode, just a plain colour index */
                red=Palette[colour][0];
                grn=Palette[colour][1];
                blu=Palette[colour][2];
            
                } /* !HoldAndModify */
            
              /* store 24-bit colour data */
              TwFourBitCard[row][(byte<<3)+(7-bit)][0]=red;
              TwFourBitCard[row][(byte<<3)+(7-bit)][1]=grn;
              TwFourBitCard[row][(byte<<3)+(7-bit)][2]=blu;
              
              } /* for(bit) */
          
            } /* for(byte) */

          break;

        case 1:

          for(pixel=0;pixel<CARD_WIDTH;pixel++) {
            red=PCPalette[PCDynCard[row][pixel]]>>7;
            grn=PCPalette[PCDynCard[row][pixel]]>>2;
            blu=PCPalette[PCDynCard[row][pixel]]<<3;
            TwFourBitCard[row][pixel][0]=red|(red>>5);
            TwFourBitCard[row][pixel][1]=grn|(grn>>5);
            TwFourBitCard[row][pixel][2]=blu|(blu>>5);
            }

          break;

        case 2:

          for(pixel=0;pixel<CARD_WIDTH;pixel++) {
            red=(PC32kCard[row][pixel]&0x3c00)>>7;
            grn=(PC32kCard[row][pixel]&0x03e0)>>2;
            blu=(PC32kCard[row][pixel]&0x001f)<<3;
            TwFourBitCard[row][pixel][0]=red|(red>>5);
            TwFourBitCard[row][pixel][1]=grn|(grn>>5);
            TwFourBitCard[row][pixel][2]=blu|(blu>>5);
            }

          break;

        } /* switch(RKP) */

      } /* for(row) */

    /* progress bar */
    printf("\bw");
    fflush(stdout);

    /* how shall we write it */
    switch(XXXMODE) {
      case IFFMODE: WriteIFF(card); break;
      case BMPMODE: WriteBMP(card); break;
      case RAWMODE: WriteRAW(card); break;
      }

    /* progress bar */
    printf("\b\376");
    fflush(stdout);

    } /* for(card) */

  putchar('\n');

  return(0);

  } /* main() */


/*
#############################################################################
#
# IFF writer
#
#############################################################################
*/

/* write an IFF chunk header */
void writechunkheader(ULONG id,ULONG size) {
  CHUNKHEADER myChunkHdr;
  myChunkHdr.id=id;
  myChunkHdr.size=size;
  swaplong(&myChunkHdr.id);
  swaplong(&myChunkHdr.size);
  if(fwrite(&myChunkHdr,sizeof(CHUNKHEADER),1,ofp)!=1)
    error("writing chunk header");
  }

/* main IFF writer */
void WriteIFF(int card) {

  FORM myFORM={idFORM,0,idILBM};
  BMHD myBMHD={CARD_ROW_BYTES*8,CARD_ROWS,0,0,0,0,1,0,0,1,1,320,256};

  int row,plane;

  /* set size of IFF file */
  myFORM.size = (
    sizeof(ULONG) +
    sizeof(CHUNKHEADER) +
    sizeof(BMHD) +
    sizeof(CHUNKHEADER) +
    (3<<RekoHeader.Planes) +
    sizeof(CHUNKHEADER) +
    sizeof(ULONG) +
    sizeof(CHUNKHEADER) +
    BODY_SIZE
    );

  /* set plane count */
  myBMHD.nPlanes=RekoHeader.Planes;

  /* create filename */
  sprintf(outfilename,"CARD%02d.LBM",card);

  /* open output file */
  if((ofp=fopen(outfilename,"wb"))==NULL)
    error("opening card file for output");

  /* make IFF header Motorola format */
  swaplong(&myFORM.id);
  swaplong(&myFORM.size);
  swaplong(&myFORM.type);

  /* write IFF header */
  if(fwrite(&myFORM,sizeof(FORM),1,ofp)!=1)
    error("writing IFF header");

  /* make BMHD chunk Motorola format */
  swapword(&myBMHD.w);
  swapword(&myBMHD.h);
  swapword((UWORD *)&myBMHD.x);
  swapword((UWORD *)&myBMHD.y);
  swapword(&myBMHD.transparentColor);
  swapword(&myBMHD.pageWidth);
  swapword(&myBMHD.pageHeight);

  /* write BMHD chunk */
  writechunkheader(idBMHD,sizeof(BMHD));
  if(fwrite(&myBMHD,sizeof(BMHD),1,ofp)!=1)
    error("writing BMHD chunk");

  /* write CMAP chunk */
  writechunkheader(idCMAP,3<<RekoHeader.Planes);
  if(fwrite(Palette,3<<RekoHeader.Planes,1,ofp)!=1)
    error("writing CMAP chunk");

  /* make REKO ViewMode Motorola format (temporary) */
  swaplong(&RekoHeader.ViewMode);
  
  /* write CAMG chunk */
  writechunkheader(idCAMG,sizeof(ULONG));
  if(fwrite(&RekoHeader.ViewMode,sizeof(ULONG),1,ofp)!=1)
    error("writing CAMG chunk");

  /* make REKO ViewMode Intel format */
  swaplong(&RekoHeader.ViewMode);

  /* write BODY chunk */  
  writechunkheader(idBODY,BODY_SIZE);
  for(row=0;row<CARD_ROWS;row++) {
    for(plane=0;plane<RekoHeader.Planes;plane++) {
      fputc(CARD_ROW_BYTES+(CARD_ROW_BYTES%1),ofp);
      if(fwrite(Card[row][plane],CARD_ROW_BYTES,1,ofp)!=1)
        error("writing BODY chunk");
      if(CARD_ROW_BYTES&1)
	fputc(0,ofp);
      }
    }

  /* close output file */
  fclose(ofp); ofp=NULL;

  }


/*
#############################################################################
#
# BMP writer
#
#############################################################################
*/

void WriteBMP(int card) {

  BITMAPFILEHEADER myBMFH = {
    BMP_TAG,
    (
      sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER) +
      ((((CARD_ROW_BYTES*8*3)+3)&0xfffffffc)*CARD_ROWS)
    ),
    0,0,
    sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
    };

  BITMAPINFOHEADER myBMIH = {
    sizeof(BITMAPINFOHEADER),
    CARD_ROW_BYTES*8,CARD_ROWS,
    1,24,0,0,
    0,0,
    0,0
    };

  int row,pixel,pad;
  UBYTE tmp;

  /* create filename */
  sprintf(outfilename,"CARD%02d.BMP",card);

  /* open output file */
  if((ofp=fopen(outfilename,"wb"))==NULL)
    error("opening card file for output");

  /* write headers */
  if(fwrite(&myBMFH,sizeof(BITMAPFILEHEADER),1,ofp)!=1)
    error("writing BMP file header");
  if(fwrite(&myBMIH,sizeof(BITMAPINFOHEADER),1,ofp)!=1)
    error("writing BMP info header");

  /* write bitmap - BMP stored upside-down and with BGR colours */
  for(row=CARD_ROWS-1;row>-1;row--) {
    for(pixel=0;pixel<CARD_ROW_BYTES*8;pixel++) {
      tmp=TwFourBitCard[row][pixel][0];
      TwFourBitCard[row][pixel][0]=TwFourBitCard[row][pixel][2];
      TwFourBitCard[row][pixel][2]=tmp;
      }
    if(fwrite(TwFourBitCard[row],CARD_ROW_BYTES*8*3,1,ofp)!=1)
      error("writing BMP bitmap");
    if((CARD_ROW_BYTES*8*3)%3) {
      for(pad=4-((CARD_ROW_BYTES*8*3)%3);pad>0;pad--)
	fputc(0,ofp);
      }
    }

  /* close output file */
  fclose(ofp); ofp=NULL;

  }


/*
#############################################################################
#
# RAW writer
#
#############################################################################
*/

void WriteRAW(int card) {

  int row;

  /* create filename */
  sprintf(outfilename,"CARD%02d.RAW",card);

  /* open output file */
  if((ofp=fopen(outfilename,"wb"))==NULL)
    error("opening card file for output");

  /* write bitmap */
  for(row=0;row<CARD_ROWS;row++) {
    if(fwrite(TwFourBitCard[row],CARD_ROW_BYTES*8*3,1,ofp)!=1)
      error("writing RAW bitmap");
    }

  /* close output file */
  fclose(ofp); ofp=NULL;

  }
