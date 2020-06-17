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
//#define DECODE_Z80_IO                3                                   // Flag to enable code, via interrupt, to capture Z80 actions on I/O ports an Memory mapped I/O.
                                                                         // 0 = No code other than direct service request interrupts.
                                                                         // 1 = Decode Z80 I/O address operations.
                                                                         // 2 = Decode Z80 I/O operations with data.
                                                                         // 3 = NZ700 memory mode decode - This doesnt work per original, the memory change occurs one instruction after the OUT instruction due to the way the Z80 functions in respect to BUSRQ.
#define REFRESH_BYTE_COUNT           8                                   // This constant controls the number of bytes read/written to the z80 bus before a refresh cycle is needed.
#define RFSH_BYTE_CNT                256                                 // Number of bytes we can write before needing a full refresh for the DRAM.

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
#define TZMM_MZ700_0                 0x0a                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the mainboard.
#define TZMM_MZ700_1                 0x0b                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 0, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the tranZPUter in block 6.
#define TZMM_MZ700_2                 0x0c                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is on the tranZPUter in block 6.
#define TZMM_MZ700_3                 0x0d                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 0, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is inaccessible.
#define TZMM_MZ700_4                 0x0e                                // MZ700 Mode - 0000:0FFF is on the tranZPUter board in block 6, 1000:CFFF is on the tranZPUter board in block 0, D000:FFFF is inaccessible.
#define TZMM_TZPU0                   0x18                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 0 is selected.
#define TZMM_TZPU1                   0x19                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 1 is selected.
#define TZMM_TZPU2                   0x1A                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 2 is selected.
#define TZMM_TZPU3                   0x1B                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 3 is selected.
#define TZMM_TZPU4                   0x1C                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 4 is selected.
#define TZMM_TZPU5                   0x1D                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 5 is selected.
#define TZMM_TZPU6                   0x1E                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 6 is selected.
#define TZMM_TZPU7                   0x1F                                // Everything is in tranZPUter domain, no access to underlying Sharp mainboard unless memory management mode is switched. tranZPUter RAM 64K block 7 is selected.

// IO addresses on the tranZPUter or mainboard.
//
#define IO_TZ_CTRLLATCH              0x60                                // Control latch which specifies the Memory Model/mode.
#define IO_TZ_SETXMHZ                0x62                                // Switch to alternate CPU frequency provided by K64F.
#define IO_TZ_SET2MHZ                0x64                                // Switch to system CPU frequency.
#define IO_TZ_CLKSELRD               0x66                                // Read the status of the clock select, ie. which clock is connected to the CPU.
#define IO_TZ_SVCREQ                 0x68                                // Service request from the Z80 to be provided by the K64F.
#define IO_TZ_SYSREQ                 0x6A                                // System request from the Z80 to be provided by the K64F.

