/*
#############################################################################
#
# music.c
#
# Music code for Klondike 95
#
#############################################################################
*/


#include <stdio.h>
#include <stdlib.h>

#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#include <dos.h>

#include "mytypes.h"
#include "sb_lib.h"
#include "music.h"
#include "protos.h"


volatile UBYTE *mdatptr;

volatile int playing=0,stopped=1,validfile=0;
volatile ULONG numevents,nextevent=0;

UBYTE *buff=NULL;

int FirstCdDrive=0;
UBYTE VTOC[2048];

sb_mod_file *modfile=NULL;

int midimode=0,cdmode=0,modmode=0;


/*
*****************************************************************************
* GETWORD/GETLONG
*****************************************************************************
*/

UWORD getword(void) {
  register UWORD tmp;
  tmp=*mdatptr++;
  return(tmp<<8|(*mdatptr++));
  }

ULONG getlong(void) {
  register ULONG tmp;
  tmp=getword();
  return(tmp<<16|getword());
  }


/*
*****************************************************************************
* MIDI STUFF
*****************************************************************************
*/

UWORD midi_base=0x330;
int midi_open=0;

#define DATAPORT (midi_base)
#define COMDPORT (midi_base+1)
#define STATPORT (midi_base+1)

#define OUTPUT_READY 0x40
#define INPUT_AVAIL  0x80

#define MPU_ACK      0xFE
#define MPU_RESET    0xFF
#define UART_MODE_ON 0x3F

#define midi_status()    inportb(STATPORT)
#define midi_inavail()   (!(midi_status()&INPUT_AVAIL))
#define midi_outready()  (!(midi_status()&OUTPUT_READY))
#define midi_cmd(cmd)    outportb(COMDPORT,cmd)
#define midi_read()      inportb(DATAPORT)
#define midi_write(byte) outportb(DATAPORT,byte)

void midi_flushin() {
  register UBYTE c;
  while(midi_inavail())
    c=midi_read();
  }

void midi_init(void) {
  int n,timeout,ok;
  if(midi_open)
    return;
  /*BEGIN:reset_sb16midi()*/
  midi_flushin();
  ok=0;
  for(n=0;n<2&&!ok;n++) {
    for(timeout=30000;timeout>0&&!midi_outready();timeout--)
      /* nothing */;
    if(!midi_outready())
      return;
    midi_cmd(MPU_RESET);
    for(timeout=50000;timeout>0&&!ok;timeout--)
      if(midi_inavail())
        if(midi_read()==MPU_ACK)
          ok=1;
    }
  if(!ok)
    return;
  /*END:reset_sb16midi()*/
  /*BEGIN:attach_sb16midi()*/
  midi_flushin();
  for(timeout=30000;timeout>0&&!midi_outready();timeout--)
    /* nothing */;
  midi_cmd(UART_MODE_ON);
  ok=0;
  for(timeout=50000;timeout>0&&!ok;timeout--)
    if(midi_inavail())
      if(midi_read()==MPU_ACK)
        ok=1;
  /*END:attach_sb16midi()*/
  if(ok)
    midi_open=1;
  }

void midi_close(void) {
  midi_open=0;
  }

void midi_output(unsigned char byte) {
  register int timeout;
  midi_flushin();
  for(timeout=30000;timeout>0&&!midi_outready();timeout--)
    /* nothing */;
  if(!midi_outready())
    return;
  midi_write(byte);
  }


/*
*****************************************************************************
* TIMER STUFF
*****************************************************************************
*/

#define RTCPORT_ADDR   0x70
#define RTCPORT_DATA   0x71

#define INT_IRQ8       0x70

/* "Normal" Interrupt Rate: 1024Hz  6, 977,100,100 */
/* "Half"                    512Hz  7,1953,100, 50 */
/* "Quarter"                 256Hz  8,3906,100, 25 */
#define TICRATE           8
#define TICMICROS      3906
#define TICTESTDELAY    100
#define TICTESTVALUE     25

#define SMALLDELAY() {int n;for(n=0;n<100;n++);}

_go32_dpmi_seginfo oldintirq8,newintirq8;
UBYTE oldrtca,oldrtcb,oldporta1,newrtca,newrtcb,newporta1;

volatile int tics=0;
volatile ULONG micros=0;

int timerokay=0;

UBYTE rtcread(UBYTE addr) {
  outportb(RTCPORT_ADDR,addr);
  SMALLDELAY();
  return(inportb(RTCPORT_DATA));
  }

void rtcwrite(UBYTE addr,UBYTE data) {
  outportb(RTCPORT_ADDR,addr);
  SMALLDELAY();
  outportb(RTCPORT_DATA,data);
  SMALLDELAY();
  }

