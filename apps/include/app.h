////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            app.h
// Created:         January 2019
// Author(s):       Philip Smart
// Description:     zOS application definitions.
//                  Header file to define prototypes and constants specific to an application
//                  compilation.
//
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written.
//                  April 2020     - Tweaked for the K64F processor.
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
#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

// Constants.


// Redefinitions.
//#define malloc     sys_malloc
//#define realloc    sys_realloc
//#define calloc     sys_calloc
//#define free       sys_free

// For the ARM Cortex-M compiler, the standard filestreams in an app are set by the CRT0 startup code,
// the original reentrant definition is undefined as it is not needed in the app.
#if defined __K64F__
  #undef stdout
  #undef stdin
  #undef stderr
  FILE *stdout;
  FILE *stdin;
  FILE *stderr;
#endif

// Prototypes for integer math - needs a new common home!
int32_t  __divsi3(int32_t,   int32_t );
int32_t  __modsi3(int32_t,   int32_t );
uint32_t __udivsi3(uint32_t, uint32_t);
uint32_t __umodsi3(uint32_t, uint32_t);

// Prototypes
void     *sys_malloc(size_t);                                              // Allocate memory managed by the OS.
void     *sys_realloc(void *, size_t);                                     // Reallocate a block of memory managed by the OS.
void     *sys_calloc(size_t, size_t);                                      // Allocate and zero a block of memory managed by the OS.
void     sys_free(void *);                                                 // Free memory managed by the OS.
uint32_t app(uint32_t, uint32_t);

// Global scope variables within the ZPUTA memory space.
GLOBALS                      *G;
SOC_CONFIG                   *cfgSoC;
#if defined __ZPU__
struct __file                *__iob[3];
#endif

// Global scope variables in the app memory space.
volatile UINT                Timer;                                        /* Performance timer (100Hz increment) */

#ifdef __cplusplus
}
#endif

#endif
