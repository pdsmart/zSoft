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

// Video display constants.
#define VC_MAX_ROWS                  25                                  // Maximum number of rows on display.
#define VC_MAX_COLUMNS               80                                  // Maximum number of columns on display.
#define VC_MAX_BUFFER_ROWS           50                                  // Maximum number of backing store rows for scrollback feature.
#define VC_DISPLAY_BUFFER_SIZE       VC_MAX_COLUMNS * VC_MAX_BUFFER_ROWS // Size of the display buffer for scrollback.

// Target ZPU memory map.
// ----------------------
//
// Y+080000:Y+0FFFFF = 512K Video address space - the video processor memory will be directly mapped into this space as follows:
//                     0x180000 - 0x18FFFF = 64K Video / Attribute RAM
//                     0x190000 - 0x19FFFF = 64K Character Generator ROM/PCG RAM.
//                     0x1A0000 - 0x1BFFFF = 128K Red Framebuffer address space.
//                     0x1C0000 - 0x1DFFFF = 128K Blue Framebuffer address space.
//                     0x1E0000 - 0x1FFFFF = 128K Green Framebuffer address space.
//                     This invokes memory read/write operations but the Video Read/Write signal is directly set, MREQ is not set. This 
//                     allows direct writes to be made to the FPGA video logic, bypassing the CPLD memory manager.
//                     All reads are 32bit, writes are 8, 16 or 32bit wide on word boundary.
//
// Z80 Bus Interface.
// ------------------
//
// 24bit address, 8 bit data.  The Z80 Memory and I/O are mapped into linear ZPU address space. The ZPU makes standard memory transactions and this state machine holds the ZPU whilst it performs the Z80 transaction.
//
// Depending on the accessed address will determine the type of transaction. In order to provide byte level access on a 32bit read CPU, a bank of addresses, word aligned per byte is assigned in addition to
// an address to read 32bit word aligned value.
//
// Y+100000:Y+17FFFF = 512K Static RAM on the tranZPUter board. All reads are 32bit, all writes are 8, 16 or 32bit wide on word boundary.
//
// Y+200000:Y+23FFFF = 64K address space on host mainboard (ie. RAM/ROM/Memory mapped I/O) accessed 1 byte at a time. The physical address is word aligned per byte, so 4 bytes on the ZPU address space = 1
//                     byte on the Z80 address space. ie. 0x00780 ZPU = 0x0078 Z80.
// Y+240000:Y+27FFFF = 64K I/O space on the host mainboard or the underlying CPLD/FPGA. 64K address space is due to the Z80 ability to address 64K via the Accumulator being set in 15:8 and the port in 7:0.
//                     The ZPU, via a direct address will mimic this ability for hardware which requires it. ie. A write to 0x3F with 0x10 in the accumulator would yield an address of 0x103f.
//                     All reads are 8 bit, writes are 8, 16 or 32bit wide on word boundary. The physical address is word aligned per byte, so 4 bytes on the ZPU address space = 1
//                     byte on the Z80 address space. ie. 0x00780 ZPU = 0x0078 Z80.
//
// Y+280000:Y+28FFFF = 64K address space on host mainboard (ie. RAM/ROM/Memory mapped I/O) accessed 4 bytes at a time, a 32 bit read will return 4 consecutive bytes,1start of read must be on a 32bit word boundary.
// Y+290000:Y+2FFFFF = Unassigned.
//
// Y = 2Mbyte sector in ZPU address space the Z80 bus interface is located. This is normally below the ZPU I/O sector and set to 0xExxxxx
//
//
// 0x000000   00000000 - Normal Sharp MZ behaviour, Video Controller controlled by Z80 bus transactions.
// Y+100000:Y+17FFFF = 512K Static RAM on the tranZPUter board. All reads are 32bit, all writes are 8, 16 or 32bit wide on word boundary.
// Y+200000:Y+23FFFF = 64K address space on host mainboard (ie. RAM/ROM/Memory mapped I/O) accessed 1 byte at a time. The physical address is word aligned per byte, so 4 bytes on the ZPU address space = 1
//                     byte on the Z80 address space. ie. 0x00780 ZPU = 0x0078 Z80.
// Y+240000:Y+27FFFF = 64K I/O space on the host mainboard or the underlying CPLD/FPGA. 64K address space is due to the Z80 ability to address 64K via the Accumulator being set in 15:8 and the port in 7:0.
//                     The ZPU, via a direct address will mimic this ability for hardware which requires it. ie. A write to 0x3F with 0x10 in the accumulator would yield an address of 0xF103f.
//                     All reads are 8 bit, writes are 8, 16 or 32bit wide on word boundary. The physical address is word aligned per byte, so 4 bytes on the ZPU address space = 1
//                     byte on the Z80 address space. ie. 0x00780 ZPU = 0x0078 Z80.
// Y+290000:Y+28FFFF = 64K address space on host mainboard (ie. RAM/ROM/Memory mapped I/O) accessed 4 bytes at a time, a 32 bit read will return 4 consecutive bytes,1start of read must be on a 32bit word boundary.
//
// Where Y is the base address, 0xC00000 in this implementation.
// -----------------------------------------------------------------------------------------------------------------------
//
// Video direct addressing. 
// ------------------------
//
//   Address    A23 -A16
// Y+0x080000   00001000 - Memory and I/O ports mapped into direct addressable memory location.
//   
//                         A15 - A8 A7 -  A0
//                         I/O registers are mapped to the bottom 256 bytes mirroring the I/O address.
// Y+0x0800D0              00000000 11010000 - 0xD0 - Set the parameter number to update.
//                         00000000 11010001 - 0xD1 - Update the lower selected parameter byte.
//                         00000000 11010010 - 0xD2 - Update the upper selected parameter byte.
//                         00000000 11010011 - 0xD3 - set the palette slot Off position to be adjusted.
//                         00000000 11010100 - 0xD4 - set the palette slot On position to be adjusted.
//                         00000000 11010101 - 0xD5 - set the red palette value according to the PALETTE_PARAM_SEL address.
//                         00000000 11010110 - 0xD6 - set the green palette value according to the PALETTE_PARAM_SEL address.
// Y+0x0800D7              00000000 11010111 - 0xD7 - set the blue palette value according to the PALETTE_PARAM_SEL address.
//   
// Y+0x0800E0              00000000 11100000 - 0xE0 MZ80B PPI
//                         00000000 11100100 - 0xE4 MZ80B PIT
// Y+0x0800E8              00000000 11101000 - 0xE8 MZ80B PIO
//   
//                         00000000 11110000 - 
//                         00000000 11110001 - 
//                         00000000 11110010 - 
// Y+0x0800F3              00000000 11110011 - 0xF3 set the VGA border colour.
//                         00000000 11110100 - 0xF4 set the MZ80B video in/out mode.
//                         00000000 11110101 - 0xF5 sets the palette.
//                         00000000 11110110 - 0xF6 set parameters.
//                         00000000 11110111 - 0xF7 set the graphics processor unit commands.
//                         00000000 11111000 - 0xF6 set parameters.
//                         00000000 11111001 - 0xF7 set the graphics processor unit commands.
//                         00000000 11111010 - 0xF8 set the video mode. 
//                         00000000 11111011 - 0xF9 set the graphics mode.
//                         00000000 11111100 - 0xFA set the Red bit mask
//                         00000000 11111101 - 0xFB set the Green bit mask
//                         00000000 11111110 - 0xFC set the Blue bit mask
// Y+0x0800FD              00000000 11111111 - 0xFD set the Video memory page in block C000:FFFF 
//   
//                         Memory registers are mapped to the E000 region as per base machines.
// Y+0x08E010              11100000 00010010 - Program Character Generator RAM. E010 - Write cycle (Read cycle = reset memory swap).
//                         11100000 00010100 - Normal display select.
//                         11100000 00010101 - Inverted display select.
//                         11100010 00000000 - Scroll display register. E200 - E2FF
// Y+0x08E2FF              11111111
//   
// Y+0x090000   00001001 - Video/Attribute RAM. 64K Window.
// Y+0x09D000              11010000 00000000 - Video RAM
// Y+0x09D7FF              11010111 11111111
// Y+0x09D800              11011000 00000000 - Attribute RAM
// Y+0x09DFFF              11011111 11111111
//   
// Y+0x0A0000   00001010 - Character Generator RAM 64K Window.
// Y+0x0A0000              00000000 00000000 - CGROM
// Y+0x0A0FFF              00001111 11111111 
// Y+0x0A1000              00010000 00000000 - CGRAM
// Y+0x0A1FFF              00011111 11111111
//   
// Y+0x0C0000   00001100 - 128K Red framebuffer.
//                         00000000 00000000 - Red pixel addressed framebuffer. Also MZ-80B GRAM I memory in lower 8K
// Y+0x0C3FFF              00111111 11111111
// Y+0x0D0000   00001101 - 128K Blue framebuffer.
//                         00000000 00000000 - Blue pixel addressed framebuffer. Also MZ-80B GRAM II memory in lower 8K
// Y+0x0D3FFF              00111111 11111111
// Y+0x0E0000   00001110 - 128K Green framebuffer.
//                         00000000 00000000 - Green pixel addressed framebuffer.
// Y+0x0E3FFF              00111111 11111111
// 

