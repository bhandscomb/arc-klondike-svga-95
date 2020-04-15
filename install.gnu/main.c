/*
#############################################################################
#
# main.c
#
#
# Main module for Klondike installer
#
#
# DOS INTERRUPTS USED
#
# --- various ---
#
#############################################################################
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
//#include <dos.h>

#include <dpmi.h>
#include <go32.h>
#include <sys\movedata.h>
#include <sys\segments.h>
#include <sys\stat.h>

#include "mytypes.h"
#include "keycodes.h"
#include "colours.h"
#include "protos.h"


/* video memory segment */
#define VIDSEG 0xb800

/* shorthand video call */
#define INTVID __dpmi_int(0x10,&regs)


/* product name (with " Installer" tacked on) */
char InstallerName[60];

/* queue basename */
char QueueBase[10];

/* Windows 95 flag string */
char Win95Flag[2];

/* long file name flag */
int LFNflag;

/* queue file */
FILE *qfp=NULL;

/* destination dir */
char DestDir[80];

/* screen stuff */
UBYTE OldMode;
UWORD CurShape;

/* viewer buffer pointer */
char *viewbuff;

/* install flag */
int DoInstall=0;

/* useful horizontal lines (80 chars each) */
char HORIZSINGLE[] = \
  "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ" \
  "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ";
char HORIZDOUBLE[] = \
  "ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ" \
  "ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";

/* temporary string */
char tmpstr[82];


/*
#############################################################################
#
# kill unwanted DJGPP startup features
#
#############################################################################
*/

char **__crt0_glob_function(char *_argument) {
  return(0);
  };
void __crt0_load_environment_file(char *appname) {
  };
void __crt0_setup_arguments(void) {
  };


/*
#############################################################################
#
# poke()
#
# DJGPP support
#
#############################################################################
*/

void poke (UWORD seg,UWORD off,UWORD dat) {
  movedata(_my_ds(),(int)&dat,_dos_ds,(seg<<4)+off,2);
  }


/*
#############################################################################
#
# GetKeyboard()
#
# Keyboard reader
#
#############################################################################
*/

UWORD GetKeyboard(void) {
  __dpmi_regs regs;
  regs.h.ah=0x00;
  __dpmi_int(0x16,&regs);
  if(regs.h.al==0x00)
    return(0x100|regs.h.ah);
  else
    return(regs.h.al);
  }


/*
#############################################################################
#
# Cursor()
# Locate()
#
# Cursor manipulation
#
#############################################################################
*/

void Cursor(int flag) {
  __dpmi_regs regs;
  regs.h.ah=0x01;
  if(flag==2)
    regs.x.cx=CurShape&0x00ff;
  else if(flag==1)
    regs.x.cx=CurShape;
  else
    regs.x.cx=0x2000;
  INTVID;
  }

void Locate(UBYTE y,UBYTE x) {
  __dpmi_regs regs;
  regs.h.ah=0x02;
  regs.h.bh=0;
  regs.h.dl=x;
  regs.h.dh=y;
  INTVID;
  }


/*
#############################################################################
#
# vputs()
#
# Video 'puts'
#
#############################################################################
*/

void vputs(char *string, int ypos, int xpos, UBYTE colour) {
  UWORD offset,attr;
  unsigned char *ptr;
  for (
    attr=colour<<8, ptr=string, offset=(ypos*160)+xpos+xpos;
    *ptr;
    ptr++, offset+=2
    )
    poke(VIDSEG,offset,attr+(*ptr));
  }


/*
#############################################################################
#
# ScreenInit()
#
# Initializes screen (set mode, cursor off, clear, display banner)
#
#############################################################################
*/

void ScreenInit(void) {

  __dpmi_regs regs;
  UWORD offset;

  regs.h.ah=0x0f;
  INTVID;
  OldMode=regs.h.al;

  regs.x.ax=0x0003;
  INTVID;

  regs.h.ah=0x03;
  regs.h.bh=0x00;
  INTVID;
  CurShape=regs.x.cx;

  Cursor(0);

  for(offset=0;offset<4000;offset+=2)
    poke(VIDSEG,offset,BACKGROUND<<8|0x20);

  sprintf(tmpstr,"É%.76s»",HORIZDOUBLE);
  vputs(tmpstr,0,1,BANNER);
  sprintf(tmpstr,"º %-51s(C) 1996 The 81st Track º",InstallerName);
  vputs(tmpstr,1,1,BANNER);
  sprintf(tmpstr,"È%.76s¼",HORIZDOUBLE);
  vputs(tmpstr,2,1,BANNER);

  }


