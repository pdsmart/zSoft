/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            emumz.c
// Created:         May 2021
// Version:         v1.0
// Author(s):       Philip Smart
// Description:     The MZ Emulator Control Logic
//                  This source file contains the logic to present an on screen display menu, interact
//                  with the user to set the config or perform machine actions (tape load) and provide
//                  overall control functionality in order to service the running Sharp MZ Series
//                  emulation.
//                 
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         v1.0 May 2021  - Initial write of the EmuMZ software.
//                  v1.1 Nov 2021  - Further work on Video Controller and infrastructure.
//                  v1.2 Nov 2021  - Adding MZ2000 logic.
//                  v1.3 Dec 2021  - Adding MZ800 logic.
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

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__K64F__)
  #include <stdio.h>
  #include <ctype.h>
  #include <stdint.h>
  #include <string.h>
  #include <unistd.h>
  #include <stdarg.h>
  #include <core_pins.h>
  #include <usb_serial.h>
  #include <Arduino.h>
  #include "k64f_soc.h"
  #include <../../libraries/include/stdmisc.h>
#elif defined(__ZPU__)
  #include <stdint.h>
  #include <stdio.h>
  #include "zpu_soc.h"
  #include <stdlib.h>
  #include <ctype.h>
  #include <stdmisc.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif

#include "ff.h"            // Declarations of FatFs API
#include "diskio.h"
#include "utils.h"
#include <fonts.h>
#include <bitmaps.h>
#include <tranzputer.h>
#include <osd.h>
#include <emumz.h>

// Debug enable.
#define __EMUMZ_DEBUG__       1

// Debug macros
#define debugf(a, ...)        if(emuControl.debug) { printf("\033[1;31mEMUMZ: " a "\033[0m\n", ##__VA_ARGS__); }
#define debugfx(a, ...)       if(emuControl.debug) { printf("\033[1;32mEMUMZ: " a "\033[0m\n", ##__VA_ARGS__); }

// Version data.
#define EMUMZ_VERSION       1.4
#define EMUMZ_VERSION_DATE  "22/12/2021"

//////////////////////////////////////////////////////////////
// Sharp MZ Series Emulation Service Methods                //
//////////////////////////////////////////////////////////////

#ifndef __APP__        // Protected methods which should only reside in the kernel on zOS.
 
// Global scope variables used within the emuMZ core.
//
// First the ROM based default constants. These are used in the initial startup/configuration or configuration reset. They are copied into working memory as needed.
const static t_emuControl          emuControlDefault = {
                                              .active = 0, .debug = 1, .activeDialog = DIALOG_MENU, 
                                              .activeMenu = {
                                                  .menu[0] = MENU_DISABLED, .activeRow[0] = 0, .menuIdx = 0
                                              },
                                              .activeDir = {
                                                  .dir[0] = NULL, .activeRow[0] = 0, .dirIdx = 0
                                              },
                                              .menu = {
                                                  .rowPixelStart = 15, .colPixelStart = 40, .padding = 2, .colPixelsEnd = 12,
                                                  .inactiveFgColour = WHITE, .inactiveBgColour = BLACK, .greyedFgColour = BLUE, .greyedBgColour = BLACK, .textFgColour = PURPLE, .textBgColour = BLACK, .activeFgColour = BLUE, .activeBgColour = WHITE,
                                                  .font = FONT_7X8, .rowFontptr = &font7x8extended,
                                                  .activeRow = -1
                                              },
                                              .fileList = {
                                                  .rowPixelStart = 15, .colPixelStart = 40, .padding = 2, .colPixelsEnd = 12, .selectDir = 0,
                                                  .inactiveFgColour = WHITE, .inactiveBgColour = BLACK, .activeFgColour = BLUE, .activeBgColour = WHITE,
                                                  .font = FONT_5X7, .rowFontptr = &font5x7extended,
                                                  .activeRow = -1
                                              }
                                          };

const static t_emuConfig           emuConfigDefault = {
                                              .machineModel = MZ80K, .machineGroup = GROUP_MZ80K, .machineChanged = 1,
                                              .params[MZ80K] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sp1002.rom",        .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80k_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_80K_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_FDC_ROM_ADDR,                       .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ80C] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sp1002.rom",        .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80c_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_80C_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_FDC_ROM_ADDR,                       .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ1200] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sa1510.rom",        .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80c_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_1200_km.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_FDC_ROM_ADDR,                       .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ80A] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sa1510.rom",        .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "0:\\TZFS\\sa1510-8.rom",      .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80a_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_80A_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_USER_ROM_ADDR,                      .loadSize = 0x00000800 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_FDC_ROM_ADDR,                       .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ700] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_COLOUR,       .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\1z-013a.rom",       .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "0:\\TZFS\\1z-013a-8.rom",     .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz700_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00001000 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_700_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_FDC_ROM_ADDR,                       .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ800] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_COLOUR,       .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz800_ipl.rom",     .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00004000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz800_cgrom.rom",   .romEnabled = 0, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00001000 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_800_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ1500] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_COLOUR,       .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz150_ipl.rom",     .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00004000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz1500_cgrom.rom",  .romEnabled = 0, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00001000 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_1500_km.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ80B] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 2,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz80b_ipl.rom",     .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00000800 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00000800 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80b_cgrom.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_80B_km.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ2000] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 4,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz2000_ipl.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz2000_cgrom.rom",  .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_2000_km.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ2200] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_MONO,         .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz2200-ipl.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz2200_cgrom.rom",  .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_2200_km.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              },
                                              .params[MZ2500] = {
                                                  .cpuSpeed = 0 ,        .memSize = 1,         .audioSource = 0,  .audioHardware = 1,    .audioVolume = 15,    .audioMute = 0,     .audioMix = 0,          .displayType = MZ_EMU_DISPLAY_COLOUR,       .displayOption = 0,                   .displayOutput = VMMODE_VGA_640x480, 
                                                  .vramMode = 0,         .vramWaitMode = 0,    .gramMode = 0,     .pcgMode = 0,          .aspectRatio = 0,     .scanDoublerFX = 0, .loadDirectFilter = 0,
                                                  .mz800Mode = 0,        .mz800Printer = 0,    .mz800TapeIn = 0,  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,   .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,         .autoStart = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz2500-ipl.rom",    .romEnabled = 1, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = MZ_EMU_ROM_ADDR,                           .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz2500_cgrom.rom",  .romEnabled = 1, .loadAddr = MZ_EMU_CGROM_ADDR,                         .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\700_2500_km.rom",   .romEnabled = 1, .loadAddr = MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_MAP_ADDR, .loadSize = 0x00000080 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x000000,                                  .loadSize = 0x00000100 },
                                                  .loadApp      = { .appFileName = "",                            .appEnabled = 0, .preKeyInsertion = {},                                 .postKeyInsertion = {} }
                                              }
                                          };
const static t_scanMap             mapToScanCode[] = { //          MZ-80K                          MZ-80C                          MZ-1200                         MZ-80A                          MZ-700                          MZ-1500                         MZ-800                          MZ-80B                          MZ-2000                         MZ-2200                         MZ-2500
                                                       { 'A',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0xf7, KEY_NOCTRL_BIT }, {    1, 0xf7, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } }, 
                                                       { 'B',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xfe, KEY_NOCTRL_BIT }, {    3, 0xfe, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'C',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xfe, KEY_NOCTRL_BIT }, {    2, 0xfe, KEY_NOCTRL_BIT }, {    4, 0xdf, KEY_NOCTRL_BIT }, {    4, 0xdf, KEY_NOCTRL_BIT }, {    4, 0xdf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'D',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xf7, KEY_NOCTRL_BIT }, {    2, 0xf7, KEY_NOCTRL_BIT }, {    4, 0xef, KEY_NOCTRL_BIT }, {    4, 0xef, KEY_NOCTRL_BIT }, {    4, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'E',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xef, KEY_NOCTRL_BIT }, {    2, 0xef, KEY_NOCTRL_BIT }, {    4, 0xf7, KEY_NOCTRL_BIT }, {    4, 0xf7, KEY_NOCTRL_BIT }, {    4, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'F',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xfb, KEY_NOCTRL_BIT }, {    3, 0xfb, KEY_NOCTRL_BIT }, {    4, 0xfb, KEY_NOCTRL_BIT }, {    4, 0xfb, KEY_NOCTRL_BIT }, {    4, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'G',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xf7, KEY_NOCTRL_BIT }, {    3, 0xf7, KEY_NOCTRL_BIT }, {    4, 0xfd, KEY_NOCTRL_BIT }, {    4, 0xfd, KEY_NOCTRL_BIT }, {    4, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'H',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xfb, KEY_NOCTRL_BIT }, {    4, 0xfb, KEY_NOCTRL_BIT }, {    4, 0xfe, KEY_NOCTRL_BIT }, {    4, 0xfe, KEY_NOCTRL_BIT }, {    4, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'I',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xdf, KEY_NOCTRL_BIT }, {    4, 0xdf, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'J',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xf7, KEY_NOCTRL_BIT }, {    4, 0xf7, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'K',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    3, 0xdf, KEY_NOCTRL_BIT }, {    3, 0xdf, KEY_NOCTRL_BIT }, {    3, 0xdf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'L',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xf7, KEY_NOCTRL_BIT }, {    5, 0xf7, KEY_NOCTRL_BIT }, {    3, 0xef, KEY_NOCTRL_BIT }, {    3, 0xef, KEY_NOCTRL_BIT }, {    3, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'M',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xfe, KEY_NOCTRL_BIT }, {    5, 0xfe, KEY_NOCTRL_BIT }, {    3, 0xf7, KEY_NOCTRL_BIT }, {    3, 0xf7, KEY_NOCTRL_BIT }, {    3, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'N',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xfd, KEY_NOCTRL_BIT }, {    4, 0xfd, KEY_NOCTRL_BIT }, {    3, 0xfb, KEY_NOCTRL_BIT }, {    3, 0xfb, KEY_NOCTRL_BIT }, {    3, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'O',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    3, 0xfd, KEY_NOCTRL_BIT }, {    3, 0xfd, KEY_NOCTRL_BIT }, {    3, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'P',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xdf, KEY_NOCTRL_BIT }, {    5, 0xdf, KEY_NOCTRL_BIT }, {    3, 0xfe, KEY_NOCTRL_BIT }, {    3, 0xfe, KEY_NOCTRL_BIT }, {    3, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'Q',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'R',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xdf, KEY_NOCTRL_BIT }, {    2, 0xdf, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'S',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xfb, KEY_NOCTRL_BIT }, {    2, 0xfb, KEY_NOCTRL_BIT }, {    2, 0xdf, KEY_NOCTRL_BIT }, {    2, 0xdf, KEY_NOCTRL_BIT }, {    2, 0xdf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'T',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xef, KEY_NOCTRL_BIT }, {    3, 0xef, KEY_NOCTRL_BIT }, {    2, 0xef, KEY_NOCTRL_BIT }, {    2, 0xef, KEY_NOCTRL_BIT }, {    2, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'U',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xef, KEY_NOCTRL_BIT }, {    4, 0xef, KEY_NOCTRL_BIT }, {    2, 0xf7, KEY_NOCTRL_BIT }, {    2, 0xf7, KEY_NOCTRL_BIT }, {    2, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'V',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xfd, KEY_NOCTRL_BIT }, {    3, 0xfd, KEY_NOCTRL_BIT }, {    2, 0xfb, KEY_NOCTRL_BIT }, {    2, 0xfb, KEY_NOCTRL_BIT }, {    2, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'W',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0xdf, KEY_NOCTRL_BIT }, {    1, 0xdf, KEY_NOCTRL_BIT }, {    2, 0xfd, KEY_NOCTRL_BIT }, {    2, 0xfd, KEY_NOCTRL_BIT }, {    2, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'X',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xfd, KEY_NOCTRL_BIT }, {    2, 0xfd, KEY_NOCTRL_BIT }, {    2, 0xfe, KEY_NOCTRL_BIT }, {    2, 0xfe, KEY_NOCTRL_BIT }, {    2, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'Y',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xdf, KEY_NOCTRL_BIT }, {    3, 0xdf, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 'Z',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0xfe, KEY_NOCTRL_BIT }, {    1, 0xfe, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                                                                                                                                                                                                                                                              
                                                       { '0',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '1',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '2',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '3',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xdf, KEY_NOCTRL_BIT }, {    5, 0xdf, KEY_NOCTRL_BIT }, {    5, 0xdf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '4',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '5',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xf7, KEY_NOCTRL_BIT }, {    5, 0xf7, KEY_NOCTRL_BIT }, {    5, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '6',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '7',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '8',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_NOCTRL_BIT }, {    5, 0xfe, KEY_NOCTRL_BIT }, {    5, 0xfe, KEY_NOCTRL_BIT }, {    5, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '9',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                                                                                                                                                                                                                                                              
                                                       { '_',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0x7f, KEY_SHIFT_BIT  }, {    5, 0x7f, KEY_SHIFT_BIT  }, {    0, 0xdf, KEY_NOCTRL_BIT }, {    0, 0xdf, KEY_NOCTRL_BIT }, {    0, 0xdf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '!',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0xbf, KEY_SHIFT_BIT  }, {    1, 0xbf, KEY_SHIFT_BIT  }, {    5, 0x7f, KEY_SHIFT_BIT  }, {    5, 0x7f, KEY_SHIFT_BIT  }, {    5, 0x7f, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '"',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    1, 0x7f, KEY_SHIFT_BIT  }, {    1, 0x7f, KEY_SHIFT_BIT  }, {    5, 0xbf, KEY_SHIFT_BIT  }, {    5, 0xbf, KEY_SHIFT_BIT  }, {    5, 0xbf, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '#',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0xbf, KEY_SHIFT_BIT  }, {    2, 0xbf, KEY_SHIFT_BIT  }, {    5, 0xdf, KEY_SHIFT_BIT  }, {    5, 0xdf, KEY_SHIFT_BIT  }, {    5, 0xdf, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '$',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    2, 0x7f, KEY_SHIFT_BIT  }, {    2, 0x7f, KEY_SHIFT_BIT  }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, {    5, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '%',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0xbf, KEY_SHIFT_BIT  }, {    3, 0xbf, KEY_SHIFT_BIT  }, {    5, 0xf7, KEY_SHIFT_BIT  }, {    5, 0xf7, KEY_SHIFT_BIT  }, {    5, 0xf7, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '&',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    3, 0x7f, KEY_SHIFT_BIT  }, {    3, 0x7f, KEY_SHIFT_BIT  }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, {    5, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '\'', { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xbf, KEY_SHIFT_BIT  }, {    4, 0xbf, KEY_SHIFT_BIT  }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '(',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0x7f, KEY_SHIFT_BIT  }, {    4, 0x7f, KEY_SHIFT_BIT  }, {    5, 0xfe, KEY_SHIFT_BIT  }, {    5, 0xfe, KEY_SHIFT_BIT  }, {    5, 0xfe, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ')',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xbf, KEY_SHIFT_BIT  }, {    5, 0xbf, KEY_SHIFT_BIT  }, {    6, 0xfb, KEY_SHIFT_BIT  }, {    6, 0xfb, KEY_SHIFT_BIT  }, {    6, 0xfb, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '^',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '~',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_SHIFT_BIT  }, {    6, 0x7f, KEY_SHIFT_BIT  }, {    6, 0xbf, KEY_SHIFT_BIT  }, {    6, 0xbf, KEY_SHIFT_BIT  }, {    6, 0xbf, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '-',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_NOCTRL_BIT }, {    1, 0xdf, KEY_SHIFT_BIT  }, {    1, 0xdf, KEY_SHIFT_BIT  }, {    1, 0xdf, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '=',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xbf, KEY_SHIFT_BIT  }, {    6, 0xbf, KEY_SHIFT_BIT  }, {    6, 0xdf, KEY_SHIFT_BIT  }, {    6, 0xdf, KEY_SHIFT_BIT  }, {    6, 0xdf, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '\\', { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xbf, KEY_NOCTRL_BIT }, {    7, 0xbf, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, {    6, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '|',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xbf, KEY_SHIFT_BIT  }, {    7, 0xbf, KEY_SHIFT_BIT  }, {    6, 0x7f, KEY_SHIFT_BIT  }, {    6, 0x7f, KEY_SHIFT_BIT  }, {    6, 0x7f, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '[',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xdf, KEY_NOCTRL_BIT }, {    6, 0xdf, KEY_NOCTRL_BIT }, {    1, 0xef, KEY_NOCTRL_BIT }, {    1, 0xef, KEY_NOCTRL_BIT }, {    1, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '{',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xdf, KEY_SHIFT_BIT  }, {    6, 0xdf, KEY_SHIFT_BIT  }, {    1, 0xef, KEY_SHIFT_BIT  }, {    1, 0xef, KEY_SHIFT_BIT  }, {    1, 0xef, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ']',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xfb, KEY_NOCTRL_BIT }, {    7, 0xfb, KEY_NOCTRL_BIT }, {    1, 0xf7, KEY_NOCTRL_BIT }, {    1, 0xf7, KEY_NOCTRL_BIT }, {    1, 0xf7, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '}',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xfb, KEY_SHIFT_BIT  }, {    7, 0xfb, KEY_SHIFT_BIT  }, {    1, 0xf7, KEY_SHIFT_BIT  }, {    1, 0xf7, KEY_SHIFT_BIT  }, {    1, 0xf7, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ':',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_NOCTRL_BIT }, {    0, 0xfd, KEY_NOCTRL_BIT }, {    0, 0xfd, KEY_NOCTRL_BIT }, {    0, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '*',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xf7, KEY_SHIFT_BIT  }, {    6, 0xf7, KEY_SHIFT_BIT  }, {    0, 0xfd, KEY_SHIFT_BIT  }, {    0, 0xfd, KEY_SHIFT_BIT  }, {    0, 0xfd, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ';',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_NOCTRL_BIT }, {    0, 0xfb, KEY_NOCTRL_BIT }, {    0, 0xfb, KEY_NOCTRL_BIT }, {    0, 0xfb, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '+',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xfb, KEY_SHIFT_BIT  }, {    6, 0xfb, KEY_SHIFT_BIT  }, {    0, 0xfb, KEY_SHIFT_BIT  }, {    0, 0xfb, KEY_SHIFT_BIT  }, {    0, 0xfb, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ',',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_NOCTRL_BIT }, {    6, 0xfd, KEY_NOCTRL_BIT }, {    6, 0xfd, KEY_NOCTRL_BIT }, {    6, 0xfd, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '<',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    5, 0xfd, KEY_SHIFT_BIT  }, {    5, 0xfd, KEY_SHIFT_BIT  }, {    6, 0xfd, KEY_SHIFT_BIT  }, {    6, 0xfd, KEY_SHIFT_BIT  }, {    6, 0xfd, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '.',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '>',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xfe, KEY_SHIFT_BIT  }, {    6, 0xfe, KEY_SHIFT_BIT  }, {    6, 0xfe, KEY_SHIFT_BIT  }, {    6, 0xfe, KEY_SHIFT_BIT  }, {    6, 0xfe, KEY_SHIFT_BIT  }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '/',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    6, 0xfd, KEY_NOCTRL_BIT }, {    6, 0xfd, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { '?',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, {    7, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 0x0d, { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    7, 0xf7, KEY_NOCTRL_BIT }, {    7, 0xf7, KEY_NOCTRL_BIT }, {    0, 0xfe, KEY_NOCTRL_BIT }, {    0, 0xfe, KEY_NOCTRL_BIT }, {    0, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { ' ',  { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    4, 0xfe, KEY_NOCTRL_BIT }, {    4, 0xfe, KEY_NOCTRL_BIT }, {    6, 0xef, KEY_NOCTRL_BIT }, {    6, 0xef, KEY_NOCTRL_BIT }, {    6, 0xef, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 0xf8, { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    0, 0xfe, KEY_NOCTRL_BIT }, {    0, 0xfe, KEY_NOCTRL_BIT }, {    8, 0xfe, KEY_NOCTRL_BIT }, {    8, 0xfe, KEY_NOCTRL_BIT }, {    8, 0xfe, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 0xf9, { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    0, 0x7f, KEY_NOCTRL_BIT }, {    0, 0x7f, KEY_NOCTRL_BIT }, {    8, 0xbf, KEY_NOCTRL_BIT }, {    8, 0xbf, KEY_NOCTRL_BIT }, {    8, 0xbf, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                                       { 0xfa, { { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, {    0, 0x7f, KEY_NOCTRL_BIT }, {    0, 0x7f, KEY_NOCTRL_BIT }, {    8, 0x7f, KEY_NOCTRL_BIT }, {    8, 0x7f, KEY_NOCTRL_BIT }, {    8, 0x7f, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT }, { 0xff, 0xff, KEY_NOCTRL_BIT } } },
                                          };

// Configuration working structures. Declared static rather than malloc'd as they are used so often and malloc doesnt offer any benefit for an integral data block.
static t_emuControl          emuControl;
static t_emuConfig           emuConfig;

// Real time millisecond counter, interrupt driven. Needs to be volatile in order to prevent the compiler optimising it away.
uint32_t volatile            *ms = &systick_millis_count;

// Method to return the emulation control software version.
static const char version[8];
const char *EMZGetVersion(void)
{
    sprintf(version, "v%.2f", EMUMZ_VERSION);
    return(version);
}

// Method to return the emulation control software version date.
static const char versionDate[sizeof(EMUMZ_VERSION_DATE)+1];
const char *EMZGetVersionDate(void)
{
    sprintf(versionDate, "%s", EMUMZ_VERSION_DATE);
    return(versionDate);
}

// Method to lookup a given key for a given machine and if found return the keyboard row/col scan codes and any key modifier.
//
t_numCnv EMZMapToScanCode(enum MACHINE_HW_TYPES machine, uint8_t key)
{
    // Locals.
    uint16_t    idx;
    uint8_t     row   = 0xff, shiftRow = 0xff, ctrlRow = 0xff, breakRow = 0xff;
    uint8_t     col   = 0xff, shiftCol = 0xff, ctrlCol = 0xff, breakCol = 0xff;
    uint8_t     mod   = 0;
    t_numCnv    result;

    // Loop through the lookup table, based on the host layout, and try to find a key match.
    for(idx=0; idx < NUMELEM(mapToScanCode); idx++)
    {
        // Key matched?
        if(mapToScanCode[idx].key == toupper(key))
        {
            row = mapToScanCode[idx].code[machine].scanRow;
            col = mapToScanCode[idx].code[machine].scanCol;
            mod = mapToScanCode[idx].code[machine].scanCtrl;
        }

        // Match SHIFT?
        if(mapToScanCode[idx].key == 0xf8)
        {
            shiftRow = mapToScanCode[idx].code[machine].scanRow;
            shiftCol = mapToScanCode[idx].code[machine].scanCol;
        }
        
        // Match CTRL?
        if(mapToScanCode[idx].key == 0xf9)
        {
            ctrlRow = mapToScanCode[idx].code[machine].scanRow;
            ctrlCol = mapToScanCode[idx].code[machine].scanCol;
        }
       
        // Match BREAK?
        if(mapToScanCode[idx].key == 0xfa)
        {
            breakRow = mapToScanCode[idx].code[machine].scanRow;
            breakCol = mapToScanCode[idx].code[machine].scanCol;
        }
    }
    // Lower case keys arent stored in the table so update the shift modifier if lower case.
    if(row != 0xff && key >= 'a' && key <= 'z')
    {
        mod = KEY_SHIFT_BIT;
    }

    // Put data into correct part of the 32bit return word. 0 = Key Row, 1 = Key Col, 2 = Modifier Row, 3 = Modifier Col. 0xff = not valid.
    result.b[0] = row;
    result.b[1] = col;
    result.b[2] = mod == KEY_SHIFT_BIT ? shiftRow : mod == KEY_CTRL_BIT ? ctrlRow : mod == KEY_BREAK_BIT ? breakRow : 0xff;
    result.b[3] = mod == KEY_SHIFT_BIT ? shiftCol : mod == KEY_CTRL_BIT ? ctrlCol : mod == KEY_BREAK_BIT ? breakCol : 0xff;
   
    // Return result.
    return(result);
}

// Method to set the menu row padding (ie. pixel spacing above/below the characters).
void EMZSetMenuRowPadding(uint8_t padding)
{
    // Sanity check.
    //
    if(padding >  ((uint16_t)OSDGet(ACTIVE_MAX_Y) / 8))
        return;

    // Store padding in private member.
    emuControl.menu.padding = padding;
    return;
}

// Method to set the font for use in row characters.
//
void EMZSetMenuFont(enum FONTS font)
{
    emuControl.menu.rowFontptr = OSDGetFont(font);
    emuControl.menu.font       = font;
}

// Method to change the row active colours.
//
void EMZSetRowColours(enum COLOUR rowFg, enum COLOUR rowBg, enum COLOUR greyedFg, enum COLOUR greyedBg, enum COLOUR textFg, enum COLOUR textBg, enum COLOUR activeFg, enum COLOUR activeBg)
{
    emuControl.menu.inactiveFgColour = rowFg;
    emuControl.menu.inactiveBgColour = rowBg;
    emuControl.menu.greyedFgColour   = greyedFg;
    emuControl.menu.greyedBgColour   = greyedBg;
    emuControl.menu.textFgColour     = textFg;
    emuControl.menu.textBgColour     = textBg;
    emuControl.menu.activeFgColour   = activeFg;
    emuControl.menu.activeBgColour   = activeBg;
}

// Method to get the maximum number of columns available for a menu row with the current selected font.
//
uint16_t EMZGetMenuColumnWidth(void)
{
    uint16_t maxPixels = OSDGet(ACTIVE_MAX_X);
    return( (maxPixels - emuControl.menu.colPixelStart - emuControl.menu.colPixelsEnd) / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing) );
}

// Get the group to which the current machine belongs:
// 0 - MZ80K/C/A type
// 1 - MZ700 type
// 2 - MZ80B/2000 type
//
short EMZGetMachineGroup(void)
{
    short machineGroup = 0;

    // Set value according to machine model.
    //
    switch(emuConfig.machineModel)
    {
        // These machines currently underdevelopment, so fall through to MZ80K
        case MZ80B:  
        case MZ2000:
        case MZ2200:
        case MZ2500:
            machineGroup = GROUP_MZ80B;
            break;

        case MZ80K:
        case MZ80C:
        case MZ1200:
        case MZ80A:
            machineGroup = GROUP_MZ80K;
            break;

        case MZ700:
        case MZ1500:
        case MZ800:
            machineGroup = GROUP_MZ700;
            break;

        default:
            machineGroup = GROUP_MZ80K;
            break;
    }

    return(machineGroup);
}

// Method to return a char string which represents the current selected machine name.
const char *EMZGetMachineModelChoice(void)
{
    // Locals.
    //

    return(MZMACHINES[emuConfig.machineModel]);
}

// Method to make the side bar title from the active machine.
char *EMZGetMachineTitle(void)
{
    static char title[MAX_MACHINE_TITLE_LEN];

    sprintf(title, "SHARP %s", EMZGetMachineModelChoice());
    return(title);
}


// Method to change the emulated machine, choice based on the actual implemented machines in the FPGA core.
void EMZNextMachineModel(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        // Forward to next active machine - skip machines under development or not instantiated.
        do {
            emuConfig.machineModel = (emuConfig.machineModel+1 >= MAX_MZMACHINES ? 0 : emuConfig.machineModel+1);
            emuConfig.machineGroup = EMZGetMachineGroup();
        } while(MZ_ACTIVE[emuConfig.machineModel] == 0);
        emuConfig.machineChanged = 1;
       
        // Need to rewrite the menu as the choice will affect displayed items.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
    return;
}

// Method to return a char string which represents the current selected CPU speed.
const char *EMZGetCPUSpeedChoice(void)
{
    // Locals.
    //

    return(SHARPMZ_CPU_SPEED[emuConfig.machineGroup][emuConfig.params[emuConfig.machineModel].cpuSpeed]);
}

// Method to change the CPU Speed, choice based on the actual selected machine.
void EMZNextCPUSpeed(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].cpuSpeed = (emuConfig.params[emuConfig.machineModel].cpuSpeed+1 >= NUMELEM(SHARPMZ_CPU_SPEED[emuConfig.machineGroup]) || SHARPMZ_CPU_SPEED[emuConfig.machineGroup][emuConfig.params[emuConfig.machineModel].cpuSpeed+1] == NULL ? 0 : emuConfig.params[emuConfig.machineModel].cpuSpeed+1);
    }
    return;
}

// Method to return a char string which represents the current selected memory size.
const char *EMZGetMemSizeChoice(void)
{
    // Locals.
    //

    return(SHARPMZ_MEM_SIZE[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].memSize]);
}

// Method to change the memory size, choice based on the actual selected machine.
void EMZNextMemSize(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        // Move to the next valid entry, looping round as necessary.
        do {
            emuConfig.params[emuConfig.machineModel].memSize = (emuConfig.params[emuConfig.machineModel].memSize+1 >= NUMELEM(SHARPMZ_MEM_SIZE[emuConfig.machineModel]) ? 0 : emuConfig.params[emuConfig.machineModel].memSize+1);
        } while(SHARPMZ_MEM_SIZE[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].memSize] == NULL);
    }
    return;
}

// Method to convert the memory size into a bit value for uploading to hardware. Normally a 1:1 but allow leeway for deviations.
uint8_t EMZGetMemSizeValue(void)
{
    // Locals.
    uint8_t  result;

    // Decode according to machine selected.
    //
    switch(emuConfig.machineModel)
    {
        case MZ80K:
        case MZ80C: 
        case MZ1200:
        case MZ80A:
        case MZ700:
        case MZ1500:
        case MZ800: 
        case MZ80B:
        case MZ2000:
        case MZ2200: 
            result = emuConfig.params[emuConfig.machineModel].memSize;
           break;

        case MZ2500: 
           result = 0x00;
           break;

    }
    return(result);
}

// Method to return a char string which represents the current selected MZ800 Mode.
const char *EMZGetMZ800ModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_MZ800_MODE[emuConfig.params[emuConfig.machineModel].mz800Mode]);
}

// Method to change the MZ800 Mode.
void EMZNextMZ800Mode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].mz800Mode = (emuConfig.params[emuConfig.machineModel].mz800Mode+1 >= NUMELEM(SHARPMZ_MZ800_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].mz800Mode+1);
    }
    return;
}