// Base addresses and sizes within the FPGA/Video Controller.
#define VIDEO_BASE_ADDR              0xD00000                            // Base address of the Video Controller.
#define Z80_BUS_BASE_ADDR            0xE00000                            // Base address of the Z80 FSM
#define VIDEO_VRAM_BASE_ADDR         VIDEO_BASE_ADDR + 0x01D000          // Base address of the character video RAM using direct addressing.
#define VIDEO_VRAM_SIZE              0x800                               // Size of the video RAM.
#define VIDEO_ARAM_BASE_ADDR         VIDEO_BASE_ADDR + 0x01D800          // Base address of the character attribute RAM using direct addressing.
#define VIDEO_ARAM_SIZE              0x800                               // Size of the attribute RAM.
#define VIDEO_IO_BASE_ADDR           VIDEO_BASE_ADDR + 0x000000
        
// Memory addresses of I/O and Memory mapped I/O in the Video Controller which are mapped to direct memory accessed addresses.
//
#define VC_8BIT_BASE_ADDR            VIDEO_BASE_ADDR + 0x000000
#define VC_32BIT_BASE_ADDR           VIDEO_BASE_ADDR + 0x000000
// 8 Bit access addresses - used for writing, read can only be on a 32bit boundary with lower address lines set to 00. Writing can write upto 4 consecutive addresses if desired.
#define VCADDR_8BIT_PALSLCTOFF       VC_8BIT_BASE_ADDR + 0xD3            // Set the palette slot Off position to be adjusted.
#define VCADDR_8BIT_PALSLCTON        VC_8BIT_BASE_ADDR + 0xD4            // Set the palette slot On position to be adjusted.
#define VCADDR_8BIT_PALSETRED        VC_8BIT_BASE_ADDR + 0xD5            // Set the red palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_PALSETGREEN      VC_8BIT_BASE_ADDR + 0xD6            // Set the green palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_PALSETBLUE       VC_8BIT_BASE_ADDR + 0xD7            // Set the blue palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_SYSCTRL          VC_8BIT_BASE_ADDR + 0xF0            // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define VCADDR_8BIT_VMBORDER         VC_8BIT_BASE_ADDR + 0xF3            // Select VGA Border colour attributes. Bit 2 = Red, 1 = Green, 0 = Blue.
#define VCADDR_8BIT_GRAMMODE         VC_8BIT_BASE_ADDR + 0xF4            // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define VCADDR_8BIT_VMPALETTE        VC_8BIT_BASE_ADDR + 0xF5            // Select Palette:
                                                                         //    0xF5 sets the palette. The Video Module supports 4 bit per colour output but there is only enough RAM for 1 bit per colour so the pallette is used to change the colours output.
                                                                         //      Bits [7:0] defines the pallete number. This indexes a lookup table which contains the required 4bit output per 1bit input.
                                                                         // GPU:
