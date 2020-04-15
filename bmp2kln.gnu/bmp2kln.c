/* bmp2kln.c */

#include <stdio.h>
#include <stdlib.h>

typedef long           LONG;
typedef unsigned long  ULONG;
typedef short          WORD;
typedef unsigned short UWORD;
typedef char           BYTE;
typedef unsigned char  UBYTE;

#define MAX_WIDTH      800
#define MAX_HEIGHT     600
#define MAX_STRING     "800x600"
#define MAX_OUT        850

#define BMP_TAG        0x4D42

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

char KLN_TAG[]="Klondike Gfx!\015\012\032";

FILE *ifp=NULL,*ofp=NULL;
UBYTE bmpbuf[3];
UWORD outbuf[MAX_OUT],outpos=0,biggestout=0,klnclr;

void err(char *msg) {
  if(ifp) fclose(ifp);
  if(ofp) fclose(ofp);
  puts(msg);
  exit(1);
  }

void output(UWORD *dat,int cnt) {
  if(cnt==0) {
    if(outpos>biggestout)
      biggestout=outpos;
    if(outpos>MAX_OUT)
      printf("MAX_OUT blown, needed %d\n",outpos);
    else {
      fwrite(&outpos,2,1,ofp);
      fwrite(outbuf,2,outpos,ofp);
      }
    outpos=0;
    return;
    }
  if((outpos+cnt)<=MAX_OUT)
    memcpy(&outbuf[outpos],dat,cnt<<1);
  outpos+=cnt;
  }

int main(int argc,char *argv[]) {
  BITMAPFILEHEADER BMFH;
  BITMAPINFOHEADER BMIH;
  int x,y,n,bmprs;
  UWORD klnbuff[MAX_WIDTH],c;
  if(argc!=3)
    err("Usage: bmp2kln <bmpfile> <klnfile>");
  if((ifp=fopen(argv[1],"rb"))==NULL)
    err("Cannot open BMP");
  if(fread(&BMFH,sizeof(BITMAPFILEHEADER),1,ifp)!=1)
    err("Cannot read BITMAPFILEHEADER");
  if(BMFH.bfType!=BMP_TAG)
    err("BMP_TAG not found");
  if(fread(&BMIH,sizeof(BITMAPINFOHEADER),1,ifp)!=1)
    err("Cannot read BITMAPINFOHEADER");
  if((BMIH.biPlanes!=1)||(BMIH.biBitCount!=24))
    err("Invalid BMP - must be 24-bit");
  if((BMIH.biWidth>MAX_WIDTH)||(BMIH.biHeight>MAX_HEIGHT))
    err("Invalid BMP - too big, max " MAX_STRING);
  if(ftell(ifp)!=BMFH.bfOffBits)
    fseek(ifp,BMFH.bfOffBits,SEEK_SET);
  if((ofp=fopen(argv[2],"wb"))==NULL)
    err("Cannot open KLN");
  fwrite(KLN_TAG,1,16,ofp);
  fwrite(&BMIH.biWidth,4,2,ofp);
  bmprs=BMIH.biWidth*3;
  for(y=0;y<BMIH.biHeight-1;y++)
    fseek(ifp,bmprs,SEEK_CUR);
  for(y=0;y<BMIH.biHeight;y++) {
    for(x=0;x<BMIH.biWidth;x++) {
      fread(bmpbuf,3,1,ifp);
      klnbuff[x] = (
	((bmpbuf[2]&0xf8)<<8) |
	((bmpbuf[1]&0xfc)<<3) |
	((bmpbuf[0]&0xf8)>>3)
	);
      }
    fseek(ifp,-(bmprs*2),SEEK_CUR);
    /* pack and output KLN data */
    for(x=0;x<BMIH.biWidth;x++) {
      /* if at last byte, store literal (1) */
      if(x==BMIH.biWidth-1) {
	c=1;
	output(&c,1);
	output(&klnbuff[x],1);
	}
      else {
	/* if next byte is equal, found a run, else literal */
	if(klnbuff[x]==klnbuff[x+1]) {
	  /* find end of run */
	  for(n=x+2;n<BMIH.biWidth;n++)
	    if(klnbuff[x]!=klnbuff[n]) break;
	  /* run is upto but not including 'n' */
	  c=0x8000|(n-x);
	  output(&c,1);
	  output(&klnbuff[x],1);
	  x=n-1;
	  }
	else {
	  /* find end of literal */
	  for(n=x+1;n<(BMIH.biWidth-1);n++)
	    if(klnbuff[n]==klnbuff[n+1]) break;
	  /* if at end, include last byte in literal */
	  if(n==BMIH.biWidth-1)
	    n++;
	  /* literal is upto but not including 'n' */
	  c=n-x;
	  output(&c,1);
	  output(&klnbuff[x],c);
	  x=n-1;
	  }
	}
      }
    output(NULL,0);
    }
  fclose(ifp);
  fclose(ofp);
  printf("BIGGEST=%d\n",biggestout);
  return(0);
  }

/* end */
