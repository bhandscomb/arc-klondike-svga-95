/* mouse.h */

#define MOUSE_OKAY    0   /* no error               */
#define MOUSE_FAILED -1   /* no mouse driver loaded */

#ifdef ERRMSG_C
extern char *MouseErrorMessages[];
#endif