#define VCADDR_8BIT_GPUPARAM         VC_8BIT_BASE_ADDR + 0xF6            //    0xF6 set parameters. Store parameters in a long word to be used by the graphics command processor.
                                                                         //      The parameter word is 128 bit and each write to the parameter word shifts left by 8 bits and adds the new byte at bits 7:0.
#define VCADDR_8BIT_GPUCMD           VC_8BIT_BASE_ADDR + 0xF7            //    0xF7 set the graphics processor unit commands.
#define VCADDR_8BIT_GPUSTATUS        VC_8BIT_BASE_ADDR + 0xF7            //         [7;1] - FSM state, [0] - 1 = busy, 0 = idle
                                                                         //      Bits [5:0] - 0 = Reset parameters.
                                                                         //                   1 = Clear to val. Start Location (16 bit), End Location (16 bit), Red Filter, Green Filter, Blue Filter
                                                                         // 
#define VCADDR_8BIT_VMCTRL           VC_8BIT_BASE_ADDR + 0xF8            // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define VCADDR_8BIT_VMGRMODE         VC_8BIT_BASE_ADDR + 0xF9            // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define VCADDR_8BIT_VMREDMASK        VC_8BIT_BASE_ADDR + 0xFA            // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMGREENMASK      VC_8BIT_BASE_ADDR + 0xFB            // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMBLUEMASK       VC_8BIT_BASE_ADDR + 0xFC            // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMPAGE           VC_8BIT_BASE_ADDR + 0xFD            // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.
#define VCADDR_8BIT_KEYPA            VC_8BIT_BASE_ADDR + 0xE000          // VideoModule 8255 Port A
#define VCADDR_8BIT_KEYPB            VC_8BIT_BASE_ADDR + 0xE001          // VideoModule 8255 Port B
#define VCADDR_8BIT_KEYPC            VC_8BIT_BASE_ADDR + 0xE002          // VideoModule 8255 Port C
#define VCADDR_8BIT_KEYPF            VC_8BIT_BASE_ADDR + 0xE003          // VideoModule 8255 Mode Control
#define VCADDR_8BIT_CSTR             VC_8BIT_BASE_ADDR + 0xE002          // VideoModule 8255 Port C
#define VCADDR_8BIT_CSTPT            VC_8BIT_BASE_ADDR + 0xE003          // VideoModule 8255 Mode Control
#define VCADDR_8BIT_CONT0            VC_8BIT_BASE_ADDR + 0xE004          // VideoModule 8253 Counter 0
#define VCADDR_8BIT_CONT1            VC_8BIT_BASE_ADDR + 0xE005          // VideoModule 8253 Counter 1
#define VCADDR_8BIT_CONT2            VC_8BIT_BASE_ADDR + 0xE006          // VideoModule 8253 Counter 1
#define VCADDR_8BIT_CONTF            VC_8BIT_BASE_ADDR + 0xE007          // VideoModule 8253 Mode Control
#define VCADDR_8BIT_SUNDG            VC_8BIT_BASE_ADDR + 0xE008          // Register for reading the tempo timer status (cursor flash). horizontal blank and switching sound on/off.
#define VCADDR_8BIT_TEMP             VC_8BIT_BASE_ADDR + 0xE008          // As above, different name used in original source when writing.
#define VCADDR_8BIT_MEMSW            VC_8BIT_BASE_ADDR + 0xE00C          // Memory swap, 0000->C000, C000->0000
#define VCADDR_8BIT_MEMSWR           VC_8BIT_BASE_ADDR + 0xE010          // Reset memory swap.
#define VCADDR_8BIT_INVDSP           VC_8BIT_BASE_ADDR + 0xE014          // Invert display.
#define VCADDR_8BIT_NRMDSP           VC_8BIT_BASE_ADDR + 0xE015          // Return display to normal.
#define VCADDR_8BIT_SCLDSP           VC_8BIT_BASE_ADDR + 0xE200          // Hardware scroll, a read to each location adds 8 to the start of the video access address therefore creating hardware scroll. 00 - reset to power up
#define VCADDR_8BIT_SCLBASE          VC_8BIT_BASE_ADDR + 0xE2            // High byte scroll base.
      
// 32 Bit access addresses for 8bit registers - used for reading, address is shifted right by 2 and the resulting byte read into bits 7:0, 31:8 are zero.
#define VCADDR_32BIT_PALSLCTOFF      VC_32BIT_BASE_ADDR + (4*0xD3)       // Set the palette slot Off position to be adjusted.
#define VCADDR_32BIT_PALSLCTON       VC_32BIT_BASE_ADDR + (4*0xD4)       // Set the palette slot On position to be adjusted.
#define VCADDR_32BIT_PALSETRED       VC_32BIT_BASE_ADDR + (4*0xD5)       // Set the red palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_32BIT_PALSETGREEN     VC_32BIT_BASE_ADDR + (4*0xD6)       // Set the green palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_32BIT_PALSETBLUE      VC_32BIT_BASE_ADDR + (4*0xD7)       // Set the blue palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_32BIT_SYSCTRL         VC_32BIT_BASE_ADDR + (4*0xF0)       // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define VCADDR_32BIT_VMBORDER        VC_32BIT_BASE_ADDR + (4*0xF3)       // Select VGA Border colour attributes. Bit 2 = Red, 1 = Green, 0 = Blue.
#define VCADDR_32BIT_GRAMMODE        VC_32BIT_BASE_ADDR + (4*0xF4)       // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define VCADDR_32BIT_VMPALETTE       VC_32BIT_BASE_ADDR + (4*0xF5)       // Select Palette:
                                                                         //    0xF5 sets the palette. The Video Module supports 4 bit per colour output but there is only enough RAM for 1 bit per colour so the pallette is used to change the colours output.
                                                                         //      Bits [7:0] defines the pallete number. This indexes a lookup table which contains the required 4bit output per 1bit input.
                                                                         // GPU:
#define VCADDR_32BIT_GPUPARAM        VC_32BIT_BASE_ADDR + (4*0xF6)       //    0xF6 set parameters. Store parameters in a long word to be used by the graphics command processor.
                                                                         //      The parameter word is 128 bit and each write to the parameter word shifts left by 8 bits and adds the new byte at bits 7:0.