// Method to return a char string which represents the current selected MZ800 Printer setting.
const char *EMZGetMZ800PrinterChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_MZ800_PRINTER[emuConfig.params[emuConfig.machineModel].mz800Printer]);
}

// Method to change the MZ800 Printer setting.
void EMZNextMZ800Printer(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].mz800Printer = (emuConfig.params[emuConfig.machineModel].mz800Printer+1 >= NUMELEM(SHARPMZ_MZ800_PRINTER) ? 0 : emuConfig.params[emuConfig.machineModel].mz800Printer+1);
    }
    return;
}

// Method to return a char string which represents the current selected MZ800 Printer setting.
const char *EMZGetMZ800TapeInChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_MZ800_TAPEIN[emuConfig.params[emuConfig.machineModel].mz800TapeIn]);
}

// Method to change the MZ800 Tape Input setting.
void EMZNextMZ800TapeIn(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].mz800TapeIn = (emuConfig.params[emuConfig.machineModel].mz800TapeIn+1 >= NUMELEM(SHARPMZ_MZ800_TAPEIN) ? 0 : emuConfig.params[emuConfig.machineModel].mz800TapeIn+1);
    }
    return;
}

// Method to return a char string which represents the current selected Audio Source.
const char *EMZGetAudioSourceChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUDIO_SOURCE[emuConfig.params[emuConfig.machineModel].audioSource]);
}

// Method to change the Audio Source, choice based on the actual selected machine.
void EMZNextAudioSource(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].audioSource = (emuConfig.params[emuConfig.machineModel].audioSource+1 >= NUMELEM(SHARPMZ_AUDIO_SOURCE) ? 0 : emuConfig.params[emuConfig.machineModel].audioSource+1);
    }
    return;
}

// Method to return a char string which represents the current selected Audio Hardware Driver.
const char *EMZGetAudioHardwareChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUDIO_HARDWARE[emuConfig.params[emuConfig.machineModel].audioHardware]);
}

// Method to change the Audio Hardware Driver, choice based on the actual selected machine.
void EMZNextAudioHardware(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].audioHardware = (emuConfig.params[emuConfig.machineModel].audioHardware+1 >= NUMELEM(SHARPMZ_AUDIO_HARDWARE) ? 0 : emuConfig.params[emuConfig.machineModel].audioHardware+1);
    }
  
    // Need to rewrite the menu as the choice will affect displayed items.
    EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    return;
}

// Method to return a char string which represents the current selected Audio Volume.
const char *EMZGetAudioVolumeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUDIO_VOLUME[emuConfig.params[emuConfig.machineModel].audioVolume]);
}

// Method to change the Audio Volume, choice based on the actual selected machine.
void EMZNextAudioVolume(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].audioVolume = (emuConfig.params[emuConfig.machineModel].audioVolume+1 >= NUMELEM(SHARPMZ_AUDIO_VOLUME) ? 0 : emuConfig.params[emuConfig.machineModel].audioVolume+1);
    }
    return;
}

// Method to return a char string which represents the current selected Audio Mute.
const char *EMZGetAudioMuteChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUDIO_MUTE[emuConfig.params[emuConfig.machineModel].audioMute]);
}

// Method to change the Audio Mute, choice based on the actual selected machine.
void EMZNextAudioMute(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].audioMute = (emuConfig.params[emuConfig.machineModel].audioMute+1 >= NUMELEM(SHARPMZ_AUDIO_MUTE) ? 0 : emuConfig.params[emuConfig.machineModel].audioMute+1);
    }
    return;
}

// Method to return a char string which represents the current selected Audio channel mix.
const char *EMZGetAudioMixChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUDIO_MIX[emuConfig.params[emuConfig.machineModel].audioMix]);
}

// Method to change the Audio channel mix, choice based on the actual selected machine.
void EMZNextAudioMix(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].audioMix = (emuConfig.params[emuConfig.machineModel].audioMix+1 >= NUMELEM(SHARPMZ_AUDIO_MIX) ? 0 : emuConfig.params[emuConfig.machineModel].audioMix+1);
    }
    return;
}

// Method to return a char string which represents the current selected Display Type.
const char *EMZGetDisplayTypeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_DISPLAY_TYPE[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].displayType]);
}

// Method to change the Display Type, choice based on the actual selected machine.
void EMZNextDisplayType(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        // Move to the next valid entry, looping round as necessary.
        do {
            emuConfig.params[emuConfig.machineModel].displayType = (emuConfig.params[emuConfig.machineModel].displayType+1 >= NUMELEM(SHARPMZ_DISPLAY_TYPE[emuConfig.machineModel]) ? 0 : emuConfig.params[emuConfig.machineModel].displayType+1);
printf("%d\n", emuConfig.params[emuConfig.machineModel].displayType);
        } while(SHARPMZ_DISPLAY_TYPE[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].displayType] == NULL);
    }
    return;
}

// Method to return a char string which represents the current selected Display Option installed.
const char *EMZGetDisplayOptionChoice(void)
{
    // Locals.
    //

    return(SHARPMZ_DISPLAY_OPTION[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].displayOption]);
}

// Method to change the installed display option, choice based on the actual selected machine.
void EMZNextDisplayOption(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        // Move to the next valid entry, looping round as necessary.
        do {
            emuConfig.params[emuConfig.machineModel].displayOption = (emuConfig.params[emuConfig.machineModel].displayOption+1 >= NUMELEM(SHARPMZ_DISPLAY_OPTION[emuConfig.machineModel]) ? 0 : emuConfig.params[emuConfig.machineModel].displayOption+1);
        } while(SHARPMZ_DISPLAY_OPTION[emuConfig.machineModel][emuConfig.params[emuConfig.machineModel].displayOption] == NULL);

        // Need to rewrite the menu as the choice will affect displayed items.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
    return;
}

// Method to translate the selected options into an option byte which can be written to hardware. This mechanism needs to be table driven eventually!
//
uint8_t EMZGetDisplayOptionValue(void)
{
    // Locals.
    uint8_t  result;

    // Decode according to machine selected.
    //
    switch(emuConfig.machineModel)
    {
        case MZ80K:
        case MZ80C: 
        case MZ1200:
           result = 0;
           break;

        case MZ80A:
        case MZ700:
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 1 ? 0x08 : 0x00;
           break;

        case MZ1500:
           result = 0x08;
           break;

        case MZ800: 
           result = emuConfig.params[emuConfig.machineModel].displayOption == 1  ? 0x10 : 0x00;
           break;

        case MZ80B:
           result = 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 1 ? 0x01 : 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 2 ? 0x03 : 0x00;
           printf("displayOption=%d,%d\n",  emuConfig.params[emuConfig.machineModel].displayOption, result);
           break;

        case MZ2000:
           result = 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 1 ? 0x01 : 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 2 ? 0x03 : 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 3 ? 0x05 : 0x00;
           result |= emuConfig.params[emuConfig.machineModel].displayOption == 4 ? 0x07 : 0x00;
           break;

        case MZ2200: 
           result = 0x07;
           break;

        case MZ2500: 
           result = 0x00;
           break;

    }
    return(result);
}

// Method to return a char string which represents the current selected Display Output.
const char *EMZGetDisplayOutputChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_DISPLAY_OUTPUT[emuConfig.params[emuConfig.machineModel].displayOutput]);
}

// Method to change the Display Output, choice based on the actual selected machine.
void EMZNextDisplayOutput(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].displayOutput = (emuConfig.params[emuConfig.machineModel].displayOutput+1 >= NUMELEM(SHARPMZ_DISPLAY_OUTPUT) ? 0 : emuConfig.params[emuConfig.machineModel].displayOutput+1);
    }
    return;
}

// Method to return a char string which represents the current selected VRAM Mode.
const char *EMZGetVRAMModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_VRAMDISABLE_MODE[emuConfig.params[emuConfig.machineModel].vramMode]);
}

// Method to change the VRAM Mode, choice based on the actual selected machine.
void EMZNextVRAMMode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].vramMode = (emuConfig.params[emuConfig.machineModel].vramMode+1 >= NUMELEM(SHARPMZ_VRAMDISABLE_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].vramMode+1);
    }
    return;
}

// Method to return a char string which represents the current selected GRAM Mode.
const char *EMZGetGRAMModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_GRAMDISABLE_MODE[emuConfig.params[emuConfig.machineModel].gramMode]);
}

// Method to change the GRAM Mode, choice based on the actual selected machine.
void EMZNextGRAMMode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].gramMode = (emuConfig.params[emuConfig.machineModel].gramMode+1 >= NUMELEM(SHARPMZ_GRAMDISABLE_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].gramMode+1);
    }
    return;
}

// Method to return a char string which represents the current selected VRAM CPU Wait Mode.
const char *EMZGetVRAMWaitModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_VRAMWAIT_MODE[emuConfig.params[emuConfig.machineModel].vramWaitMode]);
}

// Method to change the VRAM Wait Mode, choice based on the actual selected machine.
void EMZNextVRAMWaitMode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].vramWaitMode = (emuConfig.params[emuConfig.machineModel].vramWaitMode+1 >= NUMELEM(SHARPMZ_VRAMWAIT_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].vramWaitMode+1);
    }
    return;
}

// Method to return a char string which represents the current selected PCG Mode.
const char *EMZGetPCGModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_PCG_MODE[emuConfig.params[emuConfig.machineModel].pcgMode]);
}

// Method to change the PCG Mode, choice based on the actual selected machine.
void EMZNextPCGMode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].pcgMode = (emuConfig.params[emuConfig.machineModel].pcgMode+1 >= NUMELEM(SHARPMZ_PCG_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].pcgMode+1);
    }
    return;
}

// Method to return a char string which represents the current selected Aspect Ratio.
const char *EMZGetAspectRatioChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_ASPECT_RATIO[emuConfig.params[emuConfig.machineModel].aspectRatio]);
}

// Method to change the Aspect Ratio, choice based on the actual selected machine.
void EMZNextAspectRatio(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].aspectRatio = (emuConfig.params[emuConfig.machineModel].aspectRatio+1 >= NUMELEM(SHARPMZ_ASPECT_RATIO) ? 0 : emuConfig.params[emuConfig.machineModel].aspectRatio+1);
    }
    return;
}

// Method to return a char string which represents the current selected Scan Doubler.
const char *EMZGetScanDoublerFXChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_SCANDOUBLER_FX[emuConfig.params[emuConfig.machineModel].scanDoublerFX]);
}

// Method to change the Scan Doubler FX, choice based on the actual selected machine.
void EMZNextScanDoublerFX(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].scanDoublerFX = (emuConfig.params[emuConfig.machineModel].scanDoublerFX+1 >= NUMELEM(SHARPMZ_SCANDOUBLER_FX) ? 0 : emuConfig.params[emuConfig.machineModel].scanDoublerFX+1);
    }
    return;
}

// Method to return a char string which represents the current file filter.
const char *EMZGetLoadDirectFileFilterChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_FILE_FILTERS[emuConfig.params[emuConfig.machineModel].loadDirectFilter]);
}

// Method to change the Load Direct to RAM file filter, choice based on the actual selected machine.
void EMZNextLoadDirectFileFilter(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].loadDirectFilter = (emuConfig.params[emuConfig.machineModel].loadDirectFilter+1 >= NUMELEM(SHARPMZ_FILE_FILTERS) ? 0 : emuConfig.params[emuConfig.machineModel].loadDirectFilter+1);
    }
    return;
}

// Method to return a char string which represents the current Tape Queueing file filter.
const char *EMZGetQueueTapeFileFilterChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_FILE_FILTERS[emuConfig.params[emuConfig.machineModel].queueTapeFilter]);
}

// Method to change the Queue Tape file filter, choice based on the actual selected machine.
void EMZNextQueueTapeFileFilter(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].queueTapeFilter = (emuConfig.params[emuConfig.machineModel].queueTapeFilter+1 >= NUMELEM(SHARPMZ_FILE_FILTERS) ? 0 : emuConfig.params[emuConfig.machineModel].queueTapeFilter+1);
    }
    return;
}

// Method to return a char string which represents the current selected tape auto save mode.
const char *EMZGetTapeAutoSaveChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_TAPE_AUTO_SAVE[emuConfig.params[emuConfig.machineModel].tapeAutoSave]);
}

// Method to change the Tape Auto Save mode, choice based on the actual selected machine.
void EMZNextTapeAutoSave(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].tapeAutoSave = (emuConfig.params[emuConfig.machineModel].tapeAutoSave+1 >= NUMELEM(SHARPMZ_TAPE_AUTO_SAVE) ? 0 : emuConfig.params[emuConfig.machineModel].tapeAutoSave+1);
    }
    return;
}

// Method to return a char string which represents the current selected tape save path.
const char *EMZGetTapeSaveFilePathChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].tapeSavePath);
}

// Method to return a char string which represents the current selected cmt hardware selection setting.
const char *EMZGetCMTModeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_TAPE_MODE[emuConfig.params[emuConfig.machineModel].cmtMode]);
}

// Method to change the cmt hardware setting, choice based on the actual selected machine.
void EMZNextCMTMode(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].cmtMode = (emuConfig.params[emuConfig.machineModel].cmtMode+1 >= NUMELEM(SHARPMZ_TAPE_MODE) ? 0 : emuConfig.params[emuConfig.machineModel].cmtMode+1);
    }
    return;
}

// Method to select the FPGA based CMT or the hardware CMT.
//
void EMZChangeCMTMode(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        // Need to change choice then rewrite the menu as the choice will affect displayed items.
        EMZNextCMTMode(mode);
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
}

// Method to return a char string which represents the current selected fast tape load mode.
const char *EMZGetFastTapeLoadChoice(void)
{
    // Locals.
    //

    return(SHARPMZ_FAST_TAPE[emuConfig.machineGroup][emuConfig.params[emuConfig.machineModel].fastTapeLoad]);
}

// Method to change the Fast Tape Load mode, choice based on the actual selected machine.
void EMZNextFastTapeLoad(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].fastTapeLoad = (emuConfig.params[emuConfig.machineModel].fastTapeLoad+1 >= NUMELEM(SHARPMZ_FAST_TAPE[emuConfig.machineGroup]) || SHARPMZ_FAST_TAPE[emuConfig.machineGroup][emuConfig.params[emuConfig.machineModel].fastTapeLoad+1] == NULL ? 0 : emuConfig.params[emuConfig.machineModel].fastTapeLoad+1);
    }
    return;
}

// Method to return a char string which represents the current selected tape button setting.
const char *EMZGetTapeButtonsChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_TAPE_BUTTONS[emuConfig.params[emuConfig.machineModel].tapeButtons]);
}

// Method to change the tape button setting, choice based on the actual selected machine.
void EMZNextTapeButtons(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].tapeButtons = (emuConfig.params[emuConfig.machineModel].tapeButtons+1 >= NUMELEM(SHARPMZ_TAPE_BUTTONS) ? 0 : emuConfig.params[emuConfig.machineModel].tapeButtons+1);
    }
    return;
}

// Method to return a char string which represents the current selected Sharp<->ASCII mapping for CMT operations.
const char *EMZGetCMTAsciiMappingChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_ASCII_MAPPING[emuConfig.params[emuConfig.machineModel].cmtAsciiMapping]);
}

