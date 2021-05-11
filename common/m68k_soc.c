////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            m68k_soc.c
// Created:         January 2019
// Author(s):       Philip Smart
// Description:     M68000 System On a Chip utilities.
//                  A set of utilities specific to interaction with the ZPU SoC hardware.
//
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written.
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

//#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include "uart.h"
#include "m68k_soc.h"

// Global scope variables.
#ifdef USE_BOOT_ROM
    SOC_CONFIG                         cfgSoC;
#else
    SOC_CONFIG                         cfgSoC  = { .addrInsnBRAM   = INSN_BRAM_ADDR,
                                                   .sizeInsnBRAM   = INSN_BRAM_SIZE,
                                                   .addrBRAM       = BRAM_ADDR,
                                                   .sizeBRAM       = BRAM_SIZE,
                                                   .addrRAM        = RAM_ADDR,
                                                   .sizeRAM        = RAM_SIZE,
                                                   .addrSDRAM      = SDRAM_ADDR,
                                                   .sizeSDRAM      = SDRAM_SIZE,
                                                   .addrWBSDRAM    = WB_SDRAM_ADDR,
                                                   .sizeWBSDRAM    = WB_SDRAM_SIZE,
                                                   .resetVector    = CPU_RESET_ADDR,
                                                   .cpuMemBaseAddr = CPU_MEM_START,
                                                   .stackStartAddr = STACK_BRAM_ADDR,
                                                   .m68kId         = M68K_ID,
                                                   .sysFreq        = CLK_FREQ,
                                                   .memFreq        = CLK_FREQ,
                                                   .wbMemFreq      = CLK_FREQ,
                                                   .implSoCCFG     = 0,
                                                   .implWB         = WB_IMPL,
                                                   .implWBSDRAM    = WB_SDRAM_IMPL,
                                                   .implWBI2C      = WB_I2C_IMPL,
                                                   .implInsnBRAM   = INSN_BRAM_IMPL,
                                                   .implBRAM       = BRAM_IMPL,
                                                   .implRAM        = RAM_IMPL,
                                                   .implSDRAM      = SDRAM_IMPL,
                                                   .implIOCTL      = IOCTL_IMPL,
                                                   .implPS2        = PS2_IMPL,
                                                   .implSPI        = SPI_IMPL,
                                                   .implSD         = SD_IMPL,
                                                   .sdCardNo       = SD_DEVICE_CNT,
                                                   .implIntrCtl    = INTRCTL_IMPL,
                                                   .intrChannels   = INTRCTL_CHANNELS,
                                                   .implTimer1     = TIMER1_IMPL,
                                                   .timer1No       = TIMER1_TIMERS_CNT };
#endif


