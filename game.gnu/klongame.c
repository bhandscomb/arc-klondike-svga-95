/*
#############################################################################
#
# klongame.c
#
#
# Game engine for Klondike 95
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
#include <time.h>

#include "mytypes.h"
#include "game.h"
#include "text.h"
#include "vesa.h"
#include "protos.h"


/* various definitions */

#define FIRST_CLICK  0
#define SECOND_CLICK 1

/* from 1..7 = stack head (top), from -1..-7 = stack tail (bottom) */
#define FROM_DEAL    8
#define FROM_NOWHERE 9


/* screen click region definition */

typedef struct {
  WORD x,y;
  WORD w,h;
  void (*clickfunc[2])(UWORD,UWORD,WORD);
  WORD funcparam;
  } ClickRegion;


/* prefs click region definition */

typedef struct {
  WORD x1,y1;
  WORD x2,y2;
  void (*clickfunc)(WORD);
  WORD funcparam;
  } PrefsClickRegion;


/* from main module */

extern WORD quitflag;
//extern FILE *dfp;


/* game bits */

UBYTE ClickState=FIRST_CLICK,PrefCard;
WORD ClickFrom=FROM_NOWHERE,BorderFrom=FROM_NOWHERE;

/* The 7 stacks: 0 = no card, +ve = card face up, -ve = card face down */
BYTE Stack[7][20];

/* The 4 suits: 0 = no card */
BYTE Suit[4];

/* Current deck/deal stuff */
BYTE Deck[52];
WORD Dptr,Dsize,loops;

/* Border restoration buffer [x][y] */
UWORD BDRbuff[5][CARDWIDTH];

/* Border restoration bits */
UWORD BDRx,BDRy1,BDRy2;


/* screen click regions */

ClickRegion Regions[] = {
  {STACKX(0),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},1},
  {STACKX(1),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},2},
  {STACKX(2),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},3},
  {STACKX(3),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},4},
  {STACKX(4),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},5},
  {STACKX(5),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},6},
  {STACKX(6),POS_Y_STACK,CARDWIDTH,STACKHEIGHT,{StackClick1,StackClick2},7},
  {POS_X_DECK,POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{DeckClick,InvalidClick},0},
  {POS_X_DEAL,POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{DealClick1,DealClick2},0},
  {POS_X_PREF,POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{PrefClick,InvalidClick},0},
  {SUITX(0),POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{InvalidClick,SuitClick},1},
  {SUITX(1),POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{InvalidClick,SuitClick},2},
  {SUITX(2),POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{InvalidClick,SuitClick},3},
  {SUITX(3),POS_Y_SPECIAL,CARDWIDTH,CARDHEIGHT,{InvalidClick,SuitClick},4},
  {-1,-1,-1,-1,{NULL,NULL},-1}
  };


/* prefs click regions */