// Method to change the Sharp<->ASCII mapping setting, choice based on the actual selected machine.
void EMZNextCMTAsciiMapping(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].cmtAsciiMapping = (emuConfig.params[emuConfig.machineModel].cmtAsciiMapping+1 >= NUMELEM(SHARPMZ_ASCII_MAPPING) ? 0 : emuConfig.params[emuConfig.machineModel].cmtAsciiMapping+1);
    }
    return;
}

// Method to return a char string which represents the current selected 40x25 Monitor ROM setting.
const char *EMZGetMonitorROM40Choice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romMonitor40.romEnabled ? emuConfig.params[emuConfig.machineModel].romMonitor40.romFileName : "Disabled");
}

// Method to change the 40x25 Monitor ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextMonitorROM40(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romMonitor40.romEnabled = (emuConfig.params[emuConfig.machineModel].romMonitor40.romEnabled == 1 ? 0 : 1);
printf("romEnabled=%d\n", emuConfig.params[emuConfig.machineModel].romMonitor40.romEnabled );
    }
    return;
}

// Method to return a char string which represents the current selected 80x25 Monitor ROM setting.
const char *EMZGetMonitorROM80Choice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romMonitor80.romEnabled ? emuConfig.params[emuConfig.machineModel].romMonitor80.romFileName : "Disabled");
}

// Method to change the 80x25 Monitor ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextMonitorROM80(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romMonitor80.romEnabled = (emuConfig.params[emuConfig.machineModel].romMonitor80.romEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a char string which represents the current selected character generator ROM setting.
const char *EMZGetCGROMChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romCG.romEnabled ? emuConfig.params[emuConfig.machineModel].romCG.romFileName : "Disabled");
}

// Method to change the character generator ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextCGROM(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romCG.romEnabled = (emuConfig.params[emuConfig.machineModel].romCG.romEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a char string which represents the current selected key mapping ROM setting.
const char *EMZGetKeyMappingROMChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romKeyMap.romEnabled ? emuConfig.params[emuConfig.machineModel].romKeyMap.romFileName : "Disabled");
}

// Method to change the key mapping ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextKeyMappingROM(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romKeyMap.romEnabled = (emuConfig.params[emuConfig.machineModel].romKeyMap.romEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a char string which represents the current selected User ROM setting.
const char *EMZGetUserROMChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romUser.romEnabled ? emuConfig.params[emuConfig.machineModel].romUser.romFileName : "Disabled");
}

// Method to change the User ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextUserROM(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romUser.romEnabled = (emuConfig.params[emuConfig.machineModel].romUser.romEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a char string which represents the current selected Floppy Disk ROM setting.
const char *EMZGetFloppyDiskROMChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].romFDC.romEnabled ? emuConfig.params[emuConfig.machineModel].romFDC.romFileName : "Disabled");
}

// Method to change the Floppy Disk ROM setting, disabled or selected file, choice based on the actual selected machine.
void EMZNextFloppyDiskROM(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].romFDC.romEnabled = (emuConfig.params[emuConfig.machineModel].romFDC.romEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a string representation of the tape type held in the last tape accessed buffer.
const char *EMZGetTapeType(void)
{
    // Locals.
    //
    return(SHARPMZ_TAPE_TYPE[emuControl.tapeHeader.dataType >= NUMELEM(SHARPMZ_TAPE_TYPE) ? NUMELEM(SHARPMZ_TAPE_TYPE) - 1 : emuControl.tapeHeader.dataType]);
}

// Method to return a char string which represents the current selected application autostart setting.
const char *EMZGetLoadApplicationChoice(void)
{
    // Locals.
    //
    return(emuConfig.params[emuConfig.machineModel].loadApp.appEnabled ? emuConfig.params[emuConfig.machineModel].loadApp.appFileName : "Disabled");
}

// Method to change the current select application autostart setting.
void EMZNextLoadApplication(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].loadApp.appEnabled = (emuConfig.params[emuConfig.machineModel].loadApp.appEnabled == 1 ? 0 : 1);
    }
    return;
}

// Method to return a char string which represents the current state of the autostart feature.
const char *EMZGetAutoStartChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_AUTOSTART[emuConfig.params[emuConfig.machineModel].autoStart]);
}

// Method to change the start of the autostart feature, choice based on the actual selected machine.
void EMZNextAutoStart(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].autoStart = (emuConfig.params[emuConfig.machineModel].autoStart+1 >= NUMELEM(SHARPMZ_AUTOSTART) ? 0 : emuConfig.params[emuConfig.machineModel].autoStart+1);
    }
    return;
}

// Method to select the autostart mode.
//
void EMZChangeAutoStart(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        // Need to change choice then rewrite the menu as the choice will affect displayed items.
        EMZNextAutoStart(mode);
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
}

// Method to add a line into the displayed menu.
//
void EMZAddToMenu(uint8_t row, uint8_t active, char *text, char hotKey, enum MENUTYPES type, enum MENUSTATE state, t_menuCallback mcb, enum MENUCALLBACK cbAction, t_choiceCallback ccb, t_viewCallback vcb)
{   
    // Locals.
    uint32_t  textLen = strlen(text);
    uint32_t  idx;

    // Sanity check.
    if(row >= MAX_MENU_ROWS)
        return;
    
    if(emuControl.menu.data[row] != NULL)
    {
        free(emuControl.menu.data[row]);
        emuControl.menu.data[row] = NULL;
    }
    emuControl.menu.data[row] = (t_menuItem *)malloc(sizeof(t_menuItem));
    if(emuControl.menu.data[row] == NULL)
    {
        debugf("Failed to allocate %d bytes on heap for menu row data %d\n", sizeof(t_menuItem), row);
        return;
    }
    idx = textLen;
    if(textLen > 0)
    {
        // Scan the text for a hotkey, case insensitive.
        for(idx=0; idx < textLen; idx++)
        {
            if(text[idx] == hotKey)
                break;
        }
        strcpy(emuControl.menu.data[row]->text, text);
    }
    else
    {
        emuControl.menu.data[row]->text[0] = 0x00;
    }
    // Store hotkey if given and found.
    if(hotKey != 0x00 && idx < textLen)
    {
        // Store as upper case, even though lower case hot keys can be given, it is expected that 
        // the menu system doesnt contain 2 of the same upper/lower characters as hotkey.
        emuControl.menu.data[row]->hotKey = hotKey;
    } else
    {
        emuControl.menu.data[row]->hotKey = 0x00;
    }
    emuControl.menu.data[row]->type  = type;
    emuControl.menu.data[row]->state = state;
    emuControl.menu.data[row]->menuCallback = mcb;
    emuControl.menu.data[row]->choiceCallback = ccb;
    emuControl.menu.data[row]->viewCallback = vcb;
    emuControl.menu.data[row]->cbAction = cbAction;

    if(active && state == MENUSTATE_ACTIVE)
    {
        emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = row;
    }
    return;
}

// Method to get the boundaries of the current menu, ie. first item, last item and number of visible rows.
void EMZGetMenuBoundaries(int16_t *firstMenuRow, int16_t *lastMenuRow, int16_t *firstActiveRow, int16_t *lastActiveRow, int16_t *visibleRows)
{
    // Set defaults to indicate an invalid Menu structure.
    *firstMenuRow = *lastMenuRow = *firstActiveRow = *lastActiveRow = -1;
    *visibleRows = 0;

    // Npw scan the menu elements and work out first, last and number of visible rows.
    for(int16_t idx=0; idx < MAX_MENU_ROWS; idx++)
    {
        if(emuControl.menu.data[idx] != NULL)
        {
            if(*firstMenuRow == -1) { *firstMenuRow = idx; }
            *lastMenuRow  = idx;
            if(emuControl.menu.data[idx]->state != MENUSTATE_HIDDEN) { *visibleRows += 1; }
            if(emuControl.menu.data[idx]->state == MENUSTATE_ACTIVE && *firstActiveRow == -1) { *firstActiveRow = idx; }
            if(emuControl.menu.data[idx]->state == MENUSTATE_ACTIVE) { *lastActiveRow = idx; }
        }
    }
    return;
}

// Method to update the framebuffer with current Menu contents and active line selection.
// The active row is used from the control structure when activeRow = -1 otherwise it is updated.
//
int16_t EMZDrawMenu(int16_t activeRow, uint8_t direction, enum MENUMODE mode)
{
    uint16_t     xpad          = 0;
    uint16_t     ypad          = 1;
    uint16_t     rowPixelDepth = (emuControl.menu.rowFontptr->height + emuControl.menu.rowFontptr->spacing + emuControl.menu.padding + 2*ypad);
    uint16_t     maxCol        = (uint16_t)OSDGet(ACTIVE_MAX_X);
    uint16_t     colPixelEnd   = maxCol - emuControl.menu.colPixelsEnd;
    uint16_t     maxRow        = ((uint16_t)OSDGet(ACTIVE_MAX_Y)/rowPixelDepth) + 1;
    uint8_t      textChrX      = (emuControl.menu.colPixelStart / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing));
    char         activeBuf[MENU_ROW_WIDTH];
    uint16_t     attrBuf[MENU_ROW_WIDTH];
    int16_t      firstMenuRow;
    int16_t      firstActiveMenuRow;
    int16_t      lastMenuRow;
    int16_t      lastActiveMenuRow;
    int16_t      visibleRows;

    // Get menu boundaries.
    EMZGetMenuBoundaries(&firstMenuRow, &lastMenuRow, &firstActiveMenuRow, &lastActiveMenuRow, &visibleRows);
 
    // Sanity check.
    if(firstMenuRow == -1 || lastMenuRow == -1 || firstActiveMenuRow == -1 || lastActiveMenuRow == -1 || visibleRows == 0)
        return(activeRow);
   
    // Clear out old menu.
    OSDClearArea(emuControl.menu.colPixelStart, emuControl.menu.rowPixelStart, colPixelEnd, ((uint16_t)OSDGet(ACTIVE_MAX_Y) - 2), emuControl.menu.inactiveBgColour);

    // Forward to the next active row if the one provided isnt active.
    if(activeRow <= -1)
    {
        activeRow = (emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] < 0 || emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] >= MAX_MENU_ROWS ? 0 : emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]);
    }

    // Sanity check.
    if(activeRow > MAX_MENU_ROWS-1)
        activeRow = lastMenuRow;

    if(emuControl.menu.data[activeRow] == NULL || emuControl.menu.data[activeRow]->state != MENUSTATE_ACTIVE)
    {
        // Get the next or previous active menu item row.
        uint16_t loopCheck = MAX_MENU_ROWS;
        while((emuControl.menu.data[activeRow] == NULL || (emuControl.menu.data[activeRow] != NULL && emuControl.menu.data[activeRow]->state != MENUSTATE_ACTIVE)) && loopCheck > 0)
        {
            activeRow += (direction == 1 ? 1 : -1);
            if(activeRow <= 0 && mode == MENU_NORMAL) activeRow = firstActiveMenuRow;
            if(activeRow <= 0 && mode == MENU_WRAP)   activeRow = lastActiveMenuRow;
            if(activeRow >= MAX_MENU_ROWS && mode == MENU_NORMAL) activeRow = lastActiveMenuRow;
            if(activeRow >= MAX_MENU_ROWS && mode == MENU_WRAP)   activeRow = firstActiveMenuRow;
            loopCheck--;
        }
    }

    // Loop through all the visible rows and output.
    for(uint16_t dspRow=0, menuRow=activeRow < maxRow-1 ? 0 : activeRow - (maxRow-1); menuRow < MAX_MENU_ROWS; menuRow++)
    {
        // Skip inactive or hidden rows.
        if(emuControl.menu.data[menuRow] == NULL)
            continue;
        if(emuControl.menu.data[menuRow]->state == MENUSTATE_HIDDEN)
            continue;
        if(dspRow >= maxRow)
            continue;

        // Skip rendering blank lines!
        if(emuControl.menu.data[menuRow]->state != MENUSTATE_BLANK)
        {
            // Zero out attributes buffer.
            memset(attrBuf, NOATTR, MENU_ROW_WIDTH*sizeof(uint16_t));

            // For read only text, no choice or directory indicator required.
            if(emuControl.menu.data[menuRow]->state == MENUSTATE_TEXT)
            {
                sprintf(activeBuf, " %-s", emuControl.menu.data[menuRow]->text);
            } else
            {
                // Format the data into a single buffer for output according to type. The sprintf character limiter and left justify clash so manually cut the choice buffer to max length.
                uint16_t selectionWidth = EMZGetMenuColumnWidth() - MENU_CHOICE_WIDTH - 2;
                char     *ptr;
                sprintf(activeBuf, " %-*s",  selectionWidth, emuControl.menu.data[menuRow]->text);
                ptr=&activeBuf[strlen(activeBuf)];
                sprintf(ptr, "%-*s", MENU_CHOICE_WIDTH, (emuControl.menu.data[menuRow]->type & MENUTYPE_CHOICE && emuControl.menu.data[menuRow]->choiceCallback != NULL) ? emuControl.menu.data[menuRow]->choiceCallback() : "");
                sprintf(ptr+MENU_CHOICE_WIDTH, "%c", (emuControl.menu.data[menuRow]->type & MENUTYPE_SUBMENU) && !(emuControl.menu.data[menuRow]->type & MENUTYPE_ACTION) ? 0x10 : ' ');
              
                // Prepare any needed attributes.
                for(uint16_t idx=0; idx < strlen(activeBuf); idx++)
                {
                    // Highlight the Hot Key.
                    if(activeBuf[idx] == emuControl.menu.data[menuRow]->hotKey)
                    {
                        attrBuf[idx] = HILIGHT_FG_CYAN; 
                        break;
                    }
                }
            }

            // Output the row according to type.
            if(activeRow == menuRow) 
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, attrBuf, emuControl.menu.activeFgColour, emuControl.menu.activeBgColour);
                if(activeRow != -1)
                    emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = activeRow;
            } 
            else if(emuControl.menu.data[menuRow]->state == MENUSTATE_GREYED)
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, attrBuf, emuControl.menu.greyedFgColour, emuControl.menu.greyedBgColour);
            }
            else if(emuControl.menu.data[menuRow]->state == MENUSTATE_TEXT)
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, attrBuf, emuControl.menu.textFgColour, emuControl.menu.textBgColour);
            }
            else
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, attrBuf, emuControl.menu.inactiveFgColour, emuControl.menu.inactiveBgColour);
            }
            // Once the menu entry has been rendered, render view data if callback given.
            if(emuControl.menu.data[menuRow]->viewCallback != NULL)
            {
                emuControl.menu.data[menuRow]->viewCallback();
            }
        }
        dspRow++;
    }
   
    // If this is a submenu, place a back arrow to indicate you can go back.
    if(emuControl.activeMenu.menuIdx != 0)
    {
        OSDWriteString(textChrX+1, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "\x1b back",     NULL, CYAN, BLACK);
    }
    // Place scroll arrows if we span a page.
    if(activeRow >= maxRow && visibleRows > maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 71), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x17",  NULL, CYAN, BLACK);
    } else
    if(activeRow >= maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 71), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x18 ", NULL, CYAN, BLACK);
    }
    else if(visibleRows > maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 71), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x19",  NULL, CYAN, BLACK);
    } else
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 71), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "        ",     NULL, CYAN, BLACK);
    }
        
    return(activeRow);
}

// Method to free up menu heap allocated memory.
//
void EMZReleaseMenuMemory(void)
{
    // Loop through menu, free previous memory and initialise pointers.
    //
    for(uint16_t idx=0; idx < MAX_MENU_ROWS; idx++)
    {
        if(emuControl.menu.data[idx] != NULL)
        {
            free(emuControl.menu.data[idx]);
            emuControl.menu.data[idx] = NULL;
        }
    }
}

// Method to setup the intial menu screen and prepare for menu creation.
//
void EMZSetupMenu(char *sideTitle, char *menuTitle, enum FONTS font)
{
    // Locals.
    //
    fontStruct  *fontptr = OSDGetFont(font);
    uint16_t    fontWidth = fontptr->width+fontptr->spacing;
    uint16_t    menuStartX = (((((uint16_t)OSDGet(ACTIVE_MAX_X) / fontWidth) - (30/fontWidth)) / 2) - strlen(menuTitle)/2) + 2;
    uint16_t    menuTitleLineLeft  = (menuStartX * fontWidth) - 3;
    uint16_t    menuTitleLineRight = ((menuStartX+strlen(menuTitle))*fontWidth) + 1;
  
    // Release previous memory as creation of a new menu will reallocate according to requirements.
    EMZReleaseMenuMemory();

    // Prepare screen background.
    OSDClearScreen(WHITE);
    OSDClearArea(30, -1, -1, -1, BLACK);

    // Side and Top menu titles.
    OSDWriteString(0,          0, 2,  8, 0, 0, FONT_9X16, DEG270, sideTitle, NULL, BLACK, WHITE);
    OSDWriteString(menuStartX, 0, 0,  0, 0, 0, font,      NORMAL, menuTitle, NULL, WHITE, BLACK);

    // Top line indenting menu title.
    OSDDrawLine( 0,                  0,         menuTitleLineLeft,   0,         WHITE);
    OSDDrawLine( menuTitleLineLeft,  0,         menuTitleLineLeft,   fontWidth, WHITE);
    OSDDrawLine( menuTitleLineLeft,  fontWidth, menuTitleLineRight,  fontWidth, WHITE);
    OSDDrawLine( menuTitleLineRight, 0,         menuTitleLineRight,  fontWidth, WHITE);
    OSDDrawLine( menuTitleLineRight, 0,         -1,                  0,         WHITE);

    // Right and bottom lines to complete menu outline.
    OSDDrawLine( 0, -1,  -1, -1, WHITE);
    OSDDrawLine(-1,  0,  -1, -1, WHITE);

//    emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    return;
}


// Method to setup the OSD for output of a list of paths or files ready for selection.
void EMZSetupDirList(char *sideTitle, char *menuTitle, enum FONTS font)
{
    // Locals.
    //
    fontStruct  *fontptr = OSDGetFont(font);
    uint16_t    fontWidth = fontptr->width+fontptr->spacing;
    uint16_t    menuStartX = (((((uint16_t)OSDGet(ACTIVE_MAX_X) / fontWidth) - (30/fontWidth)) / 2) - strlen(menuTitle)/2) + 1;
    uint16_t    menuTitleWidth     = ((uint16_t)OSDGet(ACTIVE_MAX_X) / fontWidth) - (30/fontWidth);
    uint16_t    menuTitleLineLeft  = (menuStartX * fontWidth) - 5;
    uint16_t    menuTitleLineRight = ((menuStartX+strlen(menuTitle))*fontWidth) + 3;

    // Prepare screen background.
    OSDClearScreen(WHITE);
    OSDClearArea(30, -1, -1, -1, BLACK);

    // Side and Top menu titles.
    OSDWriteString(0,          0, 8,  8, 0, 0, FONT_9X16, DEG270, sideTitle, NULL, BLUE, WHITE);
    // Adjust the title start location when it is larger than the display area.
    OSDWriteString(menuStartX, 0, 0,  0, 0, 0, font,      NORMAL, (strlen(menuTitle) >= menuTitleWidth - 2) ? &menuTitle[menuTitleWidth - strlen(menuTitle) - 2] : menuTitle, NULL, WHITE, BLACK);

    // Top line indenting menu title.
    OSDDrawLine( 0,                  0,         menuTitleLineLeft,   0,         WHITE);
    OSDDrawLine( menuTitleLineLeft,  0,         menuTitleLineLeft,   fontWidth, WHITE);
    OSDDrawLine( menuTitleLineLeft,  fontWidth, menuTitleLineRight,  fontWidth, WHITE);
    OSDDrawLine( menuTitleLineRight, 0,         menuTitleLineRight,  fontWidth, WHITE);
    OSDDrawLine( menuTitleLineRight, 0,         -1,                  0,         WHITE);

    // Right and bottom lines to complete menu outline.
    OSDDrawLine( 0, -1,  -1, -1, WHITE);
    OSDDrawLine(-1,  0,  -1, -1, WHITE);
    return;
}

// Method to process a key event which is intended for the on-screen menu.
//
void EMZProcessMenuKey(uint8_t data, uint8_t ctrl)
{
    // Locals.
    uint16_t menuRow = MAX_MENU_ROWS;
printf("ProcessMenuKey:%02x,%02x\n", data,ctrl);
    // Does the key match a hotkey? If it does, modify active row and action.
    for(menuRow=0; menuRow < MAX_MENU_ROWS; menuRow++)
    {
        // Skip inactive or hidden rows.
        if(emuControl.menu.data[menuRow] == NULL)
            continue;
        if(emuControl.menu.data[menuRow]->state != MENUSTATE_ACTIVE)
            continue;
        // Key match?
        if(toupper(emuControl.menu.data[menuRow]->hotKey) == toupper(data))
            break;
    }

    // If key matched with a hotkey then modify action according to row type.
    if(menuRow != MAX_MENU_ROWS)
    {
        // Update the active row to the matched hotkey row.
        emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = menuRow;

        // Update the action.
        if(emuControl.menu.data[menuRow]->type & MENUTYPE_ACTION)
            data = 0x0D;
        else if(emuControl.menu.data[menuRow]->type & MENUTYPE_CHOICE)
            data = ' ';
        else if(emuControl.menu.data[menuRow]->type & MENUTYPE_SUBMENU)
            data = 0xA3;
    }

    // Process the data received.
    switch(data)
    {
        // Up key.
        case 0xA0:
            if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL)
            {
                emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = EMZDrawMenu(--emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 0, MENU_WRAP);
                OSDRefreshScreen();
            }
            break;

        // Down key.
        case 0xA1:
            if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL)
            {
                emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = EMZDrawMenu(++emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 1, MENU_WRAP);
                OSDRefreshScreen();
            }
            break;
           
        // Left key.
        case 0xA4:
            if(emuControl.activeMenu.menuIdx != 0)
            {
                emuControl.activeMenu.menuIdx = emuControl.activeMenu.menuIdx-1;
                EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
            }
            break;

        // Toggle choice
        case ' ':
            if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->type & MENUTYPE_CHOICE && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback != NULL)
            {
                emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback(ACTION_TOGGLECHOICE);
                if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->cbAction == MENUCB_REFRESH)
                {
                    EMZDrawMenu(emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 0, MENU_WRAP);
                    OSDRefreshScreen();
                }
            }
            break;

        // Carriage Return - action or select sub menu.
        case 0x0D:
        case 0xA3:
            if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->type & MENUTYPE_SUBMENU && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback != NULL)
            {
                emuControl.activeMenu.menuIdx = emuControl.activeMenu.menuIdx >= MAX_MENU_DEPTH - 1 ? MAX_MENU_DEPTH - 1 : emuControl.activeMenu.menuIdx+1;
                emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx-1]]->menuCallback(ACTION_SELECT);
            } else
            if(data == 0x0D && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL)
            {
                if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback != NULL)
                    emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback(ACTION_SELECT);
                if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->cbAction == MENUCB_REFRESH)
                {
                    EMZDrawMenu(emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 0, MENU_WRAP);
                    OSDRefreshScreen();
                }
            }
            break;

        default:
            printf("%02x",data);
            break;
    }
    return;
}

