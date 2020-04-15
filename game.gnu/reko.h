/* reko.h */

#define REKOERR_OKAY      0   /* no error                               */
#define REKOERR_READ     -1   /* error reading card file                */
#define REKOERR_BADCARD  -2   /* card not in deck                       */
#define REKOERR_NOXMS    -3   /* no XMS handler, can't use extended mem */
#define REKOERR_XMSERROR -4   /* couldn't allocate memory               */
#define REKOERR_BADDECK  -5   /* error in deck header                   */
#define REKOERR_NODECK   -6   /* unable to open deck                    */
#define REKOERR_NOPREFS  -7   /* unable to get any prefs cards          */

#ifdef ERRMSG_C
extern char *RekoErrorMessages[];
#endif
