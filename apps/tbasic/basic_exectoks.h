#ifndef __EXECTOKS_H_
#define __EXECTOKS_H_

#include "basic_tokens.h"

extern short nextLineNum;

void resetTokenExecutor(void);
void initTokenExecutor(char* space, short size);
short varSize(void);
void executeTokens(token* t);
char executeStep(char* lineBuf, token* tokenBuf);
void execBreak(void);
void executeNonParsed(numeric count);
void initParsedRun(void);
void executeParsedRun(void);
void setLastInput(short c);
void dispatchInput(void);
void dispatchDelay(void);
void setDelay(numeric millis);
char checkDelay();
void dispatchBreak(void);

#endif