// Method to release all memory used by the on screen file/path selection screen.
//
void EMZReleaseDirMemory(void)
{
    // Free up memory used by the directory name buffer from previous use prior to allocating new buffers.
    //
    for(uint16_t idx=0; idx < NUMELEM(emuControl.fileList.dirEntries); idx++)
    {
        if(emuControl.fileList.dirEntries[idx].name != NULL)
        {
            free(emuControl.fileList.dirEntries[idx].name);
            emuControl.fileList.dirEntries[idx].name = NULL;
        }
    }
}

// Method to cache a directory contents with suitable filters in order to present a perusable list to the user via the OSD.
//
uint8_t EMZReadDirectory(const char *path, const char *filter)
{
    // Locals.
    uint16_t          dirCnt = 0;
    uint8_t           result = 0;
    static DIR        dirFp;
    static FILINFO    fno;

    // Release any previous memory.
    //
    EMZReleaseDirMemory();

    // Read and process the directory contents.
    result = f_opendir(&dirFp, (char *)path);
    if(result == FR_OK)
    {
        while(dirCnt < MAX_DIRENTRY)
        {
            result = f_readdir(&dirFp, &fno);
            if(result != FR_OK || fno.fname[0] == 0) break;

            // Filter out none-files.
            if(strlen(fno.fname) == 0)
                continue;

            // Filter out all files if we are just selecting directories.
            if(!(fno.fattrib & AM_DIR) && strcmp(fno.fname, ".") == 0)
                continue;

            // Filter out files not relevant to the caller based on the provided filter.
            const char *ext = strrchr(fno.fname, '.');
            const char *filterExt = strrchr(filter, '.');
//FIX ME: Doesnt process wildcard filename           
            //   Is a File                Is not a Wildcard      Filter doesnt match the extension
            if(!(fno.fattrib & AM_DIR) && !(filterExt != NULL && strcmp(filterExt, "\*") == 0) && (ext == NULL || strcasecmp(++ext, (filterExt == NULL ? filter : ++filterExt)) != 0))
                continue;
           
            // Filter out hidden directories.
            if((fno.fattrib & AM_DIR) && fno.fname[0] == '.')
                continue;

            // Allocate memory for the filename and fill in the required information. 
            if((emuControl.fileList.dirEntries[dirCnt].name = (char *)malloc(strlen(fno.fname)+1)) == NULL)
            {
                printf("Memory exhausted, aborting!\n");
                return(1);
            }
            strcpy(emuControl.fileList.dirEntries[dirCnt].name, fno.fname);
            emuControl.fileList.dirEntries[dirCnt].isDir = fno.fattrib & AM_DIR ? 1 : 0;
            dirCnt++;
        }

        // Pre sort the list so it is alphabetic to speed up short key indexing.
        //
        uint16_t idx, idx2, idx3;
        for(idx=0; idx < MAX_DIRENTRY; idx++)
        {
            if(emuControl.fileList.dirEntries[idx].name == NULL)
                continue;

            for(idx2=0; idx2 < MAX_DIRENTRY; idx2++)
            {
                if(emuControl.fileList.dirEntries[idx2].name == NULL)
                    continue;

                // Locate the next slot just in case the list is fragmented.
                for(idx3=idx2+1; idx3 < MAX_DIRENTRY && emuControl.fileList.dirEntries[idx3].name == NULL; idx3++);
                if(idx3 == MAX_DIRENTRY)
                    break;

                // Compare the 2 closest strings and swap if needed. Priority to directories as they need to appear at the top of the list.
                if( (!emuControl.fileList.dirEntries[idx2].isDir && emuControl.fileList.dirEntries[idx3].isDir)                                                        || 
                    (((emuControl.fileList.dirEntries[idx2].isDir  && emuControl.fileList.dirEntries[idx3].isDir) || (!emuControl.fileList.dirEntries[idx2].isDir && !emuControl.fileList.dirEntries[idx3].isDir)) && strcasecmp(emuControl.fileList.dirEntries[idx2].name, emuControl.fileList.dirEntries[idx3].name) > 0) )
                {
                    t_dirEntry temp = emuControl.fileList.dirEntries[idx2];
                    emuControl.fileList.dirEntries[idx2].name  = emuControl.fileList.dirEntries[idx3].name;
                    emuControl.fileList.dirEntries[idx2].isDir = emuControl.fileList.dirEntries[idx3].isDir;
                    emuControl.fileList.dirEntries[idx3].name  = temp.name;
                    emuControl.fileList.dirEntries[idx3].isDir = temp.isDir;
                }
            }
        }
    }
    if(dirCnt == 0 && result != FR_OK)
        f_closedir(&dirFp);
    return(result);
}

// Method to get the boundaries of the displayed file list screen, ie. first item, last item and number of visible rows.
void EMZGetFileListBoundaries(int16_t *firstFileListRow, int16_t *lastFileListRow, int16_t *visibleRows)
{
    // Set defaults to indicate an invalid Menu structure.
    *firstFileListRow = *lastFileListRow = -1;
    *visibleRows = 0;

    // Npw scan the file list elements and work out first, last and number of visible rows.
    for(uint16_t idx=0; idx < MAX_DIRENTRY; idx++)
    {
        if(emuControl.fileList.dirEntries[idx].name != NULL && *firstFileListRow == -1) { *firstFileListRow = idx; }
        if(emuControl.fileList.dirEntries[idx].name != NULL)                            { *lastFileListRow  = idx; }
        if(emuControl.fileList.dirEntries[idx].name != NULL)                            { *visibleRows += 1; }
    }
    return;
}

// Method to get the maximum number of columns available for a file row with the current selected font.
//
uint16_t EMZGetFileListColumnWidth(void)
{
    uint16_t maxPixels = OSDGet(ACTIVE_MAX_X);
    return( (maxPixels - emuControl.fileList.colPixelStart - emuControl.fileList.colPixelsEnd) / (emuControl.fileList.rowFontptr->width + emuControl.fileList.rowFontptr->spacing) );
}


// Method to take a cached list of directory entries and present them to the user via the OSD. The current row is highlighted and allows for scrolling up or down within the list.
//
int16_t EMZDrawFileList(int16_t activeRow, uint8_t direction)
{
    uint8_t      xpad          = 0;
    uint8_t      ypad          = 1;
    uint16_t     rowPixelDepth = (emuControl.fileList.rowFontptr->height + emuControl.fileList.rowFontptr->spacing + emuControl.fileList.padding + 2*ypad);
    uint16_t     maxCol        = (uint16_t)OSDGet(ACTIVE_MAX_X);
    uint16_t     colPixelEnd   = maxCol - emuControl.fileList.colPixelsEnd;
    uint16_t     maxRow        = ((uint16_t)OSDGet(ACTIVE_MAX_Y)/rowPixelDepth) + 1;
    uint8_t      textChrX      = (emuControl.fileList.colPixelStart / (emuControl.fileList.rowFontptr->width + emuControl.fileList.rowFontptr->spacing));
    char         activeBuf[MENU_ROW_WIDTH];
    int16_t      firstFileListRow;
    int16_t      lastFileListRow;
    int16_t      visibleRows;

    // Get file list boundaries.
    EMZGetFileListBoundaries(&firstFileListRow, &lastFileListRow, &visibleRows);

    // Clear out old file list.
    OSDClearArea(emuControl.fileList.colPixelStart, emuControl.fileList.rowPixelStart, colPixelEnd, ((uint16_t)OSDGet(ACTIVE_MAX_Y) - 2), emuControl.fileList.inactiveBgColour);

    // If this is a sub directory, place a back arrow to indicate you can go back.
    if(emuControl.activeDir.dirIdx != 0)
    {
        OSDWriteString(textChrX,    0, 0, 4, 0, 0, FONT_5X7, NORMAL, "\x1b back",   NULL, CYAN, BLACK);
    }
    // Place scroll arrows if we span a page.
    if(activeRow >= maxRow && visibleRows > maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 70), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x17", NULL, CYAN, BLACK);
    } else
    if(activeRow >= maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 70), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x18 ",NULL, CYAN, BLACK);
    }
    else if(visibleRows > maxRow)
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 70), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x19", NULL, CYAN, BLACK);
    } else
    {
        OSDWriteString(textChrX+(maxCol < 512 ? 38 : 70), 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "        ",    NULL, CYAN, BLACK);
    }

    // Sanity check, no files or parameters out of range just exit as though the directory is empty.
    if(firstFileListRow == -1 || lastFileListRow == -1 || visibleRows == 0)
        return(activeRow);

    // Forward to the next active row if the one provided isnt active.
    if(activeRow <= -1)
    {
        activeRow = (emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] < 0 || emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] >= MAX_DIRENTRY ? 0 : emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]);
    }
    // Sanity check.
    if(activeRow > MAX_DIRENTRY-1)
        activeRow = lastFileListRow;
    if(emuControl.fileList.dirEntries[activeRow].name == NULL)
    {
        // Get the next or previous active file list row.
        uint16_t loopCheck = MAX_DIRENTRY;
        while(emuControl.fileList.dirEntries[activeRow].name == NULL && loopCheck > 0)
        {
            activeRow += (direction == 1 ? 1 : -1);
            if(activeRow < 0) activeRow = 0;
            if(activeRow >= MAX_DIRENTRY) activeRow = MAX_DIRENTRY-1;
            loopCheck--;
        }
        if(activeRow == 0 || activeRow == MAX_DIRENTRY-1) activeRow = firstFileListRow;
        if(activeRow == 0 || activeRow == MAX_DIRENTRY-1) activeRow = lastFileListRow;
    }

    // Loop through all the visible rows and output.
    for(uint16_t dspRow=0, fileRow=activeRow < maxRow-1 ? 0 : activeRow - (maxRow-1); fileRow < MAX_DIRENTRY; fileRow++)
    {
        // Skip inactive or hidden rows.
        if(emuControl.fileList.dirEntries[fileRow].name == NULL)
            continue;
        if(dspRow >= maxRow)
            continue;

        // Format the data into a single buffer for output.
        uint16_t selectionWidth = EMZGetFileListColumnWidth() - 9;
        uint16_t nameStart      = strlen(emuControl.fileList.dirEntries[fileRow].name) > selectionWidth ? strlen(emuControl.fileList.dirEntries[fileRow].name) - selectionWidth : 0;
        sprintf(activeBuf, " %-*s%-7s ",  selectionWidth, &emuControl.fileList.dirEntries[fileRow].name[nameStart], (emuControl.fileList.dirEntries[fileRow].isDir == 1) ? "<DIR> \x10" : "");

        // Finall output the row according to selection.
        if(activeRow == fileRow) 
        {
            OSDWriteString(textChrX, dspRow, 0, emuControl.fileList.rowPixelStart, xpad, ypad, emuControl.fileList.font, NORMAL, activeBuf, NULL, emuControl.fileList.activeFgColour, emuControl.fileList.activeBgColour);
            if(activeRow != -1)
                emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = activeRow;
        } 
        else
        {
            OSDWriteString(textChrX, dspRow, 0, emuControl.fileList.rowPixelStart, xpad, ypad, emuControl.fileList.font, NORMAL, activeBuf, NULL, emuControl.fileList.inactiveFgColour, emuControl.fileList.inactiveBgColour);
        }
        dspRow++;
    }

    return(activeRow);
}

void EMZGetFile(void)
{

}

// Method to process a key event according to current state. If displaying a list of files for selection then this method is called to allow user interaction with the file
// list and ultimate selection.
//
void EMZProcessFileListKey(uint8_t data, uint8_t ctrl)
{
    // Locals.
    //
    char         tmpbuf[MAX_FILENAME_LEN+1];
    uint16_t     rowPixelDepth = (emuControl.fileList.rowFontptr->height + emuControl.fileList.rowFontptr->spacing + emuControl.fileList.padding + 2);
    uint16_t     maxRow        = ((uint16_t)OSDGet(ACTIVE_MAX_Y)/rowPixelDepth) + 1;

    // data = ascii code/ctrl code for key pressed.
    // ctrl = KEY_BREAK & KEY_CTRL & KEY_SHIFT & "000" & KEY_DOWN & KEY_UP
   
    // If the break key is pressed, this is equivalent to escape so exit file list selection.
    if(ctrl & KEY_BREAK_BIT)
    {
        // Just switch back to active menu dont activate the callback for storing a selection.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    } else
    {
        // Process according to pressed key.
        //
        switch(data)
        {
            // Short keys to index into the file list.
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
                for(int16_t idx=0; idx < MAX_DIRENTRY; idx++)
                {
                    if(emuControl.fileList.dirEntries[idx].name == NULL)
                        continue;
    
                    // If the key pressed is the first letter of a filename then jump to it.
                    if((!emuControl.fileList.dirEntries[idx].isDir && emuControl.fileList.dirEntries[idx].name[0] == tolower(data)) || emuControl.fileList.dirEntries[idx].name[0] == toupper(data))
                    {
                        emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = idx;
                        EMZDrawFileList(emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx], 0);
                        OSDRefreshScreen();
                        break;
                    }
                }
                break;
    
            // Up key.
            case 0xA0:
                // Shift UP pressed?
                if(ctrl & KEY_SHIFT_BIT)
                {
                    emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] - maxRow -1 > 0 ? emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] - maxRow -1 : 0;
                }
                emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = EMZDrawFileList(--emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx], 0);
                OSDRefreshScreen();
                break;
    
            // Down key.
            case 0xA1:
                // Shift Down pressed?
                if(ctrl & KEY_SHIFT_BIT)
                {
                    emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] + maxRow -1 > 0 ? emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] + maxRow -1 : MAX_DIRENTRY-1;
                }
                emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = EMZDrawFileList(++emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx], 1);
                OSDRefreshScreen();
                break;
               
            // Left key.
            case 0xA4:
                if(emuControl.activeDir.dirIdx != 0)
                {
                    emuControl.activeDir.dirIdx = emuControl.activeDir.dirIdx-1;
    
                    EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
                    EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
                    EMZDrawFileList(0, 1);
                    OSDRefreshScreen();
                }
                break;
    
            // Carriage Return - action or select sub directory.
            case 0x0D:
            case 0xA3: // Right Key
                if(emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].name != NULL)
                {
                    // If selection is chosen by CR on a path, execute the return callback to process the path and return control to the menu system.
                    //
                    if(data == 0x0D && emuControl.fileList.selectDir && emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].isDir && emuControl.fileList.returnCallback != NULL)
                    {
                        sprintf(tmpbuf, "%s\%s", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].name);
                        emuControl.fileList.returnCallback(tmpbuf);
                        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
                    }
                    // If selection is on a directory, increase the menu depth, read the directory and refresh the file list.
                    //
                    else if(emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].isDir && emuControl.activeDir.dirIdx+1 < MAX_DIR_DEPTH)
                    {
                        emuControl.activeDir.dirIdx++;
                        if(emuControl.activeDir.dir[emuControl.activeDir.dirIdx] != NULL)
                        {
                            free(emuControl.activeDir.dir[emuControl.activeDir.dirIdx]);
                            emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = NULL;
                        }
                        if(emuControl.activeDir.dirIdx == 1)
                        {
                            sprintf(tmpbuf, "0:\\%s", emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name);
                        }
                        else
                        {
                            sprintf(tmpbuf, "%s\\%s", emuControl.activeDir.dir[emuControl.activeDir.dirIdx-1], emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name);
                        }
                        if((emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = (char *)malloc(strlen(tmpbuf)+1)) == NULL)
                        {
                            printf("Exhausted memory allocating file directory name buffer\n");
                            return;
                        }
                        strcpy(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], tmpbuf);
                        // Bring up the OSD.
                        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
    
                        // Check the directory, if it is valid and has contents then update the OSD otherwise wind back.
                        if(EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter) == 0)
                        {
                            EMZDrawFileList(0, 1);
                            OSDRefreshScreen();
                        } else
                        {
                            free(emuControl.activeDir.dir[emuControl.activeDir.dirIdx]);
                            emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = NULL;
                            emuControl.activeDir.dirIdx--;
                        }
                    }
                    // If the selection is on a file, execute the return callback to process the file and return control to the menu system.
                    else if(emuControl.fileList.returnCallback != NULL && !emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].isDir)
                    {
                        sprintf(tmpbuf, "%s\\%s", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]].name);
                        emuControl.fileList.returnCallback(tmpbuf);
                        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
                    }
                }
                break;
    
            default:
                printf("%02x",data);
                break;
        }
    }
    return;
}

// Method to redraw/refresh the menu screen.
void EMZRefreshMenu(void)
{
    EMZDrawMenu(emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 0, MENU_WRAP);
    OSDRefreshScreen();
}

// Method to redraw/refresh the file list.
void EMZRefreshFileList(void)
{
    EMZDrawFileList(emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 0);
    OSDRefreshScreen();
}

// Initial method called to select a file which will be loaded directly into the emulation RAM.
//
void EMZLoadDirectToRAM(enum ACTIONMODE mode)
{
    // Locals.
    //

    // Toggle choice?
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextLoadDirectFileFilter(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, EMZGetLoadDirectFileFilterChoice());
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        for(uint16_t idx=0; idx < MAX_DIRENTRY; idx++)
        {
            if(emuControl.fileList.dirEntries[idx].name != NULL)
            {
                printf("%-40s%s\n", emuControl.fileList.dirEntries[idx].name, emuControl.fileList.dirEntries[idx].isDir == 1 ? "<DIR>" : "");
            }
        }

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZLoadDirectToRAMSet;
    }
    return;
}

// Method to print out the details of the last processed tape.
//
void EMZPrintTapeDetails(short errCode)
{
    // Locals.
    //
    uint8_t      textChrX      = (emuControl.menu.colPixelStart / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing));
    char         strBuf[MENU_ROW_WIDTH];

    // Use the menu framework to draw the borders and title but write the details directly.
    //
    if(errCode)
        EMZSetupMenu(EMZGetMachineTitle(), "Tape Error", FONT_7X8);
    else
        EMZSetupMenu(EMZGetMachineTitle(), "Tape Details", FONT_7X8);

    sprintf(strBuf, "File Size:     %04x", emuControl.tapeHeader.fileSize);
    OSDWriteString(18, 4, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, WHITE, BLACK);
    sprintf(strBuf, "File Type:     %s",   EMZGetTapeType());
    OSDWriteString(18, 5, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, WHITE, BLACK);
    sprintf(strBuf, "File Name:     %s",   emuControl.tapeHeader.fileName);
    OSDWriteString(18, 6, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, WHITE, BLACK);
    sprintf(strBuf, "Load Addr:     %04x", emuControl.tapeHeader.loadAddress);
    OSDWriteString(18, 7, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, WHITE, BLACK);
    sprintf(strBuf, "Exec Addr:     %04x", emuControl.tapeHeader.execAddress);
    OSDWriteString(18, 8, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, WHITE, BLACK);

    if(errCode > 0 && errCode < 0x20)
    {
        sprintf(strBuf, "FAT FileSystem error code: %02x", errCode);
    }
    else if(errCode == 0x20)
    {
        sprintf(strBuf, "File header contains insufficient bytes.");
    }
    else if(errCode == 0x21)
    {
        sprintf(strBuf, "Tape Data Type is invalid: %02x", emuControl.tapeHeader.dataType);
    }
    else if(errCode == 0x22)
    {
        sprintf(strBuf, "Tape is not machine code, cannot load to RAM directly.");
    }
    else if(errCode == 0x23 || errCode == 0x24)
    {
        sprintf(strBuf, "File read error. directly.");
    }
    else
    {
        sprintf(strBuf, "Unknown error (%02x) processing tape file.", errCode);
    }
    if(errCode > 0)
    {
        OSDWriteString(((VC_MENU_MAX_X_PIXELS/7) - 4 - strlen(strBuf))/2, 12, 0, 2, 0, 0, FONT_7X8, NORMAL, strBuf, NULL, RED, BLACK);
    }
    EMZRefreshMenu();

    return;
}

// Secondary method called after a file has been chosen.
//
void EMZLoadDirectToRAMSet(char *fileName)
{
    // Locals.
    //
    short        errCode;

    // Simply call tape load with the RAM option to complete request.
    errCode = EMZLoadTapeToRAM(fileName, 0);

    // Print out the tape details.
    EMZPrintTapeDetails(errCode);

    // Slight delay to give user chance to see the details.
    delay(8000);

    return;
}

// Method to push a tape filename onto the queue.
//
void EMZTapeQueuePushFile(char *fileName)
{
    // Locals.
    char *ptr = (char *)malloc(strlen(fileName)+1);

    if(emuControl.tapeQueue.elements > MAX_TAPE_QUEUE)
    {
        free(ptr);
    } else
    {
        // Copy filename into queue.
        strcpy(ptr, fileName);
        emuControl.tapeQueue.queue[emuControl.tapeQueue.elements] = ptr;
        emuControl.tapeQueue.elements++;
    }

    return;
}

// Method to read the oldest tape filename entered and return it.
//
char *EMZTapeQueuePopFile(void)
{
    // Pop the bottom most item and shift queue down.
    //
    emuControl.tapeQueue.fileName[0] = 0;
    if(emuControl.tapeQueue.elements > 0)
    {
        strcpy(emuControl.tapeQueue.fileName, emuControl.tapeQueue.queue[0]);
        free(emuControl.tapeQueue.queue[0]);
        emuControl.tapeQueue.elements--;
        for(int i= 1; i < MAX_TAPE_QUEUE; i++)
        {
            emuControl.tapeQueue.queue[i-1] = emuControl.tapeQueue.queue[i];
        }
        emuControl.tapeQueue.queue[MAX_TAPE_QUEUE-1] = NULL;
    }

    // Return filename.
    return(emuControl.tapeQueue.fileName[0] == 0 ? 0 : emuControl.tapeQueue.fileName);
}


// Method to virtualise a tape and shift the position up and down the queue according to actions given.
// direction: 0 = rotate left (Rew), 1 = rotate right (Fwd)
//
char *EMZTapeQueueAPSSSearch(char direction)
{
    emuControl.tapeQueue.fileName[0] = 0;
    if(emuControl.tapeQueue.elements > 0)
    {
        if(direction == 0)
        {
            // Position is ahead of last, then shift down and return file.
            //
            if(emuControl.tapeQueue.tapePos > 0)
            {
                emuControl.tapeQueue.tapePos--;
                strcpy(emuControl.tapeQueue.fileName, emuControl.tapeQueue.queue[emuControl.tapeQueue.tapePos]);
            }

        } else
        {
            // Position is below max, then return current and forward.
            //
            if(emuControl.tapeQueue.tapePos < MAX_TAPE_QUEUE && emuControl.tapeQueue.tapePos < emuControl.tapeQueue.elements)
            {
                strcpy(emuControl.tapeQueue.fileName, emuControl.tapeQueue.queue[emuControl.tapeQueue.tapePos]);
                emuControl.tapeQueue.tapePos++;
            }
        }
    }

    // Return filename.
    return(emuControl.tapeQueue.fileName[0] == 0 ? 0 : emuControl.tapeQueue.fileName);
}


