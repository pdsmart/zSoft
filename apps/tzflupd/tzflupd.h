/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzflupd.h
// Created:         Jan 2021
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility to update the program in the K64F ARM Processor. This
//                  application takes a binary and flashes it into the K64F Flash Memory in order to 
//                  update zOS/ZPUTA or flash any other program (NB. flashing any other program may
//                  need an external OpenSDA programmer to reprogram the K64F with zOS).
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         Jan 2021 - Initial write of the TranZPUter software using NXP/Freescale flash
//                             driver source.
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
//
// For the Freescale driver code, please see the license at the top of the driver source file. 
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TZFLUPD_H
#define TZFLUPD_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.
//
#define FLASH_PROTECTION_START_ADDR     0x400
#define FLASH_PROTECTION_SIZE           0x10

// Components to be embedded in the program.
//
// Filesystem components to be embedded in the program.

// Application execution constants.
//

// Macros to enable/disable the K64F interrupts.
//
#define __disable_irq() __asm__ volatile("CPSID i":::"memory");
#define __enable_irq()	__asm__ volatile("CPSIE i":::"memory");

// Following two macros pulled from the FRDM code due to a clash between Teensy libraries and their libraries. We depend on the FRDM libraries for the flash routines.
//
/**
  brief   Get Priority Mask
  details Returns the current state of the priority mask bit from the Priority Mask Register.
  return               Priority Mask value
 */
__attribute__( ( always_inline ) ) inline uint32_t __get_PRIMASK(void)
{
  uint32_t result;

  __asm__ volatile ("MRS %0, primask" : "=r" (result) );
  return(result);
}

/**
  brief   Set Priority Mask
  details Assigns the given value to the Priority Mask Register.
  param [in]    priMask  Priority Mask
 */
__attribute__( ( always_inline ) ) inline void __set_PRIMASK(uint32_t priMask)
{
  __asm__ volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
}

#ifdef __cplusplus
}
#endif
#endif // TZFLUPD_H
