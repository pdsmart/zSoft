#ifndef __TOKENINT_H_
#define __TOKENINT_H_

extern token* curTok;
extern token* prevTok;

char* getCurTokPos();
void skipTokenInInput(char offset);
char parseName(char checkCmd);
char parseSymbol(void);
char parseNumber(void);
char parseNone();
void setTokenError(char* pos, char code);

#endif
