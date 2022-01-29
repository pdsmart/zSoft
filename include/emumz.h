/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            emumz.h
// Created:         May 2021
// Version:         v1.0
// Author(s):       Philip Smart
// Description:     The MZ Emulator Control Logic
//                  This source file contains the logic to present an on screen display menu, interact
//                  with the user to set the config or perform machine actions (tape load) and provide
//                  overall control functionality in order to service the running Sharp MZ Series
//                  emulation.
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
#ifndef EMUMZ_H
#define EMUMZ_H

#ifdef __cplusplus
    extern "C" {
#endif
  
// Macros.
//
#define NUMELEM(a)  (sizeof(a)/sizeof(a[0]))

// Constants.
//
#define MAX_MENU_ROWS                25                                  // Maximum number of menu rows using smallest font.
#define MENU_ROW_WIDTH               80                                  // Maximum width of a menu row.
#define MENU_CHOICE_WIDTH            20                                  // Maximum width of a choice item.
#define MAX_MENU_DEPTH               5                                   // Maximum depth of menus.
#define MAX_MACHINE_TITLE_LEN        15                                  // Maximum length of the side bar machine name title.
#define MAX_DIRENTRY                 512                                 // Maximum number of read and stored directory entries from the SD card per directory.
#define MAX_DIR_DEPTH                4                                   // Maximum depth of sub-directories to enter.
#define MAX_FILENAME_LEN             64                                  // Maximum supported length of a filename.
#define MAX_FILTER_LEN               8                                   // Maximum length of a file filter.
#define TOPLEVEL_DIR                 "0:\\"                              // Top level directory for file list and select.
#define MAX_TAPE_QUEUE               5                                   // Maximum number of files which can be queued in the virtual tape drive.
#define CONFIG_FILENAME              "0:\\EMZ.CFG"                       // Configuration file for persisting the configuration.
#define MAX_EMU_REGISTERS            16                                  // Number of programmable registers in the emulator.
#define MAX_KEY_INS_BUFFER           64                                  // Maximum number of key sequences in a FIFO which can be inserted into the emulation keyboard.
#define MAX_INJEDIT_ROWS             4                                   // Maximum number of rows in the key injection editor.
#define MAX_INJEDIT_COLS             8                                   // Maximum number of columns in the key injection editor.
#define MAX_FB_LEN                   0x4000                              // Maximum size of a bank of pixels in the graphic framebuffer
#define MAX_TEXT_VRAM_LEN            0x800                               // Maximum size of the text based character VRAM.
#define MAX_ATTR_VRAM_LEN            0x800                               // Maximum size of the text based character attribute VRAM.
#define MAX_FLOPPY_DRIVES            4                                   // Maximum number of floppy drives supported.

// Keyboard key-injection constants.
#define KEY_INJEDIT_NIBBLES          8                                   // Number of nibbles in an injected key word.
#define KEY_INJEDIT_ROWS             (MAX_KEY_INS_BUFFER/MAX_INJEDIT_COLS)
#define KEY_INJEDIT_NIBBLES_PER_ROW  (MAX_INJEDIT_COLS*KEY_INJEDIT_NIBBLES)

// Maximum number of machines currently supported by the emulation.
//
#define MAX_MZMACHINES               11

// Keyboard control bits.
//
#define KEY_BREAK_BIT                0x80                                // Break key is being pressed when set to 1
#define KEY_CTRL_BIT                 0x40                                // CTRL key is being pressed when set to 1
#define KEY_SHIFT_BIT                0x20                                // SHIFT key is being pressed when set to 1
#define KEY_NOCTRL_BIT               0x00                                // No key control overrides.
#define KEY_DOWN_BIT                 0x02                                // DATA key has been pressed when set to 1
#define KEY_UP_BIT                   0x01                                // DATA key has been released when set to 1

// Sharp MZ Series Emulator constants.
//
#define MZ_EMU_ROM_ADDR              0x100000                            // Sharp MZ Series Emulation ROM address.
#define MZ_EMU_RAM_ADDR              0x120000                            // Sharp MZ Series Emulation RAM address.
#define MZ_EMU_CGROM_ADDR            0x220000                            // VideoController CGROM address.
#define MZ_EMU_USER_ROM_ADDR         0x12E800                            // Sharp MZ Series Emulation USER ROM address.
#define MZ_EMU_FDC_ROM_ADDR          0x12F000                            // Sharp MZ Series Emulation FDC ROM address.
#define MZ_EMU_TEXT_VRAM_ADDR        0x21D000                            // Base address for the text based VRAM.
#define MZ_EMU_ATTR_VRAM_ADDR        0x21D800                            // Base address for the text based attributes VRAM.
#define MZ_EMU_RED_FB_ADDR           0x240000                            // Base address for the pixel based frame buffer - Red.
#define MZ_EMU_BLUE_FB_ADDR          0x250000                            // Base address for the pixel based frame buffer - Blue.
#define MZ_EMU_GREEN_FB_ADDR         0x260000                            // Base address for the pixel based frame buffer - Green.
#define MZ_EMU_REG_BASE_ADDR         0x300000                            // Base address, in the FPGA address domain, of the emulator registers.
#define MZ_EMU_REG_INTR_ADDR         0x300020                            // Base address of the interrupt generator.
#define MZ_EMU_REG_SND_ADDR          0x300200                            // Base address of the sound generator.
#define MZ_EMU_REG_KEYB_ADDR         0x301000                            // Base address of the keyboard register and map table.
#define MZ_EMU_CMT_HDR_ADDR          0x340000                            // Header RAM    - 128 bytes 0x340000:0x34FFFF
#define MZ_EMU_CMT_DATA_ADDR         0x350000                            // Data RAM      - 64KBytes. 0x350000:0x35FFFF
#define MZ_EMU_CMT_MAP_ADDR          0x360000                            // ASCII MAP RAM - 512 bytes 0x360000:0x36FFFF
#define MZ_EMU_CMT_REG_ADDR          0x360200                            // CMT Registers.
#define MZ_EMU_FDD_CACHE_ADDR        0x330000                            // FDD Sector Cache.
#define MZ_EMU_FDC_CTRL_ADDR         0x330800                            // WD1793 Controller Control/Status register.
#define MZ_EMU_FDC_TRACK_ADDR        0x330801                            // WD1793 Controller Track number.
#define MZ_EMU_FDC_SECTOR_ADDR       0x330802                            // WD1793 Controller Sector number.
#define MZ_EMU_FDC_DATA_ADDR         0x330803                            // WD1793 Controller Data register. 
#define MZ_EMU_FDC_LCMD_ADDR         0x330804                            // WD1793 Last Command executed.
#define MZ_EMU_FDD_CTRL_ADDR         0x331000                            // FDD IOP Control/Status register.
#define MZ_EMU_FDD_TRACK_ADDR        0x331001                            // FDD Active track number.
#define MZ_EMU_FDD_SECTOR_ADDR       0x331002                            // FDD Active sector number.
#define MZ_EMU_FDD_CST_ADDR          0x331003                            // FDD Control/Status signals.
#define MZ_EMU_FDD_DISK_ADDR         0x331004                            // FDD Disk Parameter Registers, 1 per disk 0..3

// Floppy controller and drive registers.
//
#define MZ_EMU_FDD_MAX_REGISTERS     8                                   // Number of registers in the floppy disk drive unit.
#define MZ_EMU_FDC_MAX_REGISTERS     5                                   // Number of registers in the WD1793 controller.
#define MZ_EMU_FDD_MAX_DISKS         4                                   // Number of disks supported by the FDD unit.
#define MZ_EMU_FDD_CTRL_REG          0x00                                // IOP control/status register in the FDD.
#define MZ_EMU_FDD_TRACK_REG         0x01                                // Active track in the WD1793 controller, IOP actions based on this track.
#define MZ_EMU_FDD_SECTOR_REG        0x02                                // Active sector in the WD1793 controller, IOP actions based on this sector.
#define MZ_EMU_FDD_CST_REG           0x03                                // Floppy Disk Control register, current status and signal settings within the controller.
#define MZ_EMU_FDD_DISK_REG          0x04                                // Disk parameter registers to control layout, polarity, speed and write protection.
#define MZ_EMU_FDD_DISK_0_REG        0x04                                // Disk parameter registers Disk 0
#define MZ_EMU_FDD_DISK_1_REG        0x05                                // Disk parameter registers Disk 1
#define MZ_EMU_FDD_DISK_2_REG        0x06                                // Disk parameter registers Disk 2
#define MZ_EMU_FDD_DISK_3_REG        0x07                                // Disk parameter registers Disk 3
#define MZ_EMU_FDC_CTRL_REG          0x00                                // WD1793 Controller Control/Status register.
#define MZ_EMU_FDC_TRACK_REG         0x01                                // WD1793 Controller Track number.
#define MZ_EMU_FDC_SECTOR_REG        0x02                                // WD1793 Controller Sector number.
#define MZ_EMU_FDC_DATA_REG          0x03                                // WD1793 Controller Data register. 
#define MZ_EMU_FDC_LCMD_REG          0x04                                // WD1793 Last Command executed.

// Floppy Disk Control bits.
//
#define FDD_IOP_DISK_SELECT_NO       0xE0                                // Floppy disk selected drive number.
#define FDD_IOP_SIDE                 0x08                                // IOP Control parameters: Disk side selected.
#define FDD_IOP_SERVICE_REQ          0x04                                // IOP Control parameters: Service request flag.
#define FDD_IOP_REQ_MODE             0x03                                // IOP Control parameters: 00 = NOP, 01 = sector is written into cache (IOP->WD1793), 10 = sector is read from cache (WD1793->IOP), 11 = Obtain sector information
#define FDD_IOP_REQ_NOP              0x00                                // FDD request is to do nothing.
#define FDD_IOP_REQ_READ             0x01                                // FDD request to read a sector.
#define FDD_IOP_REQ_WRITE            0x02                                // FDD request to write a sector.
#define FDD_IOP_REQ_INFO             0x03                                // FDD request to get current information on disk sector/rpm.
#define FDD_DISK_BUSY                0x40                                // Floppy disk BUSY signal state.
#define FDD_DISK_DRQ                 0x20                                // Floppy disk DRQ signal state.
#define FDD_DISK_MOTORON             0x10                                // Floppy disk MOTOR signal state, 0 = on.
#define FDD_DISK_DDEN                0x10                                // Floppy disk Double Density DDEN signal state, 0 = double density.
#define FDD_DISK_SELECT_NO           0x07                                // Floppy disk selected drive number.
#define FDD_CTRL_READY               0x01                                // Flag to indicate the I/O processor is ready. 1 = ready, 0 = busy.
#define FDD_CTRL_SECTOR              0x0E                                // Addressed sector size, may differ from disk geometry
#define FDD_CTRL_TYPE                0xF0                                // Configuration of the controller for a given disk type.
#define FDD_DISK_0_WRITEN            0x02                                // Write enable Disk 0
#define FDD_DISK_1_WRITEN            0x08                                // Write enable Disk 1
#define FDD_DISK_2_WRITEN            0x20                                // Write enable Disk 2
#define FDD_DISK_3_WRITEN            0x80                                // Write enable Disk 3
#define FDD_DISK_0_POLARITY          0x01                                // Disk data polarity. 1 = data inverted (standard), 0 = data normal. 
#define FDD_DISK_1_POLARITY          0x04                                // Disk data polarity.
#define FDD_DISK_2_POLARITY          0x10                                // Disk data polarity.
#define FDD_DISK_3_POLARITY          0x40                                // Disk data polarity.
#define FDC_STI_NOTRDY               0x80                                // WD1793 Status I command - Not Ready
#define FDC_STI_PROTECTED            0x40                                // WD1793 Status I command - Protected Disk
#define FDC_STI_HEADLOADED           0x20                                // WD1793 Status I command - Head Loaded
#define FDC_STI_SEEKERROR            0x10                                // WD1793 Status I command - Seek Error
#define FDC_STI_CRCERROR             0x08                                // WD1793 Status I command - CRC Error
#define FDC_STI_TRACK0               0x04                                // WD1793 Status I command - Track 0
#define FDC_STI_INDEX                0x02                                // WD1793 Status I command - Index
#define FDC_STI_BUSY                 0x01                                // WD1793 Status I command - Busy
#define FDC_STII_NOTRDY              0x80                                // WD1793 Status I command - Not Ready
#define FDC_STII_PROTECTED           0x40                                // WD1793 Status I command - Protected Disk
#define FDC_STII_WRITEFAULT          0x20                                // WD1793 Status I command - Write Fault
#define FDC_STII_RECORDFAULT         0x20                                // WD1793 Status I command - Record Type 
#define FDC_STII_RECORDNOTFOUND      0x10                                // WD1793 Status I command - Record Not Found
#define FDC_STII_CRCERROR            0x08                                // WD1793 Status I command - CRC Error
#define FDC_STII_LOSTDATA            0x04                                // WD1793 Status I command - Lost Data
#define FDC_STII_DATAREQUEST         0x02                                // WD1793 Status I command - Data Request
#define FDC_STII_BUSY                0x01                                // WD1793 Status I command - Busy
#define FDC_CMD_RESTORE              0x00                                // WD1793 Controller command - Restore to track 0 and initialise.
#define FDC_CMD_SEEK                 0x10                                // WD1793 Controller command - Seek track
#define FDC_CMD_STEP                 0x20                                // WD1793 Controller command - Step
#define FDC_CMD_STEP_TU              0x30                                // WD1793 Controller command - Step with track update.
#define FDC_CMD_STEP_IN              0x40                                // WD1793 Controller command - Step In
#define FDC_CMD_STEPIN_TU            0x50                                // WD1793 Controller command - Step In with track update.
#define FDC_CMD_STEPOUT              0x60                                // WD1793 Controller command - Step Out
#define FDC_CMD_STEPOUT_TU           0x70                                // WD1793 Controller command - Step Out with track update.
#define FDC_CMD_READSEC              0x80                                // WD1793 Controller command - Read Sector
#define FDC_CMD_READSEC_MULT         0x90                                // WD1793 Controller command - Read Sector multiple.
#define FDC_CMD_WRITESEC             0xA0                                // WD1793 Controller command - Write Sector
#define FDC_CMD_WRITESEC_MULT        0xB0                                // WD1793 Controller command - Write Sector multiple
#define FDC_CMD_READADDR             0xC0                                // WD1793 Controller command - Read address
#define FDC_CMD_READTRACK            0xE0                                // WD1793 Controller command - Read Track
#define FDC_CMD_WRITETRACK           0xF0                                // WD1793 Controller command - Write Track
#define FDC_CMD_FORCEINT             0xD0                                // WD1793 Controller command - Force Interrupt


// Registers within the Machine Control module. Some of the data provided within these registers
// is also available directly from the hardware modules. 
#define MZ_EMU_MAX_REGISTERS         16                                  // Maximum number of registers on the emulator.
#define MZ_EMU_REG_MODEL             0                                   // Machine MODEL configuration register.            
#define MZ_EMU_REG_DISPLAY           1                                   // DISPLAY configuration register 1.                
#define MZ_EMU_REG_DISPLAY2          2                                   // DISPLAY configuration register 2.                
#define MZ_EMU_REG_DISPLAY3          3                                   // DISPLAY configuration register 3.                
#define MZ_EMU_REG_CPU               4                                   // CPU configuration register.                      
#define MZ_EMU_REG_AUDIO             5                                   // AUDIO configuration register.                    
#define MZ_EMU_REG_CMT               6                                   // CMT (tape drive) configuration register.       
#define MZ_EMU_REG_CMT2              7                                   // CMT (tape drive) apss status register.       
#define MZ_EMU_REG_CMT3              8                                   // CMT (tape drive) status register.       
#define MZ_EMU_REG_FDD               9                                   // Floppy Disk Drive configuration register 1.
#define MZ_EMU_REG_FDD2              10                                  // Floppy Disk Drive configuration register 2.
#define MZ_EMU_REG_FREE1             11                                  // Spare register 1
#define MZ_EMU_REG_FREE2             12                                  // Spare register 2
#define MZ_EMU_REG_ROMS              13                                  // Options ROMS configuration
#define MZ_EMU_REG_SWITCHES          14                                  // Hardware switches, MZ800 = 3:0
#define MZ_EMU_REG_CTRL              15                                  // Emulator control register.

// Physical address of the registers within the Machine Control module.
#define MZ_EMU_ADDR_REG_MODEL        MZ_EMU_REG_BASE_ADDR + 0            // Address of the machine MODEL configuration register.
#define MZ_EMU_ADDR_REG_DISPLAY      MZ_EMU_REG_BASE_ADDR + 1            // Address of the DISPLAY configuration register 1.
#define MZ_EMU_ADDR_REG_DISPLAY2     MZ_EMU_REG_BASE_ADDR + 2            // Address of the DISPLAY configuration register 2.
#define MZ_EMU_ADDR_REG_DISPLAY3     MZ_EMU_REG_BASE_ADDR + 3            // Address of the DISPLAY configuration register 3.
#define MZ_EMU_ADDR_REG_CPU          MZ_EMU_REG_BASE_ADDR + 4            // Address of the CPU configuration register.
#define MZ_EMU_ADDR_REG_AUDIO        MZ_EMU_REG_BASE_ADDR + 5            // Address of the AUDIO configuration register.
#define MZ_EMU_ADDR_REG_CMT          MZ_EMU_REG_BASE_ADDR + 6            // Address of the CMT (tape drive) configuration register.
#define MZ_EMU_ADDR_REG_CMT2         MZ_EMU_REG_BASE_ADDR + 7            // Address of the CMT (tape drive) apss register.
#define MZ_EMU_ADDR_REG_CMT3         MZ_EMU_REG_BASE_ADDR + 8            // Address of the CMT (tape drive) status register.
#define MZ_EMU_ADDR_REG_FDD          MZ_EMU_REG_BASE_ADDR + 9            // Address of the Floppy Disk Drive configuration register 1.
#define MZ_EMU_ADDR_REG_FDD2         MZ_EMU_REG_BASE_ADDR + 10           // Address of the Floppy Disk Drive configuration register 2.
#define MZ_EMU_ADDR_REG_FREE1        MZ_EMU_REG_BASE_ADDR + 11           // Address of the unused regiser 1.
#define MZ_EMU_ADDR_REG_FREE2        MZ_EMU_REG_BASE_ADDR + 12           // Address of the unused regiser 2.
#define MZ_EMU_ADDR_REG_ROMS         MZ_EMU_REG_BASE_ADDR + 13           // Address of the optional ROMS configuration register.
#define MZ_EMU_ADDR_REG_SWITCHES     MZ_EMU_REG_BASE_ADDR + 14           // Address of the Hardware configuration switches.
#define MZ_EMU_ADDR_REG_CTRL         MZ_EMU_REG_BASE_ADDR + 15           // Address of the Control reigster.

// Interrupt generator control and status registers.
#define MZ_EMU_INTR_MAX_REGISTERS    1                                   // Maximum number of registers in the interrupt generator.
#define MZ_EMU_INTR_REG_ISR          0x00                                // Interupt service reason register, define what caused the interupt.
#define MZ_EMU_INTR_SRC_KEYB         0x01                                // Interrupt source = Keyboard.
#define MZ_EMU_INTR_SRC_CMT          0x02                                // Interrupt source = CMT.
#define MZ_EMU_INTR_SRC_FDD          0x04                                // Interrupt source = FDD.

// Cassette module control and status registers.
#define MZ_EMU_CMT_MAX_REGISTERS     0x04                                // Maximum number of registers in the cmt interface.
#define MZ_EMU_CMT_STATUS_REG        0x00                                // CMT status register.
#define MZ_EMU_CMT_STATUS2_REG       0x01                                // CMT2 status register (APSS).
#define MZ_EMU_CMT_STATUS_INTR_REG   0x02                                // CMT interrupt status trigger. 
#define MZ_EMU_CMT_STATUS2_INTR_REG  0x03                                // CMT2 interrupt status trigger.

// Keyboard control and status registers, mapping tables and cache.
#define MZ_EMU_KEYB_MAX_REGISTERS    8                                   // Maximum number of status and control registers in the keyboard interface, excludes debug registers.
#define MZ_EMU_KEYB_CTRL_REG         0x00                                // Keyboard control register.
#define MZ_EMU_KEYB_FIFO_REG         0x01                                // Key insertion FIFO control register.
#define MZ_EMU_KEYB_FIFO_WR_ADDR     0x02                                // FIFO write pointer value.
#define MZ_EMU_KEYB_FIFO_RD_ADDR     0x03                                // FIFO read pointer value.
#define MZ_EMU_KEYB_KEYC_REG         0x04                                // Keyboard control data register.
#define MZ_EMU_KEYB_KEYD_REG         0x05                                // Keyboard key data register.
#define MZ_EMU_KEYB_KEY_POS_REG      0x06                                // Keyboard mapped character mapping position.
#define MZ_EMU_KEYB_KEY_POS_LAST_REG 0x07                                // Keyboard mapped character previous mapping position.
#define MZ_EMU_KEYB_KEY_MATRIX       0x10                                // Key matrix array current scan.
#define MZ_EMU_KEYB_KEY_MATRIX_LAST  0x20                                // Key matrix array previous scan.
#define MZ_EMU_KEYB_FIFO_SIZE        0x40                                // Size of the key insertion FIFO.
#define MZ_EMU_KEYB_FIFO_ADDR        0x0100                              // Key insertion FIFO.
#define MZ_EMU_KEYB_MAP_ADDR         0x0800                              // Address of the emulation keyboard mapping table.
#define MZ_EMU_KEYB_IOP_MAP_ADDR     0x0900                              // Address offset to the scan code:key map array for the I/O processor keys.
#define MZ_EMU_KEYB_DISABLE_EMU      0x01                                // Disable keyboard scan codes being sent to the emulation.
#define MZ_EMU_KEYB_ENABLE_INTR      0x02                                // Enable interrupt on every key press.
#define MZ_EMU_KEYB_SEND_KEY_EVENTS  0x04                                // Send keyboard up and down interrupt events.
#define MZ_EMU_KEYB_FIFO_FULL        0x01                                // Bit in FIFO Status register to indicate the FIFO is full.
#define MZ_EMU_KEYB_FIFO_WORD_RST    0x80                                // Reset keyboard key insertion word pointer, 4 bytes are needed per word and this resets to the 1st byte in the word.

// Display control values.
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
#define MZ_EMU_C_CPU_SPEED_2M        0x00                                // CPU Freq for the MZ80K group machines.
#define MZ_EMU_C_CPU_SPEED_4M        0x01                                // CPU Freq for the MZ80K group machines.
#define MZ_EMU_C_CPU_SPEED_8M        0x02                                // CPU Freq for the MZ80K group machines.
#define MZ_EMU_C_CPU_SPEED_16M       0x03                                // CPU Freq for the MZ80K group machines.
#define MZ_EMU_C_CPU_SPEED_32M       0x04                                // CPU Freq for the MZ80K group machines.
#define MZ_EMU_C_CPU_SPEED_64M       0x05                                // CPU Freq for the MZ80K group machines.
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
// Tape(CMT) Register bits.
//
#define MZ_EMU_CMT_PLAY_READY        0x01                                // Tape drive ready for playback.
#define MZ_EMU_CMT_PLAYING           0x02                                // Tape drive playing tape.
#define MZ_EMU_CMT_RECORD_READY      0x04                                // Tape drive ready for record.
#define MZ_EMU_CMT_RECORDING         0x08                                // Tape drive recording.
#define MZ_EMU_CMT_ACTIVE            0x10                                // Tape drive is active and ready.
#define MZ_EMU_CMT_SENSE             0x20                                // Tape drive SENSE, indicates if buttons depressed and motor on.
#define MZ_EMU_CMT_WRITEBIT          0x40                                // CMT unit write head bit. Data on this bit reflects what is sent to the tape write head.
#define MZ_EMU_CMT2_APSS             0x01                                // APSS CMT APSS select.
#define MZ_EMU_CMT2_DIRECTION        0x02                                // APSS CMT APSS direction.
#define MZ_EMU_CMT2_EJECT            0x04                                // APSS CMT EJECT tape.
#define MZ_EMU_CMT2_PLAY             0x08                                // APSS CMT PLAY tape.
#define MZ_EMU_CMT2_STOP             0x10                                // APSS CMT STOP tape.
#define MZ_EMU_CMT2_AUTOREW          0x20                                // APSS Auto rewind at tape end.
#define MZ_EMU_CMT2_AUTOPLAY         0x40                                // APSS Auto play after APSS or rewind.


// Menu selection types.
enum MENUTYPES {
    MENUTYPE_SUBMENU                 = 0x01,                             // Item selects a sub-menu.
    MENUTYPE_CHOICE                  = 0x02,                             // Item selects a choice.
    MENUTYPE_ACTION                  = 0x04,                             // Item directly select a function/action.
    MENUTYPE_BLANK                   = 0x08,                             // Blank filler menu line.
    MENUTYPE_TEXT                    = 0x10                              // Static text line.
};

// Menu item states.
enum MENUSTATE {
    MENUSTATE_ACTIVE                 = 0x00,                             // Menu item is active and visible.
    MENUSTATE_HIDDEN                 = 0x01,                             // Menu item is active but not visible on the menu.
    MENUSTATE_GREYED                 = 0x02,                             // Menu item is inactive, visible but greyed out.
    MENUSTATE_BLANK                  = 0x03,                             // Menu item is visible but has no content.
    MENUSTATE_TEXT                   = 0x04,                             // Menu item is visible text for display only
    MENUSTATE_INACTIVE               = 0x05                              // Menu item is not active or visible, a placeholder state.
};

// Modes of menu display interaction.
//
enum MENUMODE {
    MENU_NORMAL                      = 0x00,                             // Menu type is normal, stops at item 0 and last item.
    MENU_WRAP                        = 0x01                              // Menu wraps, first item wraps to last, last to first.
};

enum MENUACTIVE {
    MENU_DISABLED                    = 0x00,                             // Menu is disabled and not being displayed.
    MENU_MAIN                        = 0x01,                             // Main menu is active.
    MENU_TAPE_STORAGE                = 0x02,                             // Tape Storage menu is active.
    MENU_FLOPPY_STORAGE              = 0x03,                             // Floppy Storage menu is active.
    MENU_MACHINE                     = 0x04,                             // Machine menu is active.
    MENU_DISPLAY                     = 0x05,                             // Display menu is active.
    MENU_AUDIO                       = 0x06,                             // Audio menu is active.
    MENU_SYSTEM                      = 0x07,                             // System menu is active.
    MENU_ROMMANAGEMENT               = 0x08,                             // Rom Management menu is active.
    MENU_AUTOSTART                   = 0x09                              // Autostart Application menu is active.
};

enum MENUCALLBACK {
    MENUCB_DONOTHING                 = 0x00,                             // After callback return continue, no additional processing.
    MENUCB_REFRESH                   = 0x01,                             // After callback, refresh OSD.
};

enum DIALOGTYPE {
    DIALOG_MENU                      = 0x00,                             // OSD is displaying the Menu system.
    DIALOG_FILELIST                  = 0x01,                             // OSD is displaying a file list selection screen.
    DIALOG_KEYENTRY                  = 0x02,                             // OSD is updating the key injection values.
};

enum ACTIONMODE {
    ACTION_DEFAULT                   = 0x00,                             // Action callback executes default actions.
    ACTION_SELECT                    = 0x01,                             // Action callback executes the selection action.
    ACTION_TOGGLECHOICE              = 0x02,                             // Action callback implements toggle feature.
};

// Error return codes from processing a floppy disk request.
//
enum FLOPPYERRORCODES {
    FLPYERR_NOERROR                  = 0x00,                             // No error in processing detected.
    FLPYERR_SECTOR_NOT_FOUND         = 0x01,                             // Sector wasnt found in image.
    FLPYERR_TRACK_NOT_FOUND          = 0x02,                             // Track wasnt found in image.
    FLPYERR_HEAD_NOT_FOUND           = 0x03,                             // Head wasnt found in image.
    FLPYERR_WRITE_ERROR              = 0x04,                             // Write error, ie. permissions, disk full or hardware error (SD card removed).
    FLPYERR_DISK_ERROR               = 0x05,                             // Generic error for any disk retrieval error, ie. cant open, cant read etc.
};

// Types of disks recognised by the Sharp MZ hardware.
// These definitions are coded into the WD1773 controller so any changes in this source file must be reflected in the controller VHDL.
enum DISKTYPES {
    DISKTYPE_320K                    = 0x00,                             // 40T DS 16S 256B 320K
    DISKTYPE_320KT2                  = 0x01,                             // 40T DS 8S 512B 320K
    DISKTYPE_720K                    = 0x02,                             // 80T DS 9S 512B 720K
    DISKTYPE_800K                    = 0x03,                             // 80T DS 10S 512B 800K
    DISKTYPE_640K                    = 0x04,                             // 80T DS 16S 256B 640K
    DISKTYPE_350K                    = 0x05,                             // 35T DS 10S 512B 350K
    DISKTYPE_280K                    = 0x06,                             // 35T DS 16S 256B 280K
    DISKTYPE_400K                    = 0x07,                             // 40T 2H 10S 512B 400K
    DISKTYPE_1440K                   = 0x08,                             // 80T 2H 18S 512B 1440K
    DISKTYPE_TBD5                    = 0x09,                             // 
    DISKTYPE_TBD6                    = 0x0A,                             // 
    DISKTYPE_TBD7                    = 0x0B,                             // 
    DISKTYPE_TBD8                    = 0x0C,                             // 
    DISKTYPE_TBD9                    = 0x0D,                             // 
    DISKTYPE_TBDA                    = 0x0E,                             //
    DISKTYPE_TBDB                    = 0x0F
};

// Types of disk image we recognise and process.
enum IMAGETYPES {
    IMAGETYPE_EDSK                   = 0x00,                             // Extended CPC DSK Image Format.
    IMAGETYPE_IMG                    = 0x01                              // Raw binary format.
};

// Polarity of the floppy disk image, caters for direct MB8866 inverted images or normal images.
enum POLARITYTYPES {
    POLARITY_NORMAL                  = 0x00,                             // Normal image (created and visible in a hex editor).
    POLARITY_INVERTED                = 0x01                              // Inverted image, extracted from the MB8866 controller with inverted data bus.
};

// Write mode of the floppy drive. Read/Write or Read Only (Write Protected).
enum UPDATEMODETYPES {
    UPDATEMODE_READWRITE             = 0x00,                             // Disk data can be updated.
    UPDATEMODE_READONLY              = 0x01                              // Disk data is read only.
};

// Declare the menu callback as a type. This callback is used when a menu item such as submenu is activated.
typedef void   (*t_menuCallback)(uint8_t);

// Declare the choice callback as a type. This callback is used when rendering the menu and the choice value needs to be realised from the config settings.
typedef const char * (*t_choiceCallback)(void);

// Declare the data view callback as a type. This callback is used when rendering the menu and non menu data requires rendering as read only.
typedef void   (*t_viewCallback)(void);
// Ditto but for in function rendering.
typedef void   (*t_renderCallback)(uint16_t);

// Declare the return from dialog callback which is required to process data from a non-menu dialog such as a file list.
typedef void   (*t_dialogCallback)(char *param);

// Structure to map an ascii key into a row and column scan code.
typedef struct {
    uint8_t                          scanRow;                            // Emulation scan row.
    uint8_t                          scanCol;                            // Emulation scan column.
    uint8_t                          scanCtrl;                           // Emulation control key overrides for the row/col combination.
} t_scanCode;

// Structure to map an ascii key into a row and column scan code.
typedef struct {
    uint8_t                          key;                                // Ascii key for lookup.
    t_scanCode                       code[MAX_MZMACHINES];               // Scan code per machine.
} t_scanMap;

// Type translation union.
typedef union {
    uint32_t i;
    uint8_t b[sizeof (float)];
    float f;
} t_numCnv;

// Structure to contain a menu item and its properties.
//
typedef struct {
    char                             text[MENU_ROW_WIDTH];               // Buffers to store menu item text.
    char                             hotKey;                             // Shortcut key to activate selection, NULL = disabled.
    enum MENUTYPES                   type;                               // Type of menu option, sub-menu select or choice.
    enum MENUSTATE                   state;                              // State of the menu item, ie. hidden, greyed, active.
    t_menuCallback                   menuCallback;                       // Function to call when a line is activated, by CR or toggle.
    t_choiceCallback                 choiceCallback;                     // Function to call when a choice value is required.
    t_viewCallback                   viewCallback;                       // Function to call when non-menu data requires rendering within the menu.
    enum MENUCALLBACK                cbAction;                           // Action to take after callback completed.
} t_menuItem;

// Structure to maintain menu control and data elements.
//
typedef struct {
    uint16_t                         rowPixelStart;                      // Y pixel where row output commences.
    uint16_t                         colPixelStart;                      // X pixel where column output commences.
    uint16_t                         rowPixelDepth;                      // Depth of Y pixels for each row element.
    uint16_t                         colPixelsEnd;                       // X pixels from screen end where column output ends.
    uint8_t                          padding;                            // Inter row padding.
    enum COLOUR                      inactiveFgColour;                   // Foreground colour of an inactive (not selected) row item.
    enum COLOUR                      inactiveBgColour;                   // Background colour of an inactive (not selected) row item.
    enum COLOUR                      greyedFgColour;                     // Foreground colour of an inactive (not selected) row item.
    enum COLOUR                      greyedBgColour;                     // Background colour of an inactive (not selected) row item.
    enum COLOUR                      textFgColour;                       // Foreground colour of read only text on a row.
    enum COLOUR                      textBgColour;                       // Background colour of read only text on a row.
    enum COLOUR                      activeFgColour;                     // Active (selected) foreground colour of row item.
    enum COLOUR                      activeBgColour;                     // Active (selected) background colour of row item.
    enum FONTS                       font;                               // Font as a type, used in OSD calls.
    const fontStruct                 *rowFontptr;                        // General font used by menu row items.
    int16_t                          activeRow;                          // Active (selected) row. -1 = no selection.
    t_menuItem                       *data[MAX_MENU_ROWS];               // Details of a row's data to be displayed such as text.
} t_menu;

// Structure to maintain a directory entry, wether it is a file or directory entry.
//
typedef struct {
    char                             *name;
    uint8_t                          isDir;
} t_dirEntry;

// Structure to hold and manipulate a list of files and directories which are required when the user is requested to select a file.
//
typedef struct {
    uint16_t                         rowPixelStart;                      // Y pixel where row output commences.
    uint16_t                         colPixelStart;                      // X pixel where column output commences.
    uint16_t                         rowPixelDepth;                      // Depth of Y pixels for each row element.
    uint16_t                         colPixelsEnd;                       // X pixels from screen end where column output ends.
    uint8_t                          padding;                            // Inter row padding.
    enum COLOUR                      inactiveFgColour;                   // Foreground colour of an inactive (not selected) row item.
    enum COLOUR                      inactiveBgColour;                   // Background colour of an inactive (not selected) row item.
    enum COLOUR                      activeFgColour;                     // Active (selected) foreground colour of row item.
    enum COLOUR                      activeBgColour;                     // Active (selected) background colour of row item.
    enum FONTS                       font;                               // Font as a type, used in OSD calls.
    const fontStruct                 *rowFontptr;                        // General font used by file list row items.
    int16_t                          activeRow;                          // Active (selected) row. -1 = no selection.
    t_dirEntry                       dirEntries[MAX_DIRENTRY];           // Scanned directory entries and their properties.
    uint8_t                          selectDir;                          // Flag to indicate if selection is on a path rather than file.
    t_dialogCallback                 returnCallback;                     // A callback activated when a file is selected and control is returned to the Menu state.
    char                             fileFilter[MAX_FILTER_LEN];         // Active filter applied to a directory contents read.
} t_fileList;

// Structure to store the name, load address and the size of a given rom.
//
typedef struct
{
    char                             romFileName[MAX_FILENAME_LEN];
    uint8_t                          romEnabled;
    uint32_t                         loadAddr;
    uint32_t                         loadSize;
} t_romData;

// Structure to store the cold boot application details which gets loaded on machine instantiation.
//
typedef struct
{
    char                             appFileName[MAX_FILENAME_LEN];
    uint8_t                          appEnabled;
    t_numCnv                         preKeyInsertion[MAX_KEY_INS_BUFFER];
    t_numCnv                         postKeyInsertion[MAX_KEY_INS_BUFFER];
} appData_t;

// MZ Series Tape header structure - 128 bytes.
//
typedef struct
{
    unsigned char                     dataType;                          // 01 = machine code program.
                                                                         // 02 MZ-80 Basic program.
                                                                         // 03 MZ-80 data file.
                                                                         // 04 MZ-700 data file.
                                                                         // 05 MZ-700 Basic program.
    char                              fileName[17];                      // File name.
    unsigned short                    fileSize;                          // Size of data partition.
    unsigned short                    loadAddress;                       // Load address of the program/data.
    unsigned short                    execAddress;                       // Execution address of program.
    unsigned char                     comment[104];                      // Free text or code area.
} t_tapeHeader;

// Structures to store the tape file queue.
//
typedef struct
{
    char                             *queue[MAX_TAPE_QUEUE];
    char                             fileName[MAX_FILENAME_LEN];
    uint16_t                         tapePos;
    uint16_t                         elements;
} t_tapeQueue;

// Structure for a lookup table on floppy disk definition parameters.
//
typedef struct
{
    uint8_t                          tracks;                             // Number of tracks in floppy disk image.
    uint8_t                          heads;                              // Number of heads in floppy disk image.
    uint8_t                          sectors;                            // Number of sectors in floppy disk image.
    uint16_t                         sectorSize;                         // Size of a sector.
    uint16_t                         rpm;                                // Rotational Speed of image.
} t_floppyDef;

// Structure to maintain a Floppy Disk Drive configuration.
//
typedef struct
{
    char                             fileName[MAX_FILENAME_LEN];         // Name of floppy disk image, extension indicates type of image.
    enum IMAGETYPES                  imgType;                            // Type of image for fileName.
    uint8_t                          mounted;                            // Disk image is mounted and available to the disk drive.
    enum DISKTYPES                   diskType;                           // Type of disk.
    enum POLARITYTYPES               polarity;                           // Polarity of the image data (normal, inverted).
    enum UPDATEMODETYPES             updateMode;                         // Write Protect/Read Only or read/write mode.
} t_floppyDrive;


// Structure to store floppy disk control variables.
//
typedef struct
{
    uint8_t                          ctrlReg;                            // Control register mirror to allow single bit updates.
} t_floppyCtrl;

// Structure to store the parameters for key insertion editting.
//
typedef struct
{
    // Pointer into the key buffer. This pointer points to the start of the buffer.
    t_numCnv                         *bufptr;

    // Pointer to the key being editted. This is nibble level, so 2 nibbles per byte.
    uint16_t                         editptr;

    // Cursor attribute for cursor highlighting.
    uint16_t                         cursorAttr;

    // Colour of the dislayed character,
    enum COLOUR                      fg;
    enum COLOUR                      bg;
    
    // Location in the framebuffer where the character buffer commences.
    uint8_t                          startRow;
    uint8_t                          startCol;
   
    // Screen offsets to adjust for mixed fonts.
    uint8_t                          offsetRow;
    uint8_t                          offsetCol;

    // Flash speed of the cursor in ms.
    unsigned long                    cursorFlashRate;    

    // Font used for the underlying character.
    enum FONTS                       font;

    // Current view portal. Key buffer greater than 12x4 needs to be scrolled to access the entire buffer.
    uint16_t                         curView;

    // Function to render the buffer for updates etc.
    t_renderCallback                 render;
} t_keyInjectionEdit;

// Structure to maintain individual emulation configuration parameters.
typedef struct {
    uint8_t                          cpuSpeed;                           // Select the CPU speed, original or multiples.
    uint8_t                          memSize;                            // Select the memory size to match original machine.
    uint8_t                          audioSource;                        // Select the audio source. Not used on the MZ-700   
    uint8_t                          audioHardware;                      // Select the audio hardware. Either driver the underlying host hardware directly or enable the FPGA sound hardware.
    uint8_t                          audioVolume;                        // Set audio output volume.
    uint8_t                          audioMute;                          // Mute audio output.
    uint8_t                          audioMix;                           // Channel mix, blend left/right channel sound.
    uint8_t                          displayType;
    uint8_t                          displayOption;
    uint8_t                          displayOutput;
    uint8_t                          vramMode;
    uint8_t                          gramMode;
    uint8_t                          vramWaitMode;
    uint8_t                          pcgMode;
    uint8_t                          aspectRatio;
    uint8_t                          scanDoublerFX;
    uint8_t                          loadDirectFilter;
    uint8_t                          queueTapeFilter;
    uint8_t                          cmtMode;                            // CMT Mode, physical CMT = 0, FPGA CMT = 1
    uint8_t                          fddEnabled;                         // FDD Enabled, 1 = enabled, 0 = disabled.
    uint8_t                          fddImageFilter;                     // Filter to be applied when selecting floppy image files.
    uint8_t                          tapeAutoSave;
    uint8_t                          tapeButtons;
    uint8_t                          fastTapeLoad;
    uint8_t                          cmtAsciiMapping;                    // Enable Sharp<->ASCII name conversion during Record/Play operations.
    uint8_t                          mz800Mode;                          // MZ-800 Mode setting switch.
    uint8_t                          mz800Printer;                       // MZ-800 Printer setting switch.
    uint8_t                          mz800TapeIn;                        // MZ-800 Tape Input setting switch.
    uint8_t                          autoStart;                          // Application autostart on machine instantiation.
    char                             tapeSavePath[MAX_FILENAME_LEN];     // Path where saved files should be stored.
    t_floppyDrive                    fdd[MAX_FLOPPY_DRIVES];             // Entries per floppy drive to maintain image and control data.
    t_romData                        romMonitor40;                       // Details of 40x25 rom monitor image to upload.
    t_romData                        romMonitor80;                       // Details of 80x25 rom monitor image to upload.
    t_romData                        romCG;                              // Details of rom character generator images to upload.
    t_romData                        romKeyMap;                          // Details of rom Key mapping images to upload.
    t_romData                        romUser;                            // Details of User ROM images to upload.
    t_romData                        romFDC;                             // Details of FDC ROM images to upload.
    appData_t                        loadApp;                            // Details of an application to load on machine instantiation.
} t_emuMachineConfig;

// Structure to maintain the emulator configuration which is intended to mirror the physical hardware configuration.
typedef struct {
    enum MACHINE_TYPES               machineModel;                       // Current emulated model.
    enum MACHINE_GROUP               machineGroup;                       // Group to which the current emulated model belongs.
    uint8_t                          machineChanged;                     // Flag to indicate the base machine has changed.
 //   t_emuMachineConfig               *resetParams;                       // Copy of the default parameters.
    t_emuMachineConfig               params[MAX_MZMACHINES];             // Working set of parameters.
    uint8_t                          emuRegisters[MZ_EMU_MAX_REGISTERS]; // Mirror of the emulator register contents for local manipulation prior to sync.
} t_emuConfig;

// Structure for traversing and maintaining a set of active windows as selected by a user.
typedef struct {
    enum MENUACTIVE                  menu[MAX_MENU_DEPTH];               // Flag and menu state to indicate the OSD Menu is being displayed, the active menu and history.
    uint8_t                          activeRow[MAX_MENU_DEPTH];          // Last active row in a given menu.
    uint8_t                          menuIdx;                            // Menu ptr to current menu.
} t_activeMenu;

// Structure for traversing and maintaining as set of directory/sub-directories.
typedef struct {
    char                             *dir[MAX_DIR_DEPTH];                // Entered directory list during user file selection.
    uint8_t                          activeRow[MAX_DIR_DEPTH];           // Last active row in a given directory.
    uint8_t                          dirIdx;                             // Ptr to current active directory in file selection.
} t_activeDir;

// Structure to maintain Sharp MZ Series Emulation control and data.
//
typedef struct {
    uint8_t                          active;                             // An emulation is active in the FPGA.
    enum DIALOGTYPE                  activeDialog;                       // Active dialog on the OSD.
    t_activeMenu                     activeMenu;                         // Active menu tree.
    t_activeDir                      activeDir;                          // Active directory tree.
    uint8_t                          debug;                              // Debug the emuMZ module by outputting log information if set.
    t_menu                           menu;                               // Menu control and data.
    enum MACHINE_HW_TYPES            hostMachine;                        // Host hardware emulation being hosted on.
    t_fileList                       fileList;                           // List of files for perusal and selection during OSD interaction.
    t_tapeHeader                     tapeHeader;                         // Last processed tape details.
    t_tapeQueue                      tapeQueue;                          // Linked list of files which together form a virtual tape.
    t_floppyCtrl                     fdd;                                // Floppy disk drive control.
    t_keyInjectionEdit               keyInjEdit;                         // Control structure for event callback editting of the key injection array.
} t_emuControl;

// Application execution constants.
//

// Lookup table for floppy disk parameter definitions.
const t_floppyDef FLOPPY_DEFINITIONS[16]= { { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 0  40T DS 16S 256B 320K 
                                            { .tracks = 40,     .heads = 2,       .sectors = 8,      .sectorSize = 512,  .rpm = 300 },   // 1  40T DS 8S 512B 320K 
                                            { .tracks = 80,     .heads = 2,       .sectors = 9,      .sectorSize = 512,  .rpm = 300 },   // 2  80T DS 9S 512B 720K 
                                            { .tracks = 80,     .heads = 2,       .sectors = 10,     .sectorSize = 512,  .rpm = 300 },   // 3  80T DS 10S 512B 800K
                                            { .tracks = 80,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 4  80T DS 16S 256B 640K 
                                            { .tracks = 35,     .heads = 2,       .sectors = 10,     .sectorSize = 512,  .rpm = 300 },   // 5  35T DS 10S 512B 350K 
                                            { .tracks = 35,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 6  35T DS 16S 256B 280K 
                                            { .tracks = 40,     .heads = 2,       .sectors = 10,     .sectorSize = 512,  .rpm = 300 },   // 7  40T 2H 10S 512B 400K 
                                            { .tracks = 80,     .heads = 2,       .sectors = 18,     .sectorSize = 512,  .rpm = 300 },   // 8  80T 2H 18S 512B 1440K 
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 9   
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 10  
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 11  
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 12  
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 13  
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 },   // 14  
                                            { .tracks = 40,     .heads = 2,       .sectors = 16,     .sectorSize = 256,  .rpm = 300 } }; // 15  


// Lookup tables for menu entries.
//
const uint8_t MZ_ACTIVE[MAX_MZMACHINES]  = { 1,        1,        1,         1,        1,        1,         1,         1,        1,         1,         0         };
const char *MZMACHINES[MAX_MZMACHINES]   = { "MZ-80K", "MZ-80C", "MZ1200",  "MZ-80A", "MZ-700", "MZ-800", "MZ1500",  "MZ-80B",  "MZ2000",  "MZ2200",  "MZ2500"  };
const char *SHARPMZ_FAST_TAPE[][6]       = { { "Off", "2x", "4x", "8x", "16x", "32x"                       },  // Group MZ80K
                                             { "Off", "2x", "4x", "8x", "16x", "32x"                       },  // Group MZ700
                                             { "Off", "2x", "4x", "8x", "16x", NULL                        }   // Group MZ80B
                                           };
const char *SHARPMZ_CPU_SPEED[][7]       = { { "2MHz",   "4MHz", "8MHz",  "16MHz", "32MHz", "64MHz",  NULL },  // Group MZ80K
                                             { "3.5MHz", "7MHz", "14MHz", "28MHz", "56MHz", NULL,     NULL },  // Group MZ700
                                             { "4MHz",   "8MHz", "16MHz", "32MHz", "64MHz", NULL,     NULL }   // Group MZ80B
                                           };
const char *SHARPMZ_MEM_SIZE[][3]        = { { "32K", "48K",  NULL   }, // 80K
                                             { "32K", "48K",  NULL   }, // 80C
                                             { "32K", "48K",  NULL   }, // 1200
                                             { "32K", "48K",  NULL   }, // 80A
                                             { NULL,  "64K",  NULL   }, // 700
                                             { NULL,  "64K",  NULL   }, // 800
                                             { NULL,  "64K",  NULL   }, // 1500
                                             { "32K", "64K",  NULL   }, // 80B
                                             { NULL,  "64K",  NULL   }, // 2000
                                             { NULL,  "64K",  NULL   }, // 2200
                                             { "64K", "128K", "256K" }  // 2500
                                           };
const char *SHARPMZ_TAPE_MODE[]          = { "FPGA", "MZ CMT" };
const char *SHARPMZ_TAPE_BUTTONS[]       = { "Off", "Play", "Record", "Auto" };
const char *SHARPMZ_ASCII_MAPPING[]      = { "Off", "Record", "Play", "Both" };
const char *SHARPMZ_AUDIO_SOURCE[]       = { "Sound", "Tape" };
const char *SHARPMZ_AUDIO_HARDWARE[]     = { "Host", "FPGA" };
const char *SHARPMZ_AUDIO_VOLUME[]       = { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "Max" };
const char *SHARPMZ_AUDIO_MUTE[]         = { "Off", "Mute" };
const char *SHARPMZ_AUDIO_MIX []         = { "Off", "25%", "50%", "Mono" };
//const char *SHARPMZ_USERROM_ENABLED[]    = { "Disabled",  "Enabled" };
//const char *SHARPMZ_FDCROM_ENABLED[]     = { "Disabled",  "Enabled" };
//const char *SHARPMZ_ROM_ENABLED[]        = { "Disabled",  "Enabled" };
const char *SHARPMZ_DISPLAY_TYPE[][4]    = { 
                                             { "Mono 40x25",       "Mono 80x25 ",  NULL,           NULL                            }, // 80K
                                             { "Mono 40x25",       "Mono 80x25 ",  NULL,           NULL                            }, // 80C
                                             { "Mono 40x25",       "Mono 80x25 ",  NULL,           NULL                            }, // 1200
                                             { "Mono 40x25",       "Mono 80x25 ",  "Colour 40x25", "Colour 80x25"                  }, // 80A
                                             { NULL,               NULL,           "Colour 40x25", "Colour 80x25"                  }, // 700
                                             { NULL,               NULL,           "Colour",       NULL                            }, // 800
                                             { NULL,               NULL,           "Colour 40x25", "Colour 80x25"                  }, // 1500
                                             { NULL,               NULL,           NULL,           NULL                            }, // 80B
                                             { NULL,               NULL,           NULL,           NULL                            }, // 2000
                                             { NULL,               NULL,           NULL,           NULL                            }, // 2200
                                             { NULL,               NULL,           NULL,           NULL                            }  // 2500
                                           };
const char *SHARPMZ_DISPLAY_OPTION[][5]  = { { "None",             NULL,           NULL,           NULL,            NULL           }, // 80K
                                             { "None",             NULL,           NULL,           NULL,            NULL           }, // 80C
                                             { "None",             NULL,           NULL,           NULL,            NULL           }, // 1200
                                             { "None",             "PCG",          NULL,           NULL,            NULL           }, // 80A
                                             { "None",             "PCG",          NULL,           NULL,            NULL           }, // 700
                                             { "None",             "MZ-1R25",      NULL,           NULL,            NULL           }, // 800
                                             { NULL,               "PCG",          NULL,           NULL,            NULL           }, // 1500
                                             { "None",             "GRAMI",        "GRAMI/II",     NULL,            NULL           }, // 80B
                                             { "None",             "GRAMB",        "GRAMB/R", "    GRAMB/G",        "GRAMB/R/G"    }, // 2000
                                             { NULL,               NULL,           NULL,           NULL,            "GRAMB/R/G"    }, // 2200
                                             { "None",             NULL,           NULL,           NULL,            NULL           }  // 2500
                                           };
const char *SHARPMZ_DISPLAY_OUTPUT[]     = { "Original", "Original 50Hz", "640x480@60Hz", "800x600@60Hz" };

const char *SHARPMZ_ASPECT_RATIO[]       = { "4:3", "16:9" };
const char *SHARPMZ_SCANDOUBLER_FX[]     = { "None", "HQ2x", "CRT 25%", "CRT 50%", "CRT 75%" };
const char *SHARPMZ_VRAMWAIT_MODE[]      = { "Off", "On" };
const char *SHARPMZ_VRAMDISABLE_MODE[]   = { "Enabled", "Disabled" };
const char *SHARPMZ_GRAMDISABLE_MODE[]   = { "Enabled", "Disabled" };
//const char *SHARPMZ_GRAM_BASEADDR[]      = { "0x00", "0x08", "0x10", "0x18", "0x20", "0x28", "0x30", "0x38", "0x40", "0x48", "0x50", "0x58", "0x70", "0x78",
//                                             "0x80", "0x88", "0x90", "0x98" };
const char *SHARPMZ_PCG_MODE[]           = { "ROM", "RAM" };
const char *SHARPMZ_AUTOSTART[]          = { "Disabled",  "Enabled" };
const char *SHARPMZ_MEMORY_BANK[]        = { "SysROM",      "SysRAM",      "KeyMap",      "VRAM",      "CMTHDR",       "CMTDATA",       "CGROM",      "CGRAM",      "All" };
//const char *SHARPMZ_MEMORY_BANK_FILE[]   = { "sysrom.dump", "sysram.dump", "keymap.dump", "vram.dump", "cmt_hdr.dump", "cmt_data.dump", "cgrom.dump", "cgram.dump", "all_memory.dump" };
const char *SHARPMZ_TAPE_TYPE[]          = { "N/A", "M/code", "MZ80 Basic", "MZ80 Data", "MZ700 Data", "MZ700 Basic", "Unknown" };
const char *SHARPMZ_FILE_FILTERS[]       = { "*.MZF", "*.MTI", "*.MZT", "*.*" };
const char *SHARPMZ_MZ800_MODE[]         = { "MZ-800", "MZ-700" };
const char *SHARPMZ_MZ800_PRINTER[]      = { "MZ", "Centronics" };
const char *SHARPMZ_MZ800_TAPEIN[]       = { "External", "Internal" };
const char *SHARPMZ_FDD_MODE[]           = { "Disabled", "Enabled" };
const char *SHARPMZ_FDD_DISK_TYPE[]      = { "40T DS 16S 256B 320K",
                                             "40T DS 8S 512B 320K",
                                             "80T DS 9S 512B 720K",
                                             "80T DS 10S 512B 800K",
                                             "80T DS 16S 256B 640K",
                                             "35T DS 10S 512B 350K",
                                             "35T DS 16S 256B 280K",
                                             "40T DS 10S 512B 400K",
                                             "80T DS 18S 512B 1440K",
                                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
const char *SHARPMZ_FDD_IMAGE_POLARITY[] = { "Normal", "Inverted" };
const char *SHARPMZ_FDD_UPDATE_MODE[]    = { "Read/Write", "Read Only" };
const char *SHARPMZ_FDD_FILE_FILTERS[]   = { "*.DSK", "*.D88", "*.IMG", "*.*" };
const char *SHARPMZ_FDD_MOUNT[]          = { "Ejected", "Mounted" };

// Prototypes.
//
void       EMZReleaseMenuMemory(void);
void       EMZReleaseDirMemory(void);
void       EMZSetupMenu(char *, char *, enum FONTS);
void       EMZSetupDirList(char *, char *, enum FONTS);
void       EMZAddToMenu(uint8_t, uint8_t, char *, char, enum MENUTYPES, enum MENUSTATE, t_menuCallback, enum MENUCALLBACK, t_choiceCallback, t_viewCallback);
int16_t    EMZDrawMenu(int16_t, uint8_t, enum MENUMODE);
void       EMZRefreshMenu(void);
void       EMZRefreshFileList(void);
void       EMZMainMenu(void);
void       EMZTapeStorageMenu(enum ACTIONMODE);
void       EMZFloppyStorageMenu(enum ACTIONMODE);
void       EMZMachineMenu(enum ACTIONMODE);
void       EMZDisplayMenu(enum ACTIONMODE);
void       EMZAudioMenu(enum ACTIONMODE);
void       EMZSystemMenu(enum ACTIONMODE);
void       EMZAbout(enum ACTIONMODE);
void       EMZRomManagementMenu(enum ACTIONMODE);
void       EMZAutoStartApplicationMenu(enum ACTIONMODE);
void       EMZRenderPreKeyViewTop(void);
void       EMZRenderPreKeyView(uint16_t);
void       EMZRenderPostKeyViewTop(void);    
void       EMZRenderPostKeyView(uint16_t);    
void       EMZPreKeyEntry(void);
void       EMZPostKeyEntry(void);
void       EMZSwitchToMenu(int8_t);
void       EMZProcessMenuKey(uint8_t, uint8_t);
void       EMZservice(uint8_t);
int        EMZFileSave(const char *, void *, int);
int        EMZFileLoad(const char *, void *, int);
void       EMZReadConfig(enum ACTIONMODE);
void       EMZWriteConfig(enum ACTIONMODE);
void       EMZResetConfig(enum ACTIONMODE);
void       EMZLoadConfig(void);
void       EMZSaveConfig(void);
void       EMZSwitchToMachine(uint8_t, uint8_t);
void       EMZGetFileListBoundaries(int16_t *, int16_t *, int16_t *);
uint16_t   EMZGetFileListColumnWidth(void);
int16_t    EMZDrawFileList(int16_t, uint8_t);
uint8_t    EMZReadDirectory(const char *, const char *);
void       EMZGetFile(void);
void       EMZReset(void);

void       EMZPrintTapeDetails(short);
void       EMZLoadDirectToRAM(enum ACTIONMODE);
void       EMZLoadDirectToRAMSet(char *);
void       EMZQueueTape(enum ACTIONMODE);
void       EMZQueueTapeSet(char *);    
void       EMZQueueClear(enum ACTIONMODE);
void       EMZQueueNext(enum ACTIONMODE);
void       EMZQueuePrev(enum ACTIONMODE);
void       EMZTapeSave(enum ACTIONMODE);
void       EMZTapeSaveSet(char *);
void       EMZMonitorROM40(enum ACTIONMODE);
void       EMZMonitorROM40Set(char *);
void       EMZMonitorROM80(enum ACTIONMODE);
void       EMZMonitorROM80Set(char *);
void       EMZCGROM(enum ACTIONMODE);
void       EMZCGROMSet(char *);
void       EMZKeyMappingROM(enum ACTIONMODE);
void       EMZKeyMappingROMSet(char *);
void       EMZUserROM(enum ACTIONMODE);
void       EMZUserROMSet(char *);
void       EMZFloppyDiskROM(enum ACTIONMODE);
void       EMZFloppyDiskROMSet(char *);
void       EMZLoadApplication(enum ACTIONMODE);
void       EMZLoadApplicationSet(char *);
void       EMZChangeLoadApplication(enum ACTIONMODE);


void       EMZTapeQueuePushFile(char *);
char       *EMZTapeQueuePopFile(uint8_t);
char       *EMZTapeQueueAPSSSearch(char, uint8_t);
char       *EMZNextTapeQueueFilename(char);
uint16_t   EMZClearTapeQueue(void);
void       EMZChangeCMTMode(enum ACTIONMODE);
short      EMZReadTapeDetails(const char *);
short      EMZLoadTapeToRAM(const char *, unsigned char);
short      EMZSaveTapeFromCMT(const char *);
void       EMZProcessTapeQueue(uint8_t);

// Menu choice helper functions, increment to next choice.
void       EMZNextMachineModel(enum ACTIONMODE);
void       EMZNextCPUSpeed(enum ACTIONMODE);
void       EMZNextMemSize(enum ACTIONMODE mode);
void       EMZNextAudioSource(enum ACTIONMODE);
void       EMZNextAudioHardware(enum ACTIONMODE);
void       EMZNextAudioVolume(enum ACTIONMODE);
void       EMZNextAudioMute(enum ACTIONMODE);
void       EMZNextAudioMix(enum ACTIONMODE);

// Getter/Setter methods!
void       EMZSetMenuRowPadding(uint8_t);
void       EMZSetMenuFont(enum FONTS);
void       EMZSetRowColours(enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR);  
void       EMZGetMenuBoundaries(int16_t *, int16_t *, int16_t *, int16_t *, int16_t *);
uint16_t   EMZGetMenuColumnWidth(void);

const char *EMZGetMachineModelChoice(void);
char       *EMZGetMachineTitle(void);
short      EMZGetMachineGroup(void);
const char *EMZGetCPUSpeedChoice(void);
const char *EMZGetMemSizeChoice(void);
const char *EMZGetAudioSourceChoice(void);
const char *EMZGetAudioHardwareChoice(void);
const char *EMZGetAudioVolumeChoice(void);
const char *EMZGetAudioMuteChoice(void);
const char *EMZGetAudioMixChoice(void);
const char *EMZGetCMTModeChoice(void);

const char *EMZGetFDDMountChoice(uint8_t);
const char *EMZGetFDDMount0Choice(void);
const char *EMZGetFDDMount1Choice(void);
const char *EMZGetFDDMount2Choice(void);
const char *EMZGetFDDMount3Choice(void);

const char *EMZGetDisplayTypeChoice(void);
const char *EMZGetDisplayOptionChoice(void);
const char *EMZGetDisplayOutputChoice(void);
const char *EMZGetVRAMModeChoice(void);
const char *EMZGetGRAMModeChoice(void);
const char *EMZGetVRAMWaitModeChoice(void);
const char *EMZGetPCGModeChoice(void);
const char *EMZGetAspectRatioChoice(void);
const char *EMZGetScanDoublerFXChoice(void);
const char *EMZGetLoadDirectFileFilterChoice(void);
const char *EMZGetQueueTapeFileFilterChoice(void);
const char *EMZGetTapeSaveFilePathChoice(void);
const char *EMZGetMonitorROM40Choice(void);
const char *EMZGetMonitorROM80Choice(void);
const char *EMZGetCGROMChoice(void);
const char *EMZGetKeyMappingROMChoice(void);
const char *EMZGetUserROMChoice(void);
const char *EMZGetFloppyDiskROMChoice(void);
const char *EMZGetTapeType(void);
const char *EMZGetLoadApplicationChoice(void);
const char *EMZGetAutoStartChoice(void);
const char *EMZGetMZ800ModeChoice(void);
const char *EMZGetMZ800PrinterChoice(void);
const char *EMZGetMZ800TapeInChoice(void);

void       EMZNextCMTMode(enum ACTIONMODE);
void       EMZNextDisplayType(enum ACTIONMODE);
void       EMZNextDisplayOption(enum ACTIONMODE);
uint8_t    EMZGetDisplayOptionValue(void);
uint8_t    EMZGetMemSizeValue(void);
void       EMZNextDisplayOutput(enum ACTIONMODE);
void       EMZNextVRAMMode(enum ACTIONMODE);
void       EMZNextGRAMMode(enum ACTIONMODE);
void       EMZNextVRAMWaitMode(enum ACTIONMODE);
void       EMZNextPCGMode(enum ACTIONMODE);
void       EMZNextAspectRatio(enum ACTIONMODE);
void       EMZNextScanDoublerFX(enum ACTIONMODE);
void       EMZNextLoadDirectFileFilter(enum ACTIONMODE);
void       EMZNextQueueTapeFileFilter(enum ACTIONMODE);
void       EMZNextMonitorROM40(enum ACTIONMODE);
void       EMZNextMonitorROM80(enum ACTIONMODE);
void       EMZNextCGROM(enum ACTIONMODE);
void       EMZNextKeyMappingROM(enum ACTIONMODE);
void       EMZNextUserROM(enum ACTIONMODE);
void       EMZNextFloppyDiskROM(enum ACTIONMODE);
void       EMZNextLoadApplication(enum ACTIONMODE);
void       EMZNextMZ800Mode(enum ACTIONMODE mode);
void       EMZNextMZ800Printer(enum ACTIONMODE mode);
void       EMZNextMZ800TapeIn(enum ACTIONMODE mode);

const char *EMZGetFDDModeChoice(void);
void       EMZChangeFDDMode(enum ACTIONMODE);
void       EMZNextFDDMode(enum ACTIONMODE);
const char *EMZGetFDDDriveFileFilterChoice(void);
void       EMZFDDSetDriveImage(enum ACTIONMODE, uint8_t);
void       EMZFDDSetDriveImage0(enum ACTIONMODE);
void       EMZFDDSetDriveImage1(enum ACTIONMODE);
void       EMZFDDSetDriveImage2(enum ACTIONMODE);
void       EMZFDDSetDriveImage3(enum ACTIONMODE);
void       EMZFDDDriveImageSet(char *, uint8_t);
void       EMZFDDDriveImage0Set(char *);
void       EMZFDDDriveImage1Set(char *);
void       EMZFDDDriveImage2Set(char *);
void       EMZFDDDriveImage3Set(char *);
void       EMZNextDriveImageFilter(enum ACTIONMODE);
void       EMZMountDrive(enum ACTIONMODE, uint8_t, uint8_t);
void       EMZNextMountDrive0(enum ACTIONMODE);
void       EMZNextMountDrive1(enum ACTIONMODE);
void       EMZNextMountDrive2(enum ACTIONMODE);
void       EMZNextMountDrive3(enum ACTIONMODE);
enum FLOPPYERRORCODES  EMZProcessFDDRequest(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t *, uint16_t *);
short      EMZCheckFDDImage(char *);
short      EMZSetFDDImageParams(char *, uint8_t, enum IMAGETYPES);

void       EMZNextFDDDriveType(enum ACTIONMODE, uint8_t);
void       EMZNextFDDDriveType0(enum ACTIONMODE);
void       EMZNextFDDDriveType1(enum ACTIONMODE);
void       EMZNextFDDDriveType2(enum ACTIONMODE);
void       EMZNextFDDDriveType3(enum ACTIONMODE);
void       EMZNextFDDImagePolarity(enum ACTIONMODE, uint8_t);
void       EMZNextFDDImagePolarity0(enum ACTIONMODE);
void       EMZNextFDDImagePolarity1(enum ACTIONMODE);
void       EMZNextFDDImagePolarity2(enum ACTIONMODE);
void       EMZNextFDDImagePolarity3(enum ACTIONMODE);
void       EMZNextFDDUpdateMode(enum ACTIONMODE, uint8_t);
void       EMZNextFDDUpdateMode0(enum ACTIONMODE);
void       EMZNextFDDUpdateMode1(enum ACTIONMODE);
void       EMZNextFDDUpdateMode2(enum ACTIONMODE);
void       EMZNextFDDUpdateMode3(enum ACTIONMODE);
const char *EMZGetFDDDriveTypeChoice(uint8_t);
const char *EMZGetFDDDriveType0Choice(void);
const char *EMZGetFDDDriveType1Choice(void);
const char *EMZGetFDDDriveType2Choice(void);
const char *EMZGetFDDDriveType3Choice(void);
const char *EMZGetFDDDriveFileChoice(uint8_t);
const char *EMZGetFDDDrive0FileChoice(void);
const char *EMZGetFDDImagePolarityChoice(uint8_t);
const char *EMZGetFDDImagePolarity0Choice(void);
const char *EMZGetFDDImagePolarity1Choice(void);
const char *EMZGetFDDImagePolarity2Choice(void);
const char *EMZGetFDDImagePolarity3Choice(void);
const char *EMZGetFDDUpdateModeChoice(uint8_t);
const char *EMZGetFDDUpdateMode0Choice(void);    
const char *EMZGetFDDUpdateMode1Choice(void);    
const char *EMZGetFDDUpdateMode2Choice(void);    
const char *EMZGetFDDUpdateMode3Choice(void);    

#ifdef __cplusplus
}
#endif
#endif // EMUMZ_H

// Pseudo 'public' method prototypes.
#ifdef __cplusplus
    extern "C" {
#endif

uint8_t    EMZInit(enum MACHINE_HW_TYPES, uint8_t);
void       EMZRun(void);
const char *EMZGetVersion(void);
const char *EMZGetVersionDate(void);

#ifdef __cplusplus
}
#endif
