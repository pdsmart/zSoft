////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            k64f_soc.c
// Created:         January 2019 - April 2020
// Author(s):       Philip Smart
// Description:     K64F System On a Chip utilities.
//                  A set of utilities specific to interaction with the K64F SoC hardware.
//
// Credits:         
// Copyright:       (c) 2019 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written.
//                  April 2020     - Duplicated the zpu_soc for the Freescale K64F used in the Teensy3.5
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

#include <stdlib.h>
#include <string.h>
#define uint32_t __uint32_t
#define uint16_t __uint16_t
#define uint8_t  __uint8_t
#define int32_t  __int32_t
#define int16_t  __int16_t
#define int8_t   __int8_t
#include "k64f_soc.h"
#include "xprintf.h"

// Global scope variables.
#ifdef USE_BOOT_ROM
    SOC_CONFIG                         cfgSoC;
#else
    SOC_CONFIG                         cfgSoC  = { .addrRAM        = RAM_ADDR,
                                                   .sizeRAM        = RAM_SIZE,
                                                   .addrFRAM       = FRAM_ADDR,
                                                   .sizeFRAM       = FRAM_SIZE,
                                                   .addrFRAMNV     = FRAMNV_ADDR,
                                                   .sizeFRAMNV     = FRAMNV_SIZE,
                                                   .addrFRAMNVC    = FRAMNVC_ADDR,
                                                   .sizeFRAMNVC    = FRAMNVC_SIZE,
                                                   .resetVector    = 0,
                                                   .cpuMemBaseAddr = 0,
                                                   .stackStartAddr = 0,
                                                   .sysFreq        = CLK_FREQ,
                                                   .memFreq        = CLK_FREQ,
                                                   .implRAM        = RAM_IMPL,
                                                   .implFRAM       = FRAM_IMPL,
                                                   .implFRAMNV     = FRAMNV_IMPL,
                                                   .implFRAMNVC    = FRAMNVC_IMPL,
                                                   .implPS2        = 0,
                                                   .implSPI        = 0,
                                                   .implSD         = 0,
                                                   .sdCardNo       = 0,
                                                   .implIntrCtl    = 0,
                                                   .intrChannels   = 0,
                                                   .implTimer1     = 0,
                                                   .timer1No       = 0 };
#endif


// Method to populate the Configuration structure, using in-built values as the K64F has a static design
// and components are well known.
void setupSoCConfig(void)
{
    uint32_t pnt = 0x0;
    // Teensy 3.5 uses the Freescale K64FX512 which has fixed hardware definitions.
    cfgSoC.addrRAM        = RAM_ADDR;
    cfgSoC.sizeRAM        = RAM_SIZE;
    cfgSoC.addrFRAM       = FRAM_ADDR;
    cfgSoC.sizeFRAM       = FRAM_SIZE;
    cfgSoC.addrFRAMNV     = FRAMNV_ADDR;
    cfgSoC.sizeFRAMNV     = FRAMNV_SIZE;
    cfgSoC.addrFRAMNVC    = FRAMNVC_ADDR;
    cfgSoC.sizeFRAMNVC    = FRAMNVC_SIZE;
    cfgSoC.resetVector    = *(uint32_t *)(pnt+4);
    cfgSoC.cpuMemBaseAddr = 0;
    // Reading location 0x00000000 just after reset seems to lock up the CPU, hence this convoluted method!!
    cfgSoC.stackStartAddr = *(uint8_t *)(pnt+3) << 24 | *(uint8_t *)(pnt+2) << 16 | *(uint8_t *)(pnt+1) << 8 | 0xF8;
    cfgSoC.sysFreq        = CLK_FREQ;
    cfgSoC.memFreq        = CLK_FREQ;
    cfgSoC.implFRAM       = FRAM_IMPL;
    cfgSoC.implFRAMNV     = FRAMNV_IMPL;
    cfgSoC.implFRAMNVC    = FRAMNVC_IMPL;
    cfgSoC.implRAM        = RAM_IMPL;
    cfgSoC.implPS2        = PS2_IMPL;
    cfgSoC.implSPI        = SPI_IMPL;
    cfgSoC.implSD         = SD_IMPL;
    cfgSoC.sdCardNo       = SD_DEVICE_CNT;
    cfgSoC.implIntrCtl    = 0;
    cfgSoC.intrChannels   = 0;
    cfgSoC.implTimer1     = 0;
    cfgSoC.timer1No       = 0;
}

// Method to show the current configuration via the primary uart channel.
//
#if !defined(FUNCTIONALITY) || FUNCTIONALITY <= 1
void showSoCConfig(void)
{
    xputs("K64F SoC Configuration:\n");
    xputs("On-board Devices:\n");
    if(cfgSoC.implFRAM)      { xprintf("    FRAM      (%08X:%08X).\n", cfgSoC.addrFRAM,     cfgSoC.addrFRAM     + cfgSoC.sizeFRAM); }
    if(cfgSoC.implFRAM)      { xprintf("    FRAMNV    (%08X:%08X).\n", cfgSoC.addrFRAMNV,   cfgSoC.addrFRAMNV   + cfgSoC.sizeFRAMNV); }
    if(cfgSoC.implFRAM)      { xprintf("    FRAMNVC   (%08X:%08X).\n", cfgSoC.addrFRAMNVC,  cfgSoC.addrFRAMNVC  + cfgSoC.sizeFRAMNVC); }
    if(cfgSoC.implRAM)       { xprintf("    RAM       (%08X:%08X).\n", cfgSoC.addrRAM,      cfgSoC.addrRAM      + cfgSoC.sizeRAM); }
    if(cfgSoC.implSD)        { xprintf("    SD CARD   (Devices =%02d).\n", (uint8_t)cfgSoC.sdCardNo); }
    if(cfgSoC.implTimer1)    { xprintf("    TIMER1    (Timers  =%02d).\n", (uint8_t)cfgSoC.timer1No); }
    if(cfgSoC.implIntrCtl)   { xprintf("    INTR CTRL (Channels=%02d).\n", (uint8_t)cfgSoC.intrChannels); }
    if(cfgSoC.implPS2)       { xputs("    PS2\n"); }
    if(cfgSoC.implSPI)       { xputs("    SPI\n"); }
    xputs("Addresses:\n");
    xprintf("    CPU Reset Vector Address = %08X\n",        cfgSoC.resetVector); 
    xprintf("    CPU Memory Start Address = %08X\n",        cfgSoC.cpuMemBaseAddr);
    xprintf("    Stack Start Address      = %08X\n",        cfgSoC.stackStartAddr);
    xputs("Misc:\n");
    xprintf("    System Clock Freq        = %d.%04dMHz\n",  (cfgSoC.sysFreq / 1000000), cfgSoC.sysFreq - ((cfgSoC.sysFreq / 1000000) * 1000000));
   #ifdef DRV_CFC
    xprintf("    CFC                      = %08X\n", DRV_CFC);
   #endif
   #ifdef DRV_MMC
    xprintf("    MMC                      = %08X\n", DRV_MMC);
   #endif    
    xputs("\n");
}

// Function to print out the ZPU Id in text form.
void printCPU(void)
{
    xputs("K64FX512");
}
#endif

#ifdef __cplusplus
    }
#endif
