/* music.h */


/* error codes */

#define MUSIC_OKAY          0   /* no error */
#define MUSIC_NOMIDI       -1   /* unable to open MIDI port */
#define MUSIC_NOTIMER      -2   /* unable to setup periodic timer */
#define MUSIC_NOFILE       -3   /* tried to load non-existant file */
#define MUSIC_NOMEM        -4   /* not enough memory to load file */
#define MUSIC_BADFILE      -5   /* file loaded was not an MPX2 */
#define MUSIC_NOFILELOADED -6   /* tried to play when no file loaded */
#define MUSIC_NOCD         -7   /* no CD drives or no MSCDEX */
#define MUSIC_BADMSCDEX    -8   /* MSCDEX too old (min 2.10) */
#define MUSIC_SBDRVFAIL    -9   /* SB_LIB init failed */