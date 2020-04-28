#ifndef __MY_STDLIB_H_
#define __MY_STDLIB_H_

#ifndef NO_STDLIB

#include <stdlib.h>
#include <string.h>

#else

#define NULL ( (void *) 0)

int strlen(const char* s);
void* memcpy(void* dst, const void* src, int sz);
int memcmp(const void* dst, const void* src, int sz);
void* memmove(void* dst, const void* src, int sz);

#endif

#endif