/*
#############################################################################
#
# ScreenCleanup()
#
# Set screen back to original mode
#
#############################################################################
*/

void ScreenCleanup(void) {
  __dpmi_regs regs;
  regs.x.ax=OldMode;
  INTVID;
  }


/*
#############################################################################
#
# ClrMain()
#
# Clear main screen (not banner)
#
#############################################################################
*/

void ClrMain(void) {
  UWORD offset;
  for(offset=480;offset<4000;offset+=2)
    poke(VIDSEG,offset,BACKGROUND<<8|0x20);
  }


/*
#############################################################################
#
# ViewText()
#
# Text viewer
#
#############################################################################
*/

void ViewText(void) {
  int n,m;
  char *txtptr;
  UWORD key=0;
  ClrMain();
  txtptr=viewbuff;
  sprintf(tmpstr,"Ú%.78s¿",HORIZSINGLE);
  vputs(tmpstr,3,0,TEXTBOX);
  for(n=4;n<24;n++) {
    sprintf(tmpstr,"³%78s³","");
    vputs(tmpstr,n,0,TEXTBOX);
    if(*txtptr) {
      for(m=0;m<78;m++) {
	if(*txtptr=='\r') {
	  txtptr+=2;
	  tmpstr[m]='\0';
	  break;
	  }
	if(*txtptr=='\032')
	  *txtptr='\0';
	if(*txtptr=='\0') {
	  tmpstr[m]='\0';
	  break;
	  }
	tmpstr[m]=*txtptr++;
	}
      vputs(tmpstr,n,1,TEXTTEXT);
      }
    }
  sprintf(tmpstr,"À%.78sÙ",HORIZSINGLE);
  vputs(tmpstr,24,0,TEXTBOX);
  while((key!=ENTER)&&(key!=ESCAPE))
    key=GetKeyboard();
  }


/*
#############################################################################
#
# ProgressInit()
# ProgressBar()
#
# Progress bar display
#
#############################################################################
*/

void ProgressInit(char *name) {
  ClrMain();
  sprintf(tmpstr,"ÚÄ %s %.*s¿",name,(int)(59-strlen(name)),HORIZSINGLE);
  vputs(tmpstr,12,8,PROGRESSBOX);
  sprintf(tmpstr,"³%62s³","");
  vputs(tmpstr,13,8,PROGRESSBOX);
  sprintf(tmpstr,"À%.62sÙ",HORIZSINGLE);
  vputs(tmpstr,14,8,PROGRESSBOX);
  }

void ProgressBar(int sofar,int total) {
  int sofarwidth,n;
  sofarwidth=(60*sofar)/total;
  for(n=0;n<sofarwidth;n++)
    vputs("Û",13,10+n,PROGRESSBAR);
  for(n=sofarwidth;n<60;n++)
    vputs(" ",13,10+n,PROGRESSBAR);
  }


/*
#############################################################################
#
# OpenFile()
#
# As per fopen(), but we'll make directories as we go
#
#############################################################################
*/

FILE *OpenFile(char *name,char *mode) {

  char dirname[260],*n,*d;

  /* initialise pointers */

  n=name;
  d=dirname;

  /* start copying name */

  while(*n) {

    /* copy a char */

    *d++=*n++;

    /* make specified directory if at backslash */

    if(*n=='\\') {
      *d='\0';
      mkdir(dirname,S_IRWXU|S_IRWXG|S_IRWXO);
      }
    }

  /* now we act like fopen() */

  return(fopen(name,mode));

  } /* OpenFile() */


/*
#############################################################################
#
# InstallFiles()
#
# File installation - files
#
#############################################################################
*/

