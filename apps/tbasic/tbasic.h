/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tbasic.h
// Created:         April 2029
// Author(s):       Miskatino (Miskatino Basic), RodionGork (TinyBasic), Philip Smart (zOS)
// Description:     Standalone App for the zOS/ZPU test application to provide a basic interpreter.
//                  This program implements a loadable appliation which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//                  Basic is a very useable language to make quick tests and thus I have ported the 
//                  TinyBasic/Miskatino fork to the ZPU/K64F and enhanced accordingly.
//
// Credits:         
// Copyright:       (C) Miskatino MiskatinoBasic -> 2018, (C) RodionGork TinyBasic-> 2017
//                  (c) 2019-2020 Philip Smart <philip.smart@net2net.org> zOS/ZPUTA Basic
//
// History:         April 2020   - Ported the Miskatino version to work on the ZPU and K64F, updatesdto
//                                 function with the K64F processor and zOS.
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
#ifndef TBASIC_H
#define TBASIC_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.

// Application execution constants.
//

// Components to be embedded in the program.
//

#ifdef __cplusplus
}
#endif
#endif // TBASIC_H
