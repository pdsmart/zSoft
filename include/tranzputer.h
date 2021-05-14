/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tranzputer.h
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     The TranZPUter library.
//                  This file contains methods which allow applications to access and control the traZPUter board and the underlying Sharp MZ80A host.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2020 - Initial write of the TranZPUter software.
//                  Jul 2020 - Updates to accommodate v2.1 of the tranZPUter board.
//                  Sep 2020 - Updates to accommodate v2.2 of the tranZPUter board.
//                  May 2021 - Changes to use 512K-1Mbyte Z80 Static RAM, build time configurable.
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
#ifndef TRANZPUTER_H
#define TRANZPUTER_H

#ifdef __cplusplus
    extern "C" {
#endif

// Configurable constants.
//
#define REFRESH_BYTE_COUNT           8                                   // This constant controls the number of bytes read/written to the z80 bus before a refresh cycle is needed.
#define RFSH_BYTE_CNT                256                                 // Number of bytes we can write before needing a full refresh for the DRAM.
#define HOST_MON_TEST_VECTOR         0x4                                 // Address in the host monitor to test to identify host type.
#define DEFAULT_BUSREQ_TIMEOUT       5000                                // Timeout for a Z80 Bus request operation in milliseconds.
#define DEFAULT_RESET_PULSE_WIDTH    500000                              // Pulse width of a reset signal in K64F clock ticks.
#define TZFS_AUTOBOOT_FLAG           "0:\\TZFSBOOT.FLG"                  // Filename used as a flag, if this file exists in the SD root directory then TZFS is booted automatically.
#define TZ_MAX_Z80_MEM               0x100000                            // Maximum Z80 memory available on the tranZPUter board.
#define TZ_MAX_FPGA_MEM              0x1000000                           // Maximum addressable memory area inside the FPGA.

// tranZPUter Memory Modes - select one of the 32 possible memory models using these constants.
//
#define TZMM_ORIG                    0x00                                // Original Sharp MZ80A mode, no tranZPUter features are selected except the I/O control registers (default: 0x60-063).
#define TZMM_BOOT                    0x01                                // Original mode but E800-EFFF is mapped to tranZPUter RAM so TZFS can be booted.
#define TZMM_TZFS                    0x02                                // TZFS main memory configuration. all memory is in tranZPUter RAM, E800-FFFF is used by TZFS, SA1510 is at 0000-1000 and RAM is 1000-CFFF, 64K Block 0 selected.
#define TZMM_TZFS2                   0x03                                // TZFS main memory configuration. all memory is in tranZPUter RAM, E800-EFFF is used by TZFS, SA1510 is at 0000-1000 and RAM is 1000-CFFF, 64K Block 0 selected, F000-FFFF is in 64K Block 1.
#define TZMM_TZFS3                   0x04                                // TZFS main memory configuration. all memory is in tranZPUter RAM, E800-EFFF is used by TZFS, SA1510 is at 0000-1000 and RAM is 1000-CFFF, 64K Block 0 selected, F000-FFFF is in 64K Block 2.
#define TZMM_TZFS4                   0x05                                // TZFS main memory configuration. all memory is in tranZPUter RAM, E800-EFFF is used by TZFS, SA1510 is at 0000-1000 and RAM is 1000-CFFF, 64K Block 0 selected, F000-FFFF is in 64K Block 3.
#define TZMM_CPM                     0x06                                // CPM main memory configuration, all memory on the tranZPUter board, 64K block 4 selected. Special case for F3C0:F3FF & F7C0:F7FF (floppy disk paging vectors) which resides on the mainboard.
#define TZMM_CPM2                    0x07                                // CPM main memory configuration, F000-FFFF are on the tranZPUter board in block 4, 0040-CFFF and E800-EFFF are in block 5, mainboard for D000-DFFF (video), E000-E800 (Memory control) selected.
                                                                         // Special case for 0000:003F (interrupt vectors) which resides in block 4, F3C0:F3FF & F7C0:F7FF (floppy disk paging vectors) which resides on the mainboard.
#define TZMM_COMPAT                  0x08                                // Original mode but with main DRAM in Bank 0 to allow bootstrapping of programs from other machines such as the MZ700.
#define TZMM_HOSTACCESS              0x09                                // Mode to allow code running in Bank 0, address E800:FFFF to access host memory. Monitor ROM 0000-0FFF and Main DRAM 0x1000-0xD000, video and memory mapped I/O are on the host machine, User/Floppy ROM E800-FFFF are in tranZPUter memory. 
#define TZMM_MZ700_0                 0x0a                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the mainboard.
#define TZMM_MZ700_1                 0x0b                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 0, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the tranZPUter in block 6.
#define TZMM_MZ700_2                 0x0c                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the tranZPUter in block 6.
#define TZMM_MZ700_3                 0x0d                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 0, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is inaccessible.
#define TZMM_MZ700_4                 0x0e                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is inaccessible.
#define TZMM_MZ800                   0x0f                                // MZ800 Mode - Host is an MZ-800 and mode provides for MZ-700/MZ-800 decoding per original machine.
#define TZMM_FPGA                    0x15                                // Open up access for the K64F to the FPGA resources such as memory. All other access to RAM or mainboard is blocked.
#define TZMM_TZPUM                   0x16                                // Everything is on mainboard, no access to tranZPUter memory.
#define TZMM_TZPU                    0x17                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory. K64F drives A18-A16 allowing full access to RAM.
//#define TZMM_TZPU0                   0x18                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 0 is selected.
//#define TZMM_TZPU1                   0x19                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 1 is selected.
//#define TZMM_TZPU2                   0x1A                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 2 is selected.
//#define TZMM_TZPU3                   0x1B                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 3 is selected.
//#define TZMM_TZPU4                   0x1C                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 4 is selected.
//#define TZMM_TZPU5                   0x1D                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 5 is selected.
//#define TZMM_TZPU6                   0x1E                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 6 is selected.
//#define TZMM_TZPU7                   0x1F                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 7 is selected.

// IO addresses on the tranZPUter or mainboard.
//
#define IO_TZ_CTRLLATCH              0x60                                // Control latch which specifies the Memory Model/mode.
#define IO_TZ_SETXMHZ                0x62                                // Switch to alternate CPU frequency provided by K64F.
#define IO_TZ_SET2MHZ                0x64                                // Switch to system CPU frequency.
#define IO_TZ_CLKSELRD               0x66                                // Read the status of the clock select, ie. which clock is connected to the CPU.
#define IO_TZ_SVCREQ                 0x68                                // Service request from the Z80 to be provided by the K64F.
#define IO_TZ_SYSREQ                 0x6A                                // System request from the Z80 to be provided by the K64F.
#define IO_TZ_CPUCFG                 0x6C                                // Version 2.2 CPU configuration register.
#define IO_TZ_CPUSTATUS              0x6C                                // Version 2.2 CPU runtime status register.
#define IO_TZ_CPUINFO                0x6D                                // Version 2.2 CPU information register.
#define IO_TZ_CPLDCFG                0x6E                                // Version 2.1 CPLD configuration register.
#define IO_TZ_CPLDSTATUS             0x6E                                // Version 2.1 CPLD status register.
#define IO_TZ_CPLDINFO               0x6F                                // Version 2.1 CPLD version information register.
#define IO_TZ_MMIO0                  0xE0                                // MZ-700/MZ-800 Memory management selection ports.
#define IO_TZ_MMIO1                  0xE1                                // ""
#define IO_TZ_MMIO2                  0xE2                                // ""
#define IO_TZ_MMIO3                  0xE3                                // ""
#define IO_TZ_MMIO4                  0xE4                                // ""
#define IO_TZ_MMIO5                  0xE5                                // ""
#define IO_TZ_MMIO6                  0xE6                                // ""
#define IO_TZ_MMIO7                  0xE7                                // MZ-700/MZ-800 Memory management selection ports.
#define IO_TZ_SYSCTRL                0xF0                                // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define IO_TZ_GRAMMODE               0xF4                                // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define IO_TZ_VMCTRL                 0xF8                                // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define IO_TZ_VMGRMODE               0xF9                                // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define IO_TZ_VMREDMASK              0xFA                                // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMGREENMASK            0xFB                                // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMBLUEMASK             0xFC                                // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMPAGE                 0xFD                                // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.

// Addresses on the tranZPUter board.
//
#define SRAM_BANK0_ADDR              0x00000                             // Address of the 1st 64K RAM bank in the SRAM chip.
#define SRAM_BANK1_ADDR              0x10000                             // ""
#define SRAM_BANK2_ADDR              0x20000                             // ""
#define SRAM_BANK3_ADDR              0x30000                             // ""
#define SRAM_BANK4_ADDR              0x40000                             // ""
#define SRAM_BANK5_ADDR              0x50000                             // ""
#define SRAM_BANK6_ADDR              0x60000                             // ""
#define SRAM_BANK7_ADDR              0x70000                             // Address of the 8th 64K RAM bank in the SRAM chip.

// IO register constants.
//
#define CPUMODE_SET_Z80              0x00                                // Set the CPU to the hard Z80.
#define CPUMODE_SET_T80              0x01                                // Set the CPU to the soft T80.
#define CPUMODE_SET_ZPU_EVO          0x02                                // Set the CPU to the soft ZPU Evolution.
#define CPUMODE_SET_AAA              0x04                                // Place holder for a future soft CPU.
#define CPUMODE_SET_BBB              0x08                                // Place holder for a future soft CPU.
#define CPUMODE_SET_CCC              0x10                                // Place holder for a future soft CPU.
#define CPUMODE_SET_DDD              0x20                                // Place holder for a future soft CPU.
#define CPUMODE_IS_Z80               0x00                                // Status value to indicate if the hard Z80 available.
#define CPUMODE_IS_T80               0x01                                // Status value to indicate if the soft T80 available.
#define CPUMODE_IS_ZPU_EVO           0x02                                // Status value to indicate if the soft ZPU Evolution available.
#define CPUMODE_IS_AAA               0x04                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_BBB               0x08                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_CCC               0x10                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_DDD               0x20                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_CLK_EN               0x40                                // Toggle the soft CPU clock, 1 = enable, 0 = disable.
#define CPUMODE_RESET_CPU            0x80                                // Reset the soft CPU. Active high, when high the CPU is held in RESET, when low the CPU runs.
#define CPUMODE_IS_SOFT_AVAIL        0x040                               // Marker to indicate if the underlying FPGA can support soft CPU's.
#define CPUMODE_IS_SOFT_MASK         0x03F                               // Mask to filter out the Soft CPU availability flags.

// CPLD Configuration constants.
#define MODE_MZ80K                   0x00                                // Hardware mode = MZ80K
#define MODE_MZ80C                   0x01                                // Hardware mode = MZ80C
#define MODE_MZ1200                  0x02                                // Hardware mode = MZ1200
#define MODE_MZ80A                   0x03                                // Hardware mode = MZ80A
#define MODE_MZ700                   0x04                                // Hardware mode = MZ700
#define MODE_MZ800                   0x05                                // Hardware mode = MZ800
#define MODE_MZ80B                   0x06                                // Hardware mode = MZ80B
#define MODE_MZ2000                  0x07                                // Hardware mode = MZ2000
#define MODE_VIDEO_MODULE_DISABLED   0x08                                // Hardware enable (bit 3 = 0) or disable of the Video Module.
#define MODE_PRESERVE_CONFIG         0x80                                // Preserve hardware configuration on RESET.

// Video Module control bits.
#define SYSMODE_MZ80A                0x00                                // System board mode MZ80A, 2MHz CPU/Bus.
#define SYSMODE_MZ80B                0x01                                // System board mode MZ80B, 4MHz CPU/Bus.
#define SYSMODE_MZ700                0x02                                // System board mode MZ700, 3.54MHz CPU/Bus.
#define VMMODE_MASK                  0xF8                                // Mask to mask out video mode.
#define VMMODE_MZ80K                 MODE_MZ80K                          // Video mode = MZ80K
#define VMMODE_MZ80C                 MODE_MZ80C                          // Video mode = MZ80C
#define VMMODE_MZ1200                MODE_MZ1200                         // Video mode = MZ1200
#define VMMODE_MZ80A                 MODE_MZ80A                          // Video mode = MZ80A
#define VMMODE_MZ700                 MODE_MZ700                          // Video mode = MZ700
#define VMMODE_MZ800                 MODE_MZ800                          // Video mode = MZ800
#define VMMODE_MZ80B                 MODE_MZ80B                          // Video mode = MZ80B
#define VMMODE_MZ2000                MODE_MZ2000                         // Video mode = MZ2000
#define VMMODE_80CHAR                0x08                                // Enable 80 character display.
#define VMMODE_80CHAR_MASK           0xF7                                // Mask to filter out display width control bit.
#define VMMODE_COLOUR                0x10                                // Enable colour display.
#define VMMODE_COLOUR_MASK           0xEF                                // Mask to filter out colour control bit.
#define VMMODE_PCGRAM                0x20                                // Enable PCG RAM.
#define VMMODE_VGA_MASK              0x3F                                // Mask to filter out the VGA mode bits.
#define VMMODE_VGA_OFF               0x00                                // Set VGA mode off, external monitor is driven by standard internal signals.
#define VMMODE_VGA_640x480           0x40                                // Set external monitor to VGA 640x480 @ 60Hz mode.
#define VMMODE_VGA_1024x768          0x80                                // Set external monitor to VGA 1024x768 @ 60Hz mode.
#define VMMODE_VGA_800x600           0xC0                                // Set external monitor to VGA 800x600 @ 60Hz mode.

// VGA mode border control constants.
//
#define VMBORDER_BLACK               0x00                                // VGA has a black border.
#define VMBORDER_BLUE                0x01                                // VGA has a blue border.
#define VMBORDER_RED                 0x02                                // VGA has a red border.
#define VMBORDER_PURPLE              0x03                                // VGA has a purple border.
#define VMBORDER_GREEN               0x04                                // VGA has a green border.
#define VMBORDER_CYAN                0x05                                // VGA has a cyan border.
#define VMBORDER_YELLOW              0x06                                // VGA has a yellow border.
#define VMBORDER_WHITE               0x07                                // VGA has a white border.
#define VMBORDER_MASK                0xF8                                // Mask to filter out current border setting.

// Sharp MZ colour attributes.
#define VMATTR_FG_BLACK              0x00                                // Foreground black character attribute.
#define VMATTR_FG_BLUE               0x10                                // Foreground blue character attribute.
#define VMATTR_FG_RED                0x20                                // Foreground red character attribute.
#define VMATTR_FG_PURPLE             0x30                                // Foreground purple character attribute.
#define VMATTR_FG_GREEN              0x40                                // Foreground green character attribute.
#define VMATTR_FG_CYAN               0x50                                // Foreground cyan character attribute.
#define VMATTR_FG_YELLOW             0x60                                // Foreground yellow character attribute.
#define VMATTR_FG_WHITE              0x70                                // Foreground white character attribute.
#define VMATTR_FG_MASKOUT            0x8F                                // Mask to filter out foreground attribute.
#define VMATTR_FG_MASKIN             0x70                                // Mask to filter out foreground attribute.
#define VMATTR_BG_BLACK              0x00                                // Background black character attribute.
#define VMATTR_BG_BLUE               0x01                                // Background blue character attribute.
#define VMATTR_BG_RED                0x02                                // Background red character attribute.
#define VMATTR_BG_PURPLE             0x03                                // Background purple character attribute.
#define VMATTR_BG_GREEN              0x04                                // Background green character attribute.
#define VMATTR_BG_CYAN               0x05                                // Background cyan character attribute.
#define VMATTR_BG_YELLOW             0x06                                // Background yellow character attribute.
#define VMATTR_BG_WHITE              0x07                                // Background white character attribute.
#define VMATTR_BG_MASKOUT            0xF8                                // Mask to filter out background attribute.
#define VMATTR_BG_MASKIN             0x07                                // Mask to filter out background attribute.

// Sharp MZ constants.
//
#define MZ_MROM_ADDR                 0x00000                             // Monitor ROM start address.
#define MZ_800_MROM_ADDR             0x70000                             // MZ-800 Monitor ROM address.
#define MZ_800_CGROM_ADDR            0x71000                             // MZ-800 CGROM address during reset when it is loaded into the PCG.
#define MZ_800_IPL_ADDR              0x7E000                             // Address of the 9Z_504M IPL BIOS.
#define MZ_800_IOCS_ADDR             0x7F400                             // Address of the MZ-800 common IOCS bios.
#define MZ_MROM_STACK_ADDR           0x01000                             // Monitor ROM start stack address.
#define MZ_MROM_STACK_SIZE           0x000EF                             // Monitor ROM stack size.
#define MZ_UROM_ADDR                 0x0E800                             // User ROM start address.
#define MZ_BANKRAM_ADDR              0x0F000                             // Floppy API address which is used in TZFS as the paged RAM for additional functionality.
#define MZ_ZOS_ADDR                  0x0100000                           // zOS boot location for the ZPU in FPGA BRAM memory.
#define MZ_CMT_ADDR                  0x010F0                             // Address of the CMT (tape) header record.
#define MZ_CMT_DEFAULT_LOAD_ADDR     0x01200                             // The default load address for a CMT, anything below this is normally illegal.
#define MZ_VID_RAM_ADDR              0x0D000                             // Start of Video RAM
#define MZ_VID_RAM_SIZE              2048                                // Size of Video RAM.
#define MZ_VID_DFLT_BYTE             0x00                                // Default character (SPACE) for video RAM.
#define MZ_ATTR_RAM_ADDR             0xD800                              // On machines with the upgrade, the start of the Attribute RAM.
#define MZ_ATTR_RAM_SIZE             2048                                // Size of the attribute RAM.
#define MZ_ATTR_DFLT_BYTE            0x07                                // Default colour (White on Black) for the attribute.
#define MZ_SCROL_BASE                0xE200                              // Base address of the hardware scroll registers.
#define MZ_SCROL_END                 0xE2FF                              // End address of the hardware scroll registers.
#define MZ_MEMORY_SWAP               0xE00C                              // Address when read swaps the memory from 0000-0FFF -> C000-CFFF
#define MZ_MEMORY_RESET              0xE010                              // Address when read resets the memory to the default location 0000-0FFF.
#define MZ_CRT_NORMAL                0xE014                              // Address when read sets the CRT to normal display mode.
#define MZ_CRT_INVERSE               0xE018                              // Address when read sets the CRT to inverted display mode.
#define MZ_80A_CPU_FREQ              2000000                             // CPU Speed of the Sharp MZ-80A
#define MZ_700_CPU_FREQ              3580000                             // CPU Speed of the Sharp MZ-700
#define MZ_80B_CPU_FREQ              4000000                             // CPU Speed of the Sharp MZ-80B
#define MZ_800_CPU_FREQ              3580000                             // CPU Speed of the Sharp MZ-800
#define MZ_ROM_SA1510_40C            "0:\\TZFS\\SA1510.ROM"              // Original 40 character Monitor ROM.
#define MZ_ROM_SA1510_80C            "0:\\TZFS\\SA1510-8.ROM"            // Original Monitor ROM patched for 80 character screen mode.
#define MZ_ROM_1Z_013A_40C           "0:\\TZFS\\1Z-013A.ROM"             // Original 40 character Monitor ROM for the Sharp MZ700.
#define MZ_ROM_1Z_013A_80C           "0:\\TZFS\\1Z-013A-8.ROM"           // Original Monitor ROM patched for the Sharp MZ700 patched for 80 column mode.
#define MZ_ROM_1Z_013A_KM_40C        "0:\\TZFS\\1Z-013A-KM.ROM"          // Original 40 character Monitor ROM for the Sharp MZ700 with keyboard remapped for the MZ80A.
#define MZ_ROM_1Z_013A_KM_80C        "0:\\TZFS\\1Z-013A-KM-8.ROM"        // Original Monitor ROM patched for the Sharp MZ700 with keyboard remapped for the MZ80A and patched for 80 column mode.
#define MZ_ROM_9Z_504M_COMBINED      "0:\\TZFS\\MZ800_IPL.rom"           // Original MZ-800 BIOS which comprises the 1Z_013B BIOS, 9Z_504M IPL, CGROM and IOCS.
#define MZ_ROM_9Z_504M               "0:\\TZFS\\MZ800_9Z_504M.rom"       // Modified MZ-800 9Z_504M IPL to contain a select TZFS option.
#define MZ_ROM_1Z_013B               "0:\\TZFS\\MZ800_1Z_013B.rom"       // Original MZ-800 1Z_013B MZ-700 compatible BIOS.
#define MZ_ROM_800_CGROM             "0:\\TZFS\\MZ800_CGROM.ORI"         // Original MZ-800 Character Generator ROM.
#define MZ_ROM_800_IOCS              "0:\\TZFS\\MZ800_IOCS.rom"          // Original MZ-800 common IOCS bios.
#define MZ_ROM_MZ80B_IPL             "0:\\TZFS\\MZ80B_IPL.ROM"           // Original IPL ROM for the Sharp MZ-80B.
#define MZ_ROM_TZFS                  "0:\\TZFS\\TZFS.ROM"                // tranZPUter Filing System ROM.
#define MZ_ROM_ZPU_ZOS               "0:\\ZOS\\ZOS.ROM"                  // zOS for the ZPU running on the tranZPUter SW-700 board.

// CP/M constants.
//
#define CPM_MAX_DRIVES               16                                  // Maximum number of drives in CP/M.
#define CPM_FILE_CCPBDOS             "0:\\CPM\\CPM22.BIN"                // CP/M CCP and BDOS for warm start reloads.
#define CPM_DRIVE_TMPL               "0:\\CPM\\CPMDSK%02u.RAW"           // Template for CPM disk drives stored on the SD card.
#define CPM_SECTORS_PER_TRACK        32                                  // Number of sectors in a track on the virtual CPM disk.
#define CPM_TRACKS_PER_DISK          1024                                // Number of tracks on a disk.

// Service request constants.
//
#define TZSVC_CMD_STRUCT_ADDR_TZFS   0x0ED80                             // Address of the command structure within TZFS - exists in 64K Block 0.
#define TZSVC_CMD_STRUCT_ADDR_CPM    0x4F560                             // Address of the command structure within CP/M - exists in 64K Block 4.
#define TZSVC_CMD_STRUCT_ADDR_MZ700  0x6FD80                             // Address of the command structure within MZ700 compatible programs - exists in 64K Block 6.
#define TZSVC_CMD_STRUCT_ADDR_ZOS    0x11FD80 // 0x7FD80                             // Address of the command structure for zOS use, exists in shared memory rather than FPGA. Spans top of block 6 and all of block 7.
#define TZSVC_CMD_STRUCT_SIZE        0x280                               // Size of the inter z80/K64 service command memory.
#define TZSVC_CMD_SIZE               (sizeof(t_svcControl)-TZSVC_SECTOR_SIZE)
#define TZVC_MAX_CMPCT_DIRENT_BLOCK  TZSVC_SECTOR_SIZE/TZSVC_CMPHDR_SIZE // Maximum number of directory entries per sector.
#define TZSVC_MAX_DIR_ENTRIES        255                                 // Maximum number of files in one directory, any more than this will be ignored.
#define TZSVC_CMPHDR_SIZE            32                                  // Compacted header size, contains everything except the comment field, padded out to 32bytes.
#define MZF_FILLER_LEN               8                                   // Filler to pad a compacted header entry to a power of 2 length.
#define TZVC_MAX_DIRENT_BLOCK        TZSVC_SECTOR_SIZE/MZF_HEADER_SIZE   // Maximum number of directory entries per sector.
#define TZSVC_CMD_READDIR            0x01                                // Service command to open a directory and return the first block of entries.
#define TZSVC_CMD_NEXTDIR            0x02                                // Service command to return the next block of an open directory.
#define TZSVC_CMD_READFILE           0x03                                // Service command to open a file and return the first block.
#define TZSVC_CMD_NEXTREADFILE       0x04                                // Service command to return the next block of an open file.
#define TZSVC_CMD_WRITEFILE          0x05                                // Service command to create a file and save the first block.
#define TZSVC_CMD_NEXTWRITEFILE      0x06                                // Service command to write the next block to the open file.
#define TZSVC_CMD_CLOSE              0x07                                // Service command to close any open file or directory.
#define TZSVC_CMD_LOADFILE           0x08                                // Service command to load a file directly into tranZPUter memory.
#define TZSVC_CMD_SAVEFILE           0x09                                // Service command to save a file directly from tranZPUter memory.
#define TZSVC_CMD_ERASEFILE          0x0a                                // Service command to erase a file on the SD card.
#define TZSVC_CMD_CHANGEDIR          0x0b                                // Service command to change active directory on the SD card.
#define TZSVC_CMD_LOAD40ABIOS        0x20                                // Service command requesting that the 40 column version of the SA1510 BIOS is loaded.
#define TZSVC_CMD_LOAD80ABIOS        0x21                                // Service command requesting that the 80 column version of the SA1510 BIOS is loaded.
#define TZSVC_CMD_LOAD700BIOS40      0x22                                // Service command requesting that the MZ700 1Z-013A 40 column BIOS is loaded.
#define TZSVC_CMD_LOAD700BIOS80      0x23                                // Service command requesting that the MZ700 1Z-013A 80 column patched BIOS is loaded.
#define TZSVC_CMD_LOAD80BIPL         0x24                                // Service command requesting the MZ-80B IPL is loaded.
#define TZSVC_CMD_LOAD800BIOS        0x25                                // Service command requesting that the MZ800 9Z-504M BIOS is loaded.
#define TZSVC_CMD_LOADBDOS           0x30                                // Service command to reload CPM BDOS+CCP.
#define TZSVC_CMD_ADDSDDRIVE         0x31                                // Service command to attach a CPM disk to a drive number.
#define TZSVC_CMD_READSDDRIVE        0x32                                // Service command to read an attached SD file as a CPM disk drive.
#define TZSVC_CMD_WRITESDDRIVE       0x33                                // Service command to write to a CPM disk drive which is an attached SD file.
#define TZSVC_CMD_CPU_BASEFREQ       0x40                                // Service command to switch to the mainboard frequency.
#define TZSVC_CMD_CPU_ALTFREQ        0x41                                // Service command to switch to the alternate frequency provided by the K64F.
#define TZSVC_CMD_CPU_CHGFREQ        0x42                                // Service command to set the alternate frequency in hertz.
#define TZSVC_CMD_CPU_SETZ80         0x50                                // Service command to switch to the external Z80 hard cpu.
#define TZSVC_CMD_CPU_SETT80         0x51                                // Service command to switch to the internal T80 soft cpu.
#define TZSVC_CMD_CPU_SETZPUEVO      0x52                                // Service command to switch to the internal ZPU Evolution cpu.
#define TZSVC_CMD_SD_DISKINIT        0x60                                // Service command to initialise and provide raw access to the underlying SD card.
#define TZSVC_CMD_SD_READSECTOR      0x61                                // Service command to provide raw read access to the underlying SD card.
#define TZSVC_CMD_SD_WRITESECTOR     0x62                                // Service command to provide raw write access to the underlying SD card.
#define TZSVC_CMD_EXIT               0x7F                                // Service command to terminate TZFS and restart the machine in original mode.
#define TZSVC_DEFAULT_MZF_DIR        "MZF"                               // Default directory where MZF files are stored.
#define TZSVC_DEFAULT_CAS_DIR        "CAS"                               // Default directory where BASIC CASsette files are stored.
#define TZSVC_DEFAULT_BAS_DIR        "BAS"                               // Default directory where BASIC text files are stored.
#define TZSVC_DEFAULT_MZF_EXT        "MZF"                               // Default file extension for MZF files.
#define TZSVC_DEFAULT_CAS_EXT        "CAS"                               // Default file extension for CASsette files.
#define TZSVC_DEFAULT_BAS_EXT        "BAS"                               // Default file extension for BASic script files stored in readable text.
#define TZSVC_DEFAULT_WILDCARD       "*"                                 // Default wildcard file matching.
#define TZSVC_RESULT_OFFSET          0x01                                // Offset into structure of the result byte.
#define TZSVC_DIRNAME_SIZE           20                                  // Limit is size of FAT32 directory name.
#define TZSVC_WILDCARD_SIZE          20                                  // Very basic pattern matching so small size.
#define TZSVC_FILENAME_SIZE          MZF_FILENAME_LEN                    // Length of a Sharp MZF filename.
#define TZSVC_LONG_FNAME_SIZE        (sizeof(t_svcCmpDirEnt) - 1)        // Length of a standard filename to fit inside a directory entry.
#define TZSVC_LONG_FMT_FNAME_SIZE    20                                  // Length of a standard filename formatted in a directory listing.
#define TZSVC_SECTOR_SIZE            512                                 // SD Card sector buffer size.
#define TZSVC_STATUS_OK              0x00                                // Flag to indicate the K64F processing completed successfully.
#define TZSVC_STATUS_FILE_ERROR      0x01                                // Flag to indicate a file or directory error.
#define TZSVC_STATUS_BAD_CMD         0x02                                // Flag to indicate a bad service command was requested.
#define TZSVC_STATUS_BAD_REQ         0x03                                // Flag to indicate a bad request was made, the service status request flag was not set.
#define TZSVC_STATUS_REQUEST         0xFE                                // Flag to indicate Z80 has posted a request.
#define TZSVC_STATUS_PROCESSING      0xFF                                // Flag to indicate the K64F is processing a command.
#define TZSVC_OPEN                   0x00                                // Service request to open a directory or file.
#define TZSVC_NEXT                   0x01                                // Service request to return the next directory block or file block or write the next file block.
#define TZSVC_CLOSE                  0x02                                // Service request to close open dir/file.


// Constants for the Sharp MZ80A MZF file format.
#define MZF_HEADER_SIZE              128                                 // Size of the MZF header.
#define MZF_ATTRIBUTE                0x00                                // Code Type, 01 = Machine Code.
#define MZF_FILENAME                 0x01                                // Title/Name (17 bytes).
#define MZF_FILENAME_LEN             17                                  // Length of the filename, it is not NULL terminated, generally a CR can be taken as terminator but not guaranteed.
#define MZF_FILESIZE                 0x12                                // Size of program.
#define MZF_LOADADDR                 0x14                                // Load address of program.
#define MZF_EXECADDR                 0x16                                // Exec address of program.
#define MZF_COMMENT                  0x18                                // Comment, used for details of the file or startup code.
#define MZF_COMMENT_LEN              104                                 // Length of the comment field.

// Constants for other handled file formats.
//
#define CAS_HEADER_SIZE              256                                 // Size of the CASsette header.


// Pin Constants - Pins assigned at the hardware level to specific tasks/signals.
//
#define MAX_TRANZPUTER_PINS          51
#define Z80_WR_PIN                   20   // 48
#define Z80_RD_PIN                   5    // 55
#define Z80_IORQ_PIN                 8
#define Z80_MREQ_PIN                 7
#define Z80_A0_PIN                   15
#define Z80_A1_PIN                   22
#define Z80_A2_PIN                   23
#define Z80_A3_PIN                   9
#define Z80_A4_PIN                   10
#define Z80_A5_PIN                   13
#define Z80_A6_PIN                   11
#define Z80_A7_PIN                   12
#define Z80_A8_PIN                   35
#define Z80_A9_PIN                   36
#define Z80_A10_PIN                  37
#define Z80_A11_PIN                  38
#define Z80_A12_PIN                  64   // 3
#define Z80_A13_PIN                  65   // 4
#define Z80_A14_PIN                  66   // 26
#define Z80_A15_PIN                  67   // 27
#define Z80_A16_PIN                  68   // 33
#define Z80_A17_PIN                  69   // 34
#define Z80_A18_PIN                  70   // 24
#define Z80_A19_PIN                  16
#define Z80_A20_PIN                  17
#define Z80_A21_PIN                  19
#define Z80_A22_PIN                  18
#define Z80_A23_PIN                  71   // 49
#define Z80_D0_PIN                   0
#define Z80_D1_PIN                   1
#define Z80_D2_PIN                   29
#define Z80_D3_PIN                   30
#define Z80_D4_PIN                   43
#define Z80_D5_PIN                   46
#define Z80_D6_PIN                   44
#define Z80_D7_PIN                   45
#define Z80_WAIT_PIN                 31   // 54
#define Z80_BUSACK_PIN               24   // 5
#define Z80_NMI_PIN                  39
#define Z80_INT_PIN                  28
#define Z80_RESET_PIN                6
#define SYSCLK_PIN                   25
#define CTL_RFSH_PIN                 4    // 53
#define CTL_HALT_PIN                 26   // 51
#define CTL_M1_PIN                   3    // 20
#define CTL_WAIT_PIN                 27
#define CTL_BUSRQ_PIN                2
#define CTL_MBSEL_PIN                21
#define CTL_CLK_PIN                  14
#define CTL_BUSACK_PIN               32   // 47
#define CTL_SVCREQ_PIN               33   // 56

// IRQ mask values for the different types of IRQ trigger.
//
#define IRQ_MASK_CHANGE              0x10B0000
#define IRQ_MASK_RISING              0x1090000 //0x040040
#define IRQ_MASK_FALLING             0x10A0000
#define IRQ_MASK_LOW                 0x1080000
#define IRQ_MASK_HIGH                0x10C0000

// Customised pin manipulation methods implemented as stripped down macros. The original had too much additional overhead with procedure call and validation tests,
// speed is of the essence for this project as pins change mode and value constantly.
//
#define STR(x)                       #x
#define XSTR(s)                      STR(s)
#define pinLow(a)                    *portClearRegister(pinMap[a]) = 1
#define pinHigh(a)                   *portSetRegister(pinMap[a]) = 1
#define pinSet(a, b)                 if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; }
#define pinGet(a)                    *portInputRegister(pinMap[a])
#define pinInput(a)                  { *portModeRegister(pinMap[a]) = 0; *ioPin[a] = PORT_PCR_MUX(1) | PORT_PCR_PFE | PORT_PCR_PE | PORT_PCR_PS; }
#define pinOutput(a)                 { *portModeRegister(pinMap[a]) = 1;\
                                       *ioPin[a] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                                       *ioPin[a] &= ~PORT_PCR_ODE; }