// Method to iterate through the list of filenames.
//
char *EMZNextTapeQueueFilename(char reset)
{
    static unsigned short  pos = 0;

    // Reset is active, start at beginning of list.
    //
    if(reset)
    {
        pos = 0;
    }
    emuControl.tapeQueue.fileName[0] = 0x00;

    // If we reach the queue limit or the max elements stored, cycle the pointer
    // round to the beginning.
    //
    if(pos >= MAX_TAPE_QUEUE || pos >= emuControl.tapeQueue.elements)
    {
        pos = 0;
    } else

    // Get the next element in the queue, if available.
    //
    if(emuControl.tapeQueue.elements > 0)
    {
        if(pos < MAX_TAPE_QUEUE && pos < emuControl.tapeQueue.elements)
        {
            strcpy(emuControl.tapeQueue.fileName, emuControl.tapeQueue.queue[pos++]);
        }
    }
    // Return filename if available.
    //
    return(emuControl.tapeQueue.fileName[0] == 0x00 ? NULL : &emuControl.tapeQueue.fileName);
}

// Method to clear the queued tape list.
//
uint16_t EMZClearTapeQueue(void)
{
    // Locals.
    uint16_t entries = emuControl.tapeQueue.elements;

    // Clear the queue through iteration, freeing allocated memory per entry.
    if(emuControl.tapeQueue.elements > 0)
    {
        for(int i=0; i < MAX_TAPE_QUEUE; i++)
        {
            if(emuControl.tapeQueue.queue[i] != NULL)
            {
                free(emuControl.tapeQueue.queue[i]);
            }
            emuControl.tapeQueue.queue[i] = NULL;
        }
    }
    // Reset control variables.
    emuControl.tapeQueue.elements    = 0;
    emuControl.tapeQueue.tapePos     = 0;
    emuControl.tapeQueue.fileName[0] = 0;

    // Finally, return number of entries in the queue which have been cleared out.
    return(entries);
}

// Method to select a tape file which is added to the tape queue. The tape queue is a virtual tape where
// each file is presented to the CMT hardware sequentially or in a loop for the more advanced APSS decks
// of the MZ80B/2000.
void EMZQueueTape(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextQueueTapeFileFilter(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, EMZGetQueueTapeFileFilterChoice());
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZQueueTapeSet;
    }
}

// Method to store the selected file into the tape queue.
void EMZQueueTapeSet(char *param)
{
    EMZTapeQueuePushFile(param);
}

// Simple wrapper method to clear the tape queue.
void EMZQueueClear(enum ACTIONMODE mode)
{
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        uint16_t deletedEntries = EMZClearTapeQueue();

        // Update the active row to account for number of entries deleted.
        if(emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] - deletedEntries > 0)
            emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] -= deletedEntries;

        // Need to redraw the menu as read only text has changed.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
}

// Method to select a path where tape images will be saved.
void EMZTapeSave(enum ACTIONMODE mode)
{
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Prompt to enter the path in which incoming files will be saved.
        EMZSetupDirList("Select Path", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, ".");
        emuControl.fileList.selectDir = 1;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZTapeSaveSet;
    }
}

// Method to save the entered directory path in the configuration.
void EMZTapeSaveSet(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
        strcpy(emuConfig.params[emuConfig.machineModel].tapeSavePath, param);
    emuControl.fileList.selectDir = 0;
}

void EMZReset(void)
{
    // Initiate a machine RESET via the control registers.
    //
    emuConfig.emuRegisters[MZ_EMU_REG_CTRL] |= 0x01;

    // Write the control register to initiate RESET command.
    writeZ80Array(MZ_EMU_ADDR_REG_MODEL+emuConfig.emuRegisters[MZ_EMU_REG_CTRL], &emuConfig.emuRegisters[emuConfig.emuRegisters[MZ_EMU_REG_CTRL]], 1, FPGA);
    
    // Clear any reset command from the internal register copy so the command isnt accidentally sent again.
    emuConfig.emuRegisters[MZ_EMU_REG_CTRL] &= 0xFE;

}

// Method to reset the machine. This can be done via the hardware, load up the configuration and include the machine definition and the hardware will force a reset.
//
void EMZResetMachine(enum ACTIONMODE mode)
{
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Force a reload of the machine ROM's and reset.
        EMZSwitchToMachine(emuConfig.machineModel, 1);

        // Refresh menu to update cursor.
        EMZRefreshMenu();
    }
}

// Method to read in the header of an MZF file and populate the tapeHeader structure.
//
short EMZReadTapeDetails(const char *tapeFile)
{
    // Locals.
    //
    char         loadName[MAX_FILENAME_LEN+1];
    uint32_t     actualReadSize;
    FIL          fileDesc;
    FRESULT      result;

    // If a relative path has been given we need to expand it into an absolute path.
    if(tapeFile[0] != '/' && tapeFile[0] != '\\' && (tapeFile[0] < 0x30 || tapeFile[0] > 0x32))
    {
        sprintf(loadName, "%s\%s", TOPLEVEL_DIR, tapeFile);
    } else
    {
        strcpy(loadName, tapeFile);
    }

    // Attempt to open the file for reading.
	result = f_open(&fileDesc, loadName, FA_OPEN_EXISTING | FA_READ);
	if(result)
	{
        debugf("EMZReadTapeDetails(open) File:%s, error: %d.\n", loadName, fileDesc);
        return(result);
	} 

    // Read in the tape header, this indicates crucial data such as data type, size, exec address, load address etc.
    //
    result = f_read(&fileDesc, &emuControl.tapeHeader, MZF_HEADER_SIZE, &actualReadSize);
    if(actualReadSize != 128)
    {
        debugf("Only read:%d bytes of header, aborting.\n", actualReadSize);
        f_close(&fileDesc);
        return(0x20);
    }

    // Close the open file to complete, details stored in the tapeHeader structure.
    f_close(&fileDesc);
    
    return result;
}

// Method to load a tape (MZF) file directly into RAM.
// This involves reading the tape header, extracting the size and destination and loading
// the header and program into emulator ram.
//
short EMZLoadTapeToRAM(const char *tapeFile, unsigned char dstCMT)
{
    // Locals.
    //
    FIL          fileDesc;
    FRESULT      result;
    uint32_t     loadAddress;
    uint32_t     actualReadSize;
    uint32_t     time = *ms;
    uint32_t     readSize;
    char         loadName[MAX_FILENAME_LEN+1];
    char         sectorBuffer[512];
  #if defined __EMUMZ_DEBUG__
    char          fileName[17];

    debugf("Sending tape file:%s to emulator ram", tapeFile);
  #endif

    // If a relative path has been given we need to expand it into an absolute path.
    if(tapeFile[0] != '/' && tapeFile[0] != '\\' && (tapeFile[0] < 0x30 || tapeFile[0] > 0x32))
    {
        sprintf(loadName, "%s\%s", TOPLEVEL_DIR, tapeFile);
    } else
    {
        strcpy(loadName, tapeFile);
    }

    // Attempt to open the file for reading.
	result = f_open(&fileDesc, loadName, FA_OPEN_EXISTING | FA_READ);
	if(result)
	{
        debugf("EMZLoadTapeToRAM(open) File:%s, error: %d.\n", loadName, fileDesc);
        return(result);
	} 


    // Read in the tape header, this indicates crucial data such as data type, size, exec address, load address etc.
    //
    result = f_read(&fileDesc, &emuControl.tapeHeader, MZF_HEADER_SIZE, &actualReadSize);
    if(actualReadSize != 128)
    {
        debugf("Only read:%d bytes of header, aborting.\n", actualReadSize);
        f_close(&fileDesc);
        return(0x20);
    }

    // Some sanity checks.
    //
    if(emuControl.tapeHeader.dataType == 0 || emuControl.tapeHeader.dataType > 5) return(0x21);
  #if defined __EMUMZ_DEBUG__
    for(int i=0; i < 17; i++)
    {
        fileName[i] = emuControl.tapeHeader.fileName[i] == 0x0d ? 0x00 : emuControl.tapeHeader.fileName[i];
    }

    // Debug output to indicate the file loaded and information about the tape image.
    //
    switch(emuControl.tapeHeader.dataType)
    {
        case 0x01:
            debugf("Binary File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;

        case 0x02:
            debugf("MZ-80 Basic Program(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;

        case 0x03:
            debugf("MZ-80 Data File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;

        case 0x04:
            debugf("MZ-700 Data File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;

        case 0x05:
            debugf("MZ-700 Basic Program(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;

        default:
            debugf("Unknown tape type(Type=%02x, Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", emuControl.tapeHeader.dataType, emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.execAddress, fileName);
            break;
    }
  #endif

    // Check the data type, only load machine code directly to RAM.
    //
    if(dstCMT == 0 && emuControl.tapeHeader.dataType != CMT_TYPE_OBJCD)
    {
        f_close(&fileDesc);
        return(0x22);
    }

    // Reset Emulator if loading direct to RAM. This clears out memory, resets monitor and places it in a known state.
    //
    if(dstCMT == 0)
        EMZReset();

    // Load the data from tape to RAM.
    //
    if(dstCMT == 0)                                       // Load to emulators RAM
    {
        loadAddress = MZ_EMU_RAM_ADDR + emuControl.tapeHeader.loadAddress;
    } else
    {
        loadAddress = MZ_EMU_CMT_DATA_ADDR;
    }
    for (unsigned short i = 0; i < emuControl.tapeHeader.fileSize && actualReadSize > 0; i += actualReadSize)
    {
        result = f_read(&fileDesc, sectorBuffer, 512, &actualReadSize);
        if(result)
        {
            debugf("Failed to read data from file:%s @ addr:%08lx, aborting.\n", loadName, loadAddress);
            f_close(&fileDesc);
            return(0x23);
        }

        debugf("Bytes to read, actual:%d, index:%d, sizeHeader:%d, load:%08lx", actualReadSize, i, emuControl.tapeHeader.fileSize, loadAddress);

        if(actualReadSize > 0)
        {
            // Write the sector (or part) to the fpga memory.
            writeZ80Array(loadAddress, sectorBuffer, actualReadSize, FPGA);
            loadAddress += actualReadSize;
        } else
        {
            debugf("Bad tape or corruption, should never be 0, actual:%d, index:%d, sizeHeader:%d", actualReadSize, i, emuControl.tapeHeader.fileSize);
            return(0x24);
        }
    }

    // Now load header - this is done last because the emulator monitor wipes the stack area on reset.
    //
    writeZ80Array(MZ_EMU_CMT_HDR_ADDR, &emuControl.tapeHeader, MZF_HEADER_SIZE, FPGA);

#ifdef __EMUMZ_DEBUG__
    time = *ms - time;
    debugf("Uploaded in %lu ms", time >> 20);
#endif

    // Tidy up.
    f_close(&fileDesc);

    // Remove the LF from the header filename, not needed.
    //
    for(int i=0; i < 17; i++)
    {
        if(emuControl.tapeHeader.fileName[i] == 0x0d) emuControl.tapeHeader.fileName[i] = 0x00;
    }

    // Success.
    //
    return(0);
}

// Method to save the contents of the CMT buffer onto a disk based MZF file.
// The method reads the header, writes it and then reads the data (upto size specified in header) and write it.
//
short EMZSaveTapeFromCMT(const char *tapeFile)
{
    short          dataSize = 0;
    uint32_t       readAddress;
    uint32_t       actualWriteSize;
    uint32_t       writeSize = 0;
    char           fileName[MAX_FILENAME_LEN+1];
    char           saveName[MAX_FILENAME_LEN+1];
    FIL            fileDesc;
    FRESULT        result;
    uint32_t       time = *ms;
    char           sectorBuffer[512];

    // Read the header, then data, but limit data size to the 'file size' stored in the header.
    //
    for(unsigned int mb=0; mb <= 1; mb++)
    {
        // Setup according to block being read, header or data.
        //
        if(mb == 0)
        {
            dataSize = MZF_HEADER_SIZE;
            readAddress = MZ_EMU_CMT_HDR_ADDR;
        } else
        {
            dataSize = emuControl.tapeHeader.fileSize;
            readAddress = MZ_EMU_CMT_DATA_ADDR + emuControl.tapeHeader.loadAddress;
            debugf("mb=%d, tapesize=%04x\n", mb, emuControl.tapeHeader.fileSize);
        }
        for (; dataSize > 0; dataSize -= actualWriteSize)
        {
            debugf("mb=%d, dataSize=%04x, writeSize=%04x\n", mb, dataSize, writeSize);
            if(mb == 0)
            {
                writeSize = MZF_HEADER_SIZE;
            } else
            {
                writeSize = dataSize > 512 ? 512 : dataSize;
            }

            // Read the next chunk from the CMT RAM.
            readZ80Array(readAddress, &sectorBuffer, writeSize, FPGA);

            if(mb == 0)
            {
                memcpy(&emuControl.tapeHeader, &sectorBuffer, MZF_HEADER_SIZE);

                // Now open the file for writing. If no name provided, use the one stored in the header.
                //
                if(tapeFile == 0)
                {
                    for(int i=0; i < 17; i++)
                    {
                        fileName[i] = emuControl.tapeHeader.fileName[i] == 0x0d ? 0x00 : emuControl.tapeHeader.fileName[i];
                    }
                    strcat(fileName, ".mzf");
                    debugf("File from tape:%s (%02x,%04x,%04x,%04x)\n", fileName, emuControl.tapeHeader.dataType, emuControl.tapeHeader.fileSize, emuControl.tapeHeader.loadAddress, emuControl.tapeHeader.execAddress);
                } else
                {
                    strcpy(fileName, tapeFile);
                    debugf("File provided:%s\n", fileName);
                }
                
                // If a relative path has been given we need to expand it into an absolute path.
                if(fileName[0] != '/' && fileName[0] != '\\' && (fileName[0] < 0x30 || fileName[0] > 0x32))
                {
                    sprintf(saveName, "%s\%s", TOPLEVEL_DIR, tapeFile);
                } else
                {
                    strcpy(saveName, tapeFile);
                }
   
                // Attempt to open the file for writing.
                result = f_open(&fileDesc, saveName, FA_CREATE_ALWAYS | FA_WRITE);
                if(result)
                {
                    debugf("EMZSaveFromCMT(open) File:%s, error: %d.\n", saveName, fileDesc);
                    return(3);
                }
            }
            result = f_write(&fileDesc, sectorBuffer, writeSize, &actualWriteSize);
            readAddress += actualWriteSize;
            if(result)
            {
                debugf("EMZSaveFromCMT(write) File:%s, error: %d.\n", saveName, result);
                f_close(&fileDesc);
                return(4);
            }
        }
    }

    // Close file to complete dump.
    f_close(&fileDesc);

    return(0);
}


// Method to enable/disable the 40char monitor ROM and select the image to be used in the ROM.
//
void EMZMonitorROM40(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextMonitorROM40(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZMonitorROM40Set;
    }
}

// Method to store the selected file name.
void EMZMonitorROM40Set(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romMonitor40.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romMonitor40.romEnabled = 1;
    }
}

// Method to enable/disable the 80char monitor ROM and select the image to be used in the ROM.
//
void EMZMonitorROM80(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextMonitorROM80(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZMonitorROM80Set;
    }
}

// Method to store the selected file name.
void EMZMonitorROM80Set(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romMonitor80.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romMonitor80.romEnabled = 1;
    }
}

// Method to enable/disable the Character Generator ROM and select the image to be used in the ROM.
//
void EMZCGROM(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextCGROM(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZCGROMSet;
    }
}

// Method to store the selected file name.
void EMZCGROMSet(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romCG.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romCG.romEnabled = 1;
    }
}

// Method to enable/disable the Keyboard Mapping ROM and select the image to be used in the ROM.
//
void EMZKeyMappingROM(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextKeyMappingROM(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZKeyMappingROMSet;
    }
}

// Method to store the selected file name.
void EMZKeyMappingROMSet(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romKeyMap.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romKeyMap.romEnabled = 1;
    }
}

// Method to enable/disable the User Option ROM and select the image to be used in the ROM.
//
void EMZUserROM(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextUserROM(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZUserROMSet;
    }
}

// Method to store the selected file name.
void EMZUserROMSet(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romUser.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romUser.romEnabled = 1;
    }
}

// Method to enable/disable the FDC Option ROM and select the image to be used in the ROM.
//
void EMZFloppyDiskROM(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextFloppyDiskROM(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.*");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZFloppyDiskROMSet;
    }
}

// Method to store the selected file name.
void EMZFloppyDiskROMSet(char *param)
{
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].romFDC.romFileName, param);
        emuConfig.params[emuConfig.machineModel].romFDC.romEnabled = 1;
    }
}

// Method to enable/disable the cold boot application load and select the image to be loaded.
//
void EMZLoadApplication(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
        EMZNextLoadApplication(mode);
        EMZRefreshMenu();
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        EMZSetupDirList("Select File", emuControl.activeDir.dir[emuControl.activeDir.dirIdx], FONT_7X8);
        strcpy(emuControl.fileList.fileFilter, "*.MZF");
        emuControl.fileList.selectDir = 0;
        EMZReadDirectory(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], emuControl.fileList.fileFilter);
        EMZRefreshFileList();

        // Switch to the File List Dialog mode setting the return Callback which will be activated after a file has been chosen.
        //
        emuControl.activeDialog = DIALOG_FILELIST;
        emuControl.fileList.returnCallback = EMZLoadApplicationSet;
    }
}

// Method to store the selected file name.
void EMZLoadApplicationSet(char *param)
{
    // Locals.
    uint8_t    tmpbuf[20];

    // If a file has been selected, store it within the config and also extract the load address from the file to be used as the 
    // default Post load key injection sequence.
    if(strlen(param) < MAX_FILENAME_LEN)
    {
        strcpy(emuConfig.params[emuConfig.machineModel].loadApp.appFileName, param);
        emuConfig.params[emuConfig.machineModel].loadApp.appEnabled = 1;

        // Try read the tape, set tapeHeader structure to tape details.
        if(EMZReadTapeDetails(emuConfig.params[emuConfig.machineModel].loadApp.appFileName) == 0)
        {
            // Clear out existing key entries.
            for(uint16_t idx=0; idx < MAX_KEY_INS_BUFFER; idx++) { if(emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx].i == 0) { emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx].i = 0xffffffff; } }

            // First key injection is a pause, a delay needed for the monitor BIOS to startup and present the entry prompt.
            emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[0].b[0] = 0x00;
            emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[0].b[1] = 0x00;
            emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[0].b[2] = 0x7f;
            emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[0].b[3] = 0x82;

            // Default command based on target machine.
            //
            switch(emuConfig.machineModel)
            {
                case MZ80K:
                case MZ80C:
                    sprintf(tmpbuf, "GOTO$%04x\r", emuControl.tapeHeader.execAddress);
                    break;
                   
                default:
                    sprintf(tmpbuf, "J%04x\r", emuControl.tapeHeader.execAddress);
                    break;
            }

            for(uint16_t idx=0; idx < strlen(tmpbuf); idx++)
            {
                // Map the ASCII key and insert into the key injection buffer.
                t_numCnv map = EMZMapToScanCode(emuControl.hostMachine, tmpbuf[idx]);
                emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+1].b[0] = map.b[0];
                emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+1].b[1] = map.b[1];
                emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+1].b[2] = 0x7f;
                emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+1].b[3] = 0x7f;

            }
        }
    }
}

void EMZMainMenu(void)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_MAIN;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Main Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "Tape Storage",               'T',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZTapeStorageMenu,     MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "Floppy Storage",             'F',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZFloppyStorageMenu,   MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "Machine",                    'M',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZMachineMenu,         MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "Display",                    'D',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZDisplayMenu,         MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "Audio",                      'A',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZAudioMenu,           MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "System",                     'S',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZSystemMenu,          MENUCB_REFRESH,          NULL,   NULL );
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "Reset Machine",              'R',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZResetMachine,        MENUCB_DONOTHING,        NULL,   NULL );
    EMZRefreshMenu();
}

void EMZTapeStorageMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;
    char      *fileName;
    char      lineBuf[MENU_ROW_WIDTH+1];

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_TAPE_STORAGE;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Tape Storage Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "CMT Hardware",               'C',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZChangeCMTMode,       MENUCB_REFRESH,          EMZGetCMTModeChoice,   NULL );
    EMZAddToMenu(row++,  0, "Load tape to RAM",           'L',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZLoadDirectToRAM,     MENUCB_DONOTHING,        EMZGetLoadDirectFileFilterChoice,   NULL );
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "Queue Tape",                 'Q',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZQueueTape,           MENUCB_DONOTHING,        EMZGetQueueTapeFileFilterChoice,   NULL );

    // List out the current tape queue.
    if(!emuConfig.params[emuConfig.machineModel].cmtMode)
    {
        uint16_t fileCount = 0;
        while((fileName = EMZNextTapeQueueFilename(0)) != NULL)
        {
            // Place an indicator on the active queue file.
            if((EMZGetMachineGroup() == 2 && emuControl.tapeQueue.tapePos == fileCount) ||
               (EMZGetMachineGroup() != 2 && fileCount == 0))
                sprintf(lineBuf, " >%d %.50s", fileCount++, fileName);
            else
                sprintf(lineBuf, "  %d %.50s", fileCount++, fileName);
            EMZAddToMenu(row++,  0, lineBuf,                  0x00, MENUTYPE_TEXT,                      MENUSTATE_TEXT,   NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
        }    
    }

    EMZAddToMenu(row++,  0, "Clear Queue",                'e',  MENUTYPE_ACTION,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZQueueClear,          MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "Save Tape Directory",        'T',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZTapeSave,            MENUCB_DONOTHING,        EMZGetTapeSaveFilePathChoice,   NULL );
    EMZAddToMenu(row++,  0, "Auto Save Tape",             'A',  MENUTYPE_CHOICE,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextTapeAutoSave,    MENUCB_REFRESH,          EMZGetTapeAutoSaveChoice,   NULL );
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                                                                                            MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "File Name Map Ascii",        'F',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextCMTAsciiMapping, MENUCB_REFRESH,          EMZGetCMTAsciiMappingChoice,   NULL );
    EMZAddToMenu(row++,  0, "Tape Buttons",               'B',  MENUTYPE_CHOICE,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextTapeButtons,     MENUCB_REFRESH,          EMZGetTapeButtonsChoice,   NULL );
    EMZAddToMenu(row++,  0, "Fast Tape Load",             'd',  MENUTYPE_CHOICE,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextFastTapeLoad,    MENUCB_REFRESH,          EMZGetFastTapeLoadChoice,   NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZFloppyStorageMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;
    char      *fileName;
    char      lineBuf[MENU_ROW_WIDTH+1];

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_FLOPPY_STORAGE;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Floppy Storage Menu", FONT_7X8);
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZMachineMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_MACHINE;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Machine Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "Machine Model",              'M',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMachineModel,    MENUCB_REFRESH,          EMZGetMachineModelChoice,    NULL );
    EMZAddToMenu(row++,  0, "CPU Speed",                  'C',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextCPUSpeed,        MENUCB_REFRESH,          EMZGetCPUSpeedChoice,        NULL );
    EMZAddToMenu(row++,  0, "Memory Size",                'S',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMemSize,         MENUCB_REFRESH,          EMZGetMemSizeChoice,         NULL );
    if(emuConfig.machineModel == MZ800)
    {
        EMZAddToMenu(row++,  0, "Mode",                   'o',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMZ800Mode,       MENUCB_REFRESH,          EMZGetMZ800ModeChoice,       NULL );
        EMZAddToMenu(row++,  0, "Printer",                'r',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMZ800Printer,    MENUCB_REFRESH,          EMZGetMZ800PrinterChoice,    NULL );
        EMZAddToMenu(row++,  0, "Tape In",                'a',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMZ800TapeIn,     MENUCB_REFRESH,          EMZGetMZ800TapeInChoice,     NULL );
    }
    EMZAddToMenu(row++,  0, "",                           0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                        NULL );
    EMZAddToMenu(row++,  0, "Rom Management",             'R',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZRomManagementMenu,   MENUCB_REFRESH,          NULL,                        NULL );
    EMZAddToMenu(row++,  0, "AutoStart Application",      'u',  MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZAutoStartApplicationMenu, MENUCB_REFRESH,     NULL,                        NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZDisplayMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_DISPLAY;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Display Menu", FONT_7X8);
    switch(emuConfig.machineModel)
    {
        case MZ80K:
        case MZ80C:
        case MZ1200:
        case MZ80A:
        case MZ700:
        case MZ1500:
            EMZAddToMenu(row++,  0, "Display Type",       'T',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextDisplayType,     MENUCB_REFRESH,          EMZGetDisplayTypeChoice,     NULL );
            break;

        default:
            break;
    }
    switch(emuConfig.machineModel)
    {
        case MZ80A:
        case MZ700:
        case MZ800:
        case MZ1500:
        case MZ80B:
        case MZ2000:
        case MZ2200:
        case MZ2500:
            EMZAddToMenu(row++,  0, "Display Option",     'D',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextDisplayOption,   MENUCB_REFRESH,          EMZGetDisplayOptionChoice,   NULL );
            break;

        default:
            break;
    }
    EMZAddToMenu(row++,  0, "Display Output",             'O',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextDisplayOutput,   MENUCB_REFRESH,          EMZGetDisplayOutputChoice,   NULL );
    EMZAddToMenu(row++,  0, "Video",                      'V',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextVRAMMode,        MENUCB_REFRESH,          EMZGetVRAMModeChoice,        NULL );
    switch(emuConfig.machineModel)
    {
        case MZ800:
        case MZ80B:
        case MZ2000:
        case MZ2200:
        case MZ2500:
            EMZAddToMenu(row++,  0, "Graphics",           'G',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextGRAMMode,        MENUCB_REFRESH,          EMZGetGRAMModeChoice,        NULL );
            break;

        default:
            break;
    }
    if(emuConfig.machineModel == MZ80A)
    {
        EMZAddToMenu(row++,  0, "VRAM CPU Wait",          'W',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextVRAMWaitMode,    MENUCB_REFRESH,          EMZGetVRAMWaitModeChoice,    NULL );
    }
    if(strcmp(EMZGetDisplayOptionChoice(), "PCG") == 0)
    {
        EMZAddToMenu(row++,  0, "PCG Mode",               'P',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextPCGMode,         MENUCB_REFRESH,          EMZGetPCGModeChoice,         NULL );
    }
  //EMZAddToMenu(row++,  0, "Aspect Ratio",               'R',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAspectRatio,     MENUCB_REFRESH,          EMZGetAspectRatioChoice,     NULL );
  //EMZAddToMenu(row++,  0, "Scandoubler",                'S',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextScanDoublerFX,   MENUCB_REFRESH,          EMZGetScanDoublerFXChoice,   NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZAudioMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_AUDIO;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Audio Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "Source",                     'S',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioSource,     MENUCB_REFRESH,          EMZGetAudioSourceChoice,     NULL );
    EMZAddToMenu(row++,  0, "Hardware",                   'H',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioHardware,   MENUCB_REFRESH,          EMZGetAudioHardwareChoice,   NULL );
    // If FPGA sound hardware is selected show configuration options.
    if(emuConfig.params[emuConfig.machineModel].audioHardware != 0)
    {
        EMZAddToMenu(row++,  0, "Volume",                 'V',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioVolume,     MENUCB_REFRESH,          EMZGetAudioVolumeChoice,     NULL );
        EMZAddToMenu(row++,  0, "Mute",                   'M',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioMute,       MENUCB_REFRESH,          EMZGetAudioMuteChoice,       NULL );
        EMZAddToMenu(row++,  0, "Channel Mix",            'C',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioMix,        MENUCB_REFRESH,          EMZGetAudioMixChoice,        NULL );
    }

    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZSystemMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_SYSTEM;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "System Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "Reload config",              'R',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZReadConfig,          MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "Save config",                'S',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZWriteConfig,         MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "Reset config",               'e',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZResetConfig,         MENUCB_DONOTHING,        NULL,   NULL );
    EMZAddToMenu(row++,  0, "About",                      'A',  MENUTYPE_SUBMENU | MENUTYPE_ACTION, MENUSTATE_ACTIVE, EMZAbout,               MENUCB_REFRESH,          NULL,                        NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZAbout(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint16_t  maxX      = OSDGet(ACTIVE_MAX_X);
    uint8_t   textChrX  = (emuControl.menu.colPixelStart / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing));

    // Use the menu framework to draw the borders and title but write a bitmap and text directly.
    //
    EMZSetupMenu(EMZGetMachineTitle(), "About", FONT_7X8);
    OSDWriteBitmap(48,         15, BITMAP_ARGO_MEDIUM, RED, BLACK);
    OSDWriteString(22,          9, 0, 2, 0, 0, (maxX < 512 ? FONT_5X7 : FONT_7X8), NORMAL, "Sharp MZ Series v2.01",       NULL, CYAN, BLACK);
    OSDWriteString(19,         10, 0, 2, 0, 0, (maxX < 512 ? FONT_5X7 : FONT_7X8), NORMAL, "(C) Philip Smart, 2018-2021", NULL, CYAN, BLACK);
    OSDWriteString(21,         11, 0, 2, 0, 0, (maxX < 512 ? FONT_5X7 : FONT_7X8), NORMAL, "MZ-700 Embedded Version",     NULL, CYAN, BLACK);
    OSDWriteString(textChrX+1,  0, 0, 4, 0, 0, (maxX < 512 ? FONT_5X7 : FONT_5X7), NORMAL, "\x1b back",                   NULL, CYAN, BLACK);
    EMZRefreshMenu();
}

void EMZRomManagementMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_ROMMANAGEMENT;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Rom Management Menu", FONT_7X8);

    EMZAddToMenu(row++,  0, "Monitor ROM (40x25)",        '4',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZMonitorROM40,        MENUCB_DONOTHING,        EMZGetMonitorROM40Choice,     NULL );
    EMZAddToMenu(row++,  0, "Monitor ROM (80x25)",        '8',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZMonitorROM80,        MENUCB_DONOTHING,        EMZGetMonitorROM80Choice,     NULL );
    EMZAddToMenu(row++,  0, "Char Generator ROM",         'G',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZCGROM,               MENUCB_DONOTHING,        EMZGetCGROMChoice,            NULL );
    EMZAddToMenu(row++,  0, "Key Mapping ROM",            'K',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZKeyMappingROM,       MENUCB_DONOTHING,        EMZGetKeyMappingROMChoice,    NULL );
    EMZAddToMenu(row++,  0, "User ROM",                   'U',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZUserROM,             MENUCB_DONOTHING,        EMZGetUserROMChoice,          NULL );
    EMZAddToMenu(row++,  0, "Floppy Disk ROM",            'F',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZFloppyDiskROM,       MENUCB_DONOTHING,        EMZGetFloppyDiskROMChoice,    NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZAutoStartApplicationMenu(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t   row = 0;
    char      lineBuf[MENU_ROW_WIDTH+1];

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_AUTOSTART;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "AutoStart Menu", FONT_7X8);

    EMZAddToMenu(row++,  0, "Enable AutoStart",           'E',  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZChangeAutoStart,     MENUCB_DONOTHING,        EMZGetAutoStartChoice,        NULL );
    if(emuConfig.params[emuConfig.machineModel].autoStart)
    {
        EMZAddToMenu(row++,  0, "Application to Load",    'A',  MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZLoadApplication,     MENUCB_DONOTHING,        EMZGetLoadApplicationChoice,  NULL );
        EMZAddToMenu(row++,  0, "Pre-load key injection", 'r',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZPreKeyEntry,         MENUCB_DONOTHING,        NULL,                         EMZRenderPreKeyViewTop );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );

        EMZAddToMenu(row++,  0, "Post-load key injection",'o',  MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZPostKeyEntry,        MENUCB_DONOTHING,        NULL,                         EMZRenderPostKeyViewTop );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );
        EMZAddToMenu(row++,  0, "",                       0x00, MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL,                         NULL );
    }

    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZRenderPreKeyViewTop(void)
{
    EMZRenderPreKeyView(0);
}
void EMZRenderPreKeyView(uint16_t startpos)
{
    // Locals.
    //
    uint16_t  maxX = OSDGet(ACTIVE_MAX_X);
    char      lineBuf[MAX_INJEDIT_COLS*KEY_INJEDIT_NIBBLES+MENU_ROW_WIDTH+1];
  
    // Adjust start position to get 4 view rows.
    startpos=startpos > KEY_INJEDIT_ROWS-MAX_INJEDIT_ROWS ? (KEY_INJEDIT_ROWS-MAX_INJEDIT_ROWS)*MAX_INJEDIT_COLS : startpos*MAX_INJEDIT_COLS;
  
    // Now add in the key entry text in a smaller font.
    for(uint16_t idx=startpos; idx < startpos+(MAX_INJEDIT_ROWS*MAX_INJEDIT_COLS); idx+=MAX_INJEDIT_COLS)
    {
        lineBuf[0] = 0x00;
        for(uint16_t idx2=0; idx2 < MAX_INJEDIT_COLS && idx+idx2 < MAX_KEY_INS_BUFFER; idx2++)
        {
            sprintf(&lineBuf[strlen(lineBuf)], "%s%02x%02x%02x%02x", idx2 == 0 ? "" : " ",
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[idx+idx2].b[0]), 
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[idx+idx2].b[1]),
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[idx+idx2].b[2]),
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[idx+idx2].b[3]));
        }
        OSDWriteString(10 - (maxX < 512 ? 2 : 0), 6+((idx-startpos)/MAX_INJEDIT_COLS)+(maxX < 512 ? 1 : 0), 0, 0, 0, 0,  maxX < 512 ? FONT_3X6 : FONT_5X7, NORMAL, lineBuf, NULL, PURPLE, BLACK);
    }
}
void EMZRenderPostKeyViewTop(void)
{
    EMZRenderPostKeyView(0);
}
void EMZRenderPostKeyView(uint16_t startpos)
{
    // Locals.
    //
    uint16_t  maxX = OSDGet(ACTIVE_MAX_X);
    char      lineBuf[MAX_INJEDIT_COLS*KEY_INJEDIT_NIBBLES+MENU_ROW_WIDTH+1];
   
    // Adjust start position to get 4 view rows.
    startpos=startpos > KEY_INJEDIT_ROWS-MAX_INJEDIT_ROWS ? (KEY_INJEDIT_ROWS-MAX_INJEDIT_ROWS)*MAX_INJEDIT_COLS : startpos*MAX_INJEDIT_COLS;

    for(uint16_t idx=startpos; idx < startpos+(MAX_INJEDIT_ROWS*MAX_INJEDIT_COLS); idx+=MAX_INJEDIT_COLS)
    {
        lineBuf[0] = 0x00;
        for(uint16_t idx2=0; idx2 < MAX_INJEDIT_COLS && idx+idx2 < MAX_KEY_INS_BUFFER; idx2++)
        {
            sprintf(&lineBuf[strlen(lineBuf)], "%s%02x%02x%02x%02x", idx2 == 0 ? "" : " ",
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+idx2].b[0]), 
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+idx2].b[1]),
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+idx2].b[2]),
                                                                     (emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[idx+idx2].b[3]));
        }
        OSDWriteString(10 - (maxX < 512 ? 2 : 0), 11+((idx-startpos)/MAX_INJEDIT_COLS)+(maxX < 512 ? 1 : 0), 0, 4, 0, 0, maxX < 512 ? FONT_3X6 : FONT_5X7, NORMAL, lineBuf, NULL, GREEN, BLACK);
    }
}

void EMZPreKeyEntry(void)
{
    // Locals.
    uint16_t  maxX = OSDGet(ACTIVE_MAX_X);
    uint8_t   lineBuf[10];

    // Setup the control structure to be used by the key call back eventhandler to correctly update the displayed and stored buffer.
    emuControl.keyInjEdit.bufptr = emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion;
    emuControl.keyInjEdit.editptr = 0;
    emuControl.keyInjEdit.cursorAttr = HILIGHT_BG_WHITE;
    emuControl.keyInjEdit.fg = PURPLE;
    emuControl.keyInjEdit.bg = BLACK;
    emuControl.keyInjEdit.font = maxX < 512 ? FONT_3X6 : FONT_5X7;
    emuControl.keyInjEdit.startRow = 6+(maxX < 512 ? 1 : 0);
    emuControl.keyInjEdit.startCol = 10 - (maxX < 512 ? 2 : 0);
    emuControl.keyInjEdit.offsetRow = 0;
    emuControl.keyInjEdit.offsetCol = 0;
    emuControl.keyInjEdit.cursorFlashRate = 250;
    emuControl.keyInjEdit.curView = 0;
    emuControl.keyInjEdit.render = EMZRenderPreKeyView;

    // Setup cursor on the first nibble in buffer.
    sprintf(lineBuf, "%01x", (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0]>>4));
    OSDSetCursorFlash(emuControl.keyInjEdit.startCol, emuControl.keyInjEdit.startRow, emuControl.keyInjEdit.offsetCol, emuControl.keyInjEdit.offsetRow, emuControl.keyInjEdit.font, lineBuf[0], emuControl.keyInjEdit.fg, emuControl.keyInjEdit.bg, emuControl.keyInjEdit.cursorAttr, emuControl.keyInjEdit.cursorFlashRate);
   
    // Setup to use the key injection editor event handler so all keys entered are redirected into this handler.
    emuControl.activeDialog = DIALOG_KEYENTRY;

    return;
}

void EMZPostKeyEntry(void)
{
    // Locals.
    uint16_t  maxX = OSDGet(ACTIVE_MAX_X);
    uint8_t lineBuf[10];

    // Setup the control structure to be used by the key call back eventhandler to correctly update the displayed and stored buffer.
    emuControl.keyInjEdit.bufptr = emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion;
    emuControl.keyInjEdit.editptr = 0;
    emuControl.keyInjEdit.cursorAttr = HILIGHT_BG_WHITE;
    emuControl.keyInjEdit.fg = GREEN;
    emuControl.keyInjEdit.bg = BLACK;
    emuControl.keyInjEdit.font = maxX < 512 ? FONT_3X6 : FONT_5X7;
    emuControl.keyInjEdit.startRow = 11 +(maxX < 512 ? 1 : 0);
    emuControl.keyInjEdit.startCol = 10 - (maxX < 512 ? 2 : 0);
    emuControl.keyInjEdit.offsetRow = 4;
    emuControl.keyInjEdit.offsetCol = 0;
    emuControl.keyInjEdit.cursorFlashRate = 250;
    emuControl.keyInjEdit.curView = 0;
    emuControl.keyInjEdit.render = EMZRenderPostKeyView;

    // Setup cursor on the first nibble in buffer.
    sprintf(lineBuf, "%01x", (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0]>>4));
    OSDSetCursorFlash(emuControl.keyInjEdit.startCol, emuControl.keyInjEdit.startRow, emuControl.keyInjEdit.offsetCol, emuControl.keyInjEdit.offsetRow, emuControl.keyInjEdit.font, lineBuf[0], emuControl.keyInjEdit.fg, emuControl.keyInjEdit.bg, emuControl.keyInjEdit.cursorAttr, emuControl.keyInjEdit.cursorFlashRate);
   
    // Setup to use the key injection editor event handler so all keys entered are redirected into this handler.
    emuControl.activeDialog = DIALOG_KEYENTRY;

    return;
}
void EMZKeyInjectionEdit(uint8_t data, uint8_t ctrl)
{
    // Locals.
    //
    uint8_t      lineBuf[10];
    uint8_t      row;
    uint8_t      col;
    uint8_t      key;

    char         tmpbuf[MAX_FILENAME_LEN+1];
    uint16_t     rowPixelDepth = (emuControl.fileList.rowFontptr->height + emuControl.fileList.rowFontptr->spacing + emuControl.fileList.padding + 2);
    uint16_t     maxRow        = ((uint16_t)OSDGet(ACTIVE_MAX_Y)/rowPixelDepth) + 1;

    // data = ascii code/ctrl code for key pressed.
    // ctrl = KEY_BREAK & KEY_CTRL & KEY_SHIFT & "000" & KEY_DOWN & KEY_UP
   
    // If the break key is pressed, this is equivalent to escape, ie. exit edit mode.
    if(ctrl & KEY_BREAK_BIT)
    {
        // Disable the cursor.
        OSDClearCursorFlash();

        // Just switch back to active menu dont activate the callback for storing a selection.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    } else
    {
        // Process according to pressed key.
        //
        switch(data)
        {
            // Up key.
            case 0xA0:
                if(emuControl.keyInjEdit.editptr >= KEY_INJEDIT_NIBBLES_PER_ROW)
                {
                    emuControl.keyInjEdit.editptr-=KEY_INJEDIT_NIBBLES_PER_ROW;
                }
                break;
    
            // Down key.
            case 0xA1:
                if(emuControl.keyInjEdit.editptr < ((MAX_KEY_INS_BUFFER*KEY_INJEDIT_NIBBLES)-KEY_INJEDIT_NIBBLES_PER_ROW))
                {
                    emuControl.keyInjEdit.editptr+=KEY_INJEDIT_NIBBLES_PER_ROW;
                }
                break;
               
            // Left key.
            case 0xA4:
                if(ctrl & KEY_SHIFT_BIT)
                {
                    if(emuControl.keyInjEdit.editptr > 1)
                    {
                        emuControl.keyInjEdit.editptr = emuControl.keyInjEdit.editptr >= KEY_INJEDIT_NIBBLES ? ((emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES)-1)*KEY_INJEDIT_NIBBLES : 0;
                    }
                }
                else
                {
                    if(emuControl.keyInjEdit.editptr > 0)
                    {
                        emuControl.keyInjEdit.editptr--;
                    }
                }
                break;

            case 0xA3: // Right Key
                if(ctrl & KEY_SHIFT_BIT)
                {
                    if(emuControl.keyInjEdit.editptr < ((MAX_KEY_INS_BUFFER*KEY_INJEDIT_NIBBLES)-KEY_INJEDIT_NIBBLES))
                    {
                        emuControl.keyInjEdit.editptr = ((emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES)+1)*KEY_INJEDIT_NIBBLES;
                    }
                }
                else
                {
                    if(emuControl.keyInjEdit.editptr < (MAX_KEY_INS_BUFFER*KEY_INJEDIT_NIBBLES)-1)
                    {
                        emuControl.keyInjEdit.editptr++;
                    }
                }
                break;
    
            // All other keys are processed as data.
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            default:
                // When CTRL is pressed we insert a nibble based on the key pressed.
                if(ctrl & KEY_CTRL_BIT)
                {
                    key = toupper(data);
                    if(key >= '0' && key <= '9')
                        key -= '0';
                    else if(key >= 'A' && key <= 'F')
                        key = key - 'A' + 10;
                    else
                        // Npt a hex value.
                        break;

                    // Update the nibble according to edit pointer.
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 0) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] & 0x0f | key << 4;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 1) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] & 0xf0 | key;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 2) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] & 0x0f | key << 4;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 3) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] & 0xf0 | key;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 4) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] & 0x0f | key << 4;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 5) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] & 0xf0 | key;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 6) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] & 0x0f | key << 4;
                    if(emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 7) emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] = emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] & 0xf0 | key;

                    // Move onto next nibble.
                    emuControl.keyInjEdit.editptr += emuControl.keyInjEdit.editptr < (MAX_KEY_INS_BUFFER*KEY_INJEDIT_NIBBLES)-1 ? 1 : 0;
                } else
                {
                    emuControl.keyInjEdit.editptr = (emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES) * KEY_INJEDIT_NIBBLES;
                    t_numCnv map = EMZMapToScanCode(emuControl.hostMachine, data);

                    // If a modifier key is required, it must be inserted first with 0 delay in the down/up pause timers. To avoid clash with other valid combinations,
                    // the delay is set to 0ms down key, 0S up key (stipulated with bit 7 set for seconds).
                    if(map.b[2] != 0xff && map.b[3] != 0xff)
                    {
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] = map.b[2];
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] = map.b[3];
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] = 0x00;
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] = 0x80;
                        emuControl.keyInjEdit.editptr += emuControl.keyInjEdit.editptr < (MAX_KEY_INS_BUFFER-1)*KEY_INJEDIT_NIBBLES ? KEY_INJEDIT_NIBBLES : 0;
                    }
                    // Now insert the actual key. If the buffer is full we overwrite the last position.
                    if(map.b[0] != 0xff && map.b[1] != 0xff)
                    {
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0] = map.b[0];
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1] = map.b[1];
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2] = 0x7f;
                        emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3] = 0x7f;
                        emuControl.keyInjEdit.editptr += emuControl.keyInjEdit.editptr < (MAX_KEY_INS_BUFFER-1)*KEY_INJEDIT_NIBBLES ? KEY_INJEDIT_NIBBLES : 0;
                    }
                }
                break;
        }

        // Set the view portal (to allow for bigger key buffers than the display allows) according to position of edit pointer.
        emuControl.keyInjEdit.curView = (emuControl.keyInjEdit.editptr / KEY_INJEDIT_NIBBLES_PER_ROW) > MAX_INJEDIT_ROWS-1 ? (emuControl.keyInjEdit.editptr / KEY_INJEDIT_NIBBLES_PER_ROW) - MAX_INJEDIT_ROWS +1 : 0;

        col = ((emuControl.keyInjEdit.editptr) % KEY_INJEDIT_NIBBLES_PER_ROW) + ((emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES) % MAX_INJEDIT_COLS);
        row = emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES_PER_ROW > MAX_INJEDIT_ROWS-1 ? MAX_INJEDIT_ROWS-1 : emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES_PER_ROW;

        sprintf(lineBuf, "%01x", emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 0 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0]&0xf0)>>4 : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 1 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[0]&0x0f)    : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 2 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1]&0xf0)>>4 : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 3 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[1]&0x0f)    : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 4 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2]&0xf0)>>4 : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 5 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[2]&0x0f)    : 
                                 emuControl.keyInjEdit.editptr % KEY_INJEDIT_NIBBLES == 6 ? (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3]&0xf0)>>4 : (emuControl.keyInjEdit.bufptr[emuControl.keyInjEdit.editptr/KEY_INJEDIT_NIBBLES].b[3]&0x0f) );
                
        OSDSetCursorFlash(emuControl.keyInjEdit.startCol+col, emuControl.keyInjEdit.startRow+row, emuControl.keyInjEdit.offsetCol, emuControl.keyInjEdit.offsetRow, emuControl.keyInjEdit.font, lineBuf[0], emuControl.keyInjEdit.fg, emuControl.keyInjEdit.bg, emuControl.keyInjEdit.cursorAttr, emuControl.keyInjEdit.cursorFlashRate);
        if(emuControl.keyInjEdit.render != NULL)
            emuControl.keyInjEdit.render(emuControl.keyInjEdit.curView);
    }
    return;
}