// Sharp MZ80A constants.
//
#define MZ_MROM_ADDR                 0x0000                              // Monitor ROM start address.
#define MZ_MROM_STACK_ADDR           0x1000                              // Monitor ROM start stack address.
#define MZ_MROM_STACK_SIZE           0x0200                              // Monitor ROM stack size.
#define MZ_UROM_ADDR                 0xE800                              // User ROM start address.
#define MZ_BANKRAM_ADDR              0xF000                              // Floppy API address which is used in TZFS as the paged RAM for additional functionality.
#define MZ_CMT_ADDR                  0x10F0                              // Address of the CMT (tape) header record.
#define MZ_CMT_DEFAULT_LOAD_ADDR     0x1200                              // The default load address for a CMT, anything below this is normally illegal.
#define MZ_VID_RAM_ADDR              0xD000                              // Start of Video RAM
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
#define MZ_ROM_SA1510_40C            "0:\\TZFS\\SA1510.ROM"              // Original 40 character Monitor ROM.
#define MZ_ROM_SA1510_80C            "0:\\TZFS\\SA1510-8.ROM"            // Original Monitor ROM patched for 80 character screen mode.
#define MZ_ROM_1Z_013A_40C           "0:\\TZFS\\1Z-013A.ROM"             // Original 40 character Monitor ROM for the Sharp MZ700.
#define MZ_ROM_1Z_013A_80C           "0:\\TZFS\\1Z-013A-8.ROM"           // Original Monitor ROM patched for the Sharp MZ700 patched for 80 column mode.
#define MZ_ROM_MZ80B_IPL             "0:\\TZFS\\MZ80B_IPL.ROM"           // Original IPL ROM for the Sharp MZ-80B.
#define MZ_ROM_TZFS                  "0:\\TZFS\\TZFS.ROM"                // tranZPUter Filing System ROM.

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
#define TZSVC_CMD_LOAD40BIOS         0x20                                // Service command requesting that the 40 column version of the SA1510 BIOS is loaded.
#define TZSVC_CMD_LOAD80BIOS         0x21                                // Service command requesting that the 80 column version of the SA1510 BIOS is loaded.
#define TZSVC_CMD_LOAD700BIOS40      0x22                                // Service command requesting that the MZ700 1Z-013A 40 column BIOS is loaded.
#define TZSVC_CMD_LOAD700BIOS80      0x23                                // Service command requesting that the MZ700 1Z-013A 80 column patched BIOS is loaded.
#define TZSVC_CMD_LOAD80BIPL         0x24                                // Service command requesting the MZ-80B IPL is loaded.
#define TZSVC_CMD_LOADBDOS           0x30                                // Service command to reload CPM BDOS+CCP.
#define TZSVC_CMD_ADDSDDRIVE         0x31                                // Service command to attach a CPM disk to a drive number.
#define TZSVC_CMD_READSDDRIVE        0x32                                // Service command to read an attached SD file as a CPM disk drive.
#define TZSVC_CMD_WRITESDDRIVE       0x33                                // Service command to write to a CPM disk drive which is an attached SD file.
#define TZSVC_CMD_CPU_BASEFREQ       0x40                                // Service command to switch to the mainboard frequency.
#define TZSVC_CMD_CPU_ALTFREQ        0x41                                // Service command to switch to the alternate frequency provided by the K64F.
#define TZSVC_CMD_CPU_CHGFREQ        0x42                                // Service command to set the alternate frequency in hertz.
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
#define TZSVC_SECTOR_SIZE            512                                 // SD Card sector buffer size.
#define TZSVC_STATUS_OK              0x00                                // Flag to indicate the K64F processing completed successfully.
#define TZSVC_STATUS_FILE_ERROR      0x01                                // Flag to indicate a file or directory error.
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
#define MAX_TRANZPUTER_PINS          52
#define Z80_MEM0_PIN                 46
#define Z80_MEM1_PIN                 47
#define Z80_MEM2_PIN                 48
#define Z80_MEM3_PIN                 49
#define Z80_MEM4_PIN                 50
#define Z80_WR_PIN                   10
#define Z80_RD_PIN                   12
#define Z80_IORQ_PIN                 8
#define Z80_MREQ_PIN                 9
#define Z80_A0_PIN                   39
#define Z80_A1_PIN                   38
#define Z80_A2_PIN                   37
#define Z80_A3_PIN                   36
#define Z80_A4_PIN                   35
#define Z80_A5_PIN                   34
#define Z80_A6_PIN                   33
#define Z80_A7_PIN                   32
#define Z80_A8_PIN                   31
#define Z80_A9_PIN                   30
#define Z80_A10_PIN                  29
#define Z80_A11_PIN                  28
#define Z80_A12_PIN                  27
#define Z80_A13_PIN                  26
#define Z80_A14_PIN                  25
#define Z80_A15_PIN                  24
#define Z80_A16_PIN                  23
#define Z80_A17_PIN                  22
#define Z80_A18_PIN                  21
#define Z80_D0_PIN                   0
#define Z80_D1_PIN                   1
#define Z80_D2_PIN                   2
#define Z80_D3_PIN                   3
#define Z80_D4_PIN                   4
#define Z80_D5_PIN                   5
#define Z80_D6_PIN                   6
#define Z80_D7_PIN                   7
#define Z80_WAIT_PIN                 13
#define Z80_BUSACK_PIN               17
#define Z80_NMI_PIN                  43
#define Z80_INT_PIN                  44
#define Z80_RESET_PIN                54
#define SYSCLK_PIN                   11
#define CTL_RFSH_PIN                 45
#define CTL_HALT_PIN                 18
#define CTL_M1_PIN                   20
#define CTL_BUSRQ_PIN                15
#define CTL_BUSACK_PIN               16
#define CTL_CLK_PIN                  14
#define CTL_CLKSLCT_PIN              19
#define TZ_BUSACK_PIN                55
#define TZ_SVCREQ_PIN                56
#define TZ_SYSREQ_PIN                57

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
// Studying the Teensyduino code these macros could be stripped down further and go direct to the BITBAND registers if more speed is needed.
//
#define STR(x)                       #x
#define XSTR(s)                      STR(s)
#define pinLow(a)                    *portClearRegister(pinMap[a]) = 1
#define pinHigh(a)                   *portSetRegister(pinMap[a]) = 1
#define pinSet(a, b)                 if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; }
#define pinGet(a)                    *portInputRegister(pinMap[a])
#define pinInput(a)                  { *portModeRegister(pinMap[a]) = 0; *ioPin[a] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS; }
#define pinOutput(a)                 { *portModeRegister(pinMap[a]) = 1;\
                                       *ioPin[a] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                                       *ioPin[a] &= ~PORT_PCR_ODE; }
