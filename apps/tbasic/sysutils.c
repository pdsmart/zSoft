////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            sysutils.c
// Created:         January 2019
// Author(s):       Philip Smart
// Description:     Utilities for C compilation when stdlib is not included in the linker list.
//
// Credits:         
// Copyright:       (c) 2019 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This source file is free software: you can redistribute it and#or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
    extern "C" {
#endif


#if defined(__K64F__)
//  #include <stdio.h>
//  #include <stdlib.h>
//  #include <string.h>
  #define uint32_t __uint32_t
  #define uint16_t __uint16_t
  #define uint8_t  __uint8_t
  #define int32_t  __int32_t
  #define int16_t  __int16_t
  #define int8_t   __int8_t
#elif defined(__ZPU__)
  #include <zstdio.h>
  #include <zpu-types.h>
  #include <stdlib.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
//#include "interrupts.h"
//#include "ff.h"            /* Declarations of FatFs API */
//#include "diskio.h"
//#include <string.h>
//#include <fcntl.h>
//#include <sys/stat.h>
//#include "xprintf.h"
//#include "utils.h"
//#include "basic_main.h"
//#include "basic_utils.h"
//#include "basic_textual.h"
//#include "basic_tokens.h"
//#include "basic_extern.h"
////
//#if defined __ZPUTA__
//  #include "zputa_app.h"
//#elif defined __ZOS__
//  #include "zOS_app.h"
//#else
//  #error OS not defined, use __ZPUTA__ or __ZOS__      
//#endif

int strlen(const char* s) {
    int i;
    for (i = 0; s[i]; i++) {
    }
    return i;
}

void* memcpy(void* dst, const void* src, int sz) {
    char* d = (char*) dst;
    char* s = (char*) src;
    while (sz-- > 0) {
        *(d++) = *(s++);
    }
    return dst;
}

int memcmp(const void* dst, const void* src, int sz) {
    unsigned char* d = (unsigned char*) dst;
    unsigned char* s = (unsigned char*) src;
    int i, v;
    for (i = 0; i < sz; i++) {
        v = *(d++) - *(s++);
        if (v != 0) {
            return v;
        }
    }
    return 0;
}

void* memmove(void* dst, const void* src, int sz) {
    unsigned char* d = (unsigned char*) dst;
    unsigned char* s = (unsigned char*) src;
    int i;
    if (d < s) {
        for (i = 0; i < sz; i++) {
            *(d++) = *(s++);
        }
    } else {
        d += sz;
        s += sz;
        for (i = 0; i < sz; i++) {
            *(--d) = *(--s);
        }
    }
    return dst;
}

int strcmp (const char *p1, const char *p2)
{
  const unsigned char *s1 = (const unsigned char *) p1;
  const unsigned char *s2 = (const unsigned char *) p2;
  unsigned char c1, c2;
  do
    {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
        return c1 - c2;
    }
  while (c1 == c2);
  return c1 - c2;
}

__attribute__((weak)) 
void _exit(int status)
{
	while (1);
}
        
#ifdef __cplusplus
    }
#endif
