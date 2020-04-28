////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            k64f_soc.h
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
#ifndef __K64FSOC_H__
#define __K64FSOC_H__

#ifdef __cplusplus
    extern "C" {
#endif


// Macro to omit code if deemed optional and the compile time flag MINIMUM_FUNCTIONALITY is defined.
#ifdef MINIMUM_FUNCTIONALITY
#define OPTIONAL(a) 
#else
#define OPTIONAL(a)  a
#endif

// System settings.
#define CLK_FREQ                       120000000UL                  // Default frequency of the Teensy3.5 K64F CPU

// Memory sizes and devices implemented - these can be ignored if the SoC Configuration register is implemented as this provides the exact build configuration.
#define FRAM_IMPL                      1
#define FRAMNV_IMPL                    1
#define FRAMNVC_IMPL                   1
#define RAM_IMPL                       1
#define PS2_IMPL                       1
#define SPI_IMPL                       1
#define SD_IMPL                        1
#define SD_DEVICE_CNT                  1
#define INTRCTL_IMPL                   1
#define INTRCTL_CHANNELS               16
#define TIMER1_IMPL                    1
#define TIMER1_TIMERS_CNT              1

#define FRAM_ADDR                      0x00000000
#define FRAM_SIZE                      0x0007FFFF
#define FRAMNV_ADDR                    0x10000000
#define FRAMNV_SIZE                    0x0001FFFF
#define FRAMNVC_ADDR                   0x14000000
#define FRAMNVC_SIZE                   0x00000FFF
#define RAM_ADDR                       0x1FFF0000
#define RAM_SIZE                       0x0003FFFF
#define STACK_BRAM_ADDR                0x00007800
#define STACK_BRAM_SIZE                0x000007FF
#define CPU_RESET_ADDR                 0x00000000
#define CPU_MEM_START                  0x00000000
#define BRAM_APP_START_ADDR            0x2000

// Debug only macros which dont generate code when debugging disabled.
#ifdef DEBUG

// Macro to print to the debug channel.
//
#define debugf(a, ...) ({\
            xprintf(a, ##__VA_ARGS__);\
           })
#define dbg_putchar(a) ({\
            xputc(a);\
           })
#define dbg_puts(a) ({\
            xputs(a);\
           })
#define dbg_breadcrumb(x) putc(x);

#else

#define dbg_putchar(a)
#define dbg_puts(a)
#define debugf(a, ...)
#define dbg_breadcrumb(x) 

#endif

// Prototypes.
void setupSoCConfig(void);
void showSoCConfig(void);
void printCPU(void);

// Configuration values.
typedef struct
{
    uint32_t                           addrFRAM;
    uint32_t                           sizeFRAM;
    uint32_t                           addrFRAMNV;
    uint32_t                           sizeFRAMNV;
    uint32_t                           addrFRAMNVC;
    uint32_t                           sizeFRAMNVC;
    uint32_t                           addrRAM;
    uint32_t                           sizeRAM;
    uint32_t                           resetVector;
    uint32_t                           cpuMemBaseAddr;
    uint32_t                           stackStartAddr;
    uint32_t                           sysFreq;
    uint32_t                           memFreq;
    uint8_t                            implRAM;
    uint8_t                            implFRAM;
    uint8_t                            implFRAMNV;
    uint8_t                            implFRAMNVC;
    uint8_t                            implPS2;
    uint8_t                            implSPI;
    uint8_t                            implSD;
    uint8_t                            sdCardNo;
    uint8_t                            implIntrCtl;
    uint8_t                            intrChannels;
    uint8_t                            implTimer1;
    uint8_t                            timer1No;
} SOC_CONFIG;

#ifdef __cplusplus
    }
#endif

#endif
