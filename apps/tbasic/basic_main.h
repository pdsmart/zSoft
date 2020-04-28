#ifndef __MAIN_H_
#define __MAIN_H_

#include "mytypes.h"

void init(short dataSize, short lineSize, short progSize);
void dispatch();
void processLine(char* line, token* t);

#endif