void InstallFiles(char *FilesTag,int queue) {

  int n,items,inqueue,failboxwidth,dcerr=0;
  char Tag[20],filestr[260],destfile[260],destfile95[260];
  char *qpos,*dir,*file,*file95;
  FILE *ofp;
  UWORD key;

  /* get number of files in this group */

  strcpy(Tag,FilesTag);
  strcat(Tag,"_nn");
  ConfigGet(Tag,filestr);
  items=atoi(filestr);

  /* process each file */

  for(n=1;n<=items;n++) {

    /* get & parse file item */

    sprintf(Tag,"%s_%02d",FilesTag,n);
    ConfigGet(Tag,filestr);
    for(qpos=filestr;*qpos!=',';qpos++) ;
    *qpos++='\0';
    for(dir=qpos;*dir!=',';dir++) ;
    *dir++='\0';
    for(file=dir;*file!=',';file++) ;
    *file++='\0';
    for(file95=file;*file95!=',';file95++) ;
    *file95++='\0';
    if(*file95=='.')
      file95=file;
    inqueue=atoi(filestr);

    /* if file in current queue, we can install it */

    if(inqueue==queue) {

      /* seek to position in queue */

      fseek(qfp,atol(qpos),SEEK_SET);

      /* form fully qualified file name */

      if(dir[0]=='.') {
	sprintf(destfile,"%s\\%s",DestDir,file);
	sprintf(destfile95,"%s\\%s",DestDir,file95);
	}
      else {
	sprintf(destfile,"%s\\%s\\%s",DestDir,dir,file);
	sprintf(destfile95,"%s\\%s\\%s",DestDir,dir,file95);
	}

      /* setup progress bar */

      if((LFNflag==1)&&(file!=file95))
	ProgressInit(file95);
      else
	ProgressInit(file);

      /* open output file - abort if fail */

      if((ofp=OpenFile(destfile,"wb"))!=NULL) {

	/* opened okay, decode file - abort if fail */

        if((dcerr=decodefile(qfp,ofp))<0)
	  DoInstall=0;
	fclose(ofp);

	/* rename to Win95 name if appropriate AND possible */

	if((LFNflag==1)&&(file!=file95)) {
	  LFNrename(destfile,destfile95);
	  }

	}

      else

	DoInstall=0;

      /* if we failed, tell user what file we failed with */

      if(DoInstall==0) {
        sprintf(tmpstr,"³ %d Error while installing %s ³",dcerr,file);
        failboxwidth=strlen(tmpstr);
	ClrMain();
	vputs(tmpstr,13,(80-failboxwidth)>>1,FAILBOX);
	sprintf(tmpstr,"Ú%.*s¿",failboxwidth-2,HORIZSINGLE);
        vputs(tmpstr,12,(80-failboxwidth)>>1,FAILBOX);
	sprintf(tmpstr,"À%.*sÙ",failboxwidth-2,HORIZSINGLE);
        vputs(tmpstr,14,(80-failboxwidth)>>1,FAILBOX);
	key=GetKeyboard();
	break;
	}

      } /* if(queue) */

    } /* for(n) */

  } /* InstallFiles() */


/*
#############################################################################
#
# DoFiles()
#
# File installation - config scanner
#
#############################################################################
*/

void DoFiles(char *MenuTag,int queue) {

  int n,items;
  char Tag[20],*ptr;

  /* get number of items in menu */

  strcpy(Tag,MenuTag);
  strcat(Tag,"_nn");
  ConfigGet(Tag,tmpstr);
  items=atoi(tmpstr);

  /* process each item */

  for(n=1;n<=items;n++) {

    /* get & parse item */

    sprintf(Tag,"%s_%02d",MenuTag,n);
    ConfigGet(Tag,tmpstr);
    for(ptr=tmpstr;*ptr!=',';ptr++) ;
    *ptr++='\0';
    strcpy(Tag,tmpstr);
    ConfigGet(Tag,tmpstr);

    /* if it's a menu, process (recursive) */

    if(strcmp(tmpstr,"MENU")==0) {
      DoFiles(Tag,queue);
      }

    /* if files, install them if they are wanted */

    if(strcmp(tmpstr,"FILES")==0) {

      /* get flag */

      strcat(Tag,"_ff");
      ConfigGet(Tag,tmpstr);

      /* install if '1' */

      if(tmpstr[0]=='1') {
	Tag[strlen(Tag)-3]='\0';
	InstallFiles(Tag,queue);
	if(DoInstall==0)
	  break;
	}

      } /* if */

    } /* for(n) */

  } /* DoFiles() */


/*
#############################################################################
#
# DispMenuItem()
#
# Display a menu item
#
#############################################################################
*/