#define pinOutputSet(a,b)            { if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; }\
                                       *portModeRegister(pinMap[a]) = 1;\
                                       *ioPin[a] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                                       *ioPin[a] &= ~PORT_PCR_ODE; }
#define installIRQ(a, mask)          { uint32_t cfg;\
                                       cfg = *ioPin[a];\
                                       cfg &= ~0x000F0000;\
                                       *ioPin[a] = cfg;\
                                       cfg |= mask;\
                                       *ioPin[a] = cfg;\
                                     }
#define removeIRQ(a)                 { \
                                       *ioPin[a] = ((*ioPin[a] & ~0x000F0000) | 0x01000000);\
                                     }
#define pinIndex(a)                  getPinIndex(pinMap[a])

#define setZ80Data(a)                { GPIOB_PDOR = (GPIOB_PDOR & 0xff00ffff) | ((a << 16) & 0x00ff0000); }
#define setZ80DataAsOutput()         { GPIOB_PDDR = (GPIOB_PDDR & 0x0000ffff) | 0x00ff0000; }
#define setZ80DataAsInput()          { GPIOB_PDDR = (GPIOB_PDDR & 0x0000ffff); }
#define setZ80Addr(a)                { GPIOC_PDOR = (GPIOC_PDOR & 0xfff80000) | (a & 0x0007ffff); GPIOB_PDOR = (GPIOB_PDOR & 0xFFFFFDF0) | (((a >> 14)&0x200) | ((a >> 19)&0xF)); }
#define setZ80AddrAsOutput()         { GPIOC_PDDR = 0x0007ffff; GPIOB_PDDR = GPIOB_PDDR | 0x20F; }
#define setZ80AddrAsInput()          { GPIOC_PDDR = 0x00000000; GPIOB_PDDR = GPIOB_PDDR & 0xFFFFFDF0; }
#define setZ80AddrLower(a)           { GPIOC_PDOR = (GPIOC_PDOR & 0xffffff00) | (a & 0x000000ff); }
#define setZ80RefreshAddr(a)         { GPIOC_PDOR = (GPIOC_PDOR & 0xffffff80) | (a & 0x0000007f); }
#define readZ80AddrLower()           ( GPIOC_PDIR & 0x000000ff )
#define readZ80Addr()                ( (GPIOC_PDIR & 0x0000ffff) )
#define readZ80DataBus()             ( (GPIOB_PDIR >> 16) & 0x000000ff )
//#define readCtrlLatch()              ( ((GPIOB_PDIR & 0x00000200) >> 5) | (GPIOB_PDIR & 0x0000000f) )
#define readCtrlLatchDirect()        ( inZ80IO(IO_TZ_CTRLLATCH) )
#define readCtrlLatch()              ( readZ80IO(IO_TZ_CTRLLATCH, TRANZPUTER) )
#define writeCtrlLatch(a)            { setZ80Direction(WRITE); outZ80IO(IO_TZ_CTRLLATCH, a); } 
//#define setZ80Direction(a)           { for(uint8_t idx=Z80_D0; idx <= Z80_D7; idx++) { if(a == WRITE) { pinOutput(idx); } else { pinInput(idx); } }; z80Control.busDir = a; }
#define setZ80Direction(a)           {{ if(a == WRITE) { setZ80DataAsOutput(); } else { setZ80DataAsInput(); } }; z80Control.busDir = a; }
#define reqZ80BusChange(a)           { if(a == MAINBOARD_ACCESS && z80Control.ctrlMode == TRANZPUTER_ACCESS) \
                                       {\
                                           pinHigh(CTL_MBSEL);\
                                           z80Control.ctrlMode = MAINBOARD_ACCESS;\
                                           z80Control.curCtrlLatch = TZMM_ORIG;\
                                           setZ80Direction(WRITE); \
                                           writeCtrlLatch(z80Control.curCtrlLatch); \
                                       } else if(a == TRANZPUTER_ACCESS && z80Control.ctrlMode == MAINBOARD_ACCESS)\
                                       {\
                                           pinLow(CTL_MBSEL);\
                                           z80Control.ctrlMode = TRANZPUTER_ACCESS;\
                                           z80Control.curCtrlLatch = TZMM_TZPU;\
                                           setZ80Direction(WRITE); \
                                           writeCtrlLatch(z80Control.curCtrlLatch);\
                                       } else\
                                       {\
                                           setZ80Direction(WRITE); \
                                       }\
                                     }
