/* protos.h */


/* config.c */
void ConfigRead(void);
void ConfigWrite(void);
void ConfigFlush(void);
void ConfigGet(char *,char *);
void ConfigPut(char *,char *);

/* win95.c - "void *" actually "WIN32_FIND_DATA *" */
int CheckWinRunning(void);
int CheckWin95(void);
int CheckVolumeLFN(char *);
int LFNfindfirst(char *,void *);
int LFNfindnext(int,void *);
int LFNfindclose(int);
int LFNrename(char *,char *);

/* decode.c */
int decodefile(FILE *,FILE *);