// Method to populate the Configuration structure, initially using in-built values from compile time
// which are overriden with values stored in the SoC if available.
void setupSoCConfig(void)
{
    // If the SoC Configuration register is implemented in the SoC, overwrite the compiled constants with those in the chip register.
    if( IS_IMPL_SOCCFG )
    {
        cfgSoC.addrInsnBRAM   = SOCCFG(SOCCFG_BRAMINSNADDR);
        cfgSoC.sizeInsnBRAM   = SOCCFG(SOCCFG_BRAMINSNSIZE);
        cfgSoC.addrBRAM       = SOCCFG(SOCCFG_BRAMADDR);
        cfgSoC.sizeBRAM       = SOCCFG(SOCCFG_BRAMSIZE);
        cfgSoC.addrRAM        = SOCCFG(SOCCFG_RAMADDR);
        cfgSoC.sizeRAM        = SOCCFG(SOCCFG_RAMSIZE);
        cfgSoC.addrSDRAM      = SOCCFG(SOCCFG_SDRAMADDR);
        cfgSoC.sizeSDRAM      = SOCCFG(SOCCFG_SDRAMSIZE);
        cfgSoC.addrWBSDRAM    = SOCCFG(SOCCFG_WBSDRAMADDR);
        cfgSoC.sizeWBSDRAM    = SOCCFG(SOCCFG_WBSDRAMSIZE);
        cfgSoC.resetVector    = SOCCFG(SOCCFG_CPURSTADDR);
        cfgSoC.cpuMemBaseAddr = SOCCFG(SOCCFG_CPUMEMSTART);
        cfgSoC.stackStartAddr = SOCCFG(SOCCFG_STACKSTART);
        cfgSoC.m68kId         = SOCCFG(SOCCFG_M68K_ID);
        cfgSoC.sysFreq        = SOCCFG(SOCCFG_SYSFREQ);
        cfgSoC.memFreq        = SOCCFG(SOCCFG_MEMFREQ);
        cfgSoC.wbMemFreq      = SOCCFG(SOCCFG_WBMEMFREQ);
        cfgSoC.implSoCCFG     = 1;
        cfgSoC.implWB         = IS_IMPL_WB != 0;
        cfgSoC.implWBSDRAM    = IS_IMPL_WB_SDRAM != 0;
        cfgSoC.implWBI2C      = IS_IMPL_WB_I2C != 0;
        cfgSoC.implInsnBRAM   = IS_IMPL_INSN_BRAM != 0;
        cfgSoC.implBRAM       = IS_IMPL_BRAM != 0;
        cfgSoC.implRAM        = IS_IMPL_RAM != 0;
        cfgSoC.implSDRAM      = IS_IMPL_SDRAM != 0;
        cfgSoC.implIOCTL      = IS_IMPL_IOCTL != 0;
        cfgSoC.implPS2        = IS_IMPL_PS2 != 0;
        cfgSoC.implSPI        = IS_IMPL_SPI != 0;
        cfgSoC.implSD         = IS_IMPL_SD != 0;
        cfgSoC.sdCardNo       = (uint8_t)(SOCCFG_SD_DEVICES);
        cfgSoC.implIntrCtl    = IS_IMPL_INTRCTL != 0;
        cfgSoC.intrChannels   = (uint8_t)(SOCCFG_INTRCTL_CHANNELS);
        cfgSoC.implTimer1     = IS_IMPL_TIMER1 != 0;
        cfgSoC.timer1No       = (uint8_t)(SOCCFG_TIMER1_TIMERS);
    #ifndef USE_BOOT_ROM
    }
    #else
    } else
    {
        // Store builtin constants into structure which will be used when the SoC configuration module isnt implemented.
        cfgSoC.addrInsnBRAM   = INSN_BRAM_ADDR;
        cfgSoC.sizeInsnBRAM   = INSN_BRAM_SIZE;
        cfgSoC.addrBRAM       = BRAM_ADDR;
        cfgSoC.sizeBRAM       = BRAM_SIZE;
        cfgSoC.addrRAM        = RAM_ADDR;
        cfgSoC.sizeRAM        = RAM_SIZE;
        cfgSoC.addrSDRAM      = SDRAM_ADDR;
        cfgSoC.sizeSDRAM      = SDRAM_SIZE;
        cfgSoC.addrWBSDRAM    = WB_SDRAM_ADDR;
        cfgSoC.sizeWBSDRAM    = WB_SDRAM_SIZE;
        cfgSoC.resetVector    = CPU_RESET_ADDR;
        cfgSoC.cpuMemBaseAddr = CPU_MEM_START;
        cfgSoC.stackStartAddr = STACK_BRAM_ADDR;
        cfgSoC.m68kId         = M68K_ID;
        cfgSoC.sysFreq        = CLK_FREQ;
        cfgSoC.memFreq        = CLK_FREQ;
        cfgSoC.wbMemFreq      = CLK_FREQ;
        cfgSoC.implSoCCFG     = 0;
        cfgSoC.implWB         = WB_IMPL;
        cfgSoC.implWBSDRAM    = WB_SDRAM_IMPL;
        cfgSoC.implWBI2C      = WB_I2C_IMPL;
        cfgSoC.implInsnBRAM   = INSN_BRAM_IMPL;
        cfgSoC.implBRAM       = BRAM_IMPL ;
        cfgSoC.implRAM        = RAM_IMPL;
        cfgSoC.implSDRAM      = SDRAM_IMPL;;
        cfgSoC.implIOCTL      = IOCTL_IMPL;;
        cfgSoC.implPS2        = PS2_IMPL;
        cfgSoC.implSPI        = SPI_IMPL;
        cfgSoC.implSD         = IMPL_SD;
        cfgSoC.sdCardNo       = SD_DEVICE_CNT;
        cfgSoC.implIntrCtl    = INTRCTL_IMPL;
        cfgSoC.intrChannels   = INTRCTL_CHANNELS;
        cfgSoC.implTimer1     = TIMER1_IMPL;
        cfgSoC.timer1No       = TIMER1_TIMERS_CNT;
    }
    #endif
}