#define pinOutputSet(a,b)            { *portModeRegister(pinMap[a]) = 1;\
                                       *ioPin[a] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                                       *ioPin[a] &= ~PORT_PCR_ODE;\
                                       if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; } }
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

#define setZ80Data(a)                { pinSet(Z80_D7,  ((a >>  7) & 0x1)); pinSet(Z80_D6,  ((a >>  6) & 0x1));\
                                       pinSet(Z80_D5,  ((a >>  5) & 0x1)); pinSet(Z80_D4,  ((a >>  4) & 0x1));\
                                       pinSet(Z80_D3,  ((a >>  3) & 0x1)); pinSet(Z80_D2,  ((a >>  2) & 0x1));\
                                       pinSet(Z80_D1,  ((a >>  1) & 0x1)); pinSet(Z80_D0,  ((a      ) & 0x1)); }
#define setZ80Addr(a)                { pinSet(Z80_A15, ((a >> 15) & 0x1)); pinSet(Z80_A14, ((a >> 14) & 0x1));\
                                       pinSet(Z80_A13, ((a >> 13) & 0x1)); pinSet(Z80_A12, ((a >> 12) & 0x1));\
                                       pinSet(Z80_A11, ((a >> 11) & 0x1)); pinSet(Z80_A10, ((a >> 10) & 0x1));\
                                       pinSet(Z80_A9,  ((a >>  9) & 0x1)); pinSet(Z80_A8,  ((a >>  8) & 0x1));\
                                       pinSet(Z80_A7,  ((a >>  7) & 0x1)); pinSet(Z80_A6,  ((a >>  6) & 0x1));\
                                       pinSet(Z80_A5,  ((a >>  5) & 0x1)); pinSet(Z80_A4,  ((a >>  4) & 0x1));\
                                       pinSet(Z80_A3,  ((a >>  3) & 0x1)); pinSet(Z80_A2,  ((a >>  2) & 0x1));\
                                       pinSet(Z80_A1,  ((a >>  1) & 0x1)); pinSet(Z80_A0,  ((a      ) & 0x1)); }
#define setZ80AddrLower(a)           { pinSet(Z80_A7,  ((a >>  7) & 0x1)); pinSet(Z80_A6,  ((a >>  6) & 0x1));\
                                       pinSet(Z80_A5,  ((a >>  5) & 0x1)); pinSet(Z80_A4,  ((a >>  4) & 0x1));\
                                       pinSet(Z80_A3,  ((a >>  3) & 0x1)); pinSet(Z80_A2,  ((a >>  2) & 0x1));\
                                       pinSet(Z80_A1,  ((a >>  1) & 0x1)); pinSet(Z80_A0,  ((a      ) & 0x1)); }
#define setZ80RefreshAddr(a)         { pinSet(Z80_A6,  ((a >>  6) & 0x1)); pinSet(Z80_A5,  ((a >>  5) & 0x1));\
                                       pinSet(Z80_A4,  ((a >>  4) & 0x1)); pinSet(Z80_A3,  ((a >>  3) & 0x1));\
                                       pinSet(Z80_A2,  ((a >>  2) & 0x1)); pinSet(Z80_A1,  ((a >>  1) & 0x1));\
                                       pinSet(Z80_A0,  ((a      ) & 0x1)); }
#define readZ80AddrLower()           ( pinGet(Z80_A7) << 7 | pinGet(Z80_A6) << 6 | pinGet(Z80_A5) << 5 | pinGet(Z80_A4) << 4 |\
                                       pinGet(Z80_A3) << 3 | pinGet(Z80_A2) << 2 | pinGet(Z80_A1) << 1 | pinGet(Z80_A0) )
#define readZ80Addr(a)               ( pinGet(Z80_A15) << 15 | pinGet(Z80_A14) << 14 | pinGet(Z80_A13) << 13 | pinGet(Z80_A12) << 12 |\
                                       pinGet(Z80_A11) << 11 | pinGet(Z80_A10) << 10 | pinGet(Z80_A9) << 9 | pinGet(Z80_A8) << 8  |\
                                       pinGet(Z80_A7) << 7 | pinGet(Z80_A6) << 6 | pinGet(Z80_A5) << 5 | pinGet(Z80_A4) << 4 |\
                                       pinGet(Z80_A3) << 3 | pinGet(Z80_A2) << 2 | pinGet(Z80_A1) << 1 | pinGet(Z80_A0) )
