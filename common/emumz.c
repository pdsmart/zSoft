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
#define debugf(a, ...)        if(emuControl.debug) { printf("\033[1;31mSHARPMZ: " a "\033[0m\n", ##__VA_ARGS__); }
#define debugfx(a, ...)       if(emuControl.debug) { printf("\033[1;32mSHARPMZ: " a "\033[0m\n", ##__VA_ARGS__); }

//////////////////////////////////////////////////////////////
// Sharp MZ Series Emulation Service Methods                //
//////////////////////////////////////////////////////////////

#ifndef __APP__        // Protected methods which should only reside in the kernel on zOS.
 
// Global scope variables used within the zOS kernel.
//
static t_emuControl          emuControl = {
                                              .active = 0, .debug = 1, .activeDialog = DIALOG_MENU, 
                                              .activeMenu = {
                                                  .menu[0] = MENU_DISABLED, .activeRow[0] = 0, .menuIdx = 0
                                              },
                                              .activeDir = {
                                                  .dir[0] = NULL, .activeRow[0] = 0, .dirIdx = 0
                                              },
                                              .menu = {
                                                  .rowPixelStart = 15, .colPixelStart = 40, .padding = 2, .colPixelsEnd = 12,
                                                  .inactiveFgColour = WHITE, .inactiveBgColour = BLACK, .greyedFgColour = BLUE, .greyedBgColour = BLACK, .textFgColour = CYAN, .textBgColour = BLACK, .activeFgColour = BLUE, .activeBgColour = WHITE,
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

static t_emuConfig           emuConfig = {
                                              .machineModel = MZ80K_IDX, .machineChanged = 1,
                                              .params[MZ80K_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sp1002.rom",        .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80k_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz80k_keymap.rom",  .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ80C_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sp1002.rom",        .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80c_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz80c_keymap.rom",  .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ1200_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sp1002.rom",        .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80c_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz80c_keymap.rom",  .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ80A_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\sa1510.rom",        .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "0:\\TZFS\\sa1510-8.rom",      .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80a_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz80a_keymap.rom",  .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ700_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\1z-013a.rom",       .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "0:\\TZFS\\1z-013a-8.rom",     .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz700_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz700_keymap.rom",  .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ800_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz800_ipl.rom",     .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00004000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz800_cgrom.rom",   .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz800_keymap.rom",  .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ80B_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz80b-ipl.rom",     .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz80b_cgrom.rom",   .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz80b_keymap.rom",  .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              },
                                              .params[MZ2000_IDX] = {
                                                  .cpuSpeed = 0 ,        .audioSource = 0,     .audioVolume = 0,  .audioMute = 0,     .displayType = 0,       .displayOutput = 0, 
                                                  .vramMode = 0,         .gramMode = 0,        .pcgMode = 0,      .aspectRatio = 0,   .scanDoublerFX = 0,     .loadDirectFilter = 0,
                                                  .queueTapeFilter = 0,  .tapeAutoSave = 1,    .tapeButtons = 3,  .fastTapeLoad = 0,  .tapeSavePath = "0:\\MZF",
                                                  .cmtAsciiMapping = 3,  .cmtMode = 0,
                                                  .romMonitor40 = { .romFileName = "0:\\TZFS\\mz2000-ipl.rom",    .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romMonitor80 = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romCG        = { .romFileName = "0:\\TZFS\\mz2000_cgrom.rom",  .romEnabled = 1, .loadAddr = 0x00000000, .loadSize = 0x00000800 },
                                                  .romKeyMap    = { .romFileName = "0:\\TZFS\\mz2000_keymap.rom", .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00001000 },
                                                  .romUser      = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 },
                                                  .romFDC       = { .romFileName = "",                            .romEnabled = 0, .loadAddr = 0x00000000, .loadSize = 0x00000100 }
                                              }
                                          };
uint32_t volatile            *ms = &systick_millis_count;


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
        case MZ80B_IDX:  // MZ80B
        case MZ2000_IDX: // MZ2000
            machineGroup = 2;
            break;

        case MZ80K_IDX:  // MZ80K
        case MZ80C_IDX:  // MZ80C
        case MZ1200_IDX: // MZ1200
        case MZ80A_IDX:  // MZ80A
            machineGroup = 0;
            break;

        case MZ700_IDX:  // MZ700
        case MZ800_IDX:  // MZ800
            machineGroup = 1;
            break;

        default:
            machineGroup = 0;
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
        emuConfig.machineModel = (emuConfig.machineModel+1 >= MAX_MZMACHINES ? 0 : emuConfig.machineModel+1);
        emuConfig.machineChanged = 1;
    }
    return;
}

// Method to return a char string which represents the current selected CPU speed.
const char *EMZGetCPUSpeedChoice(void)
{
    // Locals.
    //

    return(SHARPMZ_CPU_SPEED[(EMZGetMachineGroup() * 8) + emuConfig.params[emuConfig.machineModel].cpuSpeed]);
}

// Method to change the CPU Speed, choice based on the actual selected machine.
void EMZNextCPUSpeed(enum ACTIONMODE mode)
{
    // Locals.
    //
    short machineGroup = EMZGetMachineGroup();

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        if((machineGroup == 0 && emuConfig.params[emuConfig.machineModel].cpuSpeed > 5) || (machineGroup == 1 && emuConfig.params[emuConfig.machineModel].cpuSpeed > 4) || (machineGroup == 2 && emuConfig.params[emuConfig.machineModel].cpuSpeed > 4))
        {
            emuConfig.params[emuConfig.machineModel].cpuSpeed = 0;
        } else
        {
            emuConfig.params[emuConfig.machineModel].cpuSpeed = (emuConfig.params[emuConfig.machineModel].cpuSpeed+1 > 8 ? 0 : emuConfig.params[emuConfig.machineModel].cpuSpeed+1);
        }
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

// Method to return a char string which represents the current selected Display Type.
const char *EMZGetDisplayTypeChoice(void)
{
    // Locals.
    //
    return(SHARPMZ_DISPLAY_TYPE[emuConfig.params[emuConfig.machineModel].displayType]);
}

// Method to change the Display Type, choice based on the actual selected machine.
void EMZNextDisplayType(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].displayType = (emuConfig.params[emuConfig.machineModel].displayType+1 >= NUMELEM(SHARPMZ_DISPLAY_TYPE) ? 0 : emuConfig.params[emuConfig.machineModel].displayType+1);
    }
    return;
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

