/* cardinfo.c */

// v1.0      21/06/95  Original Amiga version
// v1.1 PC   20/05/96  PC port, 'if(x=y)' bugfixes, HAM/4096 test added
// v1.1a PC  12/06/96  now using DJGPP

char VersionString[]="$VER: cardinfo 1.1a PC (12.06.96)";

#include <stdio.h>
#include <stdlib.h>

// #include <exec/types.h>
typedef unsigned long  ULONG;
typedef unsigned short UWORD;
typedef unsigned char  UBYTE;

//#define mkid(a,b,c,d)	(((a)<<24)|((b)<<16)|((c)<<8)|(d))
//#define idREKO	mkid('R','E','K','O')
#define idREKO 0x52454B4F

typedef struct rekohdr {
	ULONG	RekoTag,
		CardDataSize,
		BytesPerCard;
	UWORD	Height,
		Width;
	ULONG	ViewMode;
	UBYTE	Planes,
		NumCards;
        } __attribute__ ((packed)) REKOHEADER;

FILE *ifp=NULL;

void swapword(UWORD *w) {
  *w=(*w<<8)|(*w>>8);
  }

void swaplong(ULONG *l) {
  *l=(*l<<24)|((*l&0xff00)<<8)|((*l&0xff0000)>>8)|(*l>>24);
  }

void error(char *msg) {
  if(ifp) fclose(ifp);
  puts(msg);
  exit(1);
  }

int main(int argc,char *argv[]) {
  REKOHEADER hdr;
  long colours;
  if(argc!=2)
    error("Usage: cardinfo <filename>");
  if((ifp=fopen(argv[1],"rb"))) {
    if(fread(&hdr,sizeof(REKOHEADER),1,ifp)!=1)
      error("Unable to read header");
    swaplong(&hdr.RekoTag);
    swaplong(&hdr.CardDataSize);
    swaplong(&hdr.BytesPerCard);
    swapword(&hdr.Height);
    swapword(&hdr.Width);
    swaplong(&hdr.ViewMode);
    if(hdr.RekoTag!=idREKO)
      error("REKO tag missing");
    colours=(hdr.CardDataSize-(hdr.BytesPerCard*hdr.NumCards))/3;
    if((hdr.Planes==8)&&(colours==64))
      colours=262144;
    if((hdr.Planes==6)&&(colours==16))
      colours=4096;
    printf (
      "FILE=%s CARDS=%d COLOURS=%ld%s\n",
      argv[1],
      hdr.NumCards,
      colours,
      (colours==262144)?" (HAM8)":(colours==4096)?" (HAM)":""
      );
    fclose(ifp);
    }
  else
    error("Unable to open specified cardset");
  return 0;
  }
