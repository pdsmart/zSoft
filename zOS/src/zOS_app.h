/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            zOS_app.h
// Created:         January 2019 - April 2020
// Author(s):       Philip Smart
// Description:     ZPU and Teensy 3.5 (Freescale K64F) OS and test application.
//                  This program implements methods, tools, test mechanisms and performance analysers such
//                  that a ZPU/K64F cpu and their encapsulating SoC can be used, tested, debugged,
//                  validated and rated in terms of performance.
//                                                         
// Credits:         
// Copyright:       (c) 2019 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial module written for STORM processor then changed to ZPU.
//                  April 2020     - With the advent of the tranZPUter SW, forked ZPUTA into zOS and 
//                                   enhanced to work with both the ZPU and K64F for the original purpose
//                                   of testing but also now for end application programming using the 
//                                   features of zOS where applicable.
//                  June 2021      - Remove thread control variables.
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
#ifndef ZOS_APP_H
#define ZOS_APP_H

// Constants.
#define MAX_FILE_HANDLE             3                                  // Maximum number of file handles to open per logical drive.

// Global parameters accessible in applications.
typedef struct {
    uint8_t                  fileInUse;                                // Flag to indicate if file[0] is in use.
    FIL                      File[MAX_FILE_HANDLE];                    // Maximum open file objects
    FATFS                    FatFs[FF_VOLUMES];                        // Filesystem object for each logical drive
    BYTE                     Buff[512];                                // Working buffer
    DWORD                    Sector;                                   // Sector to read
  #if defined __K64F__
	uint32_t volatile        *millis;                                  // Pointer to the K64F millisecond tick
  #endif
} GLOBALS;

#endif // ZOS_APP_H