#define readDataBus()                ( pinGet(Z80_D7) << 7 | pinGet(Z80_D6) << 6 | pinGet(Z80_D5) << 5 | pinGet(Z80_D4) << 4 |\
                                       pinGet(Z80_D3) << 3 | pinGet(Z80_D2) << 2 | pinGet(Z80_D1) << 1 | pinGet(Z80_D0) )
#define readCtrlLatch()              ( (pinGet(Z80_MEM4) << 4 | pinGet(Z80_MEM3) << 3 | pinGet(Z80_MEM2) << 2 | pinGet(Z80_MEM1) << 1 | pinGet(Z80_MEM0)) & 0x1F )
#define writeCtrlLatch(a)            { writeZ80IO(IO_TZ_CTRLLATCH, a); } 
#define setZ80Direction(a)           { for(uint8_t idx=Z80_D0; idx <= Z80_D7; idx++) { if(a == WRITE) { pinOutput(idx); } else { pinInput(idx); } }; z80Control.busDir = a; }
#define reqZ80BusChange(a)           { if(a == MAINBOARD_ACCESS && z80Control.ctrlMode == TRANZPUTER_ACCESS) \
                                       {\
                                           pinHigh(CTL_BUSACK);\
                                           z80Control.ctrlMode = MAINBOARD_ACCESS;\
                                           z80Control.curCtrlLatch = 0b00000000;\
                                           writeCtrlLatch(z80Control.curCtrlLatch);\
                                       } else if(a == TRANZPUTER_ACCESS && z80Control.ctrlMode == MAINBOARD_ACCESS)\
                                       {\
                                           pinLow(CTL_BUSACK);\
                                           z80Control.ctrlMode = TRANZPUTER_ACCESS;\
                                           z80Control.curCtrlLatch = 0b00011111;\
                                           writeCtrlLatch(z80Control.curCtrlLatch);\
                                       } }
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

    Z80_D0                           = 19,
    Z80_D1                           = 20,
    Z80_D2                           = 21,
    Z80_D3                           = 22,
    Z80_D4                           = 23,
    Z80_D5                           = 24,
    Z80_D6                           = 25,
    Z80_D7                           = 26,

    Z80_MEM0                         = 27,
    Z80_MEM1                         = 28,
    Z80_MEM2                         = 29,
    Z80_MEM3                         = 30,
    Z80_MEM4                         = 31,

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
    TZ_BUSACK                        = 42,
    TZ_SVCREQ                        = 43,
    TZ_SYSREQ                        = 44,

    CTL_BUSACK                       = 45,
    CTL_BUSRQ                        = 46,
    CTL_RFSH                         = 47,
    CTL_HALT                         = 48,
    CTL_M1                           = 49,
    CTL_CLK                          = 50,
    CTL_CLKSLCT                      = 51 
};

// Possible control modes that the K64F can be in, do nothing where the Z80 runs normally, control the Z80 and mainboard, or control the Z80 and tranZPUter.
enum CTRL_MODE {
    Z80_RUN                          = 0,
    TRANZPUTER_ACCESS                = 1,
    MAINBOARD_ACCESS                 = 2
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

// Possible machines the tranZPUter can emulate.
//
enum MACHINE_MODE {
    MZ80A                            = 0,
    MZ700                            = 1,
    MZ80B                            = 2
};

// Types of file which have handlers and can be processed.
//
enum FILE_TYPE {
    MZF                              = 0,
    CAS                              = 1,
    BAS                              = 2
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
    char                             directory[TZSVC_DIRNAME_SIZE];      // Directory this mapping is associated with.
    t_sharpToSDMap                   *file[TZSVC_MAX_DIR_ENTRIES];       // File mapping of SD file to its Sharp MZ80A name.
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
  #ifndef __APP__
    uint32_t                         svcControlAddr;                     // Address of the service control record within the 512K static RAM bank.
    uint8_t                          refreshAddr;                        // Refresh address for times when the K64F must issue refresh cycles on the Z80 bus.
    uint8_t                          disableRefresh;                     // Disable refresh if the mainboard DRAM isnt being used.
    uint8_t                          runCtrlLatch;                       // Latch value the Z80 is running with.
    uint8_t                          curCtrlLatch;                       // Latch value set during tranZPUter access of the Z80 bus.
    uint8_t                          videoRAM[2][2048];                  // Two video memory buffer frames, allows for storage of original frame in [0] and working frame in [1].
    uint8_t                          attributeRAM[2][2048];              // Two attribute memory buffer frames, allows for storage of original frame in [0] and working frame in [1].