// Lower level macro without pin mapping as this is called in the ResetHandler to halt the Z80 whilst the K64F starts up and is able to load up tranZPUter software.
#define holdZ80()                    { \
                                         *portModeRegister(CTL_BUSRQ_PIN) = 1; \
                                         *portConfigRegister(CTL_BUSRQ_PIN) = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1); \
                                         *portConfigRegister(CTL_BUSRQ_PIN) &= ~PORT_PCR_ODE; \
                                         *portClearRegister(CTL_BUSRQ_PIN) = 1; \
                                     }



// Enumeration of the various pins on the project. These enums make it easy to refer to a signal and they are mapped
// to the actual hardware pin via the pinMap array.
// One of the big advantages is that a swath of pins, such as the address lines, can be switched in a tight loop rather than 
// individual pin assignments or clunky lists.
//
enum pinIdxToPinNumMap { 
    Z80_A0                           = 0,
    Z80_A1                           = 1,
    Z80_A2                           = 2,
    Z80_A3                           = 3,
    Z80_A4                           = 4,
    Z80_A5                           = 5,
    Z80_A6                           = 6,
    Z80_A7                           = 7,
    Z80_A8                           = 8,
    Z80_A9                           = 9,
    Z80_A10                          = 10,
    Z80_A11                          = 11,
    Z80_A12                          = 12,
    Z80_A13                          = 13,
    Z80_A14                          = 14,
    Z80_A15                          = 15,
    Z80_A16                          = 16,
    Z80_A17                          = 17,
    Z80_A18                          = 18,
    Z80_A19                          = 19,
    Z80_A20                          = 20,
    Z80_A21                          = 21,
    Z80_A22                          = 22,
    Z80_A23                          = 23,