PrefsClickRegion Prefs0Regions[] = {
  {7,21,80,42,pAbout,0},        /* -> Prefs8 */
  {7,45,80,66,pPrefs,0},        /* -> Prefs1 */
  {7,69,80,90,pRestart,0},      /* quitflag=QUIT_RESTART [orig -> Prefs7] */
  {7,93,80,114,pQuit,0},        /* quitflag=QUIT_EXIT [orig -> Prefs7] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs1Regions[] = {
//{7,21,80,46,pResolution,0},   /* -> Prefs2 */
  {7,53,80,78,pCardset,0},      /* ChooseDeck() [orig -> Prefs3] */
//{7,85,80,110,pMisc,0},        /* -> Prefs4 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs2Regions[] = {
//{7,21,80,42,pResSelect,1},    /* -> Prefs0 */
//{7,45,80,66,pResSelect,2},    /* -> Prefs0 */
//{7,69,80,90,pResSelect,3},    /* -> Prefs0 */
//{7,93,80,114,pResSelect,4},   /* -> Prefs0 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 [orig -> Prefs1] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs3Regions[] = {
//{9,37,23,51,pSel,1},          /* -> Prefs0 */
//{9,53,23,67,pSel,2},          /* -> Prefs0 */
//{9,69,23,83,pSel,3},          /* -> Prefs0 */
//{9,85,23,99,pSel,4},          /* -> Prefs0 */
//{9,101,23,115,pSel,5},        /* -> Prefs0 */
//{27,37,41,51,pSel,6},         /* -> Prefs0 */
//{27,53,41,67,pSel,7},         /* -> Prefs0 */
//{27,69,41,83,pSel,8},         /* -> Prefs0 */
//{27,85,41,99,pSel,9},         /* -> Prefs0 */
//{27,101,41,115,pSel,10},      /* -> Prefs0 */
//{45,37,59,51,pSel,11},        /* -> Prefs0 */
//{45,53,59,67,pSel,12},        /* -> Prefs0 */
//{45,69,59,83,pSel,13},        /* -> Prefs0 */
//{45,85,59,99,pSel,14},        /* -> Prefs0 */
//{45,101,59,115,pSel,15},      /* -> Prefs0 */
//{63,37,77,51,pSel,16},        /* -> Prefs0 */
//{63,53,77,67,pSel,17},        /* -> Prefs0 */
//{63,69,77,83,pSel,18},        /* -> Prefs0 */
//{63,85,77,99,pSel,19},        /* -> Prefs0 */
//{63,101,77,115,pSelFile,0},   /* -> Prefs0 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 [orig -> Prefs1/Prefs5] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs4Regions[] = {
//{7,21,80,46,pWorkbench,0},    /* -> Prefs0 */
//{7,53,80,78,pMusic,0},        /* -> Prefs5 */
//{7,85,80,110,pLevel,0},       /* -> Prefs6 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 [orig -> Prefs1] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs5Regions[] = {
//{7,21,80,46,pSound,0},        /* -> Prefs0 */
//{7,53,80,78,pTune,0},         /* -> Prefs3 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 [orig -> Prefs4] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs6Regions[] = {
//{7,21,80,46,pLvlSelect,1},    /* -> Prefs0 */
//{7,53,80,78,pLvlSelect,2},    /* -> Prefs0 */
//{7,85,80,110,pLvlSelect,3},   /* -> Prefs0 */
  {75,0,87,8,pPrefBack,0},      /* -> Prefs0 [orig -> Prefs4] */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs7Regions[] = {
//{7,21,80,46,pYesNo,1},        /* -> Prefs0 */
//{7,53,80,78,pYesNo,2},        /* -> Prefs0 */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion Prefs8Regions[] = {
  {0,0,87,129,pAboutText,0},    /* -> Prefs0 */
  {-1,-1,-1,-1,NULL,-1}
  };

PrefsClickRegion *PrefsRegions[] = {
  Prefs0Regions,
  Prefs1Regions,
  Prefs2Regions,
  Prefs3Regions,
  Prefs4Regions,
  Prefs5Regions,
  Prefs6Regions,
  Prefs7Regions,
  Prefs8Regions
  };


/*
#############################################################################
#
# HandleClick()
#
# Main module calls this whenever the user clicks the left mouse button. We
# do required processing based on where the click occured, as specified by
# the click regions defined.
#
#############################################################################
*/

void HandleClick(UWORD mx,UWORD my) {
  ClickRegion *Region;
  Region=Regions;
  while(Region->x!=-1) {
    if (
      (mx>=Region->x) &&
      (my>=Region->y) &&
      (mx<(Region->x+Region->w)) &&
      (my<(Region->y+Region->h))
      ) {
      DoBorder(0);
      (*(Region->clickfunc[ClickState])) (
	mx-Region->x,
	my-Region->y,
	Region->funcparam
	);
      DoBorder(1);
      return;
      }
    Region++;
    }
  ClickState=FIRST_CLICK;
  DoBorder(0);
  }


/*
#############################################################################
#
# StackClick1()
#
# Called when "first click" is in an area of screen for a stack.
#
#############################################################################
*/

void StackClick1(UWORD x,UWORD y,WORD stack) {
  WORD pos;
  pos=StackTail(stack-1);
  if(pos==-1)
    return;
  if((y>=(pos*STEP_Y_STACK))&&(y<(pos*STEP_Y_STACK+CARDHEIGHT))) {
    ClickFrom=-stack; /* tail click */
    ClickState=SECOND_CLICK;
    return;
    }
  pos=StackHead(stack-1);
  if((y>=(pos*STEP_Y_STACK))&&(y<(pos*STEP_Y_STACK+STEP_Y_STACK))) {
    ClickFrom=stack; /* head click */
    ClickState=SECOND_CLICK;
    }
  }


/*
#############################################################################
#
# StackClick2()
#
# Called when "second click" is in an area of screen for a stack.
#
#############################################################################
*/

void StackClick2(UWORD x,UWORD y,WORD stack) {
  WORD tailpos,tailcard,headpos=0,headcard,card,suit;
  /* check for double click */
  if(ClickFrom==-stack) {
    tailcard=Stack[stack-1][StackTail(stack-1)];
    for(suit=0;suit<4;suit++) {
      if (
	(
	  (CardNumber(tailcard)==1+CardNumber(Suit[suit])) &&
	  (CardSuit(tailcard)==CardSuit(Suit[suit]))
	  ) || (
	  (CardNumber(tailcard)==1) &&
	  (Suit[suit]==-1)
	  )
	) {
	SuitClick(0,0,suit+1);
	break;
	}
      }
    ClickState=FIRST_CLICK;
    return;
    }
  /* find tail card of 'to' stack */
  tailpos=StackTail(stack-1);
  if(tailpos==-1)
    tailcard=-1;
  else
    tailcard=Stack[stack-1][tailpos];
  /* reset ClickState so that next click is "first click" */
  ClickState=FIRST_CLICK;
  if(ClickFrom==FROM_DEAL) {
    /* okay, deal to stack */
    /* empty 'to' stack? */
    if(tailpos!=-1) {
      /* no - standard card on card check */
      if (
	(CardColour(tailcard)!=CardColour(Deck[Dptr-1])) &&
	(CardNumber(tailcard)==1+CardNumber(Deck[Dptr-1]))
	) {
	/* card on card test passed, move it */
	Stack[stack-1][tailpos+1]=Deck[Dptr-1];
	Dptr--;
	Dsize--;
	RekoDrawCard(
	  CARD_DECK+Deck[Dptr],
	  STACKX(stack-1),
	  STACKY(tailpos+1)
	  );
	if(Dptr>0)
	  RekoDrawCard(CARD_DECK+Deck[Dptr-1],POS_X_DEAL,POS_Y_SPECIAL);
	else
	  RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
	ShiftDeck();
	}
      }
    else {
      /* special case, only allow kings on empty stack */
      if(CardNumber(Deck[Dptr-1])==KING) {
	/* king test passed, move it */
	Stack[stack-1][0]=Deck[Dptr-1];
	Dptr--;
	Dsize--;
	RekoDrawCard(CARD_DECK+Deck[Dptr],STACKX(stack-1),STACKY(0));
	if(Dptr>0)
	  RekoDrawCard(CARD_DECK+Deck[Dptr-1],POS_X_DEAL,POS_Y_SPECIAL);
	else
	  RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
	ShiftDeck();
	}
      }
    return;
    }
  /* stack to stack pre-processing */
  if((ClickFrom>=1)&&(ClickFrom<=7))
    /* standard - stack (head) to stack */
    headpos=StackHead(ClickFrom-1);
  else {
    if((ClickFrom<=-1)&&(ClickFrom>=-7)) {
      /* stack (tail) to stack, treat as "head" as tail position */
      ClickFrom=-ClickFrom;
      headpos=StackTail(ClickFrom-1);
      }
    }
  /* stack to stack processing */
  if((ClickFrom>=1)&&(ClickFrom<=7)) {
    headcard=Stack[ClickFrom-1][headpos];
    /* empty 'to' stack? */
    if(tailpos!=-1) {
      /* no - standard card on card check */
      if (
	(CardColour(headcard)!=CardColour(tailcard)) &&
	(CardNumber(headcard)==(CardNumber(tailcard)-1))
	) {
	/* card on card test passed */
	/* if move reveals card, ensure revealed card is face up */
	if(headpos>0) {
	  card=Stack[ClickFrom-1][headpos-1];
	  if(card<0) {
	    card=-card;
	    Stack[ClickFrom-1][headpos-1]=card;
	    }
	  RekoDrawCard(CARD_DECK+card,STACKX(ClickFrom-1),STACKY(headpos-1));
	  RekoClean (
	    STACKX(ClickFrom-1),
	    STACKY(headpos-1)+CARDHEIGHT,
	    STACKX(ClickFrom-1)+CARDWIDTH,
	    STACKY(19)+CARDHEIGHT
	    );
	  }
	else {
	  RekoDrawCard(CARD_BORDER,STACKX(ClickFrom-1),STACKY(0));
	  RekoClean (
	    STACKX(ClickFrom-1),
	    STACKY(0)+CARDHEIGHT,
	    STACKX(ClickFrom-1)+CARDWIDTH,
	    STACKY(19)+CARDHEIGHT
	    );
	  }
	/* shift the stack */
	tailpos++;
	while(Stack[ClickFrom-1][headpos]) {
	  card=Stack[ClickFrom-1][headpos];
	  Stack[stack-1][tailpos]=card;
	  Stack[ClickFrom-1][headpos]=0;
	  RekoDrawCard(CARD_DECK+card,STACKX(stack-1),STACKY(tailpos));
	  headpos++;
	  tailpos++;
	  }
	}
      }
    else {
      /* special case - only allow kings on empty stack */
      if(CardNumber(headcard)==KING) {
	/* king test passed */
	/* see above for descriptions */
	if(headpos>0) {
	  card=Stack[ClickFrom-1][headpos-1];
	  if(card<0) {
	    card=-card;
	    Stack[ClickFrom-1][headpos-1]=card;
	    }
	  RekoDrawCard(CARD_DECK+card,STACKX(ClickFrom-1),STACKY(headpos-1));
	  RekoClean (
	    STACKX(ClickFrom-1),
	    STACKY(headpos-1)+CARDHEIGHT,
	    STACKX(ClickFrom-1)+CARDWIDTH,
	    STACKY(19)+CARDHEIGHT
	    );
	  }
	else {
	  RekoDrawCard(CARD_BORDER,STACKX(ClickFrom-1),STACKY(0));
	  RekoClean (
	    STACKX(ClickFrom-1),
	    STACKY(0)+CARDHEIGHT,
	    STACKX(ClickFrom-1)+CARDWIDTH,
	    STACKY(19)+CARDHEIGHT
	    );
	  }
	tailpos++;
	while(Stack[ClickFrom-1][headpos]) {
	  card=Stack[ClickFrom-1][headpos];
	  Stack[stack-1][tailpos]=card;
	  Stack[ClickFrom-1][headpos]=0;
	  RekoDrawCard(CARD_DECK+card,STACKX(stack-1),STACKY(tailpos));
	  headpos++;
	  tailpos++;
	  }
	}
      }
    }
  }


/*
#############################################################################
#
# DeckClick()
#
# Called when "first click" is in the area of screen for the deck.
#
#############################################################################
*/

void DeckClick(UWORD x,UWORD y,WORD dummy) {
  char tmpstr[8];
  /* gone thru entire deck? */
  if(Dptr!=Dsize) {
    /* no - flip 3 cards */
    Dptr+=3;
    /* except where 3 is too many, then flip to end of deck */
    if(Dptr>Dsize)
      Dptr=Dsize;
    /* update screen 'deal' */
    RekoDrawCard(CARD_DECK+Deck[Dptr-1],POS_X_DEAL,POS_Y_SPECIAL);
    /* if no cards in non-deal portion of deck, clear screen 'deck' */
    if(Dptr==Dsize)
      RekoDrawCard(CARD_BORDER,POS_X_DECK,POS_Y_SPECIAL);
    }
  else {
    /* gone thru deck, flip back entire 'deal' (if there's anything left) */
    if(Dsize>0) {
      Dptr=0;
      RekoDrawCard(CARD_BACK,POS_X_DECK,POS_Y_SPECIAL);
      RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
      }
    loops++;
    sprintf(tmpstr,"  %2d  ",loops);
    TextWrite(tmpstr,POS_X_DEAL+CARDWIDTH+4,POS_Y_SPECIAL+(CARDHEIGHT>>1),SMALL_FONT,WHITE,BLACK);
    }
  }


/*
#############################################################################
#
# SuitClick()
#
# Called when "second click" is in an area of screen for a suit.
#
#############################################################################
*/

void SuitClick(UWORD x,UWORD y,WORD suit) {
  WORD tailpos,tailcard;
  ClickState=FIRST_CLICK;
  //if(dfp) fprintf(dfp,"SuitClick, From=%d\n",ClickFrom);
  if(ClickFrom==FROM_DEAL) {
    /* okay, from 'deal' to 'suit' */
    /* special case, only aces on empty suit */
    //if(dfp) fprintf(dfp,"FromCard=%d, CardNumber=%d\n",Deck[Dptr-1],CardNumber(Deck[Dptr-1]));
    if(Suit[suit-1]==-1) {
      if(CardNumber(Deck[Dptr-1])==1) {
	/* ace test passed, move it */
	Suit[suit-1]=Deck[Dptr-1];
	RekoDrawCard(CARD_DECK+Suit[suit-1],SUITX(suit-1),POS_Y_SPECIAL);
	Dptr--;
	Dsize--;
	if(Dptr>0)
	  RekoDrawCard(CARD_DECK+Deck[Dptr-1],POS_X_DEAL,POS_Y_SPECIAL);
	else
	  RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
	ShiftDeck();
	CheckWin();
	}
      return;
      }
    /* standard card on card check */
    if (
      (CardSuit(Deck[Dptr-1])==CardSuit(Suit[suit-1])) &&
      (CardNumber(Deck[Dptr-1])==(CardNumber(Suit[suit-1])+1))
      ) {
      /* card on card test passed, move it */
      Suit[suit-1]=Deck[Dptr-1];
      RekoDrawCard(CARD_DECK+Suit[suit-1],SUITX(suit-1),POS_Y_SPECIAL);
      Dptr--;
      Dsize--;
      if(Dptr>0)
	RekoDrawCard(CARD_DECK+Deck[Dptr-1],POS_X_DEAL,POS_Y_SPECIAL);
      else
	RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
      ShiftDeck();
      CheckWin();
      return;
      }
    }
  if((ClickFrom<=-1)&&(ClickFrom>=-7)) {
    /* okay, stack to suit */
    ClickFrom=-ClickFrom;
    tailpos=StackTail(ClickFrom-1);
    tailcard=Stack[ClickFrom-1][tailpos];
    //if(dfp) fprintf(dfp,"SuitClickFromTail, From=%d TailPos=%d TailCard=%d, CardNumber=%d\n",ClickFrom,tailpos,tailcard,CardNumber(tailcard));
    /* special case - only aces on empty suit */
    if(Suit[suit-1]==-1) {
      if(CardNumber(tailcard)==1) {
	/* ace test passed, move it */
	Suit[suit-1]=tailcard;
	RekoDrawCard(CARD_DECK+Suit[suit-1],SUITX(suit-1),POS_Y_SPECIAL);
	Stack[ClickFrom-1][tailpos]=0;
	if(tailpos>0) {
	  tailpos--;
	  if(Stack[ClickFrom-1][tailpos]<0)
	    Stack[ClickFrom-1][tailpos]=-Stack[ClickFrom-1][tailpos];
	  RekoDrawCard (
	    CARD_DECK+Stack[ClickFrom-1][tailpos],
	    STACKX(ClickFrom-1),
	    STACKY(tailpos)
	    );
	  RekoClean (
	    STACKX(ClickFrom-1),STACKY(tailpos)+CARDHEIGHT,
	    STACKX(ClickFrom-1)+CARDWIDTH,STACKY(tailpos+1)+CARDHEIGHT
	    );
	  }
	else
	  RekoDrawCard(CARD_BORDER,STACKX(ClickFrom-1),STACKY(0));
	CheckWin();
	}
      return;
      }
    /* standard card on card check */
    if (
      (CardSuit(tailcard)==CardSuit(Suit[suit-1])) &&
      (CardNumber(tailcard)==(CardNumber(Suit[suit-1])+1))
      ) {
      /* card on card test passed, move it */
      Suit[suit-1]=tailcard;
      RekoDrawCard(CARD_DECK+Suit[suit-1],SUITX(suit-1),POS_Y_SPECIAL);
      Stack[ClickFrom-1][tailpos]=0;
      if(tailpos>0) {
	tailpos--;
	if(Stack[ClickFrom-1][tailpos]<0)
	  Stack[ClickFrom-1][tailpos]=-Stack[ClickFrom-1][tailpos];
	RekoDrawCard (
	  CARD_DECK+Stack[ClickFrom-1][tailpos],
	  STACKX(ClickFrom-1),
	  STACKY(tailpos)
	  );
	RekoClean (
	  STACKX(ClickFrom-1),STACKY(tailpos)+CARDHEIGHT,
	  STACKX(ClickFrom-1)+CARDWIDTH,STACKY(tailpos+1)+CARDHEIGHT
	  );
	}
      else
	RekoDrawCard(CARD_BORDER,STACKX(ClickFrom-1),STACKY(0));
      CheckWin();
      }
    }
  }


/*
#############################################################################
#
# DealClick1()
#
# Called when "first click" is in the area of screen for the deal.
#
#############################################################################
*/

void DealClick1(UWORD x,UWORD y,WORD dummy) {
  /* only valid if there are cards dealt */
  if(Dptr>0) {
    ClickState=SECOND_CLICK;
    ClickFrom=FROM_DEAL;
    }
  }


/*
#############################################################################
#
# DealClick2()
#
# Called when "second click" is in the area of screen for the deal.
#
#############################################################################
*/

void DealClick2(UWORD x,UWORD y,WORD dummy) {
  WORD suit;
  /* only valid if first click also on deal (double click) */
  if(ClickFrom==FROM_DEAL) {
    for(suit=0;suit<4;suit++) {
      if (
	(
	  (CardNumber(Deck[Dptr-1])==1+CardNumber(Suit[suit])) &&
	  (CardSuit(Deck[Dptr-1])==CardSuit(Suit[suit]))
	  ) || (
	  (CardNumber(Deck[Dptr-1])==1) &&
	  (Suit[suit]==-1)
	  )
	) {
	SuitClick(0,0,suit+1);
	break;
	}
      }
    }
  ClickState=FIRST_CLICK;
  }


/*
#############################################################################
#
# PrefClick()
#
# Called when "first click" is in the area of screen for the prefs card.
#
#############################################################################
*/

void PrefClick(UWORD x,UWORD y,WORD dummy) {
  PrefsClickRegion *Region;
  Region=PrefsRegions[PrefCard];
  while(Region->x1!=-1) {
    if (
      (x>=Region->x1) &&
      (y>=Region->y1) &&
      (x<=Region->x2) &&
      (y<=Region->y2)
      ) {
      (*(Region->clickfunc)) (
	Region->funcparam
	);
      return;
      }
    Region++;
    }
  }


/*
#############################################################################
#
# InvalidClick()
#
# Called when any click is in an inappropriate area of screen.
#
#############################################################################
*/

void InvalidClick(UWORD x,UWORD y,WORD dummy) {
  ClickState=FIRST_CLICK;
  }


/*
#############################################################################
#
# Shuffle()
#
# Shuffles the deck. Makes 50 passes through the deck, swapping every card
# with another card, thus performing 2600 swaps.
#
#############################################################################
*/

void Shuffle(void) {
  int pass,loop,swapno;
  BYTE temp;
  /* setup deck */
  for(loop=0;loop<52;loop++)
    Deck[loop]=loop+1;
  /* 50 shuffle passes */
  for(pass=0;pass<50;pass++) {
    /* each and every card ... */
    for(loop=0;loop<52;loop++) {
      /* ... swaps with another */
      swapno=random()%52;
      temp=Deck[loop];
      Deck[loop]=Deck[swapno];
      Deck[swapno]=temp;
      }
    }
  Dptr=0;
  }


/*
#############################################################################
#
# InitDeal()
#
# Initialises game state bits and deals the first 28 cards into the familiar
# triangular pattern. Also draws the other various 'cards' onto the screen.
#
#############################################################################
*/

void InitDeal(UWORD decksize) {
  int row,stack,card,suit;
  /* clear stacks */
  for(stack=0;stack<7;stack++)
    for(row=0;row<20;row++)
      Stack[stack][row]=0;
  /* clear suits */
  for(suit=0;suit<4;suit++)
    Suit[suit]=-1;
  /* deal */
  for(row=0;row<7;row++) {
    for(stack=row;stack<7;stack++) {
      if(stack==row) {
	Stack[stack][row]=Deck[Dptr++];
	RekoDrawCard(CARD_DECK+Stack[stack][row],STACKX(stack),STACKY(row));
	}
      else {
	Stack[stack][row]=-Deck[Dptr++];
	RekoDrawCard(CARD_BACK,STACKX(stack),STACKY(row));
	}
      }
    }
  /* draw other bits */
  RekoDrawCard(CARD_BACK,POS_X_DECK,POS_Y_SPECIAL);
  RekoDrawCard(CARD_BORDER,POS_X_DEAL,POS_Y_SPECIAL);
  ShowPref(0);
  for(suit=0;suit<4;suit++) {
    if(decksize>=CARD_BASE+4)
      RekoDrawCard(CARD_BASE+suit,SUITX(suit),POS_Y_SPECIAL);
    else
      RekoDrawCard(CARD_REKO,SUITX(suit),POS_Y_SPECIAL);
    }
  TextWrite (
    "LOOPS:",
    POS_X_DEAL+CARDWIDTH+4,POS_Y_SPECIAL+(CARDHEIGHT>>1)-8,
    SMALL_FONT,
    WHITE,BLACK
    );
  TextWrite (
    "   0  ",
    POS_X_DEAL+CARDWIDTH+4,POS_Y_SPECIAL+(CARDHEIGHT>>1),
    SMALL_FONT,
    WHITE,BLACK
    );
  /* initial deck shifting */
  for(card=Dptr;card<52;card++)
    Deck[card-Dptr]=Deck[card];
  /* initialise deal/deck bits */
  Dsize=24;  /* 24 didn't work before, but 23 steals a card */
  Dptr=loops=0;
  }


/*
#############################################################################
#
# StackTail()
#
# Searches for the 'tail' of the face up portion of a stack (i.e. the card
# which can be completely seen).
#
#############################################################################
*/

WORD StackTail(UWORD stack) {
  int row;
  for(row=18;row>=0;row--)
    if(Stack[stack][row]>0)
      return(row);
  return(-1);
  }


/*
#############################################################################
#
# StackHead()
#
# Searches for the 'head' of the face up portion of a stack.
#
#############################################################################
*/

WORD StackHead(UWORD stack) {
  int row;
  for(row=0;row<19;row++)
    if(Stack[stack][row]>0)
      return(row);
  return(-1);
  }


/*
#############################################################################
#
# CardSuit()
# CardNumber()
# CardColour()
#
# Returns the suit/number/colour of a card.
#
#############################################################################
*/

UWORD CardSuit(WORD card) {
  return((card>0)?(card-1)%4:((-card)-1)%4);
  }

UWORD CardNumber(WORD card) {
  return((card>0)?((card-1)/4)+1:(((-card)-1)/4)+1);
  }

UWORD CardColour(WORD card) {
  return(((CardSuit(card)==SUIT_CLUBS)||(CardSuit(card)==SUIT_SPADES))?0:1);
  }


/*
#############################################################################
#
# ShiftDeck()
#
# When a card is taken from 'deal', 'deck' cards shuffle along to fill the
# 'gap'.
#
#############################################################################
*/

void ShiftDeck(void) {
  int card;
  for(card=Dptr+1;card<=Dsize;card++)
    Deck[card-1]=Deck[card];
  }


/*
#############################################################################
#
# CheckWin()
#
# Called everytime a card is moved to a suit.
#
#############################################################################
*/

void CheckWin(void) {
  if (
    (CardNumber(Suit[0])==KING) &&
    (CardNumber(Suit[1])==KING) &&
    (CardNumber(Suit[2])==KING) &&
    (CardNumber(Suit[3])==KING)
    )
    quitflag=QUIT_WIN;
  }


/*
#############################################################################
#
# DoBorder()
#
# Draws/undraws a border around the 'from' area.
#
#############################################################################
*/

void DoBorder(int drawflag) {
  UWORD x,y;
  if((drawflag)&&(ClickState==SECOND_CLICK)) {
    BorderFrom=ClickFrom;
    if((ClickFrom>=1)&&(ClickFrom<=7)) {
      BDRx=STACKX(ClickFrom-1);
      BDRy1=STACKY(StackHead(ClickFrom-1))-5;
      BDRy2=STACKY(StackTail(ClickFrom-1))+CARDHEIGHT;
      }
    if((ClickFrom<=-1)&&(ClickFrom>=-7)) {
      BDRx=STACKX((-ClickFrom)-1);
      BDRy1=STACKY(StackTail((-ClickFrom)-1))-5;
      BDRy2=STACKY(StackTail((-ClickFrom)-1))+CARDHEIGHT;
      }
    if(ClickFrom==FROM_DEAL) {
      BDRx=POS_X_DEAL;
      BDRy1=POS_Y_SPECIAL-5;
      BDRy2=POS_Y_SPECIAL+CARDHEIGHT;
      }
    /* draw the border */
    if(ClickFrom!=FROM_DEAL) {
      for(y=0;y<5;y++) {
	for(x=0;x<CARDWIDTH;x++) {
	  VesaPoint (
	    BDRx+x,BDRy1+y,
	    &BDRbuff[y][x]
	    );
	  }
	}
      RekoClean(BDRx,BDRy1,BDRx+CARDWIDTH,BDRy1+5);
      RekoClean(BDRx,BDRy2,BDRx+CARDWIDTH,BDRy2+5);
      }
    /* horizontal lines not done together to reduce page swaps - faster */
    VesaLine(BDRx-2,BDRy1+2,CARDWIDTH+4,WHITE);
    for(y=BDRy1+3;y<BDRy2+2;y++) {
      VesaPlot(BDRx-3,y,WHITE);
      VesaPlot(BDRx+CARDWIDTH+2,y,WHITE);
      }
    VesaLine(BDRx-2,BDRy2+2,CARDWIDTH+4,WHITE);
    }
  else {
    if(BorderFrom!=FROM_NOWHERE) {
      if(BorderFrom!=FROM_DEAL) {
	for(y=0;y<5;y++) {
	  VesaPlotRow(BDRx,BDRy1+y,CARDWIDTH,BDRbuff[y]);
	  }
	}
      else
	RekoClean(BDRx,BDRy1+2,BDRx+CARDWIDTH,BDRy1+3);
      RekoClean(BDRx,BDRy2+2,BDRx+CARDWIDTH,BDRy2+3);
      RekoClean(BDRx-3,BDRy1+2,BDRx,BDRy2+3);
      RekoClean(BDRx+CARDWIDTH,BDRy1+2,BDRx+CARDWIDTH+3,BDRy2+3);
      BorderFrom=FROM_NOWHERE;
      }
    }
  }


/*
#############################################################################
#
# ShowPref()
#
# Shows a prefs card and sets PrefCard so that prefs clicking processing may
# be handled correctly.
#
#############################################################################
*/

void ShowPref(UWORD prefcard) {
  RekoDrawCard(CARD_PREF+prefcard,POS_X_PREF,POS_Y_SPECIAL);
  PrefCard=prefcard;
  }


/*
#############################################################################
#
# Prefs Click Functions
#
#############################################################################
*/

void pAbout(WORD dummy) {
  ShowPref(8);
  }

void pPrefs(WORD dummy) {
  ShowPref(1);
  }

void pRestart(WORD dummy) {
  quitflag=QUIT_RESTART;
  }

void pQuit(WORD dummy) {
  quitflag=QUIT_EXIT;
  }

void pResolution(WORD dummy) {
  ShowPref(2);
  }

void pCardset(WORD dummy) {
  ChooseDeck();
  }

void pMisc(WORD dummy) {
  ShowPref(4);
  }

void pResSelect(WORD dummy) {
  ShowPref(0);
  }

void pSel(WORD dummy) {
  ShowPref(0);
  }

void pSelFile(WORD dummy) {
  ShowPref(0);
  }

void pWorkbench(WORD dummy) {
  ShowPref(0);
  }

void pMusic(WORD dummy) {
  ShowPref(5);
  }

void pLevel(WORD dummy) {
  ShowPref(6);
  }

void pSound(WORD dummy) {
  ShowPref(0);
  }

void pTune(WORD dummy) {
  ShowPref(3);
  }

void pLvlSelect(WORD dummy) {
  ShowPref(0);
  }

void pYesNo(WORD dummy) {
  ShowPref(0);
  }

void pAboutText(WORD dummy) {
  ShowPref(0);
  }

void pPrefBack(WORD dummy) {
  ShowPref(0);
  }
