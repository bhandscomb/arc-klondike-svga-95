/* mq.c */

#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE 32768

FILE *ifp=NULL,*ofp=NULL;
char buff[BUFFSIZE];

void err(char *msg) {
  if(ifp) fclose(ifp);
  if(ofp) fclose(ofp);
  puts(msg);
  exit(1);
  }

int main(int argc,char *argv[]) {
  int blocklen;
  char filename[80];
  if(argc<3)
    err("Usage: mq <queuename> <filename> [filename...]");
  if((ofp=fopen(argv[1],"wb"))==NULL)
    err("Cannot open queue file");
  for(;;) {
    if(argc==2)
      break;
    strcpy(filename,argv[2]);
    printf("%s at %ld\n",filename,ftell(ofp));
    if((ifp=fopen(filename,"rb"))==NULL)
      err("Cannot open input file");
    for(;;) {
      blocklen=fread(buff,1,BUFFSIZE,ifp);
      if(blocklen==0)
	break;
      if(fwrite(buff,1,blocklen,ofp)!=blocklen)
	err("Error writing to queue");
      }
    fclose(ifp);
    argv++;
    argc--;
    }
  fclose(ofp);
  return(0);
  }