void tic_handler(_go32_dpmi_registers *r) {
  UBYTE statc;
  UWORD eventlen;
  statc=rtcread(0x0c);
  if(statc&0x40) {
    if(playing) {
      micros+=TICMICROS;
      if(micros>=nextevent) {
        eventlen=getword();
        while(eventlen--)
          midi_output(*mdatptr++);
        if(--numevents)
          nextevent+=getlong();
        else
          playing=0;
        }
      }
    else
      tics++;
    }
  }

void setregs(void) {
  rtcwrite(0x0a,newrtca);
  rtcwrite(0x0b,newrtcb);
  outportb(0xa1,newporta1);
  }

int setuptimer(void) {
  int earlytics;
  _go32_dpmi_get_protected_mode_interrupt_vector(INT_IRQ8,&oldintirq8);
  newintirq8.pm_offset=(int)tic_handler;
  newintirq8.pm_selector=_go32_my_cs();
  _go32_dpmi_chain_protected_mode_interrupt_vector(INT_IRQ8,&newintirq8);
  oldrtca=rtcread(0x0a);
  oldrtcb=rtcread(0x0b);
  oldporta1=inportb(0xa1);
  newrtca=(oldrtca&0xf0)|TICRATE;
  newrtcb=oldrtcb|0x40;
  newporta1=oldporta1&0xfe;
  setregs();
  delay(TICTESTDELAY);
  earlytics=tics;
  if(earlytics<TICTESTVALUE)
    return(0);
  timerokay=1;
  return(1);
  }

void unsetuptimer(void) {
  rtcwrite(0x0a,oldrtca);
  rtcwrite(0x0b,oldrtcb);
  outportb(0xa1,oldporta1);
  _go32_dpmi_set_protected_mode_interrupt_vector(INT_IRQ8,&oldintirq8);
  }


/*
*****************************************************************************
* MAIN CODE (MIDI)
*****************************************************************************
*/

void ResetXG(void) {
  int n;
  UBYTE xgreset[]={0xf0,0x43,0x10,0x4c,0x00,0x00,0x7e,0x00,0xf7};
  for(n=0;n<9;n++)
    midi_output(xgreset[n]);
  delay(50);
  }

int MidiMusicInit(void) {
  char TMPstr[200];
  unsigned int midiport;
  ConfigGet("MIDIPORT",TMPstr);
  if(TMPstr[0]!='\0') {
    sscanf(TMPstr,"%x",&midiport);
    midi_base=(UWORD)midiport;
    }
  midi_init();
  if(!midi_open)
    return(MUSIC_NOMIDI);
  if(!setuptimer()) {
    midi_close();
    return(MUSIC_NOTIMER);
    }
  ResetXG();
  midimode=1;
  return(MUSIC_OKAY);
  }

void MidiMusicFree(void) {
  if(buff) {
    free(buff);
    buff=NULL;
    validfile=0;
    }
  }

int MidiMusicLoad(char *filename) {
  FILE *fp;
  long bufflen;
  MidiMusicFree();
  if((fp=fopen(filename,"rb"))==NULL)
    return(MUSIC_NOFILE);
  fseek(fp,0,SEEK_END);
  if((buff=malloc(bufflen=ftell(fp)))==NULL) {
    fclose(fp);
    return(MUSIC_NOMEM);
    }
  fseek(fp,0,SEEK_SET);
  fread(buff,1,bufflen,fp);
  fclose(fp);
  mdatptr=buff;
  if(getlong()!=0x4d505832) {
    free(buff);
    buff=NULL;
    return(MUSIC_BADFILE);
    }
  validfile=1;
  return(MUSIC_OKAY);
  }

int MidiMusicPlay(void) {
  if(validfile) {
    mdatptr=buff+4;
    numevents=getlong();
    nextevent=getlong();
    micros=0;
    playing=1;
    stopped=0;
    return(MUSIC_OKAY);
    }
  else
    return(MUSIC_NOFILELOADED);
  }

void MidiMusicStop(void) {
  stopped=1;
  playing=0;
  mdatptr=buff+4;
  numevents=getlong();
  nextevent=getlong();
  micros=0;
  ResetXG();
  }

void MidiMusicContinue(void) {
  if((validfile)&&(!playing)&&(!stopped)) {
    mdatptr=buff+4;
    numevents=getlong();
    nextevent=getlong();
    micros=0;
    playing=1;
    }
  setregs();
  }

void MidiMusicCleanUp(void) {
  midimode=0;
  if(midi_open) {
    stopped=1;
    playing=0;
    ResetXG();
    midi_close();
    }
  if(timerokay) {
    unsetuptimer();
    }
  MidiMusicFree();
  }

