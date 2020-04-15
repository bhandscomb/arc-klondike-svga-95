/* vesa.h */


/* video mode definitions (no internals) */

#define X_RESOLUTION 800
#define Y_RESOLUTION 600


/* error codes */

#define VESA_OKAY      0   /* no error                            */
#define VESA_FAIL     -1   /* call failed                         */
#define VESA_NOMODE   -2   /* required mode not supported         */
#define VESA_NOMEM    -3   /* not enough memory on card           */
#define VESA_BADMODE  -4   /* required mode unusable by our code  */
#define VESA_BADPICT  -5   /* picture not proper KLN format       */
#define VESA_NODOSMEM -6   /* unable to allocate conventional mem */
#define VESA_PRE_1_2  -7   /* VBE pre 1.2 found                   */
#define VESA_2_NOLIN  -8   /* VBE 2.0 (or above) but no linear... */


/* this was a function, but since it's mainly used with constants... */

#define VesaColour(r,g,b) \
  ( (((r)&0xf8)<<8) | (((g)&0xfc)<<3) | (((b)&0xf8)>>3) )


/* useful colours */

#define BLACK VesaColour(0x00,0x00,0x00)
#define DGREY VesaColour(0x80,0x80,0x80)
#define LGREY VesaColour(0xC0,0xC0,0xC0)
#define WHITE VesaColour(0xFF,0xFF,0xFF)


#ifdef ERRMSG_C
extern char *VesaErrorMessages[];
#endif