// Method to switch to a menu given its integer Id.
//
void EMZSwitchToMenu(int8_t menu)
{
    switch(menu)
    {
        case MENU_MAIN:
            EMZMainMenu();
            break;

        case MENU_TAPE_STORAGE:
            EMZTapeStorageMenu(ACTION_DEFAULT);
            break;

        case MENU_FLOPPY_STORAGE:
            EMZTapeStorageMenu(ACTION_DEFAULT);
            break;

        case MENU_MACHINE:
            EMZMachineMenu(ACTION_DEFAULT);
            break;

        case MENU_DISPLAY:
            EMZDisplayMenu(ACTION_DEFAULT);
            break;

        case MENU_AUDIO:
            EMZAudioMenu(ACTION_DEFAULT);
            break;

        case MENU_SYSTEM:
            EMZSystemMenu(ACTION_DEFAULT);
            break;

        case MENU_ROMMANAGEMENT:
            EMZRomManagementMenu(ACTION_DEFAULT);
            break;

        case MENU_AUTOSTART:
            EMZAutoStartApplicationMenu(ACTION_DEFAULT);
            break;

        default:
            break;
    }
    return;
}


// Method to write out a complete file with the given name and data.
//
int EMZFileSave(const char *fileName, void *data, int size)
{
    // Locals.
    //
    FIL          fileDesc;
    FRESULT      result;
    unsigned int writeSize;
    char         saveName[MAX_FILENAME_LEN+1];

    // If a relative path has been given we need to expand it into an absolute path.
    if(fileName[0] != '/' && fileName[0] != '\\' && (fileName[0] < 0x30 || fileName[0] > 0x32))
    {
        sprintf(saveName, "%s\%s", TOPLEVEL_DIR, fileName);
    } else
    {
        strcpy(saveName, fileName);
    }
printf("Save to File:%s,%s\n", saveName, fileName);

    // Attempt to open the file for writing.
	result = f_open(&fileDesc, saveName, FA_CREATE_ALWAYS | FA_WRITE);
	if(result)
	{
        debugf("EMZFileSave(open) File:%s, error: %d.\n", saveName, fileDesc);
	} else
    {
        // Write out the complete buffer.
        result = f_write(&fileDesc, data, size, &writeSize);
printf("Written:%d, result:%d\n", writeSize, result);
        f_close(&fileDesc);
        if(result)
        {
            debugf("FileSave(write) File:%s, error: %d.\n", saveName, result);
        }
    }
	return(result);
}

// Method to read into memory a complete file with the given name and data.
//
int EMZFileLoad(const char *fileName, void *data, int size)
{
    // Locals.
    //
    FIL          fileDesc;
    FRESULT      result;
    unsigned int readSize;
    char         loadName[MAX_FILENAME_LEN+1];

    // If a relative path has been given we need to expand it into an absolute path.
    if(fileName[0] != '/' && fileName[0] != '\\' && (fileName[0] < 0x30 || fileName[0] > 0x32))
    {
        sprintf(loadName, "%s\%s", TOPLEVEL_DIR, fileName);
    } else
    {
        strcpy(loadName, fileName);
    }

    // Attempt to open the file for reading.
	result = f_open(&fileDesc, loadName, FA_OPEN_EXISTING | FA_READ);
	if(result)
	{
        debugf("EMZFileLoad(open) File:%s, error: %d.\n", loadName, fileDesc);
	} else
    {
        // Read in the complete buffer.
        result = f_read(&fileDesc, data, size, &readSize);
        f_close(&fileDesc);
        if(result)
        {
            debugf("FileLoad(read) File:%s, error: %d.\n", loadName, result);
        }
    }

	return(result);
}

// Method to load the stored configuration, update the hardware and refresh the menu to reflect changes.
//
void EMZReadConfig(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Load config, if the load fails then we remain with the current config.
        //
        EMZLoadConfig();

        // Setup the hardware with the config values.
        EMZSwitchToMachine(emuConfig.machineModel, 0);

        // Recreate the menu with the new config values.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
    return;
}

// Method to write the stored configuration onto the SD card persistence.
//
void EMZWriteConfig(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Load config, if the load fails then we remain with the current config.
        //
        EMZSaveConfig();
      
        // Refresh menu to update cursor.
        EMZRefreshMenu();
    }
    return;
}

// Method to reset the configuration. This is achieved by copying the power on values into the working set.
//
void EMZResetConfig(enum ACTIONMODE mode)
{
    if(mode == ACTION_TOGGLECHOICE)
    {
    } else
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Restore the reset parameters into the working set.
        memcpy(emuConfig.params, emuConfigDefault.params, sizeof(emuConfig.params));
        for(uint16_t idx=0; idx < MAX_MZMACHINES; idx++) { for(uint16_t idx2=0; idx2 < MAX_KEY_INS_BUFFER; idx2++) { if(emuConfig.params[idx].loadApp.preKeyInsertion[idx2].i  == 0) { emuConfig.params[idx].loadApp.preKeyInsertion[idx2].i  = 0xffffffff; } } }
        for(uint16_t idx=0; idx < MAX_MZMACHINES; idx++) { for(uint16_t idx2=0; idx2 < MAX_KEY_INS_BUFFER; idx2++) { if(emuConfig.params[idx].loadApp.postKeyInsertion[idx2].i == 0) { emuConfig.params[idx].loadApp.postKeyInsertion[idx2].i = 0xffffffff; } } }

        // Setup the hardware with the config values.
        EMZSwitchToMachine(emuConfig.machineModel, 0);

        // Recreate the menu with the new config values.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
    return;
}


// Method to read in from disk the persisted configuration.
//
void EMZLoadConfig(void)
{
    if(EMZFileLoad(CONFIG_FILENAME, &emuConfig.params, sizeof(emuConfig.params)))
    {
        debugf("EMZLoadConfig error reading: %s.\n", CONFIG_FILENAME);
    }
}

// Method to save the current configuration onto disk for persistence.
//
void EMZSaveConfig(void)
{
    if(EMZFileSave(CONFIG_FILENAME, emuConfig.params, sizeof(emuConfig.params)))
    {
        debugf("EMZSaveConfig error writing: %s.\n", CONFIG_FILENAME);
    }
}

// Method to switch and configure the emulator according to the values input by the user OSD interaction.
//
void EMZSwitchToMachine(uint8_t machineModel, uint8_t forceROMLoad)
{
    // Locals.
    //
    uint8_t   result = 0;

printf("Machine model:%d, old:%d, change:%d, force:%d, memory:%d\n", machineModel, emuConfig.machineModel, emuConfig.machineChanged, forceROMLoad, emuConfig.params[machineModel].memSize);
    // Go through the active configuration, convert to register values and upload the register values into the FPGA.
    //
    //emuConfig.emuRegisters[MZ_EMU_REG_MODEL]    = (emuConfig.emuRegisters[MZ_EMU_REG_MODEL] & 0xF0) | emuConfig.params[machineModel].vramMode << 4 | machineModel;
    emuConfig.emuRegisters[MZ_EMU_REG_MODEL]    = (EMZGetMemSizeValue() << 4) | (machineModel&0x0f);

printf("DisplayType:%02x, VRAM:%d, GRAM:%d, WAIT:%d, PCG:%d\n", emuConfig.params[machineModel].displayType, emuConfig.params[machineModel].vramMode, emuConfig.params[machineModel].gramMode, emuConfig.params[machineModel].vramWaitMode, emuConfig.params[machineModel].pcgMode);
    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY]  = emuConfig.params[machineModel].pcgMode << 7 | 
                                                  emuConfig.params[machineModel].vramWaitMode << 6 |
                                                  emuConfig.params[machineModel].gramMode << 5 |
                                                  emuConfig.params[machineModel].vramMode << 4 |
                                                  (emuConfig.params[machineModel].displayType & 0x0f); 

    // Display register 2 configures the output type and run time options such as Wait states.
printf("DisplayOutput:%02x,%02x\n", emuConfig.params[machineModel].displayOutput,  emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2]);
    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] = (emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] & 0xF0) | emuConfig.params[machineModel].displayOutput;

    // Display register 3 configures the graphics options such as GRAM/PCG.
    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY3] = EMZGetDisplayOptionValue();

    emuConfig.emuRegisters[MZ_EMU_REG_CPU]      = (emuConfig.emuRegisters[MZ_EMU_REG_CPU] & 0xF8) | emuConfig.params[machineModel].cpuSpeed;
    emuConfig.emuRegisters[MZ_EMU_REG_AUDIO]    = emuConfig.params[machineModel].audioHardware << 7 | emuConfig.params[machineModel].audioMix << 5 | (emuConfig.params[machineModel].audioMute == 1 ? 0 : emuConfig.params[machineModel].audioVolume << 1) | emuConfig.params[machineModel].audioSource;
    emuConfig.emuRegisters[MZ_EMU_REG_CMT]      = emuConfig.params[machineModel].cmtMode << 7 | 
                                                  (emuConfig.params[machineModel].cmtAsciiMapping & 0x03) << 5 |
                                                  emuConfig.params[machineModel].tapeButtons << 3 |
                                                  (emuConfig.params[machineModel].fastTapeLoad & 0x07);
    // No current changes to CMT register 2, just a place holder until needed.
    emuConfig.emuRegisters[MZ_EMU_REG_CMT2]     = emuConfig.emuRegisters[MZ_EMU_REG_CMT2];

    // Setup the hardware switches.
    if(machineModel == MZ800)
    {
        emuConfig.emuRegisters[MZ_EMU_REG_SWITCHES] = 0x0 << 4 | emuConfig.params[machineModel].mz800TapeIn << 3 | emuConfig.params[machineModel].mz800Printer << 2 | emuConfig.params[machineModel].mz800Printer << 1 | emuConfig.params[machineModel].mz800Mode;
    } else
    {
        emuConfig.emuRegisters[MZ_EMU_REG_SWITCHES] = 0x00;
    }

    // Set the model and group according to parameter provided.
    emuConfig.machineModel = machineModel;
    emuConfig.machineGroup = EMZGetMachineGroup();

    // Based on the machine, upload the required ROMS into the emulator. Although the logic is the same for many machines it is seperated below in case specific implementations/configurations are needed.
    //
    if(emuConfig.machineChanged || forceROMLoad)
    {
printf("%s load\n", MZMACHINES[machineModel]);
        if(emuConfig.params[machineModel].romMonitor40.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romMonitor40.romFileName) > 0 && (emuConfig.params[machineModel].displayType == MZ_EMU_DISPLAY_MONO || emuConfig.params[machineModel].displayType == MZ_EMU_DISPLAY_COLOUR)) 
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
        if(emuConfig.params[machineModel].romMonitor80.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romMonitor80.romFileName) > 0 && (emuConfig.params[machineModel].displayType == MZ_EMU_DISPLAY_MONO80 || emuConfig.params[machineModel].displayType == MZ_EMU_DISPLAY_COLOUR80)) 
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor80.romFileName,  0,  emuConfig.params[machineModel].romMonitor80.loadAddr,  emuConfig.params[machineModel].romMonitor80.loadSize, 0, FPGA, 1);
        if(emuConfig.params[machineModel].romCG.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romCG.romFileName) > 0)
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romCG.romFileName,         0,  emuConfig.params[machineModel].romCG.loadAddr,         emuConfig.params[machineModel].romCG.loadSize,     0, FPGA, 1);
        if(emuConfig.params[machineModel].romKeyMap.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romKeyMap.romFileName) > 0)
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romKeyMap.romFileName,     0,  emuConfig.params[machineModel].romKeyMap.loadAddr,     emuConfig.params[machineModel].romKeyMap.loadSize, 0, FPGA, 1);
        if(machineModel == MZ80A && emuConfig.params[machineModel].romUser.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romUser.romFileName) > 0)
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romUser.romFileName,       0,  emuConfig.params[machineModel].romUser.loadAddr,       emuConfig.params[machineModel].romUser.loadSize,   0, FPGA, 1);
        if(emuConfig.params[machineModel].romFDC.romEnabled == 1 && strlen((char *)emuConfig.params[machineModel].romFDC.romFileName) > 0)
            result |= loadZ80Memory((char *)emuConfig.params[machineModel].romFDC.romFileName,        0,  emuConfig.params[machineModel].romFDC.loadAddr,        emuConfig.params[machineModel].romFDC.loadSize,    0, FPGA, 1);
        if(result)
        {
            printf("Error: Failed to load a ROM into the Sharp MZ Series Emulation ROM memory.\n");
        }

        // As the machine changed force a machine reset.
        //
        emuConfig.emuRegisters[MZ_EMU_REG_CTRL] |= 0x01;

        // Next invocation we wont load the roms unless machine changes again.
        emuConfig.machineChanged = 0;
       
        // Write the updated registers into the emulator to configure it.
        writeZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);

        // Clear any reset command.
        emuConfig.emuRegisters[MZ_EMU_REG_CTRL] &= 0xFE;
    } else
    {
        // Write the updated registers into the emulator to configure it.
        writeZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);
    }

    printf("WriteReg: "); for(uint8_t idx=0; idx < 16; idx++) { printf("%02x,", emuConfig.emuRegisters[idx]); } printf("\n");
    readZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);
    printf("ReadReg:  "); for(uint8_t idx=0; idx < 16; idx++) { printf("%02x,", emuConfig.emuRegisters[idx]); } printf("\n");
    return;
}

// Method to handle and process the tape unit, loading files as requested from the queue, saving recorded files to disk and implementing the APSS
// tape drive mechanisms.
// This method is called by interrupt (cmt status change) and periodically (process tape queue).
//
void EMZProcessTapeQueue(void)
{
    // Locals.
    static unsigned long  time = 0;
    uint32_t              timeElapsed;
    char                  *fileName;

    // Get elapsed time since last service poll.
    timeElapsed = *ms - time;    

    // If the elapsed time is too short, ie. because a manual call and a scheduled call occur almost simultaneously, exit as it is not needed.
    if(timeElapsed < 1000)
        return;
   
    // MZ80B APSS functionality.
    //
    if(emuConfig.machineGroup == GROUP_MZ80B)
    {
        // If Eject set, clear queue then issue CMT Register Clear.
        //
        if(emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_EJECT )
        {
            debugf("APSS Eject Cassette (%02x:%02x).", emuConfig.emuRegisters[MZ_EMU_REG_CMT2], MZ_EMU_CMT2_EJECT);
            EMZClearTapeQueue();
        } else

        // If APSS set, rotate queue forward (DIRECTION = 1) or backward (DIRECTION = 0).
        //
        if(emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_APSS )
        {
            debugf("APSS Search %s (%02x:%02x).", emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_DIRECTION ? "Forward" : "Reverse", emuConfig.emuRegisters[MZ_EMU_REG_CMT2], MZ_EMU_CMT2_APSS );
            EMZTapeQueueAPSSSearch(emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_DIRECTION ? 1 : 0);
        }

        // If Play is active, the cache is empty and we are not recording, load into cache the next tape image.
        //
        if((emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_PLAY) && !(emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_PLAY_READY) && !(emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_RECORDING) )
        {
            // Check the tape queue, if items available, read oldest,upload and rotate.
            //
            if(emuControl.tapeQueue.elements > 0)
            {
                // Get the filename from the queue but dont pop it.
                fileName = EMZTapeQueueAPSSSearch(1);

                // If a file was found, upload it into the CMT buffer.
                if(fileName != 0)
                {
                    debugf("APSS Play %s, Rotate Queue Forward.", fileName);
                    debugf("Loading tape: %s\n", fileName);
                    EMZLoadTapeToRAM(fileName, 1);

                    // Need to redraw the menu as the tape queue has changed.
                    if(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] == MENU_TAPE_STORAGE)
                    {
                        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
                    }
                }
            }
        }
    } else
      
    // Standard Tape Deck functionality.
    {
        // Check to see if the Tape READY signal is inactive, if it is and we have items in the queue, load up the next
        // tape file and send it.
        //
        if((emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_SENSE) && !(emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_PLAY_READY))
        {
            // Check the tape queue, if items available, pop oldest and upload.
            //
            if(emuControl.tapeQueue.elements > 0)
            {
                // Get the filename from the queue.
                fileName = EMZTapeQueuePopFile();
               
                // If a file was found, upload it into the CMT buffer.
                if(fileName != 0)
                {
                    debugf("Loading tape: %s\n", fileName);
                    EMZLoadTapeToRAM(fileName, 1);

                    // Need to redraw the menu as the tape queue has changed.
                    EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
                }
            }
        }
    }

    // Check to see if the RECORD_READY flag is set.
    if( (emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_RECORD_READY) )
    {
        EMZSaveTapeFromCMT((const char *)0);
    }
    
    // Reset the timer.
    time = *ms;
}

