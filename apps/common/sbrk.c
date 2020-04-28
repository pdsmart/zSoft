////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            sbrk.c
// Created:         January 2019
// Author(s):       Philip Smart
// Description:     System heap allocation for use when linking with the stdlib. This method is called
//                  by the stdlib malloc to allocate a chunk of heap for it to use.
//
// Credits:         
// Copyright:       modifications (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2020   - Utilities assembled from various sources.
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
#include <errno.h>


extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;

#ifndef STACK_MARGIN
#define STACK_MARGIN  8192
#endif

//char *__brkval = (char *)&_ebss;
//
//void * _sbrk(int incr)
//{
//	char *prev, *stack;
//
//	prev = __brkval;
//	if (incr != 0) {
//      #if defined __K64F__
//		__asm__ volatile("mov %0, sp" : "=r" (stack) ::);
//      #endif
//		if (prev + incr >= stack - STACK_MARGIN) {
////			errno = ENOMEM;
//			return (void *)-1;
//		}
//		__brkval = prev + incr;
//	}
//	return prev;
//}

extern char __HeapBase;   /**< Defined by the linker */
extern char __HeapLimit;  /**< Defined by the linker */
void * _sbrk(int incr)
{
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0)
  {
    heap_end = &__HeapBase;
  }
  //xprintf("Head_end=%08lx, incr=%08lx\n", heap_end, incr);

  prev_heap_end = heap_end;
  if ((heap_end + incr) > (char*) &__HeapLimit)
  {
    //xprintf("Ovfl Head_end=%08lx, incr=%08lx\n", heap_end, incr);
    //errno = ENOMEM;
    return ((void*)-1); // error - no more memory
  }
  heap_end += incr;
  //xprintf("Head_end=%08lx, incr=%08lx\n", heap_end, incr);
  return prev_heap_end;
}

#ifdef __cplusplus
    }
#endif