#define VCADDR_32BIT_GPUCMD          VC_32BIT_BASE_ADDR + (4*0xF7)       //    0xF7 set the graphics processor unit commands.
#define VCADDR_32BIT_GPUSTATUS       VC_32BIT_BASE_ADDR + (4*0xF7)       //         [7;1] - FSM state, [0] - 1 = busy, 0 = idle
                                                                         //      Bits [5:0] - 0 = Reset parameters.
                                                                         //                   1 = Clear to val. Start Location (16 bit), End Location (16 bit), Red Filter, Green Filter, Blue Filter
                                                                         // 
#define VCADDR_32BIT_VMCTRL          VC_32BIT_BASE_ADDR + (4*0xF8)       // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
                                                                         //                                [4] defines the colour mode, 0 = mono, 1 = colour - ignored on certain modes. [5] defines wether PCGRAM is enabled, 0 = disabled, 1 = enabled. [7:6] define the VGA mode.
#define VCADDR_32BIT_VMGRMODE        VC_32BIT_BASE_ADDR + (4*0xF9)       // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define VCADDR_32BIT_VMREDMASK       VC_32BIT_BASE_ADDR + (4*0xFA)       // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_32BIT_VMGREENMASK     VC_32BIT_BASE_ADDR + (4*0xFB)       // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_32BIT_VMBLUEMASK      VC_32BIT_BASE_ADDR + (4*0xFC)       // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_32BIT_VMPAGE          VC_32BIT_BASE_ADDR + (4*0xFD)       // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.
#define VCADDR_32BIT_KEYPA           VC_32BIT_BASE_ADDR + (4*0xE000)     // Video Module 8255 Port A
#define VCADDR_32BIT_KEYPB           VC_32BIT_BASE_ADDR + (4*0xE001)     // Video Module 8255 Port B
#define VCADDR_32BIT_KEYPC           VC_32BIT_BASE_ADDR + (4*0xE002)     // Video Module 8255 Port C
#define VCADDR_32BIT_KEYPF           VC_32BIT_BASE_ADDR + (4*0xE003)     // Video Module 8255 Mode Control
#define VCADDR_32BIT_CSTR            VC_32BIT_BASE_ADDR + (4*0xE002)     // Video Module 8255 Port C
#define VCADDR_32BIT_CSTPT           VC_32BIT_BASE_ADDR + (4*0xE003)     // Video Module 8255 Mode Control
#define VCADDR_32BIT_CONT0           VC_32BIT_BASE_ADDR + (4*0xE004)     // Video Module 8253 Counter 0
#define VCADDR_32BIT_CONT1           VC_32BIT_BASE_ADDR + (4*0xE005)     // Video Module 8253 Counter 1
#define VCADDR_32BIT_CONT2           VC_32BIT_BASE_ADDR + (4*0xE006)     // Video Module 8253 Counter 1
#define VCADDR_32BIT_CONTF           VC_32BIT_BASE_ADDR + (4*0xE007)     // Video Module 8253 Mode Control
#define VCADDR_32BIT_SUNDG           VC_32BIT_BASE_ADDR + (4*0xE008)     // Register for reading the tempo timer status (cursor flash). horizontal blank and switching sound on/off.
#define VCADDR_32BIT_TEMP            VC_32BIT_BASE_ADDR + (4*0xE008)     // As above, different name used in original source when writing.
#define VCADDR_32BIT_MEMSW           VC_32BIT_BASE_ADDR + (4*0xE00C)     // Memory swap, 0000->C000, C000->0000
#define VCADDR_32BIT_MEMSWR          VC_32BIT_BASE_ADDR + (4*0xE010)     // Reset memory swap.
#define VCADDR_32BIT_INVDSP          VC_32BIT_BASE_ADDR + (4*0xE014)     // Invert display.
#define VCADDR_32BIT_NRMDSP          VC_32BIT_BASE_ADDR + (4*0xE015)     // Return display to normal.
#define VCADDR_32BIT_SCLDSP          VC_32BIT_BASE_ADDR + (4*0xE200)     // Hardware scroll, a read to each location adds 8 to the start of the video access address therefore creating hardware scroll. 00 - reset to power up
#define VCADDR_32BIT_SCLBASE         VC_32BIT_BASE_ADDR + (4*0xE2)       // High byte scroll base.     

// Memory mapped I/O on the mainboard. These addresses are processed by the Z80BUS FSM which converts a 32bit ZPU cycle into several 8bit Z80 cycles.
//
#define MB_8BIT_BASE_ADDR            Z80_BUS_BASE_ADDR + 0x000000
#define MB_32BIT_BASE_ADDR           Z80_BUS_BASE_ADDR + 0x080000
#define MB_32BIT_IO_ADDR             Z80_BUS_BASE_ADDR + 0x040000