// Method to enable/disable the 80char monitor ROM and select the image to be used in the ROM.
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
    return(SHARPMZ_FAST_TAPE[emuConfig.params[emuConfig.machineModel].fastTapeLoad]);
}

// Method to change the Fast Tape Load mode, choice based on the actual selected machine.
void EMZNextFastTapeLoad(enum ACTIONMODE mode)
{
    // Locals.
    //

    if(mode == ACTION_DEFAULT || mode == ACTION_TOGGLECHOICE)
    {
        emuConfig.params[emuConfig.machineModel].fastTapeLoad = (emuConfig.params[emuConfig.machineModel].fastTapeLoad+1 >= NUMELEM(SHARPMZ_FAST_TAPE) ? 0 : emuConfig.params[emuConfig.machineModel].fastTapeLoad+1);
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

// Method to add a line into the displayed menu.
//
void EMZAddToMenu(uint8_t row, uint8_t active, char *text, enum MENUTYPES type, enum MENUSTATE state, t_menuCallback mcb, enum MENUCALLBACK cbAction, t_choiceCallback ccb)
{
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
    if(strlen(text) > 0)
        strcpy(emuControl.menu.data[row]->text, text);
    else
        emuControl.menu.data[row]->text[0] = 0x00;
    emuControl.menu.data[row]->type  = type;
    emuControl.menu.data[row]->state = state;
    emuControl.menu.data[row]->menuCallback = mcb;
    emuControl.menu.data[row]->choiceCallback = ccb;
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
    uint16_t     colPixelEnd   = (uint16_t)OSDGet(ACTIVE_MAX_X) - emuControl.menu.colPixelsEnd;
    uint16_t     maxRow        = ((uint16_t)OSDGet(ACTIVE_MAX_Y)/rowPixelDepth) + 1;
    uint8_t      textChrX      = (emuControl.menu.colPixelStart / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing));
    char         activeBuf[MENU_ROW_WIDTH];
    int16_t      firstMenuRow;
    int16_t      firstActiveMenuRow;
    int16_t      lastMenuRow;
    int16_t      lastActiveMenuRow;
    int16_t      visibleRows;

    // Get menu boundaries.
    EMZGetMenuBoundaries(&firstMenuRow, &lastMenuRow, &firstActiveMenuRow, &lastActiveMenuRow, &visibleRows);
printf("first=%d, last=%d, firstactive=%d, lastactive=%d, visible=%d\n", firstMenuRow, lastMenuRow, firstActiveMenuRow, lastActiveMenuRow, visibleRows);
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
printf("%d, %d, %d, %d, %d\n", activeRow, menuRow, emuControl.menu.data[menuRow], emuControl.menu.data[menuRow]->state, dspRow, maxRow);

        // Skip rendering blank lines!
        if(emuControl.menu.data[menuRow]->state != MENUSTATE_BLANK)
        {
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
            }

            // Output the row according to type.
            if(activeRow == menuRow) 
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, emuControl.menu.activeFgColour, emuControl.menu.activeBgColour);
                if(activeRow != -1)
                    emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = activeRow;
            } 
            else if(emuControl.menu.data[menuRow]->state == MENUSTATE_GREYED)
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, emuControl.menu.greyedFgColour, emuControl.menu.greyedBgColour);
            }
            else if(emuControl.menu.data[menuRow]->state == MENUSTATE_TEXT)
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, emuControl.menu.textFgColour, emuControl.menu.textBgColour);
            }
            else
            {
                OSDWriteString(textChrX, dspRow, 0, emuControl.menu.rowPixelStart, xpad, ypad, emuControl.menu.font, NORMAL, activeBuf, emuControl.menu.inactiveFgColour, emuControl.menu.inactiveBgColour);
            }
        }
        dspRow++;
    }

    // If this is a submenu, place a back arrow to indicate you can go back.
    if(emuControl.activeMenu.menuIdx != 0)
    {
        OSDWriteString(textChrX+1, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "\x1b back", CYAN, BLACK);
    }
    // Place scroll arrows if we span a page.
    if(activeRow >= maxRow && visibleRows > maxRow)
    {
printf("Scroll both:%d,%d,%d\n", activeRow, maxRow, visibleRows);
        OSDWriteString(textChrX+71, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x17", CYAN, BLACK);
    } else
    if(activeRow >= maxRow)
    {
printf("Scroll up:%d,%d,%d\n", activeRow, maxRow, visibleRows);
        OSDWriteString(textChrX+71, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x18 ",CYAN, BLACK);
    }
    else if(visibleRows > maxRow)
    {
printf("Scroll down:%d,%d,%d\n", activeRow, maxRow, visibleRows);
        OSDWriteString(textChrX+71, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x19", CYAN, BLACK);
    } else
    {
        OSDWriteString(textChrX+71, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "        ", CYAN, BLACK);
    }
        
printf("RETURN ACTIVEROW=%d\n", activeRow);
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
    uint16_t    menuStartX = (((((uint16_t)OSDGet(ACTIVE_MAX_X) / fontWidth) - (30/fontWidth)) / 2) - strlen(menuTitle)/2) + 1;
    uint16_t    menuTitleLineLeft  = (menuStartX * fontWidth) - 5;
    uint16_t    menuTitleLineRight = ((menuStartX+strlen(menuTitle))*fontWidth) + 3;
  
    // Release previous memory as creation of a new menu will reallocate according to requirements.
    EMZReleaseMenuMemory();

    // Prepare screen background.
    OSDClearScreen(WHITE);
    OSDClearArea(30, -1, -1, -1, BLACK);

    // Side and Top menu titles.
    OSDWriteString(0,          0, 2,  8, 0, 0, FONT_9X16, DEG270, sideTitle, BLACK, WHITE);
    OSDWriteString(menuStartX, 0, 0,  0, 0, 0, font,      NORMAL, menuTitle, WHITE, BLACK);

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
    OSDWriteString(0,          0, 8,  8, 0, 0, FONT_9X16, DEG270, sideTitle, BLUE, WHITE);
    // Adjust the title start location when it is larger than the display area.
    OSDWriteString(menuStartX, 0, 0,  0, 0, 0, font,      NORMAL, (strlen(menuTitle) >= menuTitleWidth - 2) ? &menuTitle[menuTitleWidth - strlen(menuTitle) - 2] : menuTitle, WHITE, BLACK);

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
printf("Calling down\n");
                emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = EMZDrawMenu(++emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx], 1, MENU_WRAP);
printf("Calling Refresh\n");
                OSDRefreshScreen();
printf("Calling done\n");
            }
            break;
           
        // Left key.
        case 0xA4:
printf("HERE 1:%d\n", emuControl.activeMenu.menuIdx);
            if(emuControl.activeMenu.menuIdx != 0)
            {
printf("HERE 2\n");
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
printf("HERE 4:%d\n",  emuControl.activeMenu.menuIdx);
                emuControl.activeMenu.menuIdx = emuControl.activeMenu.menuIdx >= MAX_MENU_DEPTH - 1 ? MAX_MENU_DEPTH - 1 : emuControl.activeMenu.menuIdx+1;
                emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx-1]]->menuCallback(ACTION_SELECT);
            } else
            if(data == 0x0D && emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]] != NULL)
            {
printf("HERE 5:%d\n",  emuControl.activeMenu.menuIdx);
                if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback != NULL)
                    emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->menuCallback(ACTION_SELECT);
                if(emuControl.menu.data[emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx]]->cbAction == MENUCB_REFRESH)
                {
printf("HERE 5 DM:%d\n",  emuControl.activeMenu.menuIdx);
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
    uint16_t     colPixelEnd   = (uint16_t)OSDGet(ACTIVE_MAX_X) - emuControl.fileList.colPixelsEnd;
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
        OSDWriteString(textChrX,    0, 0, 4, 0, 0, FONT_5X7, NORMAL, "\x1b back", CYAN, BLACK);
    }
    // Place scroll arrows if we span a page.
    if(activeRow >= maxRow && visibleRows > maxRow)
    {
        OSDWriteString(textChrX+70, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x17", CYAN, BLACK);
    } else
    if(activeRow >= maxRow)
    {
        OSDWriteString(textChrX+70, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x18 ",CYAN, BLACK);
    }
    else if(visibleRows > maxRow)
    {
        OSDWriteString(textChrX+70, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "scroll \x19", CYAN, BLACK);
    } else
    {
        OSDWriteString(textChrX+70, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "        ", CYAN, BLACK);
    }

printf("first=%d, last=%d, visible=%d\n", firstFileListRow, lastFileListRow, visibleRows);
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
printf("%d, %d, %d, %d, %d\n", activeRow, fileRow, emuControl.fileList.dirEntries[fileRow].name, emuControl.fileList.dirEntries[fileRow].isDir, dspRow, maxRow);

        // Format the data into a single buffer for output.
        uint16_t selectionWidth = EMZGetFileListColumnWidth() - 9;
        uint16_t nameStart      = strlen(emuControl.fileList.dirEntries[fileRow].name) > selectionWidth ? strlen(emuControl.fileList.dirEntries[fileRow].name) - selectionWidth : 0;
        sprintf(activeBuf, " %-*s%-7s ",  selectionWidth, &emuControl.fileList.dirEntries[fileRow].name[nameStart], (emuControl.fileList.dirEntries[fileRow].isDir == 1) ? "<DIR> \x10" : "");

        // Finall output the row according to selection.
        if(activeRow == fileRow) 
        {
            OSDWriteString(textChrX, dspRow, 0, emuControl.fileList.rowPixelStart, xpad, ypad, emuControl.fileList.font, NORMAL, activeBuf, emuControl.fileList.activeFgColour, emuControl.fileList.activeBgColour);
            if(activeRow != -1)
                emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = activeRow;
        } 
        else
        {
            OSDWriteString(textChrX, dspRow, 0, emuControl.fileList.rowPixelStart, xpad, ypad, emuControl.fileList.font, NORMAL, activeBuf, emuControl.fileList.inactiveFgColour, emuControl.fileList.inactiveBgColour);
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
void processFileListKey(uint8_t data, uint8_t ctrl)
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
printf("BREAK pressed\n");
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
    printf("ACTIVE ROW:%d\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] );
                OSDRefreshScreen();
                break;
    
            // Down key.
            case 0xA1:
                // Shift Down pressed?
                if(ctrl & KEY_SHIFT_BIT)
                {
                    emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] + maxRow -1 > 0 ? emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] + maxRow -1 : MAX_DIRENTRY-1;
                }
    printf("BEFORE:%d\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]);
                emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] = EMZDrawFileList(++emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx], 1);
    printf("AFTER:%d\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx]);
    printf("ACTIVE ROW:%d\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx] );
                OSDRefreshScreen();
                break;
               
            // Left key.
            case 0xA4:
    printf("HERE 1:%d\n", emuControl.activeDir.dirIdx);
                if(emuControl.activeDir.dirIdx != 0)
                {
    printf("HERE 2\n");
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
    printf("ACTIVE ROW=%d,%s\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx],emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name );
                            sprintf(tmpbuf, "0:\\%s", emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name);
                        }
                        else
                        {
    printf("ACTIVE ROW=%d\n", emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx],emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name );
                            sprintf(tmpbuf, "%s\\%s", emuControl.activeDir.dir[emuControl.activeDir.dirIdx-1], emuControl.fileList.dirEntries[emuControl.activeDir.activeRow[emuControl.activeDir.dirIdx-1]].name);
                        }
                        if((emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = (char *)malloc(strlen(tmpbuf)+1)) == NULL)
                        {
                            printf("Exhausted memory allocating file directory name buffer\n");
                            return;
                        }
                        strcpy(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], tmpbuf);
    printf("DATA:%d,%s,%s\n", emuControl.activeDir.dirIdx, tmpbuf, emuControl.activeDir.dir[emuControl.activeDir.dirIdx]);
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

// Secondary method called after a file has been chosen.
//
void EMZLoadDirectToRAMSet(char *param)
{
    printf("IVE GOT A FILE:%s\n", param);
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
void EMZClearTapeQueue(void)
{
    // Locals.

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
    emuControl.tapeQueue.elements    = 0;
    emuControl.tapeQueue.tapePos     = 0;
    emuControl.tapeQueue.fileName[0] = 0;
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
        EMZClearTapeQueue();

        // Need to redraw the menu as read only text has changed.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
}

// Method to select a path where tape images will be saved.
void EMZTapeSave(enum ACTIONMODE mode)
{
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Test to see if a recording is in the tape buffer, if it is then save to the name given in the MZF header
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

void EMZReset(unsigned long preResetSleep, unsigned long postResetSleep)
{
}

// Method to reset the machine. This can be done via the hardware, load up the configuration and include the machine definition and the hardware will force a reset.
//
void EMZResetMachine(enum ACTIONMODE mode)
{
    if(mode == ACTION_DEFAULT || mode == ACTION_SELECT)
    {
        // Force a reload of the machine ROM's and reset.
        EMZSwitchToMachine(emuConfig.machineModel, 1);
    }
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
    t_tapeHeader tapeHeader;
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
    result = f_read(&fileDesc, &tapeHeader, MZF_HEADER_SIZE, &actualReadSize);
    if(actualReadSize != 128)
    {
        debugf("Only read:%d bytes of header, aborting.\n", actualReadSize);
        f_close(&fileDesc);
        return(2);
    }

    // Some sanity checks.
    //
    if(tapeHeader.dataType == 0 || tapeHeader.dataType > 5) return(4);
  #if defined __EMUMZ_DEBUG__
    for(int i=0; i < 17; i++)
    {
        fileName[i] = tapeHeader.fileName[i] == 0x0d ? 0x00 : tapeHeader.fileName[i];
    }

    // Debug output to indicate the file loaded and information about the tape image.
    //
    switch(tapeHeader.dataType)
    {
        case 0x01:
            debugf("Binary File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;

        case 0x02:
            debugf("MZ-80 Basic Program(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;

        case 0x03:
            debugf("MZ-80 Data File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;

        case 0x04:
            debugf("MZ-700 Data File(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;

        case 0x05:
            debugf("MZ-700 Basic Program(Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;

        default:
            debugf("Unknown tape type(Type=%02x, Load Addr=%04x, Size=%04x, Exec Addr=%04x, FileName=%s)\n", tapeHeader.dataType, tapeHeader.loadAddress, tapeHeader.fileSize, tapeHeader.execAddress, fileName);
            break;
    }
  #endif

    // Check the data type, only load machine code directly to RAM.
    //
    if(dstCMT == 0 && tapeHeader.dataType != CMT_TYPE_OBJCD)
    {
        f_close(&fileDesc);
        return(3);
    }

    // Reset Emulator if loading direct to RAM. This clears out memory, resets monitor and places it in a known state.
    //
    if(dstCMT == 0)
        EMZReset(10, 50000);

    // Load the data from tape to RAM.
    //
    if(dstCMT == 0)                                       // Load to emulators RAM
    {
        loadAddress = MZ_EMU_RAM_ADDR + tapeHeader.loadAddress;
    } else
    {
        loadAddress = MZ_EMU_CMT_DATA_ADDR;
    }
    for (unsigned short i = 0; i < tapeHeader.fileSize && actualReadSize > 0; i += actualReadSize)
    {
        result = f_read(&fileDesc, sectorBuffer, 512, &actualReadSize);
        if(result)
        {
            debugf("Failed to read data from file:%s @ addr:%08lx, aborting.\n", loadName, loadAddress);
            f_close(&fileDesc);
            return(4);
        }

        debugf("Bytes to read, actual:%d, index:%d, sizeHeader:%d, load:%08lx", actualReadSize, i, tapeHeader.fileSize, loadAddress);

        if(actualReadSize > 0)
        {
            // Write the sector (or part) to the fpga memory.
            writeZ80Array(loadAddress, sectorBuffer, actualReadSize, FPGA);
            loadAddress += actualReadSize;
        } else
        {
            debugf("Bad tape or corruption, should never be 0, actual:%d, index:%d, sizeHeader:%d", actualReadSize, i, tapeHeader.fileSize);
            return(4);
        }
    }

    // Now load header - this is done last because the emulator monitor wipes the stack area on reset.
    //
    writeZ80Array(MZ_EMU_CMT_HDR_ADDR, &tapeHeader, MZF_HEADER_SIZE, FPGA);

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
        if(tapeHeader.fileName[i] == 0x0d) tapeHeader.fileName[i] = 0x00;
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
    t_tapeHeader   tapeHeader;

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
            dataSize = tapeHeader.fileSize;
            readAddress = MZ_EMU_CMT_DATA_ADDR + tapeHeader.loadAddress;
        }
        debugf("mb=%d, tapesize=%04x\n", mb, tapeHeader.fileSize);
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
                memcpy(&tapeHeader, &sectorBuffer, MZF_HEADER_SIZE);

                // Now open the file for writing. If no name provided, use the one stored in the header.
                //
                if(tapeFile == 0)
                {
                    for(int i=0; i < 17; i++)
                    {
                        fileName[i] = tapeHeader.fileName[i] == 0x0d ? 0x00 : tapeHeader.fileName[i];
                    }
                    strcat(fileName, ".mzf");
                    debugf("File from tape:%s (%02x,%04x,%04x,%04x)\n", fileName, tapeHeader.dataType, tapeHeader.fileSize, tapeHeader.loadAddress, tapeHeader.execAddress);
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

void EMZMainMenu(void)
{
    // Locals.
    //
    uint8_t   row = 0;

    // Set active menu for the case when this method is invoked as a menu callback.
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_MAIN;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Main Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "Tape Storage",               MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZTapeStorageMenu,     MENUCB_REFRESH,          NULL );
    EMZAddToMenu(row++,  0, "Machine",                    MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZMachineMenu,         MENUCB_REFRESH,          NULL );
    EMZAddToMenu(row++,  0, "Display",                    MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZDisplayMenu,         MENUCB_REFRESH,          NULL );
    EMZAddToMenu(row++,  0, "System",                     MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZSystemMenu,          MENUCB_REFRESH,          NULL );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Reset",                      MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZResetMachine,        MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Reload config",              MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZReadConfig,          MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Save config",                MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZWriteConfig,         MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Reset config",               MENUTYPE_ACTION,                    MENUSTATE_ACTIVE, EMZResetConfig,         MENUCB_DONOTHING,        NULL );
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
    emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_STORAGE;
    emuControl.activeDialog = DIALOG_MENU;

    EMZSetupMenu(EMZGetMachineTitle(), "Tape Storage Menu", FONT_7X8);
    EMZAddToMenu(row++,  0, "CMT Hardware",               MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZChangeCMTMode,       MENUCB_REFRESH,          EMZGetCMTModeChoice );
    EMZAddToMenu(row++,  0, "Load tape direct to RAM",    MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZLoadDirectToRAM,     MENUCB_DONOTHING,        EMZGetLoadDirectFileFilterChoice );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Queue Tape",                 MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZQueueTape,           MENUCB_DONOTHING,        EMZGetQueueTapeFileFilterChoice );

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
            EMZAddToMenu(row++,  0, lineBuf,                  MENUTYPE_TEXT,                      MENUSTATE_TEXT,   NULL,                   MENUCB_DONOTHING,        NULL );
        }    
    }

    EMZAddToMenu(row++,  0, "Clear Queue",                MENUTYPE_ACTION,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZQueueClear,          MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Save Tape Directory",        MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZTapeSave,            MENUCB_DONOTHING,        EMZGetTapeSaveFilePathChoice );
    EMZAddToMenu(row++,  0, "Auto Save Tape",             MENUTYPE_CHOICE,                    !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextTapeAutoSave,    MENUCB_REFRESH,          EMZGetTapeAutoSaveChoice );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "File Name Ascii Mapping",    MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextCMTAsciiMapping, MENUCB_REFRESH,          EMZGetCMTAsciiMappingChoice );
    EMZAddToMenu(row++,  0, "Tape Buttons",               MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextTapeButtons,     MENUCB_REFRESH,          EMZGetTapeButtonsChoice );
    EMZAddToMenu(row++,  0, "Fast Tape Load",             MENUTYPE_ACTION | MENUTYPE_CHOICE,  !emuConfig.params[emuConfig.machineModel].cmtMode ? MENUSTATE_ACTIVE : MENUSTATE_HIDDEN, EMZNextFastTapeLoad,    MENUCB_REFRESH,          EMZGetFastTapeLoadChoice );
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
    EMZAddToMenu(row++,  0, "Machine Model",              MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextMachineModel,    MENUCB_REFRESH,          EMZGetMachineModelChoice );
    EMZAddToMenu(row++,  0, "CPU Speed",                  MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextCPUSpeed,        MENUCB_REFRESH,          EMZGetCPUSpeedChoice );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Audio Source",               MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioSource,     MENUCB_REFRESH,          EMZGetAudioSourceChoice );
    EMZAddToMenu(row++,  0, "Audio Volume",               MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioVolume,     MENUCB_REFRESH,          EMZGetAudioVolumeChoice );
    EMZAddToMenu(row++,  0, "Audio Mute",                 MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAudioMute,       MENUCB_REFRESH,          EMZGetAudioMuteChoice );
    EMZAddToMenu(row++,  0, "",                           MENUTYPE_BLANK,                     MENUSTATE_BLANK , NULL,                   MENUCB_DONOTHING,        NULL );
    EMZAddToMenu(row++,  0, "Rom Management",             MENUTYPE_SUBMENU,                   MENUSTATE_ACTIVE, EMZRomManagementMenu,   MENUCB_REFRESH,          NULL );
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
    EMZAddToMenu(row++,  0, "Display Type",               MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextDisplayType,     MENUCB_REFRESH,          EMZGetDisplayTypeChoice );
    EMZAddToMenu(row++,  0, "Display Output",             MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextDisplayOutput,   MENUCB_REFRESH,          EMZGetDisplayOutputChoice );
    EMZAddToMenu(row++,  0, "Video",                      MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextVRAMMode,        MENUCB_REFRESH,          EMZGetVRAMModeChoice );
    EMZAddToMenu(row++,  0, "Graphics",                   MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextGRAMMode,        MENUCB_REFRESH,          EMZGetGRAMModeChoice );
    EMZAddToMenu(row++,  0, "VRAM CPU Wait",              MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextVRAMWaitMode,    MENUCB_REFRESH,          EMZGetVRAMWaitModeChoice );
    EMZAddToMenu(row++,  0, "PCG Mode",                   MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextPCGMode,         MENUCB_REFRESH,          EMZGetPCGModeChoice );
    EMZAddToMenu(row++,  0, "Aspect Ratio",               MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextAspectRatio,     MENUCB_REFRESH,          EMZGetAspectRatioChoice );
    EMZAddToMenu(row++,  0, "Scandoubler",                MENUTYPE_CHOICE,                    MENUSTATE_ACTIVE, EMZNextScanDoublerFX,   MENUCB_REFRESH,          EMZGetScanDoublerFXChoice );
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
    EMZAddToMenu(row++,  0, "About",                      MENUTYPE_SUBMENU | MENUTYPE_ACTION, MENUSTATE_ACTIVE, EMZAbout,               MENUCB_REFRESH,          NULL );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
}

void EMZAbout(enum ACTIONMODE mode)
{
    // Locals.
    //
    uint8_t      textChrX      = (emuControl.menu.colPixelStart / (emuControl.menu.rowFontptr->width + emuControl.menu.rowFontptr->spacing));

    // Use the menu framework to draw the borders and title but write a bitmap and text directly.
    //
    EMZSetupMenu(EMZGetMachineTitle(), "About", FONT_7X8);
    OSDWriteBitmap(48,  15, BITMAP_ARGO_MEDIUM, RED, BLACK);
    OSDWriteString(22,   9, 0, 2, 0, 0, FONT_7X8, NORMAL, "Sharp MZ Series v2.0", CYAN, BLACK);
    OSDWriteString(19,  10, 0, 2, 0, 0, FONT_7X8, NORMAL, "(C) Philip Smart, 2018-2021", CYAN, BLACK);
    OSDWriteString(21,  11, 0, 2, 0, 0, FONT_7X8, NORMAL, "MZ-700 Embedded Version", CYAN, BLACK);
    OSDWriteString(textChrX+1, 0, 0, 4, 0, 0, FONT_5X7, NORMAL, "\x1b back", CYAN, BLACK);
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

    EMZAddToMenu(row++,  0, "Monitor ROM (40x25)",        MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZMonitorROM40,        MENUCB_DONOTHING,        EMZGetMonitorROM40Choice );
    EMZAddToMenu(row++,  0, "Monitor ROM (80x25)",        MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZMonitorROM80,        MENUCB_DONOTHING,        EMZGetMonitorROM80Choice );
    EMZAddToMenu(row++,  0, "Character Generator ROM",    MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZCGROM,               MENUCB_DONOTHING,        EMZGetCGROMChoice );
    EMZAddToMenu(row++,  0, "Key Mapping ROM",            MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZKeyMappingROM,       MENUCB_DONOTHING,        EMZGetKeyMappingROMChoice );
    EMZAddToMenu(row++,  0, "User ROM",                   MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZUserROM,             MENUCB_DONOTHING,        EMZGetUserROMChoice );
    EMZAddToMenu(row++,  0, "Floppy Disk ROM",            MENUTYPE_ACTION | MENUTYPE_CHOICE,  MENUSTATE_ACTIVE, EMZFloppyDiskROM,       MENUCB_DONOTHING,        EMZGetFloppyDiskROMChoice );
    // When called as a select callback then the menus are moving forward so start the active row at the top.
    if(mode == ACTION_SELECT) emuControl.activeMenu.activeRow[emuControl.activeMenu.menuIdx] = 0;
    EMZRefreshMenu();
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

        case MENU_STORAGE:
            EMZTapeStorageMenu(ACTION_DEFAULT);
            break;

        case MENU_MACHINE:
            EMZMachineMenu(ACTION_DEFAULT);
            break;

        case MENU_DISPLAY:
            EMZDisplayMenu(ACTION_DEFAULT);
            break;

        case MENU_SYSTEM:
            EMZSystemMenu(ACTION_DEFAULT);
            break;

        case MENU_ROMMANAGEMENT:
            EMZRomManagementMenu(ACTION_DEFAULT);
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
        memcpy(emuConfig.params, emuConfig.resetParams, sizeof(emuConfig.params));

        // Setup the hardware with the config values.

        // Recreate the menu with the new config values.
        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
    }
    return;
}


// Method to read in from disk the persisted configuration.
//
void EMZLoadConfig(void)
{
    if(!EMZFileLoad(CONFIG_FILENAME, emuConfig.params, sizeof(emuConfig.params)))
    {
        debugf("EMZLoadConfig error reading: %s.\n", CONFIG_FILENAME);
    }
}

// Method to save the current configuration onto disk for persistence.
//
void EMZSaveConfig(void)
{
    if(!EMZFileSave(CONFIG_FILENAME, emuConfig.params, sizeof(emuConfig.params)))
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
    uint8_t   result;

printf("Machine model:%d, old:%d, change:%d, force:%d\n", machineModel, emuConfig.machineModel, emuConfig.machineChanged, forceROMLoad);
    // Go through the active configuration, convert to register values and upload the register values into the FPGA.
    //
    emuConfig.emuRegisters[MZ_EMU_REG_MODEL]    = (emuConfig.emuRegisters[MZ_EMU_REG_MODEL] & 0xF0) | machineModel;

    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY]  = emuConfig.params[machineModel].pcgMode << 7 | 
                                                  emuConfig.params[machineModel].vramWaitMode << 6 |
                                                  emuConfig.params[machineModel].gramMode << 5 |
                                                  emuConfig.params[machineModel].vramMode << 4 |
                                                  (emuConfig.params[machineModel].displayType & 0x0f); 

    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] = (emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY2] & 0xF8) | emuConfig.params[machineModel].displayOutput;
    // No current changes to display register 3, just a place holder until needed.
    emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY3] = emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY3];

    emuConfig.emuRegisters[MZ_EMU_REG_CPU]      = (emuConfig.emuRegisters[MZ_EMU_REG_CPU] & 0xF8) | emuConfig.params[machineModel].cpuSpeed;
    emuConfig.emuRegisters[MZ_EMU_REG_AUDIO]    = (emuConfig.emuRegisters[MZ_EMU_REG_AUDIO] & 0xFE)  | emuConfig.params[machineModel].audioSource;
    emuConfig.emuRegisters[MZ_EMU_REG_CMT]      = emuConfig.params[machineModel].cmtMode << 7 | 
                                                  (emuConfig.params[machineModel].cmtAsciiMapping & 0x03) << 5 |
                                                  emuConfig.params[machineModel].tapeButtons << 3 |
                                                  (emuConfig.params[machineModel].fastTapeLoad & 0x07);
    // No current changes to CMT register 2, just a place holder until needed.
    emuConfig.emuRegisters[MZ_EMU_REG_CMT2]     = emuConfig.emuRegisters[MZ_EMU_REG_CMT2];

    // Set the model according to parameter provided.
    emuConfig.machineModel = machineModel;

    printf("Reg: ");
    for(uint8_t idx=0; idx < 16; idx++)
    {
        printf("%02x,", emuConfig.emuRegisters[idx]);
    }
    printf("\n");
  
    // Based on the machine, upload the required ROMS into the emulator. Although the logic is the same for many machines it is seperated below in case specific implementations/configurations are needed.
    //
    if(emuConfig.machineChanged)
    {
        switch(machineModel)
        {
            case MZ80K:
printf("MZ80K load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ80C:
printf("MZ80C load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ1200:
printf("MZ1200 load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ700:
printf("MZ700 load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ800:
printf("MZ800 load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ80B:
printf("MZ80B load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ2000:
printf("MZ2000 load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;

            case MZ80A:
            default:
printf("MZ80A load\n");
                result=loadZ80Memory((char *)emuConfig.params[machineModel].romMonitor40.romFileName,  0,  MZ_EMU_ROM_ADDR+emuConfig.params[machineModel].romMonitor40.loadAddr,  emuConfig.params[machineModel].romMonitor40.loadSize, 0, FPGA, 1);
                break;
        }
        if(result)
        {
            printf("Error: Failed to load BIOS ROM into Sharp MZ Series Emulation ROM memory.\n");
        }

        // Next invocation we wont load the roms unless machine changes again.
        emuConfig.machineChanged = 0;
       
        // Write the updated registers into the emulator to configure it, including the machine mode as this will cause a reset prior to run mode.
        writeZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);
    } else
    {
        // Write the updated registers into the emulator to configure it ready for run mode.
        writeZ80Array(MZ_EMU_ADDR_REG_MODEL+1, &emuConfig.emuRegisters[1], MZ_EMU_MAX_REGISTERS-1, FPGA);
    }

    readZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);
    printf("ReadBack Reg: ");
    for(uint8_t idx=0; idx < 16; idx++)
    {
        printf("%02x,", emuConfig.emuRegisters[idx]);
    }
    printf("\n");
    return;
}

void EMZProcessTapeQueue(void)
{
    // Locals.
    static unsigned long  time = 0;
    uint32_t              timeElapsed;
    char                  *fileName;

    // Get elapsed time since last service poll.
    timeElapsed = *ms - time;    
   
    // Every 2 seconds (READY signal takes 1 second to become active after previous load) to see if the tape buffer is empty.
    //
    if(timeElapsed > 1000)
    {
        // Take snapshot of registers.
        //
        readZ80Array(MZ_EMU_ADDR_REG_MODEL, emuConfig.emuRegisters, MZ_EMU_MAX_REGISTERS, FPGA);

    printf("Poll Reg: ");
    for(uint8_t idx=0; idx < 16; idx++)
    {
        printf("%02x,", emuConfig.emuRegisters[idx]);
    }
    printf("\n");

        // When debugging output register change as text message.
        debugf("CMT/CMT2 (%s%s%s%s%s%s%s:%s%s%s%s%s).", emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_PLAY_READY   ? "PLAY_READY,"  : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_PLAYING      ? "PLAYING,"     : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_RECORD_READY ? "RECORD_READY,": "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_RECORDING    ? "RECORDING,"   : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_ACTIVE       ? "ACTIVE,"      : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_SENSE        ? "SENSE,"       : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_WRITEBIT     ? "WRITEBIT,"    : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_APSS        ? "APSS,"        : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_DIRECTION   ? "DIRECTION,"   : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_EJECT       ? "EJECT,"       : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_PLAY        ? "PLAY,"        : "",
                                                        emuConfig.emuRegisters[MZ_EMU_REG_CMT2] & MZ_EMU_CMT2_STOP        ? "STOP"         : "");

        // MZ80B APSS functionality.
        //
        if(EMZGetMachineGroup() == 2)
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
                        EMZSwitchToMenu(emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx]);
                    }
                }
            }
        } else
        {
            // Check to see if the Tape READY signal is inactive, if it is and we have items in the queue, load up the next
            // tape file and send it.
            //
            if((emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_SENSE) && !(emuConfig.emuRegisters[MZ_EMU_REG_CMT3] & MZ_EMU_CMT_PLAY_READY) )
            {
debugf("Tape drive ready to load\n");
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

}

// Method to invoke necessary services for the Sharp MZ Series emulations, ie. User OSD Menu, Tape Queuing, Floppy Disk loading etc.
// When interrupt = 1 then an interrupt from the FPGA has occurred, when 0 it is a scheduling call.
//
void EMZservice(uint8_t interrupt)
{
    // Locals.
    static uint32_t       entryScreenTimer = 0xFFFFFFFF;
    uint8_t               result;
    uint8_t               emuInData[256];
    uint8_t               emuOutData[256];

    // Has an interrupt been generated by the emulator support hardware?
    if(interrupt)
    {
        printf("Interrupt received.\n");

        // Read the reason code register.
        //
        result=readZ80Array(MZ_EMU_REG_INTR_ADDR, emuInData, MZ_EMU_INTR_MAX_REGISTERS, FPGA);
        if(!result)
        {
            printf("Reason code:%02x\n", emuInData[MZ_EMU_INTR_ISR]);
            // Keyboard interrupt?
            if(emuInData[0] & 0x01)
            {
                // Read the key pressed.
                result=readZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_CTRL_REG, &emuInData[MZ_EMU_KEYB_CTRL_REG], 5, FPGA);
                if(!result)
                {
                    printf("Received key:%02x, %02x, %d, %d\n", emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG], emuInData[MZ_EMU_KEYB_KEY_POS_REG], emuInData[MZ_EMU_KEYB_KEY_POS_LAST_REG]);

                    // First time the menu key has been pressed, pop up the menu and redirect all key input to the I/O processor.
                    if(emuControl.activeMenu.menu[0] == MENU_DISABLED && emuInData[MZ_EMU_KEYB_KEYD_REG] == 0xFE)
                    {
                        // Disable keyboard scan being sent to emulation, enable interrupts on each key press.
                        emuOutData[MZ_EMU_KEYB_CTRL_REG] = MZ_EMU_KEYB_DISABLE_EMU | MZ_EMU_KEYB_ENABLE_INTR;
                        writeZ80Array(MZ_EMU_REG_KEYB_ADDR+MZ_EMU_KEYB_CTRL_REG, &emuOutData[MZ_EMU_KEYB_CTRL_REG], 1, FPGA);
                        emuControl.activeMenu.menuIdx = 0;
                        emuControl.activeMenu.menu[emuControl.activeMenu.menuIdx] = MENU_MAIN;

                        // Draw the initial main menu.
                        EMZMainMenu();
                        OSDRefreshScreen();

                        // Enable the OSD.
                        emuOutData[0] = 0x1;
                        writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY3, emuOutData, 1, FPGA);

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

                        // Disable the OSD, this is done by updating the local register copy as the final act is to upload the latest configuration, OSD mode included.
                        emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY3] = emuConfig.emuRegisters[MZ_EMU_REG_DISPLAY3] & 0xfe;
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
                                processFileListKey(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                break;

                            case DIALOG_MENU:
                            default:
                                EMZProcessMenuKey(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                              //EMZProcessMenuKeyDebug(emuInData[MZ_EMU_KEYB_KEYD_REG], emuInData[MZ_EMU_KEYB_KEYC_REG]);
                                break;
                        }
                    }
                } else
                {
                    printf("Key retrieval error.\n");
                }
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
            OSDWriteString(31,  6, 0, 10, 0, 0, FONT_9X16, NORMAL, "Sharp MZ Series", BLUE, BLACK);
            OSDRefreshScreen();
            entryScreenTimer = 0x01FFFFF;
            
            // Enable the OSD.
            emuOutData[0] = 0x1;
            writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY3, emuOutData, 1, FPGA);
        }
        else if(entryScreenTimer != 0xFFFFFFFF && entryScreenTimer > 0)
        {
            switch(--entryScreenTimer)
            {
                // Quick wording change...
                case 0x80000:
                    OSDClearScreen(BLACK);
                    OSDWriteBitmap(128,0,BITMAP_ARGO, RED, BLACK);
                    OSDWriteString(31,  6, 0, 10, 0, 0, FONT_9X16, NORMAL, "Argo Inside", BLUE, BLACK);
                    OSDRefreshScreen();
                    break;

                // Near expiry, clear the OSD and disable it.
                case 0x00100:
                    // Set initial menu on-screen for user to interact with.
                    OSDClearScreen(BLACK);

                    // Disable the OSD.
                    //
                    emuOutData[0] = 0x0;
                    writeZ80Array(MZ_EMU_ADDR_REG_DISPLAY3, emuOutData, 1, FPGA);
                    break;

                default:
                    break;
            }
        }

        // When the startup banner has been displayed, normal operations can commence as this module has been initialised.
        else if(entryScreenTimer == 0x00000000)
        {
            // Process the tape queue according to signals received from the hardware.
            EMZProcessTapeQueue();
        }
    }

    return;
}


