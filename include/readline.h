/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            readline.h
// Created:         April 2020
// Author(s):       Alfred Klomp (www.alfredklomp.com) - original readline.c
//                  Philip Smart - Port to K64f/ZPU and additions of history buffer logic.
// Description:     A readline module for embedded systems without the overhead of GNU readline or the
//                  need to use > C99 standard as the ZPU version of GCC doesnt support it!
//                  The readline modules originally came from Alfred Klomp's Raduino project and whilst
//                  I was searching for an alternative to GNU after failing to compile it with the ZPU
//                  version of GCC (as well as the size), I came across RADUINO and this readline
//                  module was very well written and easily portable to this project.
//
// Credits:         
// Copyright:       (c) -> 2016    - Alfred Klomp (www.alfredklomp.com)
//                  (C) 2020       - Philip Smart <philip.smart@net2net.org> porting and history buf.
//
// History:         April 2020     - With the advent of the tranZPUter SW, I needed a better command
//                                   line module for entering, recalling and editting commands. This
//                                   module after adding history buffer mechanism is the ideal
//                                   solution.
//                                   features of zOS where applicable.
//
// Notes:           See Makefile to enable/disable conditional components
//                  __SD_CARD__           - Add the SDCard logic.
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

uint8_t *readline (uint8_t *, int, const char *);


#ifdef __cplusplus
    }
#endif
