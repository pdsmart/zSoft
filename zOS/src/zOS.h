/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            zOS.h
// Created:         January 2019 - April 2020
// Author(s):       Philip Smart
// Description:     ZPU and Teensy 3.5 (Freescale K64F) OS and test application.
//                  This program implements methods, tools, test mechanisms and performance analysers such
//                  that a ZPU/K64F cpu and their encapsulating SoC can be used, tested, debugged,
//                  validated and rated in terms of performance.
//
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial module written for STORM processor then changed to ZPU.
//                  April 2020     - With the advent of the tranZPUter SW, forked ZPUTA into zOS and 
//                                   enhanced to work with both the ZPU and K64F for the original purpose
//                                   of testing but also now for end application programming using the 
//                                   features of zOS where applicable.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This source file is free software: you can redistribute it and/or modify
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
#ifndef ZOS_H
#define ZOS_H

#ifdef __cplusplus
extern "C" {
#endif

// Constants.

// Components to be embedded in the program.
//
#define BUILTIN_DEFAULT             1
#define BUILTIN_READLINE            1
#define BUILTIN_FS_LOAD             1
#define BUILTIN_FS_EXEC             1

// Memory components to be embedded in the program.
#define BUILTIN_MEM_CLEAR           1
#define BUILTIN_MEM_COPY            1
#define BUILTIN_MEM_DIFF            1
#define BUILTIN_MEM_DUMP            1
#define BUILTIN_MEM_EDIT_BYTES      1
#define BUILTIN_MEM_EDIT_HWORD      1
#define BUILTIN_MEM_EDIT_WORD       1
#define BUILTIN_MEM_SRCH            0

// Miscellaneous components to be embedded in this program.
#define BUILTIN_MISC_SETTIME        0

// Application execution constants.
//
#if defined __ZPU__
  #define APP_CMD_EXTENSION         "ZPU"
#elif defined __K64F__
  #define APP_CMD_EXTENSION         "K64"
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
#define HISTORY_FILE                "zOS.hst"
#define AUTOEXEC_FILE               "autoexec.bat"
#define APP_CMD_LOAD_ADDR           OS_APPADDR
#define APP_CMD_EXEC_ADDR           OS_APPADDR
#define APP_CMD_BIN_DIR             "bin"
#define APP_CMD_BIN_DRIVE           0

// Prototypes.
void    interrupt_handler();
void    initTimer();
void    enableTimer();
void    PrintFSCode(FRESULT);
int     cmdProcessor(void);
extern void _restart(void);

#ifndef TOOLS_H
// Global scope variables.
//static struct stat           statbuf;
FRESULT                      fResult;
UINT                         bw;
static GLOBALS               G;
extern SOC_CONFIG            cfgSoC;

volatile UINT                Timer;                                    /* Performance timer (100Hz increment) */
#endif

#ifdef __cplusplus
}
#endif
#endif // ZOS_H
