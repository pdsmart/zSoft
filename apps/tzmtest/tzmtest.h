/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzmtest.h
// Created:         May 2021
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing to test the onboard tranZPUter card memory
//                  or externally accessible FPGA BRAM.
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
#ifndef TZMTEST_H
#define TZMTEST_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.

// Application execution constants.
//

// Components to be embedded in the program.
//
// Memory components to be embedded in the program.
#define BUILTIN_MEM_TEST            1

#ifdef __cplusplus
}
#endif
#endif // TZMTEST_H
