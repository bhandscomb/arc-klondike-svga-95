/*
#############################################################################
#
# config.c
#
#
# Configuration file code for Klondike 95 / installer
#
#
# DOS INTERRUPTS USED
#
# --- none ---
#
#############################################################################
*/


/* includes */

#include <stdio.h>
#include <stdlib.h>

#include "files.h"


/* configuration entry structure */

typedef struct configstruct {
  struct configstruct *next;
  char name[20];
  char string[1];   /* variable length to save valuable heap space */
  } CONFIG;


/* configuration list */

CONFIG *confighead=NULL,*configtail=NULL;
int changed=0;


/*
#############################################################################
#
# ConfigRead()
#
# Read in configuration file
#
#############################################################################
*/

void ConfigRead(void) {
  FILE *fp;
  CONFIG *newconfig;
  char configline[272],*configstring;
  /* disallow multiple configuration file reads */
  if(confighead)
    return;
  /* read configuration file */
  if((fp=fopen(CONFIGFILENAME,"r"))==NULL)
    return;
  fgets(configline,271,fp);
  fgets(configline,271,fp);
  for(;;) {
    if(fgets(configline,271,fp)==NULL)
      break;
    configline[strlen(configline)-1]='\0';
    configstring=configline;
    while(*configstring) {
      if(*configstring=='=') {
	*configstring++='\0';
	break;
	}
      configstring++;
      }
    if(*configstring) {
      if((newconfig=malloc(sizeof(CONFIG)+strlen(configstring)))==NULL) {
	fclose(fp);
	return;
	}
      newconfig->next=NULL;
      strcpy(newconfig->name,configline);
      strcpy(newconfig->string,configstring);
      if(confighead) {
	configtail->next=newconfig;
	configtail=newconfig;
	}
      else {
	confighead=configtail=newconfig;
	}
      }
    }
  fclose(fp);
  }


/*
#############################################################################
#
# ConfigWrite()
#
# Write out configuration file
#
#############################################################################
*/

void ConfigWrite(void) {
  FILE *fp;
  CONFIG *item;
  /* disallow writes with no changed data or no data at all */
  if((!confighead)||(!changed))
    return;
  /* write configuratio file */
  if((fp=fopen(CONFIGFILENAME,"w"))==NULL)
    return;
  fprintf(fp,"MACHINE GENERATED - PLEASE DO NOT EDIT\n");
  fprintf(fp,"======================================\n");
  item=confighead;
  while(item) {
    fprintf(fp,"%s=%s\n",item->name,item->string);
    item=item->next;
    }
  fclose(fp);
  }


/*
#############################################################################
#
# ConfigFlush()
#
# Flush configuration from memory
#
#############################################################################
*/

void ConfigFlush(void) {
  CONFIG *item;
  /* disallow flushes with no data */
  if(!confighead)
    return;
  /* flush configuration */
  while(confighead) {
    item=confighead->next;
    free(confighead);
    confighead=item;
    }
  configtail=NULL;
  }


/*
#############################################################################
#
# ConfigGet()
#
# Get a configuration entry from the list
#
#############################################################################
*/

void ConfigGet(char *name,char *string) {
  CONFIG *item;
  /* find config entry and return */
  item=confighead;
  while(item) {
    if(!strcmp(item->name,name)) {
      strcpy(string,item->string);
      return;
      }
    item=item->next;
    }
  /* not found or no config list, return null string */
  *string='\0';
  }


/*
#############################################################################
#
# ConfigPut()
#
# Put a configuration entry into the list
#
#############################################################################
*/

void ConfigPut(char *name,char *string) {
  CONFIG *item,*upd,*prev=NULL;
  /* update existing entry (if present) */
  item=confighead;
  while(item) {
    if(!strcmp(item->name,name)) {
      if((upd=malloc(sizeof(CONFIG)+strlen(string)))==NULL)
	return;
      if(prev!=NULL)
	prev->next=upd;
      upd->next=item->next;
      strcpy(upd->name,name);
      strcpy(upd->string,string);
      if(item==confighead)
	confighead=upd;
      if(item==configtail)
	configtail=upd;
      free(item);
      changed=1;
      return;
      }
    prev=item;
    item=item->next;
    }
  /* not found or no config list, add */
  if((item=malloc(sizeof(CONFIG)+strlen(string)))==NULL)
    return;
  item->next=NULL;
  strcpy(item->name,name);
  strcpy(item->string,string);
  if(confighead) {
    configtail->next=item;
    configtail=item;
    }
  else {
    confighead=configtail=item;
    }
  changed=1;
  }