void MidiMusicOneShot(UBYTE *ptr) {
  mdatptr=ptr+4;
  numevents=getlong();
  nextevent=getlong();
  micros=0;
  playing=1;
  stopped=0;
  }


/*
*****************************************************************************
* MAIN CODE (CD)
*****************************************************************************
*/


void CdRequest(int req) {
#if 0
  #define unaligned __attribute__ ((packed))
  typedef struct dosptr {
    UWORD dpoff;
    UWORD dpseg;
    } unaligned DOSPTR;
  typedef struct devicerequest {
    UBYTE reqlen;
    UBYTE subunit;
    UBYTE cmd;
    UWORD stat;
    UBYTE reserved[8];
    /* following at +0Dh */
    union {
      struct ioctlinput {
        UBYTE mediadescriptor;
        DOSPTR ioctlxfer;
        } unaligned ioctlin;
      struct playaudiorequest {
        UBYTE addrmode;
        ULONG start;
        ULONG length;
        } unaligned aud;
      };
    } unaligned DEVREQ;
  typedef union devicerequestdata {
    struct ioctlinputdata {
      UBYTE subfunc;
      union {
        DOSPTR devhdr;
        struct {
          UBYTE format;
          ULONG position;
          } unaligned readpos;
        };
      } unaligned ioctldat;
    } unaligned DEVREQDAT;

  __dpmi_regs regs;
  int DOSSEG,DOSSEL,rc;

  DOSSEG=__dpmi_allocate_dos_memory (
    (sizeof(struct devicerequest)+15)>>4,&DOSSEL
    );
  if(DOSSEG==-1)
    return;

  regs.x.ax=0x1510;
  regs.x.cx=FirstCdDrive;
  regs.x.es=DOSSEG;
  regs.x.bx=0;
#endif
  }

int CdMusicInit(void) {
#if 0
  __dpmi_regs regs;
  regs.x.ax=0x1500;
  regs.x.bx=0;
  __dpmi_int(0x2f,&regs);
  if(regs.x.bx==0)
    return(MUSIC_NOCD);
  FirstCdDrive=regs.x.cx;
  regs.x.ax=0x150c;
  regs.x.bx=0;
  __dpmi_int(0x2f,&regs);
  if(regs.h.bh<2)
    return(MUSIC_BADMSCDEX);
  if((regs.h.bh==2)&&(regs.h.bl<10))
    return(MUSIC_BADMSCDEX);
  cdmode=1;
#endif
  return(MUSIC_OKAY);
  }

int CdMusicPlay(char *type) {
  return(MUSIC_OKAY);
  }

void CdMusicStop(void) {
  }

void CdMusicContinue(void) {
  }

void CdMusicCleanUp(void) {
  cdmode=0;
  }


/*
*****************************************************************************
* MAIN CODE (MOD)
*****************************************************************************
*/

int ModMusicInit(void) {
  sb_status rc;
  char TMPstr[200];
  int mixrate=44100;
  ConfigGet("MIXRATE",TMPstr);
  if(TMPstr[0]!='\0')
    mixrate=atoi(TMPstr);
  rc=sb_install_driver(mixrate);
  if(rc!=SB_SUCCESS)
    return(MUSIC_SBDRVFAIL);
  modmode=1;
  return(MUSIC_OKAY);
  }

void ModMusicFree(void) {
  if(modfile) {
    sb_free_mod_file(modfile);
    modfile=NULL;
    validfile=0;
    }
  }

int ModMusicLoad(char *filename) {
  ModMusicFree();
  if((modfile=sb_load_mod_file(filename))==NULL)
    return(MUSIC_NOFILE);
  validfile=1;
  return(MUSIC_OKAY);
  }

int ModMusicPlay(void) {
  if(validfile) {
    sb_mod_play(modfile);
    stopped=0;
    return(MUSIC_OKAY);
    }
  else
    return(MUSIC_NOFILELOADED);
  }

void ModMusicStop(void) {
  stopped=1;
  sb_mod_pause();
  }

void ModMusicContinue(void) {
  if((validfile)&&(!sb_mod_active)&&(!stopped)) {
    sb_mod_play(modfile);
    }
  }

void ModMusicCleanUp(void) {
  ModMusicStop();
  ModMusicFree();
  sb_uninstall_driver();
  modmode=0;
  }


/*
*****************************************************************************
* MAIN CODE (COMMON)
*****************************************************************************
*/

void MusicContinue(void) {
  if(midimode)
    MidiMusicContinue();
  if(cdmode)
    CdMusicContinue();
  if(modmode)
    ModMusicContinue();
  }
