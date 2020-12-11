/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            sharpmz.c
// Created:         December 2020
// Version:         v1.0
// Author(s):       Philip Smart
// Description:     The Sharp MZ library.
//                  This file contains methods which allow the ZPU to access and control the Sharp MZ
//                  series computer hardware. The ZPU is instantiated within a physical Sharp MZ machine
//                  or an FPGA hardware emulation and provides either a host CPU running zOS or as an 
//                  I/O processor providing services.
//
//                  NB. This library is NOT yet thread safe.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         v1.0 Dec 2020  - Initial write of the Sharp MZ series hardware interface software.
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
#ifndef SHARPMZ_H
#define SHARPMZ_H

#ifdef __cplusplus
    extern "C" {
#endif

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
#define TZMM_ENIOWAIT                0x20                                // Enable wait state generator for Sharp system IO operations in region 0xE0-0xFF.

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
#define IO_TZ_SYSCTRL                0xF0                                // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define IO_TZ_GRAMMODE               0xF4                                // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define IO_TZ_VMCTRL                 0xF8                                // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define IO_TZ_VMGRMODE               0xF9                                // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define IO_TZ_VMREDMASK              0xFA                                // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMGREENMASK            0xFB                                // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMBLUEMASK             0xFC                                // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMPAGE                 0xFD                                // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.

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
#define CPUMODE_IS_ZPU_EVOL          0x02                                // Status value to indicate if the soft ZPU Evolution available.
#define CPUMODE_IS_AAA               0x04                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_BBB               0x08                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_CCC               0x10                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_IS_DDD               0x20                                // Place holder to indicate if a future soft CPU is available.
#define CPUMODE_RESET_CPU            0x80                                // Reset the soft CPU. Active high, when high the CPU is held in RESET, when low the CPU runs.
#define CPUMODE_IS_SOFT_AVAIL        0x040                               // Marker to indicate if the underlying FPGA can support soft CPU's.
#define CPUMODE_IS_SOFT_MASK         0x0C0                               // Mask to filter out the Soft CPU availability flags.

// Sharp MZ constants.
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
#define TZSVC_CMD_LOAD40ABIOS        0x20                                // Service command requesting that the 40 column version of the SA1510 BIOS is loaded.
#define TZSVC_CMD_LOAD80ABIOS        0x21                                // Service command requesting that the 80 column version of the SA1510 BIOS is loaded.
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
#define TZSVC_CMD_CPU_SETZ80         0x50                                // Service command to switch to the external Z80 hard cpu.
#define TZSVC_CMD_CPU_SETT80         0x51                                // Service command to switch to the internal T80 soft cpu.
#define TZSVC_CMD_CPU_SETZPUEVO      0x52                                // Service command to switch to the internal ZPU Evolution cpu.
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

// Possible machines the tranZPUter can be hosted on and can emulate.
//
enum MACHINE_TYPES {
    MZ80K                            = 0x00,                             // Machine = MZ-80K.
    MZ80C                            = 0x01,                             // Machine = MZ-80C.
    MZ1200                           = 0x02,                             // Machine = MZ-1200.
    MZ80A                            = 0x03,                             // Machine = MZ-80A.
    MZ700                            = 0x04,                             // Machine = MZ-700.
    MZ800                            = 0x05,                             // Machine = MZ-800.
    MZ80B                            = 0x06,                             // Machine = MZ-80B.
    MZ2000                           = 0x07                              // Machine = MZ-2000.
};

// Get and Set flags within the CPLD config and status registers.
//
enum CPLD_FLAGS {
    VIDEO_FPGA                       = 0x08,                             // Bit to test for available functionality or enabling of the FPGA video hardware.
    CPLD_VERSION                     = 0xE0                              // CPLD version mask bits.
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
  #ifndef __APP__
    uint32_t                         svcControlAddr;                     // Address of the service control record within the 512K static RAM bank.
    uint8_t                          refreshAddr;                        // Refresh address for times when the K64F must issue refresh cycles on the Z80 bus.
    uint8_t                          disableRefresh;                     // Disable refresh if the mainboard DRAM isnt being used.
    uint8_t                          runCtrlLatch;                       // Latch value the Z80 is running with.
    uint8_t                          curCtrlLatch;                       // Latch value set during tranZPUter access of the Z80 bus.
    uint8_t                          videoRAM[2][2048];                  // Two video memory buffer frames, allows for storage of original frame in [0] and working frame in [1].
    uint8_t                          attributeRAM[2][2048];              // Two attribute memory buffer frames, allows for storage of original frame in [0] and working frame in [1].

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

// Mapping table from Ascii to Sharp MZ display code.
//
typedef struct {
    uint8_t    dispCode;
} t_dispCodeMap;

// Application execution constants.
//

// Prototypes.
//
int mzPrintChar(char, FILE *);
int mzGetChar(FILE *);

// Getter/Setter methods!

#ifdef __cplusplus
}
#endif
#endif // SHARPMZ_H