    enum CTRL_MODE                   ctrlMode;                           // Mode of control, ie normal Z80 Running, controlling mainboard, controlling tranZPUter.
    enum BUS_DIRECTION               busDir;                             // Direction the bus has been configured for.

    enum MACHINE_MODE                machineMode;                        // Machine compatibility, 0 = Sharp MZ-80A, 1 = MZ-700, 2 = MZ-80B
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
    };
    uint16_t                         trackNo;                            // For virtual drives with track and sector this is the track number
    uint16_t                         sectorNo;                           // For virtual drives with tracl and sector this is the sector number.
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
    uint8_t    asciiCode;
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
void          yield(void);
#endif
void          setupZ80Pins(uint8_t, volatile uint32_t *);
void          resetZ80(void);
uint8_t       reqZ80Bus(uint32_t);
uint8_t       reqMainboardBus(uint32_t);
uint8_t       reqTranZPUterBus(uint32_t);
void          setupSignalsForZ80Access(enum BUS_DIRECTION);
void          releaseZ80(void);
void          refreshZ80(void);
void          setCtrlLatch(uint8_t);
uint32_t      setZ80CPUFrequency(float, uint8_t);
uint8_t       copyFromZ80(uint8_t *, uint32_t, uint32_t, uint8_t);
uint8_t       copyToZ80(uint32_t, uint8_t *, uint32_t, uint8_t);
uint8_t       writeZ80Memory(uint16_t, uint8_t);
uint8_t       readZ80Memory(uint16_t);
uint8_t       writeZ80IO(uint16_t, uint8_t);
uint8_t       readZ80IO(uint16_t);
void          fillZ80Memory(uint32_t, uint32_t, uint8_t, uint8_t);
void          captureVideoFrame(enum VIDEO_FRAMES, uint8_t);
void          refreshVideoFrame(enum VIDEO_FRAMES, uint8_t, uint8_t);
FRESULT       loadVideoFrameBuffer(char *, enum VIDEO_FRAMES);
FRESULT       saveVideoFrameBuffer(char *, enum VIDEO_FRAMES);   
char          *getVideoFrame(enum VIDEO_FRAMES);
char          *getAttributeFrame(enum VIDEO_FRAMES);
FRESULT       loadZ80Memory(const char *, uint32_t, uint32_t, uint32_t, uint32_t *, uint8_t, uint8_t);
FRESULT       saveZ80Memory(const char *, uint32_t, uint32_t, t_svcDirEnt *, uint8_t);
FRESULT       loadMZFZ80Memory(const char *, uint32_t, uint32_t *, uint8_t, uint8_t);

// Getter/Setter methods!
uint8_t       isZ80Reset(void);
uint8_t       isZ80MemorySwapped(void);
uint8_t       getZ80IO(uint8_t *);
void          clearZ80Reset(void);
void          convertSharpFilenameToAscii(char *, char *, uint8_t);

// tranZPUter OS i/f methods.
uint8_t       setZ80SvcStatus(uint8_t);
void          svcSetDefaults(enum FILE_TYPE);
uint8_t       svcReadDir(uint8_t);
uint8_t       svcFindFile(char *, char *, uint8_t);
uint8_t       svcReadDirCache(uint8_t);
uint8_t       svcFindFileCache(char *, char *, uint8_t);
uint8_t       svcCacheDir(const char *, uint8_t);
uint8_t       svcReadFile(uint8_t, enum FILE_TYPE);
uint8_t       svcWriteFile(uint8_t, enum FILE_TYPE);
uint8_t       svcLoadFile(enum FILE_TYPE);
uint8_t       svcSaveFile(enum FILE_TYPE);
uint8_t       svcEraseFile(enum FILE_TYPE);
uint8_t       svcAddCPMDrive(void);
uint8_t       svcReadCPMDrive(void);
uint8_t       svcWriteCPMDrive(void);
uint32_t      getServiceAddr(void);
void          processServiceRequest(void);
void          loadTranZPUterDefaultROMS(void);
void          tranZPUterControl(void);
uint8_t       testTZFSAutoBoot(void);
void          setupTranZPUter(void);

#if defined __APP__
int           memoryDumpZ80(uint32_t, uint32_t, uint32_t, uint8_t, uint8_t);
#endif

// Debug methods.
#if defined __APP__ && defined __TZPU_DEBUG__
void          displaySignals(void);
#endif

#ifdef __cplusplus
}
#endif
#endif // TRANZPUTER_H
