/* protos.h */


/* reko.c */
WORD RekoGetDeck(char *,UWORD);
void RekoDiscardDeck(void);
void RekoClearTable(void);
void RekoClean(UWORD,UWORD,UWORD,UWORD);
WORD RekoDrawCard(UWORD,UWORD,UWORD);

/* vesa.c */
void VesaSetPage(UWORD,UWORD);
void VesaLine(UWORD,UWORD,UWORD,UWORD);
void VesaPlotRow(UWORD,UWORD,UWORD,UWORD *);
WORD VesaShowPic(char *);
void VesaPlot(UWORD,UWORD,UWORD);
void VesaPoint(UWORD,UWORD,UWORD *);
WORD VesaInit(void);
void VesaCleanUp(void);
void VesaClear(UWORD);

/* mouse.c */
WORD MouseInit(void);
UWORD MouseCheck(void);
void MouseWait(UWORD *,UWORD *,UWORD);

/* text.c */
void TextInit(void);
void TextWrite(char *,WORD,WORD,UWORD,UWORD,UWORD);

/* config.c */
void ConfigRead(void);
void ConfigWrite(void);
void ConfigFlush(void);
void ConfigGet(char *,char *);
void ConfigPut(char *,char *);

/* chooser.c */
void ChooseDeck(void);
void ListClick(WORD);
void ListScroll(WORD);
void ListSelect(WORD);
void InfoScroll(WORD);
void Back2System(WORD);
int CheckWin95(void);

/* file.c */
void FileInit(void);

/* errmsg.c */
void WhatError(UWORD,WORD);

/* klongame.c */
void HandleClick(UWORD,UWORD);
void StackClick1(UWORD,UWORD,WORD);
void StackClick2(UWORD,UWORD,WORD);
void DeckClick(UWORD,UWORD,WORD);
void SuitClick(UWORD,UWORD,WORD);
void DealClick1(UWORD,UWORD,WORD);
void DealClick2(UWORD,UWORD,WORD);
void PrefClick(UWORD,UWORD,WORD);
void InvalidClick(UWORD,UWORD,WORD);
void Shuffle(void);
void InitDeal(UWORD);
WORD StackTail(UWORD);
WORD StackHead(UWORD);
UWORD CardSuit(WORD);
UWORD CardNumber(WORD);
UWORD CardColour(WORD);
void ShiftDeck(void);
void CheckWin(void);
void DoBorder(int);
void ShowPref(UWORD);
void pAbout(WORD);
void pPrefs(WORD);
void pRestart(WORD);
void pQuit(WORD);
void pResolution(WORD);
void pCardset(WORD);
void pMisc(WORD);
void pResSelect(WORD);
void pSel(WORD);
void pSelFile(WORD);
void pWorkbench(WORD);
void pMusic(WORD);
void pLevel(WORD);
void pSound(WORD);
void pTune(WORD);
void pLvlSelect(WORD);
void pYesNo(WORD);
void pAboutText(WORD);
void pPrefBack(WORD);

/* win95.c */
void SetAppTitle(char *);
void SetupCloseAware(void);
void ClearCloseAware(void);
int QueryClose(void);

/* music.c */
int MidiMusicInit(void);
void MidiMusicFree(void);
int MidiMusicLoad(char *);
int MidiMusicPlay(void);
void MidiMusicStop(void);
void MidiMusicContinue(void);
void MidiMusicCleanUp(void);
void MidiMusicOneShot(UBYTE *);
int CdMusicInit(void);
int CdMusicPlay(char *);
void CdMusicStop(void);
void CdMusicContinue(void);
void CdMusicCleanUp(void);
int ModMusicInit(void);
void ModMusicFree(void);
int ModMusicLoad(char *);
int ModMusicPlay(void);
void ModMusicStop(void);
void ModMusicContinue(void);
void ModMusicCleanUp(void);
void MusicContinue(void);

/* main.c */
void err (int,WORD,char *);
