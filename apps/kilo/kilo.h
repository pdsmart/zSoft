/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            kilo.h
// Created:         April 2020
// Author(s):       Salvatore Sanfilippo
//                  Philip Smart
// Description:     A very simple editor in less than 1-kilo lines of code (as counted
//                  by "cloc"). Does not depend on libcurses, directly emits VT100
//                  escapes on the terminal.
//                  This program implements a loadable application which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//
// Credits:         
// Copyright:       Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
//                  (c) 2019-2020 Philip Smart <philip.smart@net2net.org> zOS/ZPUTA enhancements, bug
//                  fixes and adaptation to the zOS/ZPUTA framework.
//
// History:         April 2020   - Ported v0.0.1 of Kilo and made quite a few changes for it to work
//                                 in an embedded environment.
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
#ifndef KILO_H
#define KILO_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.

// Application execution constants.
//

#ifdef __cplusplus
}
#endif
#endif // KILO_H