// 8 Bit access addresses - used for writing and reading on a 32bit boundary with lower address lines set to 00. Writing is 1 byte only.
#define MBADDR_8BIT_KEYPA            MB_8BIT_BASE_ADDR + (4*0xE000)      // Mainboard 8255 Port A
#define MBADDR_8BIT_KEYPB            MB_8BIT_BASE_ADDR + (4*0xE001)      // Mainboard 8255 Port B
#define MBADDR_8BIT_KEYPC            MB_8BIT_BASE_ADDR + (4*0xE002)      // Mainboard 8255 Port C
#define MBADDR_8BIT_KEYPF            MB_8BIT_BASE_ADDR + (4*0xE003)      // Mainboard 8255 Mode Control
#define MBADDR_8BIT_CSTR             MB_8BIT_BASE_ADDR + (4*0xE002)      // Mainboard 8255 Port C
#define MBADDR_8BIT_CSTPT            MB_8BIT_BASE_ADDR + (4*0xE003)      // Mainboard 8255 Mode Control
#define MBADDR_8BIT_CONT0            MB_8BIT_BASE_ADDR + (4*0xE004)      // Mainboard 8253 Counter 0
#define MBADDR_8BIT_CONT1            MB_8BIT_BASE_ADDR + (4*0xE005)      // Mainboard 8253 Counter 1
#define MBADDR_8BIT_CONT2            MB_8BIT_BASE_ADDR + (4*0xE006)      // Mainboard 8253 Counter 1
#define MBADDR_8BIT_CONTF            MB_8BIT_BASE_ADDR + (4*0xE007)      // Mainboard 8253 Mode Control
#define MBADDR_8BIT_SUNDG            MB_8BIT_BASE_ADDR + (4*0xE008)      // Register for reading the tempo timer status (cursor flash). horizontal blank and switching sound on/off.
#define MBADDR_8BIT_TEMP             MB_8BIT_BASE_ADDR + (4*0xE008)      // As above, different name used in original source when writing.
#define MBADDR_8BIT_MEMSW            MB_8BIT_BASE_ADDR + (4*0xE00C)      // Memory swap, 0000->C000, C000->0000
#define MBADDR_8BIT_MEMSWR           MB_8BIT_BASE_ADDR + (4*0xE010)      // Reset memory swap.
#define MBADDR_8BIT_INVDSP           MB_8BIT_BASE_ADDR + (4*0xE014)      // Invert display.
#define MBADDR_8BIT_NRMDSP           MB_8BIT_BASE_ADDR + (4*0xE015)      // Return display to normal.
#define MBADDR_8BIT_SCLDSP           MB_8BIT_BASE_ADDR + (4*0xE200)      // Hardware scroll, a read to each location adds 8 to the start of the video access address therefore creating hardware scroll. 00 - reset to power up
#define MBADDR_8BIT_SCLBASE          MB_8BIT_BASE_ADDR + (4*0xE2)        // High byte scroll base.     

// 32 Bit access addresses - used for reading and writing, read and write can only be 1 byte to 1 address.
#define MBADDR_32BIT_KEYPA           MB_32BIT_BASE_ADDR + (4*0xE000)     // Mainboard 8255 Port A
#define MBADDR_32BIT_KEYPB           MB_32BIT_BASE_ADDR + (4*0xE001)     // Mainboard 8255 Port B
#define MBADDR_32BIT_KEYPC           MB_32BIT_BASE_ADDR + (4*0xE002)     // Mainboard 8255 Port C
#define MBADDR_32BIT_KEYPF           MB_32BIT_BASE_ADDR + (4*0xE003)     // Mainboard 8255 Mode Control
#define MBADDR_32BIT_CSTR            MB_32BIT_BASE_ADDR + (4*0xE002)     // Mainboard 8255 Port C
#define MBADDR_32BIT_CSTPT           MB_32BIT_BASE_ADDR + (4*0xE003)     // Mainboard 8255 Mode Control
#define MBADDR_32BIT_CONT0           MB_32BIT_BASE_ADDR + (4*0xE004)     // Mainboard 8253 Counter 0
#define MBADDR_32BIT_CONT1           MB_32BIT_BASE_ADDR + (4*0xE005)     // Mainboard 8253 Counter 1
#define MBADDR_32BIT_CONT2           MB_32BIT_BASE_ADDR + (4*0xE006)     // Mainboard 8253 Counter 1
#define MBADDR_32BIT_CONTF           MB_32BIT_BASE_ADDR + (4*0xE007)     // Mainboard 8253 Mode Control
#define MBADDR_32BIT_SUNDG           MB_32BIT_BASE_ADDR + (4*0xE008)     // Register for reading the tempo timer status (cursor flash). horizontal blank and switching sound on/off.
#define MBADDR_32BIT_TEMP            MB_32BIT_BASE_ADDR + (4*0xE008)     // As above, different name used in original source when writing.
#define MBADDR_32BIT_MEMSW           MB_32BIT_BASE_ADDR + (4*0xE00C)     // Memory swap, 0000->C000, C000->0000
#define MBADDR_32BIT_MEMSWR          MB_32BIT_BASE_ADDR + (4*0xE010)     // Reset memory swap.
#define MBADDR_32BIT_INVDSP          MB_32BIT_BASE_ADDR + (4*0xE014)     // Invert display.
#define MBADDR_32BIT_NRMDSP          MB_32BIT_BASE_ADDR + (4*0xE015)     // Return display to normal.
#define MBADDR_32BIT_SCLDSP          MB_32BIT_BASE_ADDR + (4*0xE200)     // Hardware scroll, a read to each location adds 8 to the start of the video access address therefore creating hardware scroll. 00 - reset to power up
#define MBADDR_32BIT_SCLBASE         MB_32BIT_BASE_ADDR + (4*0xE2)       // High byte scroll base.     

// Z80 I/O addresses - mapped into the ZPU direct addressable memory space, 4 bytes = 1 byte in the Z80 I/O range.
#define MBADDR_8BIT_IOW_CTRLLATCH    MB_32BIT_IO_ADDR + 0x60             // Control latch which specifies the Memory Model/mode.
#define MBADDR_8BIT_IOW_SETXMHZ      MB_32BIT_IO_ADDR + 0x62             // Switch to alternate CPU frequency provided by K64F.
#define MBADDR_8BIT_IOW_SET2MHZ      MB_32BIT_IO_ADDR + 0x64             // Switch to system CPU frequency.
#define MBADDR_8BIT_IOW_CLKSELRD     MB_32BIT_IO_ADDR + 0x66             // Read the status of the clock select, ie. which clock is connected to the CPU.
#define MBADDR_8BIT_IOW_SVCREQ       MB_32BIT_IO_ADDR + 0x68             // Service request from the Z80 to be provided by the K64F.
#define MBADDR_8BIT_IOW_SYSREQ       MB_32BIT_IO_ADDR + 0x6A             // System request from the Z80 to be provided by the K64F.
#define MBADDR_8BIT_IOW_CPUCFG       MB_32BIT_IO_ADDR + 0x6C             // Version 2.2 CPU configuration register.
#define MBADDR_8BIT_IOW_CPUSTATUS    MB_32BIT_IO_ADDR + 0x6C             // Version 2.2 CPU runtime status register.
#define MBADDR_8BIT_IOW_CPUINFO      MB_32BIT_IO_ADDR + 0x6D             // Version 2.2 CPU information register.
#define MBADDR_8BIT_IOW_CPLDCFG      MB_32BIT_IO_ADDR + 0x6E             // Version 2.1 CPLD configuration register.
#define MBADDR_8BIT_IOW_CPLDSTATUS   MB_32BIT_IO_ADDR + 0x6E             // Version 2.1 CPLD status register.
#define MBADDR_8BIT_IOW_CPLDINFO     MB_32BIT_IO_ADDR + 0x6F             // Version 2.1 CPLD version information register.
#define MBADDR_8BIT_IOW_SYSCTRL      MB_32BIT_IO_ADDR + 0xF0             // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define MBADDR_8BIT_IOW_GRAMMODE     MB_32BIT_IO_ADDR + 0xF4             // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define MBADDR_8BIT_IOW_VMCTRL       MB_32BIT_IO_ADDR + 0xF8             // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define MBADDR_8BIT_IOW_VMGRMODE     MB_32BIT_IO_ADDR + 0xF9             // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define MBADDR_8BIT_IOW_VMREDMASK    MB_32BIT_IO_ADDR + 0xFA             // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_8BIT_IOW_VMGREENMASK  MB_32BIT_IO_ADDR + 0xFB             // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_8BIT_IOW_VMBLUEMASK   MB_32BIT_IO_ADDR + 0xFC             // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_8BIT_IOW_VMPAGE       MB_32BIT_IO_ADDR + 0xFD             // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.

