/* colours.h */


/* colours */

#define BLACK    0x0
#define BLUE     0x1
#define GREEN    0x2
#define CYAN     0x3
#define RED      0x4
#define MAGENTA  0x5
#define BROWN    0x6
#define GREY     0x7
#define DGREY    0x8
#define LBLUE    0x9
#define LGREEN   0xA
#define LCYAN    0xB
#define LRED     0xC
#define LMAGENTA 0xD
#define YELLOW   0xE
#define WHITE    0xF


/* for readability */

#define BG
#define FG <<4|


/* colour definitions */

#define BACKGROUND    (BG BLUE	FG YELLOW)
#define BANNER	      (BG BLUE	FG YELLOW)
#define FOOTER	      (BG RED	FG WHITE)
#define MENUTITLE     (BG BLUE	FG WHITE)
#define MENUITEM      (BG BLUE	FG GREY)
#define MENUSELECT    (BG GREY	FG RED)
#define WHEREBOX      (BG RED	FG WHITE)
#define WHERETEXT     (BG GREY	FG BLACK)
#define PROGRESSBOX   (BG RED	FG WHITE)
#define PROGRESSBAR   (BG BLACK FG YELLOW)
#define QUEUEBOX      (BG GREY	FG BLACK)
#define FAILBOX       (BG GREY	FG BLACK)
#define W95BOX	      (BG GREY	FG BLACK)
#define TEXTBOX       (BG RED	FG WHITE)
#define TEXTTEXT      (BG RED	FG GREY)