void DispMenuItem(char *MenuTag,int item,UBYTE colour) {

  char Tag[20],itemstr[82],scrstr[82],*ptr;

  /* get menu item */

  sprintf(Tag,"%s_%02d",MenuTag,item);
  ConfigGet(Tag,itemstr);

  /* title or item? */

  if(item==0)

    vputs(itemstr,4,(80-strlen(itemstr))>>1,MENUTITLE);

  else {

    /* item - parse item string and find what sort of item it is */

    for(ptr=itemstr;*ptr!=',';ptr++) ;
    *ptr++='\0';
    sprintf(scrstr,"%-32s",ptr);
    vputs(scrstr,5+(item<<1),22,colour);
    strcpy(Tag,itemstr);
    ConfigGet(Tag,itemstr);

    /* if menu, mark with right chevron */

    if(strcmp(itemstr,"MENU")==0)
      vputs("¯",5+(item<<1),53,colour);

    /* if files, show whether install flag set or not */

    if(strcmp(itemstr,"FILES")==0) {
      strcat(Tag,"_ff");
      ConfigGet(Tag,itemstr);
      if(itemstr[0]=='1')
	vputs("Yes",5+(item<<1),55,MENUITEM);
      else
	vputs("No ",5+(item<<1),55,MENUITEM);
      }

    } /* else */

  } /* DispMenuItem() */


/*
#############################################################################
#
# DoMenu()
#
# Menu routine
#
#############################################################################
*/

void DoMenu(char *MenuTag) {

  int n,items,menuptr;
  char Tag[20],*ptr;
  UWORD key=0;

redraw:

  /* 'drop out' of any recursion if install started */

  if(DoInstall)
    return;

  /* clean screen and draw menu footer */

  ClrMain();

  sprintf (
    tmpstr,
    "  %-38s%38s  ",
    (strcmp(MenuTag,"MAIN")==0) ?
      "ESC: Abort Install" :
      "ESC: Previous Menu",
    "ENTER: Go To Sub Menu / Toggle Item"
    );
  vputs(tmpstr,23,0,FOOTER);
  sprintf(tmpstr,"%59s%21s","Press F10 To Begin Actual Installation","");
  vputs(tmpstr,24,0,FOOTER);

  /* get number of menuitems */

  strcpy(Tag,MenuTag);
  strcat(Tag,"_nn");
  ConfigGet(Tag,tmpstr);
  items=atoi(tmpstr);

  /* draw menu items */

  for(n=0;n<=items;n++)
    DispMenuItem(MenuTag,n,MENUITEM);

  /* draw initial menu selector */

  DispMenuItem(MenuTag,menuptr=1,MENUSELECT);

  /* run menu */

  while((key!=ESCAPE)&&(key!=F10)) {

    /* read keystroke */

    key=GetKeyboard();

    /* cursor movement */

    if((key==CURSOR_DOWN)&&(menuptr<items)) {
      DispMenuItem(MenuTag,menuptr,MENUITEM);
      menuptr++;
      DispMenuItem(MenuTag,menuptr,MENUSELECT);
      }

    if((key==CURSOR_UP)&&(menuptr>1)) {
      DispMenuItem(MenuTag,menuptr,MENUITEM);
      menuptr--;
      DispMenuItem(MenuTag,menuptr,MENUSELECT);
      }

    /* menu select key */

    if(key==ENTER) {

      /* get item type */

      sprintf(Tag,"%s_%02d",MenuTag,menuptr);
      ConfigGet(Tag,tmpstr);
      for(ptr=tmpstr;*ptr!=',';ptr++) ;
      *ptr++='\0';
      strcpy(Tag,tmpstr);
      ConfigGet(Tag,tmpstr);

      /* if menu, do menu (recursive) */

      if(strcmp(tmpstr,"MENU")==0) {
	DoMenu(Tag);
	goto redraw;
	}

      /* if files, toggle install flag */

      if(strcmp(tmpstr,"FILES")==0) {
	strcat(Tag,"_ff");
	ConfigGet(Tag,tmpstr);
	tmpstr[0]=(tmpstr[0]=='1')?'0':'1';
	ConfigPut(Tag,tmpstr);
	DispMenuItem(MenuTag,menuptr,MENUSELECT);
	}

      /* if view, view file */

      if(strcmp(tmpstr,"VIEW")==0) {
	strcat(Tag,"_01");
	ConfigGet(Tag,tmpstr);
	fseek(qfp,atol(tmpstr),SEEK_SET);
        if(decodefile(qfp,NULL)>=0) {
	  ViewText();
	  goto redraw;
	  }
	}

      }

    } /* while */

  if(key==F10)
    DoInstall=1;

  } /* DoMenu() */