// Method to show the current configuration via the primary uart channel.
//
#if !defined(FUNCTIONALITY) || FUNCTIONALITY <= 1
void showSoCConfig(void)
{
  #if defined(__ZOS__) || defined(__ZPUTA__)
    printf("SoC Configuration");
    if(cfgSoC.implSoCCFG)    { printf(" (from SoC config)"); }
    printf(":\nDevices implemented:\n");
    if(cfgSoC.implWBSDRAM)   { printf("    WB SDRAM  (%08X:%08X).\n", cfgSoC.addrWBSDRAM,  cfgSoC.addrWBSDRAM  + cfgSoC.sizeWBSDRAM); }
    if(cfgSoC.implSDRAM)     { printf("    SDRAM     (%08X:%08X).\n", cfgSoC.addrSDRAM,    cfgSoC.addrSDRAM    + cfgSoC.sizeSDRAM); }
    if(cfgSoC.implInsnBRAM)  { printf("    INSN BRAM (%08X:%08X).\n", cfgSoC.addrInsnBRAM, cfgSoC.addrInsnBRAM + cfgSoC.sizeInsnBRAM); }
    if(cfgSoC.implBRAM)      { printf("    BRAM      (%08X:%08X).\n", cfgSoC.addrBRAM,     cfgSoC.addrBRAM     + cfgSoC.sizeBRAM); }
    if(cfgSoC.implRAM)       { printf("    RAM       (%08X:%08X).\n", cfgSoC.addrRAM,      cfgSoC.addrRAM      + cfgSoC.sizeRAM); }
    if(cfgSoC.implSD)        { printf("    SD CARD   (Devices =%02d).\n", (uint8_t)cfgSoC.sdCardNo); }
    if(cfgSoC.implTimer1)    { printf("    TIMER1    (Timers  =%02d).\n", (uint8_t)cfgSoC.timer1No); }
    if(cfgSoC.implIntrCtl)   { printf("    INTR CTRL (Channels=%02d).\n", (uint8_t)cfgSoC.intrChannels); }
    if(cfgSoC.implWB)        { printf("    WISHBONE BUS\n"); }
    if(cfgSoC.implWBI2C)     { printf("    WB I2C\n"); }
    if(cfgSoC.implIOCTL)     { printf("    IOCTL\n"); }
    if(cfgSoC.implPS2)       { printf("    PS2\n"); }
    if(cfgSoC.implSPI)       { printf("    SPI\n"); }
    printf("Addresses:\n");
    printf("    CPU Reset Vector Address = %08X\n",        cfgSoC.resetVector); 
    printf("    CPU Memory Start Address = %08X\n",        cfgSoC.cpuMemBaseAddr);
    printf("    Stack Start Address      = %08X\n",        cfgSoC.stackStartAddr);
    printf("Misc:\n");
    printf("    M68K Id                  = %04X\n",        cfgSoC.m68kId);
    printf("    System Clock Freq        = %d.%04dMHz\n",  (cfgSoC.sysFreq / 1000000), cfgSoC.sysFreq - ((cfgSoC.sysFreq / 1000000) * 1000000));
    if(cfgSoC.implSDRAM)
        printf("    SDRAM Clock Freq         = %d.%04dMHz\n",  (cfgSoC.memFreq / 1000000), cfgSoC.memFreq - ((cfgSoC.memFreq / 1000000) * 1000000));
    if(cfgSoC.implWBSDRAM)
        printf("    Wishbone SDRAM Clock Freq= %d.%04dMHz\n",  (cfgSoC.wbMemFreq / 1000000), cfgSoC.wbMemFreq - ((cfgSoC.wbMemFreq / 1000000) * 1000000));
   #ifdef DRV_CFC
    printf("    CFC                      = %08X\n", DRV_CFC);
   #endif
   #ifdef DRV_MMC
    printf("    MMC                      = %08X\n", DRV_MMC);
   #endif    
    //printf("\n");
  #else
    puts("SoC Configuration");
    if(cfgSoC.implSoCCFG)    { puts(" (from SoC config)"); }
    puts(":\nDevices implemented:\n");
    if(cfgSoC.implWBSDRAM)   { puts("    WB SDRAM  ("); printdhex(cfgSoC.addrWBSDRAM);  puts(":"); printdhex(cfgSoC.addrWBSDRAM  + cfgSoC.sizeWBSDRAM);  puts(").\n"); }
    if(cfgSoC.implSDRAM)     { puts("    SDRAM     ("); printdhex(cfgSoC.addrSDRAM);    puts(":"); printdhex(cfgSoC.addrSDRAM    + cfgSoC.sizeSDRAM);    puts(").\n"); }
    if(cfgSoC.implInsnBRAM)  { puts("    INSN BRAM ("); printdhex(cfgSoC.addrInsnBRAM); puts(":"); printdhex(cfgSoC.addrInsnBRAM + cfgSoC.sizeInsnBRAM); puts(").\n"); }
    if(cfgSoC.implBRAM)      { puts("    BRAM      ("); printdhex(cfgSoC.addrBRAM);     puts(":"); printdhex(cfgSoC.addrBRAM     + cfgSoC.sizeBRAM);     puts(").\n"); }
    if(cfgSoC.implRAM)       { puts("    RAM       ("); printdhex(cfgSoC.addrRAM);      puts(":"); printdhex(cfgSoC.addrRAM      + cfgSoC.sizeRAM);      puts(").\n"); }
    if(cfgSoC.implSD)        { puts("    SD CARD   (Devices ="); printhexbyte((uint8_t)cfgSoC.sdCardNo);     puts(").\n"); }
    if(cfgSoC.implTimer1)    { puts("    TIMER1    (Timers  ="); printnibble( (uint8_t)cfgSoC.timer1No);     puts(").\n"); }
    if(cfgSoC.implIntrCtl)   { puts("    INTR CTRL (Channels="); printhexbyte((uint8_t)cfgSoC.intrChannels); puts(").\n"); }
    if(cfgSoC.implWB)        { puts("    WISHBONE BUS\n"); }
    if(cfgSoC.implWB)        { puts("    WB I2C\n"); }
    if(cfgSoC.implIOCTL)     { puts("    IOCTL\n"); }
    if(cfgSoC.implPS2)       { puts("    PS2\n"); }
    if(cfgSoC.implSPI)       { puts("    SPI\n"); }
    puts("Addresses:\n");
    puts("    CPU Reset Vector Address = "); printdhex(cfgSoC.resetVector); puts("\n");
    puts("    CPU Memory Start Address = "); printdhex(cfgSoC.cpuMemBaseAddr); puts("\n");
    puts("    Stack Start Address      = "); printdhex(cfgSoC.stackStartAddr); puts("\n");
    puts("Misc:\n");
    puts("    M68K Id                  = "); printhex((uint16_t)cfgSoC.m68kId); puts("\n");
    puts("    System Clock Freq        = "); printdhex(cfgSoC.sysFreq); puts("\n");
    if(cfgSoC.implSDRAM)
        puts("    SDRAM Clock Freq         = "); printdhex(cfgSoC.memFreq); puts("\n");
    if(cfgSoC.implWBSDRAM)
        puts("    Wishbone SDRAM Clock Freq= "); printdhex(cfgSoC.wbMemFreq); puts("\n");
   #ifdef DRV_CFC
    puts("    CFC                      = "); printdhex(DRV_CFC); puts("\n");
   #endif
   #ifdef DRV_MMC
    puts("    MMC                      = "); printdhex(DRV_MMC); puts("\n");
   #endif    
    puts("\n");
  #endif
}

// Function to print out the M68000 Id in text form.
void printM68KId(uint32_t m68kId)
{
    switch((uint8_t)(m68kId >> 8))
    {
        case M68K_ID_M68008:
            printf("M68008");
            break;

        case M68K_ID_M68000:
            printf("M68000");
            break;

        case M68K_ID_M68020:
            printf("M68020");
            break;

        default:
            printf("Unknown");
            break;
    }
}
#endif

#ifdef __cplusplus
    }
#endif