    Z80_D0                           = 24,
    Z80_D1                           = 25,
    Z80_D2                           = 26,
    Z80_D3                           = 27,
    Z80_D4                           = 28,
    Z80_D5                           = 29,
    Z80_D6                           = 30,
    Z80_D7                           = 31,

    Z80_IORQ                         = 32,
    Z80_MREQ                         = 33,
    Z80_RD                           = 34,
    Z80_WR                           = 35,
    Z80_WAIT                         = 36,
    Z80_BUSACK                       = 37,

    Z80_NMI                          = 38,
    Z80_INT                          = 39,
    Z80_RESET                        = 40,
    MB_SYSCLK                        = 41,
    CTL_SVCREQ                       = 42,

    CTL_MBSEL                        = 43,
    CTL_BUSRQ                        = 44,
    CTL_RFSH                         = 45,
    CTL_HALT                         = 46,
    CTL_M1                           = 47,
    CTL_WAIT                         = 48,
    CTL_CLK                          = 49,
    CTL_BUSACK                       = 50 
};

// Possible control modes that the K64F can be in, do nothing where the Z80 runs normally, control the Z80 and mainboard, or control the Z80 and tranZPUter.
enum CTRL_MODE {
    Z80_RUN                          = 0,
    TRANZPUTER_ACCESS                = 1,
    MAINBOARD_ACCESS                 = 2
};