#define MBADDR_32BIT_IOR_CTRLLATCH   MB_32BIT_IO_ADDR + (4*0x60)         // Control latch which specifies the Memory Model/mode.
#define MBADDR_32BIT_IOR_SETXMHZ     MB_32BIT_IO_ADDR + (4*0x62)         // Switch to alternate CPU frequency provided by K64F.
#define MBADDR_32BIT_IOR_SET2MHZ     MB_32BIT_IO_ADDR + (4*0x64)         // Switch to system CPU frequency.
#define MBADDR_32BIT_IOR_CLKSELRD    MB_32BIT_IO_ADDR + (4*0x66)         // Read the status of the clock select, ie. which clock is connected to the CPU.
#define MBADDR_32BIT_IOR_SVCREQ      MB_32BIT_IO_ADDR + (4*0x68)         // Service request from the Z80 to be provided by the K64F.
#define MBADDR_32BIT_IOR_SYSREQ      MB_32BIT_IO_ADDR + (4*0x6A)         // System request from the Z80 to be provided by the K64F.
#define MBADDR_32BIT_IOR_CPUCFG      MB_32BIT_IO_ADDR + (4*0x6C)         // Version 2.2 CPU configuration register.
#define MBADDR_32BIT_IOR_CPUSTATUS   MB_32BIT_IO_ADDR + (4*0x6C)         // Version 2.2 CPU runtime status register.
#define MBADDR_32BIT_IOR_CPUINFO     MB_32BIT_IO_ADDR + (4*0x6D)         // Version 2.2 CPU information register.
#define MBADDR_32BIT_IOR_CPLDCFG     MB_32BIT_IO_ADDR + (4*0x6E)         // Version 2.1 CPLD configuration register.
#define MBADDR_32BIT_IOR_CPLDSTATUS  MB_32BIT_IO_ADDR + (4*0x6E)         // Version 2.1 CPLD status register.
#define MBADDR_32BIT_IOR_CPLDINFO    MB_32BIT_IO_ADDR + (4*0x6F)         // Version 2.1 CPLD version information register.
#define MBADDR_32BIT_IOR_SYSCTRL     MB_32BIT_IO_ADDR + (4*0xF0)         // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define MBADDR_32BIT_IOR_GRAMMODE    MB_32BIT_IO_ADDR + (4*0xF4)         // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define MBADDR_32BIT_IOR_VMCTRL      MB_32BIT_IO_ADDR + (4*0xF8)         // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define MBADDR_32BIT_IOR_VMGRMODE    MB_32BIT_IO_ADDR + (4*0xF9)         // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define MBADDR_32BIT_IOR_VMREDMASK   MB_32BIT_IO_ADDR + (4*0xFA)         // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_32BIT_IOR_VMGREENMASK MB_32BIT_IO_ADDR + (4*0xFB)         // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_32BIT_IOR_VMBLUEMASK  MB_32BIT_IO_ADDR + (4*0xFC)         // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define MBADDR_32BIT_IOR_VMPAGE      MB_32BIT_IO_ADDR + (4*0xFD)         // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.




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

// Video Module control bits.
#define SYSMODE_MZ80A                0x00                                // System board mode MZ80A, 2MHz CPU/Bus.
#define SYSMODE_MZ80B                0x01                                // System board mode MZ80B, 4MHz CPU/Bus.
#define SYSMODE_MZ700                0x02                                // System board mode MZ700, 3.54MHz CPU/Bus.
#define VMMODE_MASK                  0xF8                                // Mask to mask out video mode.
#define VMMODE_MZ80K                 0x00                                // Video mode = MZ80K
#define VMMODE_MZ80C                 0x01                                // Video mode = MZ80C
#define VMMODE_MZ1200                0x02                                // Video mode = MZ1200
#define VMMODE_MZ80A                 0x03                                // Video mode = MZ80A
#define VMMODE_MZ700                 0x04                                // Video mode = MZ700
#define VMMODE_MZ800                 0x05                                // Video mode = MZ800
#define VMMODE_MZ80B                 0x06                                // Video mode = MZ80B
#define VMMODE_MZ2000                0x07                                // Video mode = MZ2000
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
#define TZSVC_CMD_STRUCT_ADDR_ZOS    0x1FD80 // Z80_BUS_BASE_ADDR + 0x7FD80         // Address of the command structure for zOS use.
#define TZSVC_CMD_STRUCT_SIZE        0x280                               // Size of the inter z80/K64 service command memory.
#define TZSVC_CMD_SIZE               (sizeof(t_svcControl)-TZSVC_SECTOR_SIZE)
#define TZVC_MAX_CMPCT_DIRENT_BLOCK  TZSVC_SECTOR_SIZE/TZSVC_CMPHDR_SIZE // Maximum number of directory entries per sector.
#define TZSVC_MAX_DIR_ENTRIES        255                                 // Maximum number of files in one directory, any more than this will be ignored.
#define TZSVC_CMPHDR_SIZE            32                                  // Compacted header size, contains everything except the comment field, padded out to 32bytes.
#define MZF_FILLER_LEN               8                                   // Filler to pad a compacted header entry to a power of 2 length.
#define TZSVC_RETRY_COUNT            5                                   // Number of times to retry a service request on failure.
#define TZSVC_TIMEOUT                10000                               // Time period in milliseconds to wait for a service request to complete, expiry indicates failure.
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


