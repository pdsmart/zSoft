/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            rtctime.h
// Created:         July 2019
// Author(s):       Philip Smart
// Description:     Standalone App for the zOS/ZPU test application.
//                  This program implements a loadable appliation which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         July 2019    - Initial framework creation.
//                  April 2020   - Updates to function with the K64F processor and zOS.
//
// Notes:           See Makefile to enable/disable conditional components
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
#ifndef RTCTIME_H
#define RTCTIME_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.

// Application execution constants.
//

// Components to be embedded in the program.
//
#define BUILTIN_DEFAULT             1
// Disk low level components to be embedded in the program.
#define BUILTIN_DISK_DUMP           0
#define BUILTIN_DISK_STATUS         0
// Disk buffer components to be embedded in the program.
#define BUILTIN_BUFFER_DUMP         0
#define BUILTIN_BUFFER_EDIT         0
#define BUILTIN_BUFFER_READ         0
#define BUILTIN_BUFFER_WRITE        0
#define BUILTIN_BUFFER_FILL         0
#define BUILTIN_BUFFER_LEN          0
// Memory components to be embedded in the program.
#define BUILTIN_MEM_CLEAR           0
#define BUILTIN_MEM_DUMP            0
#define BUILTIN_MEM_EDIT_BYTES      0
#define BUILTIN_MEM_EDIT_HWORD      0
#define BUILTIN_MEM_EDIT_WORD       0
// Hardware components to be embedded in the program.
#define BUILTIN_HW_SHOW_REGISTER    0
#define BUILTIN_HW_TEST_TIMERS      0
// Filesystem components to be embedded in the program.
#define BUILTIN_FS_STATUS           0
#define BUILTIN_FS_DIRLIST          0
#define BUILTIN_FS_OPEN             0
#define BUILTIN_FS_CLOSE            0
#define BUILTIN_FS_SEEK             0
#define BUILTIN_FS_READ             0
#define BUILTIN_FS_CAT              0
#define BUILTIN_FS_INSPECT          0
#define BUILTIN_FS_WRITE            0
#define BUILTIN_FS_TRUNC            0
#define BUILTIN_FS_RENAME           0
#define BUILTIN_FS_DELETE           0
#define BUILTIN_FS_CREATEDIR        0
#define BUILTIN_FS_ALLOCBLOCK       0
#define BUILTIN_FS_CHANGEATTRIB     0
#define BUILTIN_FS_CHANGETIME       0
#define BUILTIN_FS_COPY             0
#define BUILTIN_FS_CHANGEDIR        0
#define BUILTIN_FS_CHANGEDRIVE      0
#define BUILTIN_FS_SHOWDIR          0
#define BUILTIN_FS_SETLABEL         0
#define BUILTIN_FS_CREATEFS         0
#define BUILTIN_FS_LOAD             0
#define BUILTIN_FS_DUMP             0
#define BUILTIN_FS_CONCAT           0
#define BUILTIN_FS_XTRACT           0
#define BUILTIN_FS_SAVE             0
#define BUILTIN_FS_EXEC             0
// Test components to be embedded in the program.
#define BUILTIN_TST_DHRYSTONE       0
#define BUILTIN_TST_COREMARK        0
// Miscellaneous components to be embedded in this program.
#define BUILTIN_MISC_HELP           0
#define BUILTIN_MISC_SETTIME        1

// Prototypes.
uint32_t app(uint32_t, uint32_t);

// Global scope variables within the ZPUTA memory space.
GLOBALS                      *G;
SOC_CONFIG                   *cfgSoC;
#if defined __ZPU__
struct __file                *__iob[3];
#endif

// Global scope variables in the app memory space.
volatile UINT                Timer;                                    /* Performance timer (100Hz increment) */

#ifdef __cplusplus
}
#endif
#endif // RTCTIME_H