// Possible targets the K64F can read from/write to.
enum TARGETS {
    MAINBOARD                        = 0,
    TRANZPUTER                       = 1,
    FPGA                             = 2
};

// Possible bus directions that the K64F can setup for controlling the Z80.
enum BUS_DIRECTION {
    READ                             = 0,
    WRITE                            = 1,
    TRISTATE                         = 2
};

// Possible video frames stored internally.
//
enum VIDEO_FRAMES {
    SAVED                            = 0,
    WORKING                          = 1
};

// Possible machines the tranZPUter can be hosted on and can emulate.
//
enum MACHINE_TYPES {
    MZ80K                            = MODE_MZ80K,                       // Machine = MZ-80K.
    MZ80C                            = MODE_MZ80C,                       // Machine = MZ-80C.
    MZ1200                           = MODE_MZ1200,                      // Machine = MZ-1200.
    MZ80A                            = MODE_MZ80A,                       // Machine = MZ-80A.
    MZ700                            = MODE_MZ700,                       // Machine = MZ-700.
    MZ800                            = MODE_MZ800,                       // Machine = MZ-800.
    MZ80B                            = MODE_MZ80B,                       // Machine = MZ-80B.
    MZ2000                           = MODE_MZ2000                       // Machine = MZ-2000.
};