//Common character definitions.
#define SCROLL                       0x01                                // Set scroll direction UP.
#define BELL                         0x07
#define SPACE                        0x20
#define TAB                          0x09                                // TAB ACROSS (8 SPACES FOR SD-BOARD)
#define CR                           0x0D
#define LF                           0x0A
#define FF                           0x0C
#define DELETE                       0x7F
#define BACKS                        0x08
#define SOH                          0x01                                // For XModem etc.
#define EOT                          0x04
#define ACK                          0x06
#define NAK                          0x15
#define NUL                          0x00
//#define NULL                         0x00
#define CTRL_A                       0x01
#define CTRL_B                       0x02
#define CTRL_C                       0x03
#define CTRL_D                       0x04
#define CTRL_E                       0x05
#define CTRL_F                       0x06
#define CTRL_G                       0x07
#define CTRL_H                       0x08
#define CTRL_I                       0x09
#define CTRL_J                       0x0A
#define CTRL_K                       0x0B
#define CTRL_L                       0x0C
#define CTRL_M                       0x0D
#define CTRL_N                       0x0E
#define CTRL_O                       0x0F
#define CTRL_P                       0x10
#define CTRL_Q                       0x11
#define CTRL_R                       0x12
#define CTRL_S                       0x13
#define CTRL_T                       0x14
#define CTRL_U                       0x15
#define CTRL_V                       0x16
#define CTRL_W                       0x17
#define CTRL_X                       0x18
#define CTRL_Y                       0x19
#define CTRL_Z                       0x1A
#define ESC                          0x1B
#define CTRL_SLASH                   0x1C
#define CTRL_LB                      0x1B
#define CTRL_RB                      0x1D
#define CTRL_CAPPA                   0x1E
#define CTRL_UNDSCR                  0x1F
#define CTRL_AT                      0x00
#define FUNC1                        0x80
#define FUNC2                        0x81
#define FUNC3                        0x82
#define FUNC4                        0x83
#define FUNC5                        0x84
#define FUNC6                        0x85
#define FUNC7                        0x86
#define FUNC8                        0x87
#define FUNC9                        0x88
#define FUNC10                       0x89
#define PAGEUP                       0xE0
#define PAGEDOWN                     0xE1
#define CURHOMEKEY                   0xE2
#define NOKEY                        0xF0
#define CURSRIGHT                    0xF1
#define CURSLEFT                     0xF2
#define CURSUP                       0xF3
#define CURSDOWN                     0xF4
#define DBLZERO                      0xF5
#define INSERT                       0xF6
#define CLRKEY                       0xF7
#define HOMEKEY                      0xF8
#define ENDKEY                       0xF9
#define ANSITGLKEY                   0xFA
#define BREAKKEY                     0xFB
#define GRAPHKEY                     0xFC
#define ALPHAKEY                     0xFD
#define DEBUGKEY                     0xFE                                // Special key to enable debug features such as the ANSI emulation.

// Keyboard constants.
#define KEYB_AUTOREPEAT_INITIAL_TIME 1000                                // Time in milliseconds before starting autorepeat.
#define KEYB_AUTOREPEAT_TIME         250                                 // Time in milliseconds between auto repeating characters.
#define KEYB_FLASH_TIME              500                                 // Time in milliseconds for the cursor flash change.
#define CURSOR_THICK_BLOCK           0x43                                // Thick block cursor for lower case CAPS OFF
#define CURSOR_BLOCK                 0xEF                                // Block cursor for SHIFT Lock.
#define CURSOR_UNDERLINE             0x3E                                // Thick underscore for CAPS Lock.
#define MAX_KEYB_BUFFER_SIZE         32                                  // Maximum size of the keyboard buffer.

// Macros.
//
// Convert big endiam to little endian.
#define convBigToLittleEndian(num)   ((num>>24)&0xff) | ((num<<8)&0xff0000) | ((num>>8)&0xff00) | ((num<<24)&0xff000000)

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

// Cursor flash mechanism control states.
//
enum CURSOR_STATES {
    CURSOR_OFF                       = 0x00,                             // Turn the cursor off.
    CURSOR_ON                        = 0x01,                             // Turn the cursor on.
    CURSOR_RESTORE                   = 0x02,                             // Restore the saved cursor character.
    CURSOR_FLASH                     = 0x03                              // If enabled, flash the cursor.
};

// Cursor positioning states.
enum CURSOR_POSITION {
    CURSOR_UP                        = 0x00,                             // Move the cursor up.
    CURSOR_DOWN                      = 0x01,                             // Move the cursor down.
    CURSOR_LEFT                      = 0x02,                             // Move the cursor left.
    CURSOR_RIGHT                     = 0x03,                             // Move the cursor right.
    CURSOR_COLUMN                    = 0x04,                             // Set cursor column to absolute value.
    CURSOR_NEXT_LINE                 = 0x05,                             // Move the cursor to the beginning of the next line.
    CURSOR_PREV_LINE                 = 0x06,                             // Move the cursor to the beginning of the previous line.
};