/*
#############################################################################
#
# AskWhere()
#
# Ask user where he wants to install
#
#############################################################################
*/

void AskWhere() {

  UWORD key=0;
  int length,curpos,ins=1,n,okay=0;

  /* repeat asking until responded to okay */

  while((!okay)&&(DoInstall==1)) {

    /* clean screen and draw destination input box */

    ClrMain();

    sprintf(tmpstr,"ÚÄ Please select destination %.30s¿",HORIZSINGLE);
    vputs(tmpstr,12,10,WHEREBOX);
    sprintf(tmpstr,"³%58s³","");
    vputs(tmpstr,13,10,WHEREBOX);
    sprintf(tmpstr,"À%.58sÙ",HORIZSINGLE);
    vputs(tmpstr,14,10,WHEREBOX);

    /* initialise input string and cursor */

    sprintf(tmpstr,"%-56s",DestDir);
    curpos=length=strlen(DestDir);
    Cursor(1);

    /* process keystrokes until user done or aborts */

    while((key!=ESCAPE)&&(key!=ENTER)) {

      /* display current string and set cursor position */

      vputs(tmpstr,13,12,WHERETEXT);
      Locate(13,(UBYTE)(12+curpos));

      /* read keyboard */

      key=GetKeyboard();

      /* cursor positioning */

      if((key==CURSOR_LEFT)&&(curpos>0))
	curpos--;

      if((key==CURSOR_RIGHT)&&(curpos<length))
	curpos++;

      if(key==HOME)
	curpos=0;

      if(key==END)
	curpos=length;

      /* insert/overtype mode */

      if(key==INSERT)
	Cursor(2-(ins=1-ins));

      /* delete/backspace keys */

      if((key==BACKSPACE)&&(curpos>0)) {
	curpos--;
	key=DELETE;
	}

      if((key==DELETE)&&(curpos!=length)) {
	for(n=curpos;n<length;n++)
	  tmpstr[n]=tmpstr[n+1];
	length--;
	}

      /* ASCII keystrokes */

      if((key>=0x20)&&(key<=0x7e)&&(length<55)) {
	if(ins) {
	  /* insert mode, shift anything to right of cursor */
	  for(n=length;n>curpos;n--)
	    tmpstr[n]=tmpstr[n-1];
	  length++;
	  }
	tmpstr[curpos]=key;
	/* advance cursor only if not at end */
	if(curpos<56)
	  curpos++;
	}

      } /* while */

    /* cancel install if abort, else trim spaces from input string */

    if(key==ESCAPE)
      DoInstall=0;
    else
      sprintf(DestDir,"%.*s",length,tmpstr);

    /* make sure ':' is second character (i.e. got a drive) */

    if(DestDir[1]==':')
      okay=1;

    } /* while */

  /* cursor off */

  Cursor(0);

  } /* AskWhere() */


/*
#############################################################################
#
# GetQueue()
#
# Get queue file
#
#############################################################################
*/

void GetQueue(int q) {

  char qname[14];
  UWORD key;

  /* close old queue (if we've got one) */

  if(qfp) {
    fclose(qfp);
    qfp=NULL;
    }

  /* form name of queue file */

  sprintf(qname,"%s.Q%02d",QueueBase,q);

  /* continue asking for disk with queue until got it or user aborts */

  while(((qfp=fopen(qname,"rb"))==NULL)&&(DoInstall)) {

    ClrMain();

    sprintf(tmpstr,"Ú%.22s¿",HORIZSINGLE);
    vputs(tmpstr,12,28,QUEUEBOX);
    sprintf(tmpstr,"³ Please insert disk %d ³",q);
    vputs(tmpstr,13,28,QUEUEBOX);
    sprintf(tmpstr,"À%.22sÙ",HORIZSINGLE);
    vputs(tmpstr,14,28,QUEUEBOX);

    key=GetKeyboard();
    if(key==ESCAPE)
      DoInstall=0;

    } /* while */

  } /* GetQueue() */


/*
#############################################################################
#
# SuggestW95()
#
# If got Win95 and not running windows, suggest run within windows
#
#############################################################################
*/

