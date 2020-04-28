#ifndef __EDITOR_H_
#define __EDITOR_H_

#include "basic_utils.h"

extern char* prgStore;
extern short prgSize;

void resetEditor(void);
void initEditor(char* prgBody, short progSpaceSize);
char readLine();
prgline* findLine(short num);
void injectLine(char* s, short num);
char editorSave(void);
char editorLoad(void);
char editorLoadParsed(void);

#endif