// Get and Set flags within the CPLD config and status registers.
//
enum CPLD_FLAGS {
    VIDEO_FPGA                       = 0x08,                             // Bit to test for available functionality or enabling of the FPGA video hardware.
    CPLD_VERSION                     = 0xE0                              // CPLD version mask bits.
};

// Types of file which have handlers and can be processed.
//
enum FILE_TYPE {
    MZF                              = 0,                                // Sharp MZF tape image files.
    MZFHDR                           = 1,                                // Sharp MZF Header from file only.
    CAS                              = 2,                                // BASIC CASsette image files.
    BAS                              = 3,                                // BASic ASCII text script files.

    ALL                              = 10,                               // All files to be considered.
    ALLFMT                           = 11                                // Special case for directory listings, all files but truncated and formatted.
};

// Structure to define a Sharp MZ80A MZF directory structure. This header appears at the beginning of every Sharp MZ80A tape (and more recently archived/emulator) images.
//
typedef struct __attribute__((__packed__)) {
    uint8_t                          attr;                               // MZF attribute describing the file.
    uint8_t                          fileName[MZF_FILENAME_LEN];         // Each directory entry is the size of an MZF filename.
    uint16_t                         fileSize;                           // Size of file.
    uint16_t                         loadAddr;                           // Load address for the file.
    uint16_t                         execAddr;                           // Execution address where the Z80 starts processing.
    uint8_t                          comment[MZF_COMMENT_LEN];           // Text comment field but often contains a startup machine code program.
} t_svcDirEnt;

// Structure to define a compacted Sharp MZ80A MZF directory structure (no comment) for use in directory listings.
// This header appears at the beginning of every Sharp MZ80A tape (and more recently archived/emulator) images.
//
typedef struct __attribute__((__packed__)) {
    uint8_t                          attr;                               // MZF attribute describing the file.
    uint8_t                          fileName[MZF_FILENAME_LEN];         // Each directory entry is the size of an MZF filename.
    uint16_t                         fileSize;                           // Size of file.
    uint16_t                         loadAddr;                           // Load address for the file.
    uint16_t                         execAddr;                           // Execution address where the Z80 starts processing.
    uint8_t                          filler[MZF_FILLER_LEN];             // Filler to pad to a power of 2 length.
} t_svcCmpDirEnt;

// Structure to hold the map betwen an SD filename and the Sharp file it contains. The file is an MZF format file with a 128 byte header
// and this header contains the name understood on the Sharp MZ80A.
//
typedef struct __attribute__((__packed__)) {
    uint8_t                          *sdFileName;                        // Name of file on the SD card.
    t_svcCmpDirEnt                   mzfHeader;                          // Compact Sharp header data of this file.
} t_sharpToSDMap;