void SuggestW95(void) {
  UWORD key=0;
  if(CheckWin95()==1) {
    if(CheckWinRunning()>=4)
      return;
    ClrMain();
    sprintf(tmpstr,"Ú%.57s¿",HORIZSINGLE);
    vputs(tmpstr,11,10,W95BOX);
    vputs("³ It is suggested that you run this installer from within ³",
      12,10,W95BOX);
    vputs("³ Windows 95, else some minor functionality will be lost. ³",
      13,10,W95BOX);
    vputs("³    Press ENTER to continue anyway or ESCAPE to abort    ³",
      14,10,W95BOX);
    sprintf(tmpstr,"À%.57sÙ",HORIZSINGLE);
    vputs(tmpstr,15,10,W95BOX);
    while((key!=ENTER)&&(key!=ESCAPE))
      key=GetKeyboard();
    if(key==ESCAPE)
      Win95Flag[0]='-';
    }
  }


/*
#############################################################################
#
# NeedW95()
#
# If not got Win95, tell user it's required
#
#############################################################################
*/

void NeedW95(void) {
  UWORD key;
  if(CheckWin95()==1)
    return;
  ClrMain();
  sprintf(tmpstr,"Ú%.52s¿",HORIZSINGLE);
  vputs(tmpstr,11,13,W95BOX);
  vputs("³ The program you are installing requires Windows 95 ³",
    12,13,W95BOX);
  vputs("³               Press any key to abort               ³",
    13,13,W95BOX);
  sprintf(tmpstr,"À%.52sÙ",HORIZSINGLE);
  vputs(tmpstr,14,13,W95BOX);
  key=GetKeyboard();
  Win95Flag[0]='-';
  }


/*
#############################################################################
#
# main()
#
# Main function for the installer
#
#############################################################################
*/

int main(void) {

  int n,queues;
  char DriveRoot[4];

  /* read config file and check for 'MAIN=MENU' */

  ConfigRead();
  ConfigGet("MAIN",tmpstr);
  if(strcmp(tmpstr,"MENU")!=0) {

    /* not a valid install config file - complain and exit */

    ConfigFlush();
    puts (
      "Please run installer from within install directory\n"
      "example:    C:\\>A:\n"
      "            A:\\>INSTALL"
      );
    exit(1);

    } /* if */

  /* get some required bits and setup screen */

  ConfigGet("TITLE",InstallerName);
  ConfigGet("QUEUEBASE",QueueBase);
  ConfigGet("WIN95",Win95Flag);
  strcat(InstallerName," Installer");
  ScreenInit();

  /* handle "WIN95" flag */

  switch(Win95Flag[0]) {
    case '1':
      SuggestW95();
      break;
    case '2':
      NeedW95();
      break;
    case '3':
      NeedW95();
      if(Win95Flag[0]!='-')
	SuggestW95();
      break;
    }

  if(Win95Flag[0]=='-') {
    ScreenCleanup();
    ConfigFlush();
    puts("Installation aborted");
    return(0);
    }

  /* get first queue */

  GetQueue(1);

  /* run menu system (if got queue) - this really is it! */

  if(qfp)
    DoMenu("MAIN");
  else
    DoInstall=0;

  /* only proceed if the user wants to install */

  if(DoInstall) {

    /* ask where to install */

    ConfigGet("DEST",DestDir);
    AskWhere();

    /* set long file name flag ref destination drive */

    sprintf(DriveRoot,"%c:\\",DestDir[0]);
    LFNflag=CheckVolumeLFN(DriveRoot);

    /* if still wants to install... */

    if(DoInstall) {

      /* get number of 'queues' */

      ConfigGet("QUEUES",tmpstr);
      queues=atoi(tmpstr);

      for(n=1;n<=queues;n++) {

	/* get queue */

	GetQueue(n);
	if(DoInstall==0)
	  break; /* out of for(n) */

	/* do files in this queue */

	DoFiles("MAIN",n);
	if(DoInstall==0)
	  break; /* out of for(n) */

	} /* for(n) */

      } /* if(DoInstall) */

    } /* if(DoInstall) */

  /* close queue if we've got one open */

  if(qfp)
    fclose(qfp);

  /* clean up screen */

  ScreenCleanup();

  /* user gratification message :-) */

  if(DoInstall)
    puts("Installation complete");
  else
    puts("Installation aborted");

  /* no need for config file now */

  ConfigFlush();

  /* bye! */

  return(0);

  } /* main() */