// Method to invoke necessary services for the Sharp MZ Series emulations, ie. User OSD Menu, Tape Queuing, Floppy Disk loading etc.
// When interrupt = 1 then an interrupt from the FPGA has occurred, when 0 it is a scheduling call.
//
void EMZservice(uint8_t interrupt)
{
    // Locals.
    static uint32_t       entryScreenTimer = 0xFFFFFFFF;
    uint8_t               result;
    uint8_t               emuISRReason[MZ_EMU_INTR_MAX_REGISTERS];
    uint8_t               emuInData[256];
    uint8_t               emuOutData[256];

    // Has an interrupt been generated by the emulator support hardware?
    if(interrupt)
    {
        // Read the reason code register.
        //
        result=readZ80Array(MZ_EMU_REG_INTR_ADDR, emuISRReason, MZ_EMU_INTR_MAX_REGISTERS, FPGA);
printf("IntrReg:"); for(uint16_t idx=0; idx < MZ_EMU_INTR_MAX_REGISTERS; idx++) { printf("%02x ", emuISRReason[idx]); } printf("\n");
        if(!result)
        {
            // Keyboard interrupt?
            if(emuISRReason[MZ_EMU_INTR_REG_ISR] & MZ_EMU_INTR_SRC_KEYB)
            {
                // Read the key pressed.
                result=readZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_CTRL_REG, &emuInData[MZ_EMU_KEYB_CTRL_REG], MZ_EMU_KEYB_MAX_REGISTERS, FPGA);
printf("KeyReg:"); for(uint16_t idx=MZ_EMU_KEYB_CTRL_REG; idx < MZ_EMU_KEYB_CTRL_REG+MZ_EMU_KEYB_MAX_REGISTERS; idx++) { printf("%02x ", emuInData[idx]); } printf("\n");
                if(!result)
                {
                    printf("Received key:%02x, %02x, %d, %d (%d,%d)\n", emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG], emuInData[MZ_EMU_KEYB_KEY_POS_REG], emuInData[MZ_EMU_KEYB_KEY_POS_LAST_REG], emuInData[MZ_EMU_KEYB_FIFO_WR_ADDR], emuInData[MZ_EMU_KEYB_FIFO_RD_ADDR]);

                    // Not interested in up key events or control events except for CTRL+BREAK, discard.
                    if(emuInData[MZ_EMU_KEYB_KEYC_REG] & KEY_DOWN_BIT)
                    {
                        // First time the menu key has been pressed, pop up the menu and redirect all key input to the I/O processor.
                        if(emuControl.activeMenu.menu[0] == MENU_DISABLED && emuInData[MZ_EMU_KEYB_KEYD_REG] == 0xFE)
                        {
                            // Recheck OSD parameters - the running machine may change resolution which in turn changes the OSD settings.
                            OSDUpdateScreenSize();

                            // Setup Menu Font according to available X pixels. Original screen formats only have 320 pixel width.
                            if((uint16_t)OSDGet(ACTIVE_MAX_X) < 512)
                            {
                                EMZSetMenuFont(FONT_5X7);
                            } else
                            {
                                EMZSetMenuFont(FONT_7X8);
                            }

                            // Disable keyboard scan being sent to emulation, enable interrupts on each key press.
                            emuOutData[MZ_EMU_KEYB_CTRL_REG] = MZ_EMU_KEYB_DISABLE_EMU | MZ_EMU_KEYB_ENABLE_INTR;
                            writeZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_CTRL_REG, &emuOutData[MZ_EMU_KEYB_CTRL_REG], 1, FPGA);
                            emuControl.activeMenu.menuIdx = 0;
                            emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_MAIN;

                            // Draw the initial main menu.
                            EMZMainMenu();
                            OSDRefreshScreen();

                            // Enable the OSD.
                            emuOutData[0] = 0x40 | emuConfig.params[emuConfig.machineModel].displayOutput;
                            emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] = emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] | 0x40; 
                            writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY2, emuOutData, 1, FPGA);

                        // Menu active and user has requested to return to emulation?
                        } 
                        else if(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] != MENU_DISABLED && emuInData[MZ_EMU_KEYB_KEYD_REG] == 0xFE)
                        {
                            // Enable keyboard scan being sent to emulation, disable interrupts on each key press.
                            emuOutData[MZ_EMU_KEYB_CTRL_REG] = 0;
                            writeZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_CTRL_REG, &emuOutData[MZ_EMU_KEYB_CTRL_REG], 1, FPGA);
                            emuControl.activeMenu.menuIdx = 0;
                            emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_DISABLED;

                            // Release any allocated menu or file list memory as next invocation restarts at the main menu.
                            EMZReleaseDirMemory();
                            EMZReleaseMenuMemory();

                            // Disable the OSD cursor.
                            OSDClearCursorFlash();

                            // Disable the OSD, this is done by updating the local register copy as the final act is to upload the latest configuration, OSD mode included.
                            emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] = emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] & 0xbf;
                            //emuOutData[0] = 0x0;
                            //writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY3, emuOutData, 1, FPGA);
                          
                            // Switch to the requested machine if changed, update the configuration in the FPGA.
                            //
                            EMZSwitchToMachine(emuConfig.machineModel, 0);

                        // Direct key intercept, process according to state.
                        } else
                        {
                            // Event driven key processing
                            switch(emuControl.activeDialog)
                            {
                                case DIALOG_FILELIST:
                                    EMZProcessFileListKey(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                    break;

                                case DIALOG_KEYENTRY:
                                    EMZKeyInjectionEdit(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                    break;

                                case DIALOG_MENU:
                                default:
                                    EMZProcessMenuKey(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                  //EMZProcessMenuKeyDebug(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                    break;
                            }
                        }
                    }
                } else
                {
                    printf("Key retrieval error.\n");
                }
            }

            // CMT generated an interrupt?
            if(emuISRReason[MZ_EMU_INTR_REG_ISR] & MZ_EMU_INTR_SRC_CMT)
            {
                // Read the cmt status directly. This data can also be ready from the Master Control register of the emulator.
                result=readZ80Array(MZ_EMU_CMT_REG_ADDR, emuInData, MZ_EMU_CMT_MAX_REGISTERS, FPGA);

                // Update the local copy.
                emuConfig.emuRegisters[MZ_EMU_REG_CMT3] = emuInData[MZ_EMU_CMT_STATUS_INTR_REG];
                emuConfig.emuRegisters[MZ_EMU_REG_CMT2] = emuInData[MZ_EMU_CMT_STATUS2_INTR_REG];

                // When debugging output register change as text message.
                debugf("CMT/CMT2 (%s%s%s%s%s%s:%s%s%s%s%s).", emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_PLAY_READY    ? "PLAY_READY,"  : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_PLAYING       ? "PLAYING,"     : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_RECORD_READY  ? "RECORD_READY,": "",
                                                              emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_RECORDING     ? "RECORDING,"   : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_ACTIVE        ? "ACTIVE,"      : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_REG]       & MZ_EMU_CMT_SENSE         ? "SENSE,"       : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_REG]      & MZ_EMU_CMT2_APSS         ? "APSS,"        : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_REG]      & MZ_EMU_CMT2_DIRECTION    ? "DIRECTION,"   : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_REG]      & MZ_EMU_CMT2_EJECT        ? "EJECT,"       : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_REG]      & MZ_EMU_CMT2_PLAY         ? "PLAY,"        : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_REG]      & MZ_EMU_CMT2_STOP         ? "STOP"         : "");
                debugf("CMT/CMT2i(%s%s%s%s%s%s:%s%s%s%s%s).", emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_PLAY_READY    ? "PLAY_READY,"  : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_PLAYING       ? "PLAYING,"     : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_RECORD_READY  ? "RECORD_READY,": "",
                                                              emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_RECORDING     ? "RECORDING,"   : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_ACTIVE        ? "ACTIVE,"      : "",
                                                              emuInData[MZ_EMU_CMT_STATUS_INTR_REG]  & MZ_EMU_CMT_SENSE         ? "SENSE,"       : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_INTR_REG] & MZ_EMU_CMT2_APSS         ? "APSS,"        : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_INTR_REG] & MZ_EMU_CMT2_DIRECTION    ? "DIRECTION,"   : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_INTR_REG] & MZ_EMU_CMT2_EJECT        ? "EJECT,"       : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_INTR_REG] & MZ_EMU_CMT2_PLAY         ? "PLAY,"        : "",
                                                              emuInData[MZ_EMU_CMT_STATUS2_INTR_REG] & MZ_EMU_CMT2_STOP         ? "STOP"         : "");
               
                // Process the tape queue according to signals received from the hardware.
                EMZProcessTapeQueue();
            }
        } else
        {
            printf("Interrupt reason retrieval error.\n");
        }
    }

    // Scheduling block, called periodically by the zOS scheduling.
    else
    {
        // On the first hot-key menu selection, draw the Argo logo and branding.
        if(entryScreenTimer == 0xFFFFFFFF && emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] == MENU_DISABLED)
        {
            OSDClearScreen(BLACK);
            OSDWriteBitmap(128,0,BITMAP_ARGO, RED, BLACK);
            OSDWriteString(31,  6, 0, 10, 0, 0, FONT_9X16, NORMAL, "Sharp MZ Series", NULL, BLUE, BLACK);
            OSDRefreshScreen();
            entryScreenTimer = 0x00FFFFF;
            
            // Enable the OSD.
            emuOutData[0] = 0x40 | emuConfig.params[emuConfig.machineModel].displayOutput;
            writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY2, emuOutData, 1, FPGA);
        }
        else if(entryScreenTimer != 0xFFFFFFFF && entryScreenTimer > 0)
        {
            switch(--entryScreenTimer)
            {
                // Quick wording change...
                case 0x40000:
                    OSDClearScreen(BLACK);
                    OSDWriteBitmap(128,0,BITMAP_ARGO, RED, BLACK);
                    OSDWriteString(31,  6, 0, 10, 0, 0, FONT_9X16, NORMAL, "Argo Inside", NULL, BLUE, BLACK);
                    OSDRefreshScreen();
                    break;

                // Near expiry, clear the OSD and disable it.
                case 0x00100:
                    // Set initial menu on-screen for user to interact with.
                    OSDClearScreen(BLACK);

                    // Disable the OSD.
                    //
                    emuOutData[0] = 0x00 | emuConfig.params[emuConfig.machineModel].displayOutput;
                    writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY2, emuOutData, 1, FPGA);
                    break;

                default:
                    break;
            }
        }

        // When the startup banner has been displayed, normal operations can commence as this module has been initialised.
        else if(entryScreenTimer == 0x00000000)
        {
            // Call the OSD service scheduling routine to allow any OSD updates to take plave.
            OSDService();

            // Process the tape queue periodically, this is generally only needed for uploading a new file from the queue, all other actions are serviced 
            // via interrupt.
            EMZProcessTapeQueue();
        }
    }

    return;
}


// Initialise the EmuMZ subsystem. This method is called at startup to intialise control structures and hardware settings.
//
uint8_t EMZInit(enum MACHINE_HW_TYPES hostMachine)
{
    // Locals.
    //
    uint8_t  result = 0;

    // On initialisation, copy the default configuration into the working data structures and if an SD configuration is found, overwrite as necessary.
    memcpy(&emuControl, &emuControlDefault, sizeof(t_emuControl));
    memcpy(&emuConfig,  &emuConfigDefault,  sizeof(t_emuConfig));
    for(uint16_t idx=0; idx < MAX_MZMACHINES; idx++) { for(uint16_t idx2=0; idx2 < MAX_KEY_INS_BUFFER; idx2++) { if(emuConfig.params[idx].loadApp.preKeyInsertion[idx2].i  == 0) { emuConfig.params[idx].loadApp.preKeyInsertion[idx2].i  = 0xffffffff; } } }
    for(uint16_t idx=0; idx < MAX_MZMACHINES; idx++) { for(uint16_t idx2=0; idx2 < MAX_KEY_INS_BUFFER; idx2++) { if(emuConfig.params[idx].loadApp.postKeyInsertion[idx2].i == 0) { emuConfig.params[idx].loadApp.postKeyInsertion[idx2].i = 0xffffffff; } } }

    // Initialise the on screen display.
    if(!(result=OSDInit(MENU)))
    {
        // Store the host machine under which the emulation is running, needed for features such as keyboard scan code mapping.
        emuControl.hostMachine = hostMachine;

        // Allocate first file list top level directory buffer.
        emuControl.activeDir.dirIdx = 0;

        if((emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = (char *)malloc(strlen(TOPLEVEL_DIR)+1)) == NULL)
        {
            printf("Memory exhausted during init of directory cache, aborting!\n");
        } else
        {
            strcpy(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], TOPLEVEL_DIR);
        }

        // Initialise tape queue.
        //
        for(int i=0; i < MAX_TAPE_QUEUE; i++)
        {
            emuControl.tapeQueue.queue[i] = NULL;
        }
        emuControl.tapeQueue.tapePos      = 0;
        emuControl.tapeQueue.elements     = 0;
        emuControl.tapeQueue.fileName[0]  = 0;        

        // Read in the persisted configuration.
        //
        EMZLoadConfig();

        // Setup the local copy of the emulator register contents.
        if(readZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA))
        {
            printf("Failed to read initial emulator register configuration.\n");
        }
    }

    return(result);
}

// Once the tranZPUter has been engaged and configured, start the emulation with the required machine.
//
void EMZRun(uint8_t machineModel)
{
    // Locals.
    uint8_t     errCode;
    uint8_t     keyCnt;

    // Switch to the requested machine and upload the configuration to the FPGA.
    //
    EMZSwitchToMachine(machineModel, 1);

    // If startup application is enabled and pre key insertion keys given, send them to the emulation keyboard module. This functionality works without 
    // an actual application for load being specified and these keys can also be used to interact with the I/O processor menu, ie. to set a CPU speed.
    if(emuConfig.params[emuConfig.machineModel].loadApp.appEnabled && emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[0].i != 0xffffffff)
    {
        // Count the number of keys for insertion.
        for(keyCnt=0; keyCnt < MAX_KEY_INS_BUFFER; keyCnt++) { if(emuConfig.params[emuConfig.machineModel].loadApp.preKeyInsertion[keyCnt].i == 0xffffffff) break; }

    }

    // If a startup application is specified, load it direct to RAM.
    //
    if(emuConfig.params[emuConfig.machineModel].loadApp.appEnabled && strlen(emuConfig.params[emuConfig.machineModel].loadApp.appFileName) > 0)
    {
        // Use the tape method for loading direct to memory. Currently startup applications limited to tape but this will change as the project progresses.
        errCode = EMZLoadTapeToRAM(emuConfig.params[emuConfig.machineModel].loadApp.appFileName, 0);
        if(errCode != 0)
        {
            debugf("Failed to load startup application:%s to memory.", emuConfig.params[emuConfig.machineModel].loadApp.appFileName);
        }
    }

    // If a startup application is enabled and there are post keys to be injected into the keyboard module, send them.
    //
    if(emuConfig.params[emuConfig.machineModel].loadApp.appEnabled && emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[0].i != 0xffffffff)
    {
        // Count the number of keys for insertion.
        for(keyCnt=0; keyCnt < MAX_KEY_INS_BUFFER; keyCnt++) { if(emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion[keyCnt].i == 0xffffffff) break; }
printf("KeyCnt:%d, addr=%08lx, mem=%08lx\n", keyCnt, MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_FIFO_ADDR, (uint8_t *)&emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion);
        writeZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_FIFO_ADDR, (uint8_t *)&emuConfig.params[emuConfig.machineModel].loadApp.postKeyInsertion, keyCnt*4, FPGA);
    }
}

void EMZProcessMenuKeyDebug(uint8_t data, uint8_t ctrl)
{
    // Locals.
    static uint8_t fg = WHITE;
    static uint8_t bg = BLACK;
    static int8_t row = 0;

    if(data == 'A')
    {
        OSDClearScreen(BLACK);
        OSDWriteChar(0, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'H', fg, bg);
        OSDWriteChar(1, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'E', fg, bg);
        OSDWriteChar(2, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'L', fg, bg);
        OSDWriteChar(3, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'L', fg, bg);
        OSDWriteChar(4, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_5X7, NORMAL, 'y', fg, bg);
                        
        OSDWriteChar(0, 3, 0, 0, 0, 0, FONT_3X6, NORMAL, 'H', fg, bg);
        OSDWriteChar(1, 3, 0, 0, 0, 0, FONT_3X6, NORMAL, 'E', fg, bg);
        OSDWriteChar(2, 3, 0, 0, 0, 0, FONT_3X6, NORMAL, 'L', fg, bg);
        OSDWriteChar(3, 3, 0, 0, 0, 0, FONT_3X6, NORMAL, 'L', fg, bg);
        OSDWriteChar(4, 3, 0, 0, 0, 0, FONT_3X6, NORMAL, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_3X6, NORMAL, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_3X6, NORMAL, 'y', fg, bg);
                        
        OSDWriteChar(0, 4, 0, 0, 0, 0, FONT_7X8, NORMAL, 'H', fg, bg);
        OSDWriteChar(1, 4, 0, 0, 0, 0, FONT_7X8, NORMAL, 'E', fg, bg);
        OSDWriteChar(2, 4, 0, 0, 0, 0, FONT_7X8, NORMAL, 'L', fg, bg);
        OSDWriteChar(3, 4, 0, 0, 0, 0, FONT_7X8, NORMAL, 'L', fg, bg);
        OSDWriteChar(4, 4, 0, 0, 0, 0, FONT_7X8, NORMAL, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_7X8, NORMAL, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_7X8, NORMAL, 'y', fg, bg);
                        
        OSDWriteChar(0, 4, 0, 0, 0, 0, FONT_9X16, NORMAL, 'H', fg, bg);
        OSDWriteChar(1, 4, 0, 0, 0, 0, FONT_9X16, NORMAL, 'E', fg, bg);
        OSDWriteChar(2, 4, 0, 0, 0, 0, FONT_9X16, NORMAL, 'L', fg, bg);
        OSDWriteChar(3, 4, 0, 0, 0, 0, FONT_9X16, NORMAL, 'L', fg, bg);
        OSDWriteChar(4, 4, 0, 0, 0, 0, FONT_9X16, NORMAL, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_9X16, NORMAL, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_9X16, NORMAL, 'y', fg, bg);
                        
        OSDWriteChar(0, 6, 0, 0, 0, 0, FONT_11X16, NORMAL, 'H', fg, bg);
        OSDWriteChar(1, 6, 0, 0, 0, 0, FONT_11X16, NORMAL, 'E', fg, bg);
        OSDWriteChar(2, 6, 0, 0, 0, 0, FONT_11X16, NORMAL, 'L', fg, bg);
        OSDWriteChar(3, 6, 0, 0, 0, 0, FONT_11X16, NORMAL, 'L', fg, bg);
        OSDWriteChar(4, 6, 0, 0, 0, 0, FONT_11X16, NORMAL, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_11X16, NORMAL, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_11X16, NORMAL, 'y', fg, bg);
        OSDRefreshScreen();
    } else if(data == 'B')
    {
        OSDClearScreen(BLACK);
        OSDWriteChar(0, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'H', fg, bg);
        OSDWriteChar(1, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'E', fg, bg);
        OSDWriteChar(2, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'L', fg, bg);
        OSDWriteChar(3, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'L', fg, bg);
        OSDWriteChar(4, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'O', fg, bg);
        OSDWriteChar(5, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'g', fg, bg);
        OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_5X7, DEG90, 'y', fg, bg);
                           
      //OSDWriteChar(3, 0, 0, 0, 0, 0, FONT_3X6, DEG90, 'H', fg, bg);
      //OSDWriteChar(3, 1, 0, 0, 0, 0, FONT_3X6, DEG90, 'E', fg, bg);
      //OSDWriteChar(3, 2, 0, 0, 0, 0, FONT_3X6, DEG90, 'L', fg, bg);
      //OSDWriteChar(3, 3, 0, 0, 0, 0, FONT_3X6, DEG90, 'L', fg, bg);
      //OSDWriteChar(3, 4, 0, 0, 0, 0, FONT_3X6, DEG90, 'O', fg, bg);
      //                   
      //OSDWriteChar(4, 0, 0, 0, 0, 0, FONT_7X8, DEG90, 'H', fg, bg);
      //OSDWriteChar(4, 1, 0, 0, 0, 0, FONT_7X8, DEG90, 'E', fg, bg);
      //OSDWriteChar(4, 2, 0, 0, 0, 0, FONT_7X8, DEG90, 'L', fg, bg);
      //OSDWriteChar(4, 3, 0, 0, 0, 0, FONT_7X8, DEG90, 'L', fg, bg);
      //OSDWriteChar(4, 4, 0, 0, 0, 0, FONT_7X8, DEG90, 'O', fg, bg);
      //                   
      //OSDWriteChar(4, 0, 0, 0, 0, 0, FONT_9X16, DEG90, 'H', fg, bg);
      //OSDWriteChar(4, 1, 0, 0, 0, 0, FONT_9X16, DEG90, 'E', fg, bg);
      //OSDWriteChar(4, 2, 0, 0, 0, 0, FONT_9X16, DEG90, 'L', fg, bg);
      //OSDWriteChar(4, 3, 0, 0, 0, 0, FONT_9X16, DEG90, 'L', fg, bg);
      //OSDWriteChar(4, 4, 0, 0, 0, 0, FONT_9X16, DEG90, 'O', fg, bg);
      //                   
      //OSDWriteChar(6, 0, 0, 0, 0, 0, FONT_11X16, DEG90, 'H', fg, bg);
      //OSDWriteChar(6, 1, 0, 0, 0, 0, FONT_11X16, DEG90, 'E', fg, bg);
      //OSDWriteChar(6, 2, 0, 0, 0, 0, FONT_11X16, DEG90, 'L', fg, bg);
      //OSDWriteChar(6, 3, 0, 0, 0, 0, FONT_11X16, DEG90, 'L', fg, bg);
      //OSDWriteChar(6, 4, 0, 0, 0, 0, FONT_11X16, DEG90, 'O', fg, bg);
      OSDRefreshScreen();
    } else if(data == 'C')
    {
        OSDClearScreen(RED);
        OSDWriteChar(0,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'H', fg, bg);
        OSDWriteChar(1,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'E', fg, bg);
        OSDWriteChar(2,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'L', fg, bg);
        OSDWriteChar(3,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'L', fg, bg);
        OSDWriteChar(4,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'O', fg, bg);
        OSDWriteChar(5,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'g', fg, bg);
        OSDWriteChar(6,  1, 0, 0, 0, 0, FONT_5X7, DEG180, 'y', fg, bg);

        OSDWriteChar(0,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'H', fg, bg);
        OSDWriteChar(1,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'E', fg, bg);
        OSDWriteChar(2,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'L', fg, bg);
        OSDWriteChar(3,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'L', fg, bg);
        OSDWriteChar(4,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'O', fg, bg);
        OSDWriteChar(5,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'g', fg, bg);
        OSDWriteChar(6,  3, 0, 0, 0, 0, FONT_3X6, DEG180, 'y', fg, bg);
                           
        OSDWriteChar(0,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'H', fg, bg);
        OSDWriteChar(1,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'E', fg, bg);
        OSDWriteChar(2,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'L', fg, bg);
        OSDWriteChar(3,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'L', fg, bg);
        OSDWriteChar(4,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'O', fg, bg);
        OSDWriteChar(5,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'g', fg, bg);
        OSDWriteChar(6,  4, 0, 0, 0, 0, FONT_7X8, DEG180, 'y', fg, bg);
                           
        OSDWriteChar(0,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'H', fg, bg);
        OSDWriteChar(1,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'E', fg, bg);
        OSDWriteChar(2,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'L', fg, bg);
        OSDWriteChar(3,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'L', fg, bg);
        OSDWriteChar(4,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'O', fg, bg);
        OSDWriteChar(5,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'g', fg, bg);
        OSDWriteChar(6,  5, 0, 0, 0, 0, FONT_9X16, DEG180, 'y', fg, bg);
                           
        OSDWriteChar(0,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'H', fg, bg);
        OSDWriteChar(1,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'E', fg, bg);
        OSDWriteChar(2,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'L', fg, bg);
        OSDWriteChar(3,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'L', fg, bg);
        OSDWriteChar(4,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'O', fg, bg);
        OSDWriteChar(5,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'g', fg, bg);
        OSDWriteChar(6,  7, 0, 0, 0, 0, FONT_11X16, DEG180, 'y', fg, bg);
        OSDRefreshScreen();
    } else if(data == 'D')
    {
        EMZSetupMenu("SHARP MZ-80A", "Main Menu", FONT_7X8);
        OSDRefreshScreen();
    } else if(data == 'E')
    {
        OSDClearScreen(BLACK);
        OSDWriteString(0,  0, 0, 0, 0, 0, FONT_5X7, NORMAL, "Sharp MZ Series Emulator", NULL, fg, bg);
        OSDRefreshScreen();
    } else if(data == 'F')
    {
        OSDClearScreen(BLACK);
        OSDWriteString(0,  0, 0, 0, 0, 0, FONT_5X7, DEG270, "Sharp MZ Series Emulator", NULL, fg, bg);
        OSDRefreshScreen();
    } else if(data == 'G')
    {
        OSDClearScreen(BLACK);
        OSDDrawCircle(40, 40, 20, WHITE);
        OSDDrawCircle(40, 40, 20, WHITE);
        OSDDrawCircle(60, 60, 20, WHITE);
        OSDRefreshScreen();
    } else if(data == 'H')
    {
        OSDClearScreen(BLACK);
        OSDDrawEllipse(10, 10, 50, 50, RED);
        OSDDrawEllipse(20, 20, 80, 100, BLUE);
        OSDDrawEllipse(100, 20, 200, 100, GREEN);
        OSDRefreshScreen();
    } else if(data == 'I')
    {
        OSDClearScreen(BLACK);
        OSDDrawFilledCircle(40, 40, 20, RED);
        OSDDrawFilledCircle(40, 40, 20, GREEN);
        OSDDrawFilledCircle(60, 60, 20, BLUE);
        OSDRefreshScreen();
    } else if(data == 0xa1)
    {
        row = EMZDrawMenu(++row, 1, MENU_NORMAL);
        OSDRefreshScreen();
    } else if(data == 0xa0)
    {
        row = EMZDrawMenu(--row, 0, MENU_NORMAL);
        OSDRefreshScreen();
    } else if(data == 'M')
    {
        OSDClearScreen(BLACK);
        uint8_t x = 0, y = 0;
        for(uint16_t c=0; c < 256; c++)
        {
            OSDWriteChar(x++, y, 0, 0, 0, 0, FONT_5X7, NORMAL, c,   fg, bg);
            OSDWriteChar(x++, y, 0, 0, 0, 0, FONT_5X7, NORMAL, ' ', fg, bg);
            if((x % 60) == 0)
            {
                x=0;
                y++;
            }
        }
        OSDRefreshScreen();
    } else if(data == 'N')
    {
        EMZMainMenu();
    } else if(data == 'Y')
    {
        fg = (fg + 1)%8;
    } else if(data == 'Z')
    {
        bg = (bg + 1)%8;
    }
    else
    {
        printf("%02x",data);
    }
}


#endif


#ifdef __cplusplus
}
#endif