// Keyboard operating states according to buttons pressed.
//
enum KEYBOARD_MODES {
    KEYB_LOWERCASE                   = 0x00,                             // Keyboard in lower case mode.
    KEYB_CAPSLOCK                    = 0x01,                             // Keyboard in CAPS lock mode.
    KEYB_SHIFTLOCK                   = 0x02,                             // Keyboard in SHIFT lock mode.
    KEYB_CTRL                        = 0x03,                             // Keyboard in Control mode.
    KEYB_GRAPHMODE                   = 0x04                              // Keyboard in Graphics mode.
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
        uint8_t                      vDriveNo;                           // Virtual or physical SD card drive number.
    };
    union {
        struct {
            uint16_t                 trackNo;                            // For virtual drives with track and sector this is the track number
            uint16_t                 sectorNo;                           // For virtual drives with track and sector this is the sector number. NB For LBA access, this is 32bit and overwrites fileNo/fileType which arent used during raw SD access.
        };
        uint32_t                     sectorLBA;                          // For LBA access, this is 32bit and used during raw SD access.
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

// Mapping table from Ascii to Sharp MZ display code.
//
typedef struct {
    uint8_t                          dispCode;
} t_dispCodeMap;

// Mapping table from keyboard scan codes to Sharp MZ-700 keys.
//
typedef struct {
    uint8_t                          scanCode[80];
} t_scanCodeMap;

// Mapping table of a sharp keycode to an ANSI escape sequence string.
//
typedef struct {
    uint8_t                          key;
    const char*                      ansiKeySequence;
} t_ansiKeyMap;

// Structure to maintain the Sharp MZ display output parameters and data.
//
typedef struct {
    uint8_t                          screenAttr;
    uint16_t                         screenRow;

    // Location on the physical screen to output data. displayCol is also used in the backing store.
    uint8_t                          displayRow;
    uint8_t                          displayCol;

    // History and backing screen store. The physical display outputs a portion of this backing store.
    uint8_t                          screenCharBuf[VC_DISPLAY_BUFFER_SIZE];
    uint8_t                          screenAttrBuf[VC_DISPLAY_BUFFER_SIZE];

    // Maxims, dynamic to allow for future changes.
    uint8_t                          maxScreenRow;
    uint8_t                          maxDisplayRow;
    uint8_t                          maxScreenCol;

    // Features.
    uint8_t                          lineWrap;                           // Wrap line at screen edge (1) else stop printing at screen edge.
    uint8_t                          useAnsiTerm;                        // Enable (1) Ansi Terminal Emulator, (0) disable.
    uint8_t                          debug;                              // Enable debugging features.
    uint8_t                          inDebug;                            // Prevent recursion when outputting debug information.
} t_displayBuffer;

// Structure for maintaining the Sharp MZ keyboard parameters and data. Used to retrieve and map a key along with associated
// attributes such as cursor flashing.
//
typedef struct {
    uint8_t                          scanbuf[2][10];
    uint8_t                          keydown[10];
    uint8_t                          keyup[10];
    uint8_t                          keyhold[10];
    uint32_t                         holdTimer;
    uint8_t                          breakKey;                           // Break key pressed.
    uint8_t                          ctrlKey;                            // Ctrl key pressed.
    uint8_t                          shiftKey;                           // Shift key pressed.
    uint8_t                          repeatKey;
    uint8_t                          autorepeat;
    enum KEYBOARD_MODES              mode;
    uint8_t                          keyBuf[MAX_KEYB_BUFFER_SIZE];       // Keyboard buffer.
    uint8_t                          keyBufPtr;                          // Pointer into the keyboard buffer for stored key,
    uint8_t                          cursorOn;                           // Flag to indicate Cursor is switched on.
    uint8_t                          displayCursor;                      // Cursor being displayed = 1
    uint32_t                         flashTimer;                         // Timer to indicate next flash time for cursor.
} t_keyboard;

// Structure to maintain the Ansi Terminal Emulator state and parameters.
//
typedef struct {
	enum {
		ANSITERM_ESC,
		ANSITERM_BRACKET,
		ANSITERM_PARSE,
	}                                state;                              // States and current state of the FSM parser.
    uint8_t                          charcnt;                            // Number of characters read into the buffer.
    uint8_t                          paramcnt;                           // Number of parameters parsed and stored.
    uint8_t                          setScreenMode;                      // Screen mode command detected.
    uint8_t                          setExtendedMode;                    // Extended mode command detected.
    uint8_t                          charbuf[80];                        // Storage for the parameter characters as they are received.
    uint16_t                         param[10];                          // Parsed paraemters.
    uint8_t                          saveRow;                            // Store the current row when requested.
    uint8_t                          saveCol;                            // Store the current column when requested.
    uint8_t                          saveScreenRow;                      // Store the current screen buffer row when requested.
} t_AnsiTerm;

// Application execution constants.
//

// Prototypes.
//
uint8_t                              mzInitMBHardware(void);
uint8_t                              mzInit(void);
uint8_t                              mzMoveCursor(enum CURSOR_POSITION, uint8_t);
uint8_t                              mzSetCursor(uint8_t, uint8_t);    
int                                  mzPutChar(char, FILE *);
int                                  mzPutRaw(char);
uint8_t                              mzSetAnsiAttribute(uint8_t);
int                                  mzAnsiTerm(char);
int                                  mzPrintChar(char, FILE *);
uint8_t                              mzFlashCursor(enum CURSOR_STATES);
uint8_t                              mzPushKey(char *);
int                                  mzGetKey(uint8_t);
int                                  mzGetChar(FILE *);
void                                 mzClearScreen(uint8_t, uint8_t);
void                                 mzClearLine(int, int, int, uint8_t);
uint8_t                              mzGetScreenWidth(void);
uint8_t                              mzSetScreenWidth(uint8_t);
uint8_t                              mzSetMachineVideoMode(uint8_t);
uint8_t                              mzSetVGAMode(uint8_t);
uint8_t                              mzSetVGABorder(uint8_t);
void                                 mzRefreshScreen(void);
uint8_t                              mzScrollUp(uint8_t, uint8_t);
uint8_t                              mzScrollDown(uint8_t);
void                                 mzDebugOut(uint8_t, uint8_t);
int                                  mzGetTest();
void                                 mzSetZ80(void);
int                                  mzServiceCall(uint8_t);
int                                  mzSDGetStatus(uint32_t, uint8_t);
int                                  mzSDServiceCall(uint8_t, uint8_t);
uint8_t                              mzSDInit(uint8_t);
uint8_t                              mzSDRead(uint8_t, uint32_t, uint32_t);
uint8_t                              mzSDWrite(uint8_t, uint32_t, uint32_t);    
void                                 testRoutine(void);

// Getter/Setter methods!

#ifdef __cplusplus
}
#endif
#endif // SHARPMZ_H