// Structure to define the control information for a CP/M disk drive.
//
typedef struct {
    uint8_t                          *fileName;                          // FQFN of the CPM disk image file.
    uint32_t                         lastTrack;                          // Track of last successful operation.
    uint32_t                         lastSector;                         // Sector of last successful operation.
    FIL                              File;                               // Opened file handle of the CPM disk image.
} t_cpmDrive;

// Structure to define which CP/M drives are added to the system, mapping a number from CP/M into a record containing the details of the file on the SD card.
//
typedef struct {
    t_cpmDrive                       *drive[CPM_MAX_DRIVES];             // 1:1 map of CP/M drive number to an actual file on the SD card.
} t_cpmDriveMap;

// Structure to hold a map of an entire directory of files on the SD card and their associated Sharp MZ0A filename.
typedef struct __attribute__((__packed__)) {
    uint8_t                          valid;                              // Is this mapping valid?
    uint8_t                          entries;                            // Number of entries in cache.
    uint8_t                          type;                               // Type of file being cached.
    char                             directory[TZSVC_DIRNAME_SIZE];      // Directory this mapping is associated with.
    union {
        t_sharpToSDMap               *mzfFile[TZSVC_MAX_DIR_ENTRIES];    // File mapping of SD file to its Sharp MZ80A name.
        uint8_t                      *sdFileName[TZSVC_MAX_DIR_ENTRIES]; // No mapping for SD filenames, just the file name.
    };
} t_dirMap;


// Structure to maintain all MZ700 hardware control information in order to emulate the machine.
//
typedef struct {
    uint32_t                         config;                             // Compacted control register, 31:19 = reserved, 18 = Inhibit mode, 17 = Upper D000:FFFF is RAM (=1), 16 = Lower 0000:0FFF is RAM (=1), 15:8 = old memory mode, 7:0 = current memory mode.
    //uint8_t                          memoryMode;                         // The memory mode the MZ700 is currently running under, this is determined by the memory control commands from the MZ700.
    //uint8_t                          lockMemoryMode;                     // The preserved memory mode when entering the locked state.
    //uint8_t                          inhibit;                            // The inhibit flag, blocks the upper 0xD000:0xFFFF region from being accessed, affects the memoryMode temporarily.
    //uint8_t                          update;                             // Update flag, indicates to the ISR that a memory mode update is needed.
    //uint8_t                          b0000;                              // Block 0000:0FFF mode.
    //uint8_t                          bD000;                              // Block D000:FFFF mode.
} t_mz700;

// Structure to maintain all MZ-80B hardware control information in oder to emulate the machine as near as possible.
typedef struct {
    uint32_t                         config;                             // Compacted control register, 31:19 = reserved, 18 = Inhibit mode, 17 = Upper D000:FFFF is RAM (=1), 16 = Lower 0000:0FFF is RAM (=1), 15:8 = old memory mode, 7:0 = current memory mode.
} t_mz80b;

// Structure to maintain all the control and management variables of the Z80 and underlying hardware so that the state of run is well known by any called method.
//
typedef struct {
  #if !defined(__APP__) || defined(__TZFLUPD__)
    uint32_t                         svcControlAddr;                     // Address of the service control record within the Z80 static RAM bank.
    uint8_t                          refreshAddr;                        // Refresh address for times when the K64F must issue refresh cycles on the Z80 bus.
    uint8_t                          disableRefresh;                     // Disable refresh if the mainboard DRAM isnt being used.
    uint8_t                          runCtrlLatch;                       // Latch value the Z80 is running with.
    uint8_t                          curCtrlLatch;                       // Latch value set during tranZPUter access of the Z80 bus.
    uint8_t                          holdZ80;                            // A flag to hold the Z80 bus when multiple transactions need to take place.
    uint8_t                          videoRAM[2][2048];                  // Two video memory buffer frames, allows for storage of original frame in [0] and working frame in [1].
    uint8_t                          attributeRAM[2][2048];              // Two attribute memory buffer frames, allows for storage of original frame in [0] and working frame in [1].

    enum CTRL_MODE                   ctrlMode;                           // Mode of control, ie normal Z80 Running, controlling mainboard, controlling tranZPUter.
    enum BUS_DIRECTION               busDir;                             // Direction the bus has been configured for.
    enum MACHINE_TYPES               hostType;                           // The underlying host machine, 0 = Sharp MZ-80A, 1 = MZ-700, 2 = MZ-80B
    enum MACHINE_TYPES               machineMode;                        // Machine compatibility, 0 = Sharp MZ-80A, 1 = MZ-700, 2 = MZ-80B
    t_mz700                          mz700;                              // MZ700 emulation control to detect IO commands and adjust the memory map accordingly.
    t_mz80b                          mz80b;                              // MZ-80B emulation control to detect IO commands and adjust the memory map and I/O forwarding accordingly.

    uint8_t                          resetEvent;                         // A Z80_RESET event occurred, probably user pressing RESET button.
    uint8_t                          svcRequest;                         // A service request has been made by the Z80 (1).
    uint8_t                          sysRequest;                         // A system request has been made by the Z80 (1).
    uint8_t                          ioAddr;                             // Address of a Z80 IO instruction.
    uint8_t                          ioEvent;                            // Event flag to indicate that an IO instruction was captured.
    uint8_t                          ioData;                             // Data of a Z80 IO instruction.
    uint8_t                          memorySwap;                         // A memory Swap event has occurred, 0000-0FFF -> C000-CFFF (1), or C000-CFFF -> 0000-0FFF (0)
    uint8_t                          crtMode;                            // A CRT event has occurred, Normal mode (0) or Reverse Mode (1)
    uint8_t                          scroll;                             // Hardware scroll offset.
    volatile uint32_t                portA;                              // ISR store of GPIO Port A used for signal decoding.
    volatile uint32_t                portB;                              // ISR store of GPIO Port B used for signal decoding.
    volatile uint32_t                portC;                              // ISR store of GPIO Port C used for signal decoding.
    volatile uint32_t                portD;                              // ISR store of GPIO Port D used for signal decoding.
    volatile uint32_t                portE;                              // ISR store of GPIO Port E used for signal decoding.
  #endif
} t_z80Control;

// Structure to maintain higher level OS control and management variables typically used for TZFS and CPM.
//
typedef struct {
	uint8_t                          tzAutoBoot;                         // Autoboot the tranZPUter into TZFS mode.
    t_dirMap                         dirMap;                             // Directory map of SD filenames to Sharp MZ80A filenames.
    t_cpmDriveMap                    cpmDriveMap;                        // Map of file number to an open SD disk file to be used as a CPM drive.
    uint8_t                          *lastFile;                          // Last file loaded - typically used for CPM to reload itself.
} t_osControl;

