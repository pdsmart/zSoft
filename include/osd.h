/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            osd.h
// Created:         May 2021
// Version:         v1.0
// Author(s):       Philip Smart
// Description:     The On Screen Display library.
//                  This source file contains the On Screen Display Class definitions and methods.
//                  The OSD is a popup area on the video controller which can be used to display
//                  text/menus and accept user input. Notably this class is intended to be instantiated
//                  inside an I/O processor onboard the FPGA hosting the Sharp MZ Series emulation
//                  and provide a user interface in order to configure/interact with the emulation.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2020 - Initial write of the OSD software.
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
#ifndef OSD_H
#define OSD_H

#ifdef __cplusplus
    extern "C" {
#endif

// Video display constants.
#define VC_STATUS_MAX_X_PIXELS       640                                 // Maximum number of X pixels in the Status window.
#define VC_STATUS_MAX_Y_PIXELS       80                                  // Maximum number of Y pixels in the Status window.
#define VC_STATUS_RGB_BITS           3                                   // Number of colour bits in the Status window. (/3 per colour).
#define VC_MENU_MAX_X_PIXELS         512                                 // Maximum number of X pixels in the Menu window.
#define VC_MENU_MAX_Y_PIXELS         128                                 // Maximum number of Y pixels in the Menu window.
#define VC_MENU_RGB_BITS             3                                   // Number of colour bits in the Menu window. (/3 per colour).
#define VC_STATUS_BUFFER_SIZE        (VC_STATUS_MAX_X_PIXELS * VC_STATUS_MAX_Y_PIXELS) / 8
#define VC_MENU_BUFFER_SIZE          (VC_MENU_MAX_X_PIXELS * VC_MENU_MAX_Y_PIXELS) / 8
#define VC_OSD_X_CORRECTION          1                                   // Correction factor to be applied to horizontal to compensate for pixel multiplication.
#define VC_OSD_Y_CORRECTION          2                                   // Correction factor to be applied to vertical to compensate for pixel multiplication.

// Base addresses and sizes within the FPGA/Video Controller.
#define VIDEO_BASE_ADDR              0x200000                            // Base address of the Video Controller.
#define VIDEO_VRAM_BASE_ADDR         VIDEO_BASE_ADDR + 0x01D000          // Base address of the character video RAM using direct addressing.
#define VIDEO_VRAM_SIZE              0x800                               // Size of the video RAM.
#define VIDEO_ARAM_BASE_ADDR         VIDEO_BASE_ADDR + 0x01D800          // Base address of the character attribute RAM using direct addressing.
#define VIDEO_ARAM_SIZE              0x800                               // Size of the attribute RAM.
#define VIDEO_IO_BASE_ADDR           VIDEO_BASE_ADDR + 0x000000
#define MZ_EMU_BASE_ADDR             0x300000                            // Base address of the Sharp MZ Series Emulator control registers/memory.
#define VIDEO_OSD_BLUE_ADDR          0x270000                            // Base address of the OSD Menu/Status buffer, Blue colour.
#define VIDEO_OSD_RED_ADDR           0x280000                            // Base address of the OSD Menu/Status buffer, Red colour.
#define VIDEO_OSD_GREEN_ADDR         0x290000                            // Base address of the OSD Menu/Status buffer, Green colour.
#define VIDEO_OSD_WHITE_ADDR         0x2a0000                            // Base address of the OSD Menu/Status buffer, all colours (White).
        
// Memory addresses of I/O and Memory mapped I/O in the Video Controller which are mapped to direct memory accessed addresses.
//
#define VC_8BIT_BASE_ADDR            VIDEO_BASE_ADDR + 0x000000
#define VC_32BIT_BASE_ADDR           VIDEO_BASE_ADDR + 0x000000
// 8 Bit access addresses - used for writing, read can only be on a 32bit boundary with lower address lines set to 00. Writing can write upto 4 consecutive addresses if desired.
#define VCADDR_8BIT_PALSLCTOFF       VC_8BIT_BASE_ADDR + 0xA3            // Set the palette slot Off position to be adjusted.
#define VCADDR_8BIT_PALSLCTON        VC_8BIT_BASE_ADDR + 0xA4            // Set the palette slot On position to be adjusted.
#define VCADDR_8BIT_PALSETRED        VC_8BIT_BASE_ADDR + 0xA5            // Set the red palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_PALSETGREEN      VC_8BIT_BASE_ADDR + 0xA6            // Set the green palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_PALSETBLUE       VC_8BIT_BASE_ADDR + 0xA7            // Set the blue palette value according to the PALETTE_PARAM_SEL address.
#define VCADDR_8BIT_OSDMNU_SZX       VC_8BIT_BASE_ADDR + 0xA8            // Get OSD Menu Horizontal Size (X).
#define VCADDR_8BIT_OSDMNU_SZY       VC_8BIT_BASE_ADDR + 0xA9            // Get OSD Menu Vertical Size (Y).
#define VCADDR_8BIT_OSDHDR_SZX       VC_8BIT_BASE_ADDR + 0xAA            // Get OSD Status Header Horizontal Size (X).
#define VCADDR_8BIT_OSDHDR_SZY       VC_8BIT_BASE_ADDR + 0xAB            // Get OSD Status Header Vertical Size (Y).
#define VCADDR_8BIT_OSDFTR_SZX       VC_8BIT_BASE_ADDR + 0xAC            // Get OSD Status Footer Horizontal Size (X).
#define VCADDR_8BIT_OSDFTR_SZY       VC_8BIT_BASE_ADDR + 0xAD            // Get OSD Status Footer Vertical Size (Y).   
#define VCADDR_8BIT_VMPALETTE        VC_8BIT_BASE_ADDR + 0xB0            // Sets the palette. The Video Module supports 4 bit per colour output but there is only enough RAM for 1 bit per colour so the pallette is used to change the colours output.
                                                                         //    Bits [7:0] defines the pallete number. This indexes a lookup table which contains the required 4bit output per 1bit input.
#define VCADDR_8BIT_GPUPARAM         VC_8BIT_BASE_ADDR + 0xB2            // Set parameters. Store parameters in a long word to be used by the graphics command processor.
                                                                         //    The parameter word is 128 bit and each write to the parameter word shifts left by 8 bits and adds the new byte at bits 7:0.
#define VCADDR_8BIT_GPUCMD           VC_8BIT_BASE_ADDR + 0xB3            // Set the graphics processor unit commands.
                                                                         //    Bits [5:0] - 0 = Reset parameters.
                                                                         //                 1 = Clear to val. Start Location (16 bit), End Location (16 bit), Red Filter, Green Filter, Blue Filter
#define VCADDR_8BIT_VMCTRL           VC_8BIT_BASE_ADDR + 0xB8            // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define VCADDR_8BIT_VMGRMODE         VC_8BIT_BASE_ADDR + 0xB9            // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define VCADDR_8BIT_VMREDMASK        VC_8BIT_BASE_ADDR + 0xBA            // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMGREENMASK      VC_8BIT_BASE_ADDR + 0xBB            // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMBLUEMASK       VC_8BIT_BASE_ADDR + 0xBC            // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define VCADDR_8BIT_VMPAGE           VC_8BIT_BASE_ADDR + 0xBD            // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.
#define VCADDR_8BIT_VMVGATTR         VC_8BIT_BASE_ADDR + 0xBE            // Select VGA Border colour and attributes. Bit 2 = Red, 1 = Green, 0 = Blue, 4:3 = VGA Mode, 00 = Off, 01 = 640x480, 10 = 800x600, 11 = 50Hz Internal
#define VCADDR_8BIT_VMVGAMODE        VC_8BIT_BASE_ADDR + 0xBF            // Select VGA Output mode. [3:0] - required output resolution/frequency.
#define VCADDR_8BIT_SYSCTRL          VC_8BIT_BASE_ADDR + 0xF0            // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define VCADDR_8BIT_GRAMMODE         VC_8BIT_BASE_ADDR + 0xF4            // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
#define VCADDR_8BIT_VMPALETTE        VC_8BIT_BASE_ADDR + 0xF5            // Select Palette:
                                                                         //    0xF5 sets the palette. The Video Module supports 4 bit per colour output but there is only enough RAM for 1 bit per colour so the pallette is used to change the colours output.
                                                                         //      Bits [7:0] defines the pallete number. This indexes a lookup table which contains the required 4bit output per 1bit input.

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

// Memory addresses of Sharp MZ Series emulator registers and memory.
//
#define MZ_EMU_REG_INTR_ADDR         MZ_EMU_BASE_ADDR + 0x020            // Base address of the interrupt generator.
#define MZ_EMU_REG_KEYB_ADDR         MZ_EMU_BASE_ADDR + 0x200            // Base address of the keyboard register and map table.
#define MZ_EMU_ADDR_REG_MODEL        MZ_EMU_BASE_ADDR + 0                // Address of the machine MODEL configuration register.
#define MZ_EMU_ADDR_REG_DISPLAY      MZ_EMU_BASE_ADDR + 1                // Address of the DISPLAY configuration register 1.
#define MZ_EMU_ADDR_REG_DISPLAY2     MZ_EMU_BASE_ADDR + 2                // Address of the DISPLAY configuration register 2.
#define MZ_EMU_ADDR_REG_DISPLAY3     MZ_EMU_BASE_ADDR + 3                // Address of the DISPLAY configuration register 3.
#define MZ_EMU_ADDR_REG_CPU          MZ_EMU_BASE_ADDR + 4                // Address of the CPU configuration register.
#define MZ_EMU_ADDR_REG_AUDIO        MZ_EMU_BASE_ADDR + 5                // Address of the AUDIO configuration register.
#define MZ_EMU_ADDR_REG_CMT          MZ_EMU_BASE_ADDR + 6                // Address of the CMT (tape drive) configuration register 1.
#define MZ_EMU_ADDR_REG_CMT2         MZ_EMU_BASE_ADDR + 7                // Address of the CMT (tape drive) configuration register 2.
#define MZ_EMU_ADDR_REG_CMT3         MZ_EMU_BASE_ADDR + 8                // Address of the CMT (tape drive) configuration register 3.
#define MZ_EMU_ADDR_REG_FDD          MZ_EMU_BASE_ADDR + 9                // Address of the Floppy Disk Drive configuration register 1.
#define MZ_EMU_ADDR_REG_FDD2         MZ_EMU_BASE_ADDR + 10               // Address of the Floppy Disk Drive configuration register 2.
#define MZ_EMU_ADDR_REG_FDD3         MZ_EMU_BASE_ADDR + 11               // Address of the Floppy Disk Drive configuration register 3.
#define MZ_EMU_ADDR_REG_FDD4         MZ_EMU_BASE_ADDR + 12               // Address of the Floppy Disk Drive configuration register 4.
#define MZ_EMU_ADDR_REG_ROMS         MZ_EMU_BASE_ADDR + 13               // Address of the optional ROMS configuration register.
#define MZ_EMU_ADDR_REG_SWITCHES     MZ_EMU_BASE_ADDR + 14               // Address of the Hardware configuration switches.
#define MZ_EMU_ADDR_REG_CTRL         MZ_EMU_BASE_ADDR + 15               // Address of the Control reigster.
#define MZ_EMU_INTR_ISR              0x00                                // Interupt service reason register, define what caused the interupt.
#define MZ_EMU_KEYB_KEY_MATRIX       0x00                                // Key matrix array current scan.
#define MZ_EMU_KEYB_KEY_MATRIX_LAST  0x10                                // Key matrix array previous scan.
#define MZ_EMU_KEYB_CTRL_REG         0x20                                // Keyboard control register.
#define MZ_EMU_KEYB_KEYD_REG         0x21                                // Keyboard key data register.
#define MZ_EMU_KEYB_KEYC_REG         0x22                                // Keyboard control data register.
#define MZ_EMU_KEYB_KEY_POS_REG      0x23                                // Keyboard mapped character mapping position.
#define MZ_EMU_KEYB_KEY_POS_LAST_REG 0x24                                // Keyboard mapped character previous mapping position.

#define MZ_EMU_INTR_MAX_REGISTERS    1                                   // Maximum number of registers in the interrupt generator.
#define MZ_EMU_KEYB_MAX_REGISTERS    37                                  // Maximum number of registers in the keyboard interface.

#define MZ_EMU_REG_MODEL             0                                   // Machine MODEL configuration register.            
#define MZ_EMU_REG_DISPLAY           1                                   // DISPLAY configuration register 1.                
#define MZ_EMU_REG_DISPLAY2          2                                   // DISPLAY configuration register 2.                
#define MZ_EMU_REG_DISPLAY3          3                                   // DISPLAY configuration register 3.                
#define MZ_EMU_REG_CPU               4                                   // CPU configuration register.                      
#define MZ_EMU_REG_AUDIO             5                                   // AUDIO configuration register.                    
#define MZ_EMU_REG_CMT               6                                   // CMT (tape drive) configuration register 1.       
#define MZ_EMU_REG_CMT2              7                                   // CMT (tape drive) configuration register 2.       
#define MZ_EMU_REG_CMT3              8                                   // CMT (tape drive) configuration register 2.       
#define MZ_EMU_REG_FDD               9                                   // Floppy Disk Drive configuration register 1.
#define MZ_EMU_REG_FDD2              10                                  // Floppy Disk Drive configuration register 2.
#define MZ_EMU_REG_FDD3              11                                  // Floppy Disk Drive configuration register 3.
#define MZ_EMU_REG_FDD4              12                                  // Floppy Disk Drive configuration register 4.
#define MZ_EMU_REG_ROMS              13                                  // Options ROMS configuration
#define MZ_EMU_REG_SWITCHES          14                                  // Hardware switches, MZ800 = 3:0
#define MZ_EMU_REG_CTRL              15                                  // Emulation control register.
#define MZ_EMU_MAX_REGISTERS         16                                  // Maximum number of registers on the emulator.
#define MZ_EMU_KEYB_DISABLE_EMU      0x01                                // Disable keyboard scan codes being sent to the emulation.
#define MZ_EMU_KEYB_ENABLE_INTR      0x02                                // Enable interrupt on every key press.

#define MZ_EMU_DISPLAY_MONO          0x00                                // Monochrome display.
#define MZ_EMU_DISPLAY_MONO80        0x01                                // Monochrome 80 column display.
#define MZ_EMU_DISPLAY_COLOUR        0x02                                // Colour display.
#define MZ_EMU_DISPLAY_COLOUR80      0x03                                // Colour 80 column display.
#define MZ_EMU_DISPLAY_VRAM_ON       0x00                                // Video RAM enable.
#define MZ_EMU_DISPLAY_VRAM_OFF      0x04                                // Video RAM disable.
#define MZ_EMU_DISPLAY_GRAM_ON       0x00                                // Graphics RAM enable.
#define MZ_EMU_DISPLAY_GRAM_OFF      0x08                                // Graphics RAM disable.
#define MZ_EMU_DISPLAY_VIDWAIT_ON    0x10                                // Enable Video Wait States
#define MZ_EMU_DISPLAY_VIDWAIT_OFF   0x00                                // Disable Video Wait States
#define MZ_EMU_DISPLAY_PCG_ON        0x80                                // Enable the Programmable Character Generator.
#define MZ_EMU_DISPLAY_PCG_OFF       0x00                                // Disable the Programmable Character Generator.
#define MZ_EMU_B_CPU_SPEED_4M        0x00                                // CPU Freq for the MZ80B group machines.
#define MZ_EMU_B_CPU_SPEED_8M        0x01                                // CPU Freq for the MZ80B group machines.
#define MZ_EMU_B_CPU_SPEED_16M       0x02                                // CPU Freq for the MZ80B group machines.
#define MZ_EMU_B_CPU_SPEED_32M       0x03                                // CPU Freq for the MZ80B group machines.
#define MZ_EMU_B_CPU_SPEED_64M       0x04                                // CPU Freq for the MZ80B group machines.
#define MZ_EMU_C_CPU_SPEED_2M        0x00                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_C_CPU_SPEED_4M        0x01                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_C_CPU_SPEED_8M        0x02                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_C_CPU_SPEED_16M       0x03                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_C_CPU_SPEED_32M       0x04                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_C_CPU_SPEED_64M       0x05                                // CPU Freq for the MZ80C group machines.
#define MZ_EMU_78_CPU_SPEED_3M5      0x00                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_78_CPU_SPEED_7M       0x01                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_78_CPU_SPEED_14M      0x02                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_78_CPU_SPEED_28M      0x03                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_78_CPU_SPEED_56M      0x04                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_78_CPU_SPEED_112M     0x05                                // CPU Freq for the MZ80/700/800 group machines.
#define MZ_EMU_CMT_SPEED_NORMAL      0x00                                // CMT runs at normal speed for the selected model.
#define MZ_EMU_CMT_SPEED_2X          0x01                                // CMT runs at 2x speed.
#define MZ_EMU_CMT_SPEED_4X          0x02                                // CMT runs at 4x speed.
#define MZ_EMU_CMT_SPEED_8X          0x03                                // CMT runs at 8x speed.
#define MZ_EMU_CMT_SPEED_16X         0x04                                // CMT runs at 16x speed.
#define MZ_EMU_CMT_SPEED_32X         0x05                                // CMT runs at 32x speed.
#define MZ_EMU_CMT_BUTTON_OFF        0x00                                // CMT keys are off.
#define MZ_EMU_CMT_BUTTON_PLAY       0x08                                // CMT PLAY button is pressed.
#define MZ_EMU_CMT_BUTTON_RECORD     0x10                                // CMT RECORD button is pressed.
#define MZ_EMU_CMT_BUTTON_AUTO       0x18                                // CMT auto logic enabled to select button according to state.
#define MZ_EMU_CMT_ASCIIIN           0x20                                // Enable ASCII translation of filenames going into the CMT.
#define MZ_EMU_CMT_ASCIIOUT          0x40                                // Enable ASCII translation of filenames output by the CMT.
#define MZ_EMU_CMT_HARDWARE          0x80                                // Enable use of the physical host cassette drive instead of the CMT emulation.

// IO addresses on the tranZPUter or mainboard.
//
#define IO_TZ_CTRLLATCH              0x60                                // Control latch which specifies the Memory Model/mode.
#define IO_TZ_SETXMHZ                0x62                                // Switch to alternate CPU frequency provided by K64F.
#define IO_TZ_SET2MHZ                0x64                                // Switch to system CPU frequency.
#define IO_TZ_CLKSELRD               0x66                                // Read the status of the clock select, ie. which clock is connected to the CPU.
#define IO_TZ_SVCREQ                 0x68                                // Service request from the Z80 to be provided by the K64F.
#define IO_TZ_SYSREQ                 0x6A                                // System request from the Z80 to be provided by the K64F.
#define IO_TZ_CPLDSTATUS             0x6B                                // Version 2.1 CPLD status register.
#define IO_TZ_CPUCFG                 0x6C                                // Version 2.2 CPU configuration register.
#define IO_TZ_CPUSTATUS              0x6C                                // Version 2.2 CPU runtime status register.
#define IO_TZ_CPUINFO                0x6D                                // Version 2.2 CPU information register.
#define IO_TZ_CPLDCFG                0x6E                                // Version 2.1 CPLD configuration register.
#define IO_TZ_CPLDINFO               0x6F                                // Version 2.1 CPLD version information register.
#define IO_TZ_CPLDINFO               0x6F                                // Version 2.1 CPLD version information register.
#define IO_TZ_PALSLCTOFF             0xA3                                // Set the palette slot (PALETTE_PARAM_SEL) Off position to be adjusted.
#define IO_TZ_PALSLCTON              0xA4                                // Set the palette slot (PALETTE_PARAM_SEL) On position to be adjusted.
#define IO_TZ_PALSETRED              0xA5                                // Set the red palette value according to the PALETTE_PARAM_SEL address.
#define IO_TZ_PALSETGREEN            0xA6                                // Set the green palette value according to the PALETTE_PARAM_SEL address.
#define IO_TZ_PALSETBLUE             0xA7                                // Set the blue palette value according to the PALETTE_PARAM_SEL address.
#define IO_TZ_OSDMNU_SZX             0xA8                                // Get OSD Menu Horizontal Size (X).
#define IO_TZ_OSDMNU_SZY             0xA9                                // Get OSD Menu Vertical Size (Y).
#define IO_TZ_OSDHDR_SZX             0xAA                                // Get OSD Status Header Horizontal Size (X).
#define IO_TZ_OSDHDR_SZY             0xAB                                // Get OSD Status Header Vertical Size (Y).
#define IO_TZ_OSDFTR_SZX             0xAC                                // Get OSD Status Footer Horizontal Size (X).
#define IO_TZ_OSDFTR_SZY             0xAD                                // Get OSD Status Footer Vertical Size (Y).   
#define IO_TZ_PALETTE                0xB0                                // Sets the palette. The Video Module supports 4 bit per colour output but there is only enough RAM for 1 bit per colour so the pallette is used to change the colours output.
                                                                         //    Bits [7:0] defines the pallete number. This indexes a lookup table which contains the required 4bit output per 1bit input.
#define IO_TZ_GPUPARAM               0xB2                                // Set parameters. Store parameters in a long word to be used by the graphics command processor.
                                                                         //    The parameter word is 128 bit and each write to the parameter word shifts left by 8 bits and adds the new byte at bits 7:0.
#define IO_TZ_GPUCMD                 0xB3                                // Set the graphics processor unit commands.
                                                                         //    Bits [5:0] - 0 = Reset parameters.
                                                                         //                 1 = Clear to val. Start Location (16 bit), End Location (16 bit), Red Filter, Green Filter, Blue Filter
#define IO_TZ_VMCTRL                 0xB8                                // Video Module control register. [2:0] - 000 (default) = MZ80A, 001 = MZ-700, 010 = MZ800, 011 = MZ80B, 100 = MZ80K, 101 = MZ80C, 110 = MZ1200, 111 = MZ2000. [3] = 0 - 40 col, 1 - 80 col.
#define IO_TZ_VMGRMODE               0xB9                                // Video Module graphics mode. 7/6 = Operator (00=OR,01=AND,10=NAND,11=XOR), 5=GRAM Output Enable, 4 = VRAM Output Enable, 3/2 = Write mode (00=Page 1:Red, 01=Page 2:Green, 10=Page 3:Blue, 11=Indirect), 1/0=Read mode (00=Page 1:Red, 01=Page2:Green, 10=Page 3:Blue, 11=Not used).
#define IO_TZ_VMREDMASK              0xBA                                // Video Module Red bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMGREENMASK            0xBB                                // Video Module Green bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMBLUEMASK             0xBC                                // Video Module Blue bit mask (1 bit = 1 pixel, 8 pixels per byte).
#define IO_TZ_VMPAGE                 0xBD                                // Video Module memory page register. [1:0] switches in 1 16Kb page (3 pages) of graphics ram to C000 - FFFF. Bits [1:0] = page, 00 = off, 01 = Red, 10 = Green, 11 = Blue. This overrides all MZ700/MZ80B page switching functions. [7] 0 - normal, 1 - switches in CGROM for upload at D000:DFFF.
#define IO_TZ_VMVGATTR               0xBE                                // Select VGA Border colour and attributes. Bit 2 = Red, 1 = Green, 0 = Blue, 4:3 = VGA Mode, 00 = Off, 01 = 640x480, 10 = 800x600, 11 = 50Hz Internal
#define IO_TZ_VMVGAMODE              0xBF                                // Select VGA Output mode. [3:0] - required output resolution/frequency.
#define IO_TZ_GDGWF                  0xCC                                // MZ-800      write format register
#define IO_TZ_GDGRF                  0xCD                                // MZ-800      read format register
#define IO_TZ_GDCMD                  0xCE                                // MZ-800 CRTC Mode register
#define IO_TZ_GDCMD                  0xCF                                // MZ-800 CRTC control register
#define IO_TZ_MMIO0                  0xE0                                // MZ-700/MZ-800 Memory management selection ports.
#define IO_TZ_MMIO1                  0xE1                                // ""
#define IO_TZ_MMIO2                  0xE2                                // ""
#define IO_TZ_MMIO3                  0xE3                                // ""
#define IO_TZ_MMIO4                  0xE4                                // ""
#define IO_TZ_MMIO5                  0xE5                                // ""
#define IO_TZ_MMIO6                  0xE6                                // ""
#define IO_TZ_MMIO7                  0xE7                                // MZ-700/MZ-800 Memory management selection ports.
#define IO_TZ_PPIA                   0xE0                                // MZ80B/MZ2000 8255 PPI Port A
#define IO_TZ_PPIB                   0xE1                                // MZ80B/MZ2000 8255 PPI Port B
#define IO_TZ_PPIC                   0xE2                                // MZ80B/MZ2000 8255 PPI Port C
#define IO_TZ_PPICTL                 0xE3                                // MZ80B/MZ2000 8255 PPI Control Register
#define IO_TZ_PIT0                   0xE4                                // MZ80B/MZ2000 8253 PIT Timer 0
#define IO_TZ_PIT1                   0xE5                                // MZ80B/MZ2000 8253 PIT Timer 1
#define IO_TZ_PIT2                   0xE6                                // MZ80B/MZ2000 8253 PIT Timer 2
#define IO_TZ_PITCTL                 0xE7                                // MZ80B/MZ2000 8253 PIT Control Register
#define IO_TZ_PIOA                   0xE8                                // MZ80B/MZ2000 Z80 PIO Port A
#define IO_TZ_PIOCTLA                0xE9                                // MZ80B/MZ2000 Z80 PIO Port A Control Register
#define IO_TZ_PIOB                   0xEA                                // MZ80B/MZ2000 Z80 PIO Port B
#define IO_TZ_PIOCTLB                0xEB                                // MZ80B/MZ2000 Z80 PIO Port B Control Register
#define IO_TZ_SYSCTRL                0xF0                                // System board control register. [2:0] - 000 MZ80A Mode, 2MHz CPU/Bus, 001 MZ80B Mode, 4MHz CPU/Bus, 010 MZ700 Mode, 3.54MHz CPU/Bus.
#define IO_TZ_GRAMMODE               0xF4                                // MZ80B Graphics mode.  Bit 0 = 0, Write to Graphics RAM I, Bit 0 = 1, Write to Graphics RAM II. Bit 1 = 1, blend Graphics RAM I output on display, Bit 2 = 1, blend Graphics RAM II output on display.
//#define IO_TZ_GRAMOPT                0xF4                                // MZ80B/MZ2000 GRAM configuration option.
#define IO_TZ_CRTGRPHPRIO            0xF5                                // MZ2000 Graphics priority register, character or a graphics colour has front display priority.
#define IO_TZ_CRTGRPHSEL             0xF6                                // MZ2000 Graphics output select on CRT or external CRT
#define IO_TZ_GRAMCOLRSEL            0xF7                                // MZ2000 Graphics RAM colour bank select.


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
#define VMMODE_MZ1500                0x06                                // Video mode = MZ1500
#define VMMODE_MZ80B                 0x07                                // Video mode = MZ80B
#define VMMODE_MZ2000                0x08                                // Video mode = MZ2000
#define VMMODE_MZ2200                0x09                                // Video mode = MZ2200
#define VMMODE_MZ2500                0x0A                                // Video mode = MZ2500
#define VMMODE_80CHAR                0x10                                // Enable 80 character display.
#define VMMODE_80CHAR_MASK           0xEF                                // Mask to filter out display width control bit.
#define VMMODE_COLOUR                0x20                                // Enable colour display.
#define VMMODE_COLOUR_MASK           0xDF                                // Mask to filter out colour control bit.
#define VMMODE_PCGRAM                0x40                                // Enable PCG RAM.
#define VMMODE_VGA_MASK              0xF0                                // Mask to filter out the VGA output mode bits.
#define VMMODE_VGA_OFF               0x00                                // Set VGA mode off, external monitor is driven by standard internal 60Hz signals.
#define VMMODE_VGA_INT               0x00                                // Set VGA mode off, external monitor is driven by standard internal 60Hz signals.
#define VMMODE_VGA_INT50             0x01                                // Set VGA mode off, external monitor is driven by standard internal 50Hz signals.
#define VMMODE_VGA_640x480           0x02                                // Set external monitor to VGA 640x480 @ 60Hz mode.
#define VMMODE_VGA_800x600           0x03                                // Set external monitor to VGA 800x600 @ 60Hz mode.

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

// Macros.
//
// Convert big endiam to little endian.
#define convBigToLittleEndian(num)   ((num>>24)&0xff) | ((num<<8)&0xff0000) | ((num>>8)&0xff00) | ((num<<24)&0xff000000)
#define setPixel(x,y,colour)         if(y >= 0 && y < osdWindow.params[osdWindow.mode].maxY && x >= 0 && x < osdWindow.params[osdWindow.mode].maxX) \
                                     { \
                                         for(uint8_t c=0; c < (VC_MENU_RGB_BITS > VC_STATUS_RGB_BITS ? VC_MENU_RGB_BITS : VC_STATUS_RGB_BITS); c++) \
                                         { \
                                             if(colour & (1 << c)) \
                                             { \
                                                 osdWindow.display[c][((y * osdWindow.params[osdWindow.mode].maxX) + x)/8] |= 0x80 >> x%8; \
                                             } \
                                         } \
                                     }
#define clearPixel(x,y,colour)       if(y >= 0 && y < osdWindow.params[osdWindow.mode].maxY && x >= 0 && x < osdWindow.params[osdWindow.mode].maxX) \
                                     { \
                                         for(uint8_t c=0; c < (VC_MENU_RGB_BITS > VC_STATUS_RGB_BITS ? VC_MENU_RGB_BITS : VC_STATUS_RGB_BITS); c++) \
                                         { \
                                             if(colour & (1 << c)) \
                                             { \
                                                 osdWindow.display[c][((y * osdWindow.params[osdWindow.mode].maxX) + x)/8] &= ~(0x80 >> x%8); \
                                             } \
                                         } \
                                     }

// Supported windows.
enum WINDOWS {
    STATUS                           = 0x00,                             // Status Window
    MENU                             = 0x01                              // Menu Window
};

// Supported orientation.
enum ORIENTATION {
    NORMAL                           = 0x00,                             // Normal character orientation.
    DEG90                            = 0x01,                             // 90 degree rotation
    DEG180                           = 0x02,                             // 180 degree rotation
    DEG270                           = 0x03                              // 270 degree rotation
};

// Supported colours.
enum COLOUR {
    BLACK                            = 0x00,                             // No pixels active.
    BLUE                             = 0x01,                             // Blue pixel active.
    RED                              = 0x02,                             // Red pixel active.
    PURPLE                           = 0x03,                             // Red and Blue pixels active.
    GREEN                            = 0x04,                             // Green pixel active.
    CYAN                             = 0x05,                             // Green and Blue pixels active.
    YELLOW                           = 0x06,                             // Green and Red pixels active.
    WHITE                            = 0x07                              // Green, Red and Blue pixels active.
};

// Supported attriutes/
enum ATTRIBUTES {
    NOATTR                           = 0x0000,                             // No attributes.
    HILIGHT_FG_ACTIVE                = 0x0008,                             // Highlight flag.
    HILIGHT_FG_BLACK                 = 0x0008 + 0x00,                      // Highlight the character foreground in black.
    HILIGHT_FG_BLUE                  = 0x0008 + 0x01,                      // Highlight ""     ""     ""       ""   blue.
    HILIGHT_FG_RED                   = 0x0008 + 0x02,                      // Highlight ""     ""     ""       ""   red.
    HILIGHT_FG_PURPLE                = 0x0008 + 0x03,                      // Highlight ""     ""     ""       ""   purple.
    HILIGHT_FG_GREEN                 = 0x0008 + 0x04,                      // Highlight ""     ""     ""       ""   green.
    HILIGHT_FG_CYAN                  = 0x0008 + 0x05,                      // Highlight ""     ""     ""       ""   cyan.
    HILIGHT_FG_YELLOW                = 0x0008 + 0x06,                      // Highlight ""     ""     ""       ""   yellow.
    HILIGHT_FG_WHITE                 = 0x0008 + 0x07,                      // Highlight ""     ""     ""       ""   white.
    HILIGHT_BG_ACTIVE                = 0x0010,                             // Highlight flag.
    HILIGHT_BG_BLACK                 = 0x0010 + 0x00,                      // Highlight the character background in black.
    HILIGHT_BG_BLUE                  = 0x0010 + 0x01,                      // Highlight ""     ""     ""       ""   blue.
    HILIGHT_BG_RED                   = 0x0010 + 0x02,                      // Highlight ""     ""     ""       ""   red.
    HILIGHT_BG_PURPLE                = 0x0010 + 0x03,                      // Highlight ""     ""     ""       ""   purple.
    HILIGHT_BG_GREEN                 = 0x0010 + 0x04,                      // Highlight ""     ""     ""       ""   green.
    HILIGHT_BG_CYAN                  = 0x0010 + 0x05,                      // Highlight ""     ""     ""       ""   cyan.
    HILIGHT_BG_YELLOW                = 0x0010 + 0x06,                      // Highlight ""     ""     ""       ""   yellow.
    HILIGHT_BG_WHITE                 = 0x0010 + 0x07                       // Highlight ""     ""     ""       ""   white.
};

// Public settings, accessed via enumerated value.
enum OSDPARAMS {
    ACTIVE_MAX_X                     = 0x00,                             // Width in pixels of the active framebuffer.
    ACTIVE_MAX_Y                     = 0x01                              // Depth in pixels of the active framebuffer.
};

// Structure to maintain data relevant to flashing a cursor at a given location.
//
typedef struct {
    // Attributes to be used when cursor is showing.
    uint16_t                         attr;

    // Colour of the character,
    enum COLOUR                      fg;
    enum COLOUR                      bg;
    
    // Location in the framebuffer where the character commences.
    uint8_t                          row;
    uint8_t                          col;
   
    // Offset in pixels to the given row/col. Allows for finer placing within mixed fonts.
    uint8_t                          ofrow;
    uint8_t                          ofcol;

    // Font used for the underlying character.
    enum FONTS                       font;

    // Flash speed of the cursor in ms.
    unsigned long                    speed;

    // Character being displayed.
    uint8_t                          dispChar;

    // Switch to enable/disable the cursor.
    uint8_t                          enabled;

    // Flash State.
    uint8_t                          flashing;
} t_CursorFlash;

// Structure to maintain the OSD Menu and Status display output parameters and data.
//
typedef struct {
    // Attributes for output data.
    uint8_t                          attr;

    // Location in the framebuffer to output data.
    uint8_t                          row;
    uint8_t                          col;

    // Maxims, dynamic to allow for changes due to selected font.
    uint8_t                          maxCol;
    uint8_t                          maxRow;

    // Window Features.
    uint8_t                          lineWrap;                           // Wrap line at status window edge (1) else stop printing at status window edge.
    uint16_t                         maxX;                               // Maximum X plane pixels.
    uint16_t                         maxY;                               // Maximum Y plane pixels.

    // Cursor data.
    t_CursorFlash                    cursor;                             // Data for enabling a flashing cursor at a given screen coordinate.
} t_WindowParams;

// Structure to maintain the OSD window data.
typedef struct {
    // Mode in which the OSD is operating.
    enum WINDOWS                     mode;

    // Per window parameters, currently a MENU and STATUS window exist.
    t_WindowParams                   params[sizeof(enum WINDOWS)+1];

    // Global features.
    uint8_t                          debug;                              // Enable debugging features.
    uint8_t                          inDebug;                            // Prevent recursion when outputting debug information.

    // Framebuffer backing store. Data for display is assembled in this buffer prior to bulk copy into the FPGA memory.
    uint8_t                          (*display)[VC_MENU_BUFFER_SIZE > VC_STATUS_BUFFER_SIZE ? VC_MENU_BUFFER_SIZE : VC_STATUS_BUFFER_SIZE];
} t_OSDWindow;

// Application execution constants.
//

// Prototypes.
//

uint32_t   OSDGet(enum OSDPARAMS);
fontStruct *OSDGetFont(enum FONTS);
bitmapStruct *OSDGetBitmap(enum BITMAPS);
void       OSDSetPixel(uint16_t, uint16_t, enum COLOUR);
void       OSDClearPixel(uint16_t, uint16_t, enum COLOUR);
void       OSDChangePixelColour(uint16_t, uint16_t, enum COLOUR, enum COLOUR);
void       _OSDwrite(uint8_t, uint8_t, int8_t, int8_t, uint8_t, uint8_t, enum ORIENTATION, uint8_t, uint16_t, enum COLOUR, enum COLOUR, fontStruct *);
void       OSDWriteBitmap(uint16_t, uint16_t, enum BITMAPS, enum COLOUR, enum COLOUR);
void       OSDWriteChar(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, enum FONTS, enum ORIENTATION, char, enum COLOUR, enum COLOUR);
void       OSDWriteString(uint8_t, uint8_t, int8_t, int8_t, uint8_t, uint8_t, enum FONTS, enum ORIENTATION, char *, uint16_t *, enum COLOUR, enum COLOUR);
void       OSDUpdateScreenSize(void);
void       OSDRefreshScreen(void);
void       OSDClearScreen(enum COLOUR);
void       OSDClearArea(int16_t, int16_t, int16_t, int16_t, enum COLOUR);
void       OSDDrawLine(int16_t, int16_t, int16_t, int16_t, enum COLOUR);
void       OSDDrawCircle(int16_t, int16_t, int16_t, enum COLOUR);
void       OSDDrawFilledCircle(int16_t, int16_t, int16_t, enum COLOUR);
void       OSDDrawEllipse(int16_t, int16_t, int16_t, int16_t, enum COLOUR);   
void       OSDSetActiveWindow(enum WINDOWS);
void       OSDSetCursorFlash(uint8_t, uint8_t, uint8_t, uint8_t, enum FONTS, uint8_t, enum COLOUR, enum COLOUR, uint16_t, unsigned long);
void       OSDClearCursorFlash(void);
void       OSDCursorFlash(void);
void       OSDService(void);
uint8_t    OSDInit(enum WINDOWS);

// Getter/Setter methods!


// Prototypes.
//

#ifdef __cplusplus
}
#endif
#endif // OSD_H
