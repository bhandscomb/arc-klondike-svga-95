/* game.h */

#define POS_X_STACK    32
#define POS_Y_STACK    17

#define STEP_X_STACK  108
#define STEP_Y_STACK   16

#define CARDWIDTH      88
#define CARDHEIGHT    130
#define STACKHEIGHT   (18*STEP_Y_STACK+CARDHEIGHT)

#define STACKX(x)     (POS_X_STACK+(STEP_X_STACK*(x)))
#define STACKY(y)     (POS_Y_STACK+(STEP_Y_STACK*(y)))

#define POS_X_DECK     12
#define POS_X_DEAL    105
#define POS_X_PREF    263
#define POS_X_SUIT    421
#define POS_Y_SPECIAL 453

#define STEP_X_SUIT    93

#define SUITX(x)      (POS_X_SUIT+(STEP_X_SUIT*(x)))

#define SUIT_CLUBS      0
#define SUIT_DIAMONDS   1
#define SUIT_HEARTS     2
#define SUIT_SPADES     3

#define CARD_REKO       0
#define CARD_BORDER     1
#define CARD_BACK       2
/* CARD_DECK is 2 because card '1' is cardset position 3 */
#define CARD_DECK       2

#define CARD_BASE      55

#define CARD_PREF      59

#define JACK           11
#define QUEEN          12
#define KING           13

#define QUIT_EXIT       1
#define QUIT_RESTART    2
#define QUIT_WIN        3
#define QUIT_FRONT      4