// Initialise the EmuMZ subsystem. This method is called at startup to intialise control structures and hardware settings.
//
uint8_t EMZInit(uint8_t machineModel)
{
    // Locals.
    //
    uint8_t  result = 0;

    // Initialise the on screen display.
    if(!(result=OSDInit(MENU)))
    {
        // Allocate first file list top level directory buffer.
        emuControl.activeDir.dirIdx = 0;
        if((emuControl.activeDir.dir[emuControl.activeDir.dirIdx] = (char *)malloc(strlen(TOPLEVEL_DIR)+1)) == NULL)
        {
            printf("Memory exhausted during init of directory cache, aborting!\n");
        } else
        {
            strcpy(emuControl.activeDir.dir[emuControl.activeDir.dirIdx], TOPLEVEL_DIR);
        }

        // Allocate a block of memory to store the config reset parameters.
        //
        if((emuConfig.resetParams = (t_emuMachineConfig*)malloc(sizeof(emuConfig.params))) == NULL)
        {
            printf("Memory exhausted during init of parameter reset config, aborting!\n");
        } else
        {
            // Backup the default parameter values as the reset parameters.
            memcpy(emuConfig.resetParams, emuConfig.params, sizeof(emuConfig.params));
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

        // Switch to the requested machine and upload the configuration to the FPGA.
        //
        EMZSwitchToMachine(machineModel, 0);
    }

    return(result);
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
        OSDWriteString(0,  0, 0, 0, 0, 0, FONT_5X7, NORMAL, "Sharp MZ Series Emulator", fg, bg);
        OSDRefreshScreen();
    } else if(data == 'F')
    {
        OSDClearScreen(BLACK);
        OSDWriteString(0,  0, 0, 0, 0, 0, FONT_5X7, DEG270, "Sharp MZ Series Emulator", fg, bg);
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