// Structure to contain inter CPU communications memory for command service processing and results.
// Typically the z80 places a command into the structure in it's memory space and asserts an I/O request,
// the K64F detects the request and reads the lower portion of the struct from z80 memory space, 
// determines the command and then either reads the remainder or writes to the remainder. This struct
// exists in both the z80 and K64F domains and data is sync'd between them as needed.
//
typedef struct __attribute__((__packed__)) {
    uint8_t                          cmd;                                // Command request.
    uint8_t                          result;                             // Result code. 0xFE - set by Z80, command available, 0xFE - set by K64F, command ack and processing. 0x00-0xF0 = cmd complete and result of processing.
    union {
        uint8_t                      dirSector;                          // Virtual directory sector number.
        uint8_t                      fileSector;                         // Sector within open file to read/write.
        uint8_t                      vDriveNo;                           // Virtual or physical SD card drive number.
    };
    union {
        struct {
            uint16_t                 trackNo;                            // For virtual drives with track and sector this is the track number
            uint16_t                 sectorNo;                           // For virtual drives with track and sector this is the sector number. NB For LBA access, this is 32bit and overwrites fileNo/fileType which arent used during raw SD access.
        };
        uint32_t                     sectorLBA;                          // For LBA access, this is 32bit and used during raw SD access.
        struct {
            uint8_t                  memTarget;                          // Target memory for operation, 0 = tranZPUter, 1 = mainboard.
            uint8_t                  spare1;                             // Unused variable.
            uint16_t                 spare2;                             // Unused variable.
        };
    };
    uint8_t                          fileNo;                             // File number of a file within the last directory listing to open/update.
    uint8_t                          fileType;                           // Type of file being processed.
    union {
        uint16_t                     loadAddr;                           // Load address for ROM/File images which need to be dynamic.
        uint16_t                     saveAddr;                           // Save address for ROM/File images which need to be dynamic.
        uint16_t                     cpuFreq;                            // CPU Frequency in KHz - used for setting of the alternate CPU clock frequency.
    };
    union {
        uint16_t                     loadSize;                           // Size for ROM/File to be loaded.
        uint16_t                     saveSize;                           // Size for ROM/File to be saved.
    };
    uint8_t                          directory[TZSVC_DIRNAME_SIZE];      // Directory in which to look for a file. If no directory is given default to MZF.
    uint8_t                          filename[TZSVC_FILENAME_SIZE];      // File to open or create.
    uint8_t                          wildcard[TZSVC_WILDCARD_SIZE];      // A basic wildcard pattern match filter to be applied to a directory search.
    uint8_t                          sector[TZSVC_SECTOR_SIZE];          // Sector buffer generally for disk read/write.
} t_svcControl;

// Structure to define all the directory entries which are packed into a single SD sector which is used between the Z80<->K64F.
//
typedef struct __attribute__((__packed__)) {
    t_svcDirEnt                      dirEnt[TZVC_MAX_DIRENT_BLOCK];      // Fixed number of directory entries per sector/block. 
} t_svcDirBlock;

// Structure to hold compacted directory entries which are packed into a single SD sector which is used between the Z80<->K64F.
//
typedef struct __attribute__((__packed__)) {
    t_svcCmpDirEnt                   dirEnt[TZVC_MAX_CMPCT_DIRENT_BLOCK];// Fixed number of compacted directory entries per sector/block. 
} t_svcCmpDirBlock;

// Mapping table from Sharp MZ80A Ascii to real Ascii.
//
typedef struct {
    uint8_t                          asciiCode;
} t_asciiMap;

// Application execution constants.
//

// For the ARM Cortex-M compiler, the standard filestreams in an app are set by the CRT0 startup code,
// the original reentrant definition is undefined as it is not needed in the app.
#if defined __APP__ && defined __K64F__
  #undef stdout
  #undef stdin
  #undef stderr
  FILE   *stdout;
  FILE   *stdin;
  FILE   *stderr;
#endif

// References to variables within the main library code.
extern volatile uint32_t              *ioPin[MAX_TRANZPUTER_PINS];
extern uint8_t                        pinMap[MAX_TRANZPUTER_PINS];

// Prototypes.
//
#if defined __APP__
void                                  yield(void);
#endif
void                                  setupZ80Pins(uint8_t, volatile uint32_t *);
void                                  resetZ80(uint8_t);
uint8_t                               reqZ80Bus(uint32_t);
uint8_t                               reqMainboardBus(uint32_t);
uint8_t                               reqTranZPUterBus(uint32_t, enum TARGETS);
void                                  setupSignalsForZ80Access(enum BUS_DIRECTION);
void                                  releaseZ80(void);
void                                  refreshZ80(void);
void                                  refreshZ80AllRows(void);
void                                  setCtrlLatch(uint8_t);
uint32_t                              setZ80CPUFrequency(float, uint8_t);
uint8_t                               copyFromZ80(uint8_t *, uint32_t, uint32_t, enum TARGETS);
uint8_t                               copyToZ80(uint32_t, uint8_t *, uint32_t, enum TARGETS);
uint8_t                               writeZ80Memory(uint32_t, uint8_t, enum TARGETS);
uint8_t                               readZ80Memory(uint32_t);
uint8_t                               outZ80IO(uint32_t, uint8_t);
uint8_t                               inZ80IO(uint32_t);
uint8_t                               writeZ80IO(uint32_t, uint8_t, enum TARGETS);
uint8_t                               readZ80IO(uint32_t, enum TARGETS);
void                                  fillZ80Memory(uint32_t, uint32_t, uint8_t, enum TARGETS);
void                                  captureVideoFrame(enum VIDEO_FRAMES, uint8_t);
void                                  refreshVideoFrame(enum VIDEO_FRAMES, uint8_t, uint8_t);
FRESULT                               loadVideoFrameBuffer(char *, enum VIDEO_FRAMES);
FRESULT                               saveVideoFrameBuffer(char *, enum VIDEO_FRAMES);   
char                                  *getVideoFrame(enum VIDEO_FRAMES);
char                                  *getAttributeFrame(enum VIDEO_FRAMES);
FRESULT                               loadZ80Memory(const char *, uint32_t, uint32_t, uint32_t, uint32_t *, enum TARGETS, uint8_t);
FRESULT                               saveZ80Memory(const char *, uint32_t, uint32_t, t_svcDirEnt *, enum TARGETS);
FRESULT                               loadMZFZ80Memory(const char *, uint32_t, uint32_t *, uint8_t, enum TARGETS, uint8_t);

// Getter/Setter methods!
uint8_t                               isZ80Reset(void);
uint8_t                               isZ80MemorySwapped(void);
uint8_t                               getZ80IO(uint8_t *);
void                                  clearZ80Reset(void);
void                                  convertSharpFilenameToAscii(char *, char *, uint8_t);
void                                  convertToFAT32FileNameFormat(char *);

// tranZPUter OS i/f methods.
uint8_t                               setZ80SvcStatus(uint8_t);
void                                  svcSetDefaults(enum FILE_TYPE);
uint8_t                               svcReadDir(uint8_t, enum FILE_TYPE);
uint8_t                               svcFindFile(char *, char *, uint8_t, enum FILE_TYPE);
uint8_t                               svcReadDirCache(uint8_t, enum FILE_TYPE);
uint8_t                               svcFindFileCache(char *, char *, uint8_t, enum FILE_TYPE);
uint8_t                               svcCacheDir(const char *, enum FILE_TYPE, uint8_t);
uint8_t                               svcReadFile(uint8_t, enum FILE_TYPE);
uint8_t                               svcWriteFile(uint8_t, enum FILE_TYPE);
uint8_t                               svcLoadFile(enum FILE_TYPE);
uint8_t                               svcSaveFile(enum FILE_TYPE);
uint8_t                               svcEraseFile(enum FILE_TYPE);
uint8_t                               svcAddCPMDrive(void);
uint8_t                               svcReadCPMDrive(void);
uint8_t                               svcWriteCPMDrive(void);
uint32_t                              getServiceAddr(void);
void                                  processServiceRequest(void);
uint8_t                               loadBIOS(const char *biosFileName, uint8_t machineMode, uint32_t loadAddr);
void                                  hardResetTranZPUter(void);
void                                  loadTranZPUterDefaultROMS(uint8_t);
void                                  tranZPUterControl(void);
uint8_t                               testTZFSAutoBoot(void);
void                                  setHost(void);
void                                  setupTranZPUter(void);
void                                  testRoutine(void);

#if defined __APP__
int                                   memoryDumpZ80(uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, enum TARGETS);
#endif

// Debug methods.
#if defined __APP__ && defined __TZPU_DEBUG__
void                                  displaySignals(void);
#endif

#ifdef __cplusplus
}
#endif
#endif // TRANZPUTER_H
