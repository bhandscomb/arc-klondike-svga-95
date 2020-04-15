/*
#############################################################################
#
# errmsg.c
#
# Error message code for Klondike 95
#
#############################################################################
*/


#define ERRMSG_C


/* includes */

#include <stdio.h>
#include <stdlib.h>

#include "mytypes.h"
#include "vesa.h"
#include "reko.h"
#include "mouse.h"
#include "errmsg.h"


/*
#############################################################################
#
# WhatError()
#
# Translate error codes into meaningful messages (provided within module
# that reports the error).
#
#############################################################################
*/

void WhatError(UWORD where,WORD rc) {

  char nullmsg[]="?",*msg=nullmsg;

  switch(where) {
    case ERR_IN_VESA:
      msg=VesaErrorMessages[(-rc)-1];
      break;
    case ERR_IN_REKO:
      msg=RekoErrorMessages[(-rc)-1];
      break;
    case ERR_IN_MOUSE:
      msg=MouseErrorMessages[(-rc)-1];
      break;
    }

  printf(" => %s\n",msg);

  }
