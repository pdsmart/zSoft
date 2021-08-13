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
#define MAX_FILENAME_LEN             256                                 // Maximum supported length of a filename.
#define MAX_FILTER_LEN               8                                   // Maximum length of a file filter.
#define TOPLEVEL_DIR                 "0:\\"                              // Top level directory for file list and select.
#define MAX_TAPE_QUEUE               5                                   // Maximum number of files which can be queued in the virtual tape drive.
#define CONFIG_FILENAME              "0:\\EMZ.CFG"                       // Configuration file for persisting the configuration.
#define MAX_EMU_REGISTERS            16                                  // Number of programmable registers in the emulator.

// Maximum number of machines currently supported by the emulation.
//
#define MAX_MZMACHINES               8

// Numeric index of each machine.
//
#define MZ80K_IDX                    0    // 000
#define MZ80C_IDX                    1    // 001
#define MZ1200_IDX                   2    // 010
#define MZ80A_IDX                    3    // 011
#define MZ700_IDX                    4    // 100
#define MZ800_IDX                    5    // 101
#define MZ80B_IDX                    6    // 110
#define MZ2000_IDX                   7    // 111

// Keyboard control bits.
//
#define KEY_BREAK_BIT                0x80                                // Break key is being pressed when set to 1
#define KEY_CTRL_BIT                 0x40                                // CTRL key is being pressed when set to 1
#define KEY_SHIFT_BIT                0x20                                // SHIFT key is being pressed when set to 1
#define KEY_DOWN_BIT                 0x02                                // DATA key has been pressed when set to 1
#define KEY_UP_BIT                   0x01                                // DATA key has been released when set to 1

// Sharp MZ Series Emulator constants.
//
#define MZ_EMU_ROM_ADDR              0x100000                            // Sharp MZ Series Emulation ROM address.
#define MZ_EMU_RAM_ADDR              0x120000                            // Sharp MZ Series Emulation RAM address.
#define MZ_EMU_REG_BASE_ADDR         0x300000                            // Base address, in the FPGA address domain, of the emulator registers.
#define MZ_EMU_REG_INTR_ADDR         0x300020                            // Base address of the interrupt generator.
#define MZ_EMU_REG_KEYB_ADDR         0x300200                            // Base address of the keyboard register and map table.
#define MZ_EMU_CMT_HDR_ADDR          0x340000                            // Header RAM    - 128 bytes 0x340000:0x34FFFF
#define MZ_EMU_CMT_DATA_ADDR         0x350000                            // Data RAM      - 64KBytes. 0x350000:0x35FFFF
#define MZ_EMU_CMT_MAP_ADDR          0x360000                            // ASCII MAP RAM - 512 bytes 0x360000:0x36FFFF
#define MZ_EMU_REG_MODEL             0                                   // Machine MODEL configuration register.            
#define MZ_EMU_REG_DISPLAY           1                                   // DISPLAY configuration register 1.                
#define MZ_EMU_REG_DISPLAY2          2                                   // DISPLAY configuration register 2.                
#define MZ_EMU_REG_DISPLAY3          3                                   // DISPLAY configuration register 3.                
#define MZ_EMU_REG_CPU               4                                   // CPU configuration register.                      
#define MZ_EMU_REG_AUDIO             5                                   // AUDIO configuration register.                    
#define MZ_EMU_REG_CMT               6                                   // CMT (tape drive) configuration register.       
#define MZ_EMU_REG_CMT2              7                                   // CMT (tape drive) apss status register.       
#define MZ_EMU_REG_CMT3              8                                   // CMT (tape drive) status register.       
#define MZ_EMU_REG_USERROM           9                                   // USER ROM selection register (not currently used.)
#define MZ_EMU_REG_FDCROM            10                                  // Floppy Disk ROM selection register.              
#define MZ_EMU_REG_SETUP             13                                  // Emulator current setup (configuration) register. 
#define MZ_EMU_MAX_REGISTERS         16                                  // Maximum number of registers on the emulator.
#define MZ_EMU_ADDR_REG_MODEL        MZ_EMU_REG_BASE_ADDR + 0            // Address of the machine MODEL configuration register.
#define MZ_EMU_ADDR_REG_DISPLAY      MZ_EMU_REG_BASE_ADDR + 1            // Address of the DISPLAY configuration register 1.
#define MZ_EMU_ADDR_REG_DISPLAY2     MZ_EMU_REG_BASE_ADDR + 2            // Address of the DISPLAY configuration register 2.
#define MZ_EMU_ADDR_REG_DISPLAY3     MZ_EMU_REG_BASE_ADDR + 3            // Address of the DISPLAY configuration register 3.
#define MZ_EMU_ADDR_REG_CPU          MZ_EMU_REG_BASE_ADDR + 4            // Address of the CPU configuration register.
#define MZ_EMU_ADDR_REG_AUDIO        MZ_EMU_REG_BASE_ADDR + 5            // Address of the AUDIO configuration register.
#define MZ_EMU_ADDR_REG_CMT          MZ_EMU_REG_BASE_ADDR + 6            // Address of the CMT (tape drive) configuration register.
#define MZ_EMU_ADDR_REG_CMT2         MZ_EMU_REG_BASE_ADDR + 7            // Address of the CMT (tape drive) apss register.
#define MZ_EMU_ADDR_REG_CMT3         MZ_EMU_REG_BASE_ADDR + 8            // Address of the CMT (tape drive) status register.
#define MZ_EMU_ADDR_REG_USERROM      MZ_EMU_REG_BASE_ADDR + 9            // Address of the USER ROM selection register (not currently used.).
#define MZ_EMU_ADDR_REG_FDCROM       MZ_EMU_REG_BASE_ADDR + 10           // Address of the Floppy Disk ROM selection register. 
#define MZ_EMU_ADDR_REG_SETUP        MZ_EMU_REG_BASE_ADDR + 13           // Address of the emulator current setup (configuration) register.
#define MZ_EMU_INTR_ISR              0x00                                // Interupt service reason register, define what caused the interupt.
#define MZ_EMU_INTR_MAX_REGISTERS    1                                   // Maximum number of registers in the interrupt generator.
#define MZ_EMU_KEYB_MAX_REGISTERS    37                                  // Maximum number of registers in the keyboard interface.
#define MZ_EMU_KEYB_KEY_MATRIX       0x00                                // Key matrix array current scan.
#define MZ_EMU_KEYB_KEY_MATRIX_LAST  0x10                                // Key matrix array previous scan.
#define MZ_EMU_KEYB_CTRL_REG         0x20                                // Keyboard control register.
#define MZ_EMU_KEYB_KEYD_REG         0x21                                // Keyboard key data register.
#define MZ_EMU_KEYB_KEYC_REG         0x22                                // Keyboard control data register.
#define MZ_EMU_KEYB_KEY_POS_REG      0x23                                // Keyboard mapped character mapping position.
#define MZ_EMU_KEYB_KEY_POS_LAST_REG 0x24                                // Keyboard mapped character previous mapping position.
#define MZ_EMU_KEYB_MAP_ADDR         0x100                               // Address offset to the scan code:key map array.
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
    MENUSTATE_TEXT                   = 0x04                              // Menu item is visible text for display only
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
    MENU_STORAGE                     = 0x02,                             // Storage menu is active.
    MENU_MACHINE                     = 0x03,                             // Machine menu is active.
    MENU_DISPLAY                     = 0x04,                             // Display menu is active.
    MENU_SYSTEM                      = 0x05,                             // System menu is active.
    MENU_ROMMANAGEMENT               = 0x06                              // Rom Management menu is active.
};

enum MENUCALLBACK {
    MENUCB_DONOTHING                 = 0x00,                             // After callback return continue, no additional processing.
    MENUCB_REFRESH                   = 0x01,                             // After callback, refresh OSD.
};

enum DIALOGTYPE {
    DIALOG_MENU                      = 0x00,                             // OSD is displaying the Menu system.
    DIALOG_FILELIST                  = 0x01,                             // OSD is displaying a file list selection screen.
};

enum ACTIONMODE {
    ACTION_DEFAULT                   = 0x00,                             // Action callback executes default actions.
    ACTION_SELECT                    = 0x01,                             // Action callback executes the selection action.
    ACTION_TOGGLECHOICE              = 0x02,                             // Action callback implements toggle feature.
};


// Declare the menu callback as a type. This callback is used when a menu item such as submenu is activated.
typedef void   (*t_menuCallback)(uint8_t);

// Declare the choice callback as a type. This callback is used when rendering the menu and the choice value needs to be realised from the config settings.
typedef const char * (*t_choiceCallback)(void);

// Declare the return from dialog callback which is required to process data from a non-menu dialog such as a file list.
typedef void   (*t_dialogCallback)(char *param);

// Structure to contain a menu item and its properties.
//
typedef struct {
    char                             text[MENU_ROW_WIDTH];               // Buffers to store menu item text.
    enum MENUTYPES                   type;                               // Type of menu option, sub-menu select or choice.
    enum MENUSTATE                   state;                              // State of the menu item, ie. hidden, greyed, active.
    t_menuCallback                   menuCallback;                       // Function to call when a line is activated, by CR or toggle.
    t_choiceCallback                 choiceCallback;                     // Function to call when a choice value is required.
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
} romData_t;

// MZ Series Tape header structure - 128 bytes.
//
typedef struct
{
    unsigned char  dataType;                           // 01 = machine code program.
                                                       // 02 MZ-80 Basic program.
                                                       // 03 MZ-80 data file.
                                                       // 04 MZ-700 data file.
                                                       // 05 MZ-700 Basic program.
    char           fileName[17];                       // File name.
    unsigned short fileSize;                           // Size of data partition.
    unsigned short loadAddress;                        // Load address of the program/data.
    unsigned short execAddress;                        // Execution address of program.
    unsigned char  comment[104];                       // Free text or code area.
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

// Structure to maintain individual emulation configuration parameters.
typedef struct {
    uint8_t                          cpuSpeed;
    uint8_t                          audioSource;
    uint8_t                          audioVolume;
    uint8_t                          audioMute;
    uint8_t                          displayType;
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
    uint8_t                          tapeAutoSave;
    uint8_t                          tapeButtons;
    uint8_t                          fastTapeLoad;
    uint8_t                          cmtAsciiMapping;                    // Enable Sharp<->ASCII name conversion during Record/Play operations.
    char                             tapeSavePath[MAX_FILENAME_LEN];     // Path where saved files should be stored.
    romData_t                        romMonitor40;                       // Details of 40x25 rom monitor image to upload.
    romData_t                        romMonitor80;                       // Details of 80x25 rom monitor image to upload.
    romData_t                        romCG;                              // Details of rom character generator images to upload.
    romData_t                        romKeyMap;                          // Details of rom Key mapping images to upload.
    romData_t                        romUser;                            // Details of User ROM images to upload.
    romData_t                        romFDC;                             // Details of FDC ROM images to upload.
} t_emuMachineConfig;

// Structure to maintain the emulator configuration which is intended to mirror the physical hardware configuration.
typedef struct {
    uint8_t                          machineModel;                       // Current emulated model.
    uint8_t                          machineChanged;                     // Flag to indicate the base machine has changed.
    t_emuMachineConfig               *resetParams;                       // Copy of the default parameters.
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
    t_fileList                       fileList;                           // List of files for perusal and selection during OSD interaction.
    t_tapeQueue                      tapeQueue;                          // Linked list of files which together form a virtual tape.
} t_emuControl;

// Application execution constants.
//

// Lookup tables for menu entries.
//
const char *MZMACHINES[MAX_MZMACHINES]   = { "MZ-80K", "MZ-80C", "MZ1200", "MZ-80A", "MZ-700", "MZ-800", "MZ-80B", "MZ2000" };
const char *SHARPMZ_FAST_TAPE[]          = { "Off", "2x", "4x", "8x", "16x", "32x", "Off", "Off",
                                             "Off", "2x", "4x", "8x", "16x", "32x", "Off", "Off",
                                             "Off", "2x", "4x", "8x", "16x", "Off", "Off", "Off"
                                           };
const char *SHARPMZ_CPU_SPEED[]          = { "2MHz",   "4MHz", "8MHz",  "16MHz", "32MHz", "64MHz",  "2MHz",   "2MHz",
                                             "3.5MHz", "7MHz", "14MHz", "28MHz", "56MHz", "3.5MHz", "3.5MHz", "3.5MHz", 
                                             "4MHz",   "8MHz", "16MHz", "32MHz", "64MHz", "4MHz",   "4MHz",   "4MHz"
                                           };
const char *SHARPMZ_TAPE_MODE[]          = { "FPGA", "MZ-700" };
const char *SHARPMZ_TAPE_BUTTONS[]       = { "Off", "Play", "Record", "Auto" };
const char *SHARPMZ_ASCII_MAPPING[]      = { "Off", "Record", "Play", "Both" };
const char *SHARPMZ_AUDIO_SOURCE[]       = { "Sound", "Tape" };
const char *SHARPMZ_AUDIO_VOLUME[]       = { "Max", "14", "13", "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1", "Min" };
const char *SHARPMZ_AUDIO_MUTE[]         = { "Off", "Mute" };
const char *SHARPMZ_USERROM_ENABLED[]    = { "Disabled",  "Enabled" };
const char *SHARPMZ_FDCROM_ENABLED[]     = { "Disabled",  "Enabled" };
const char *SHARPMZ_ROM_ENABLED[]        = { "Disabled",  "Enabled" };
const char *SHARPMZ_DISPLAY_TYPE[]       = { "Mono 40x25", "Mono 80x25 ", "Colour 40x25", "Colour 80x25" };
const char *SHARPMZ_DISPLAY_OUTPUT[]     = { "Original", "640x480@60Hz", "1024x768@60Hz", "800x600@60Hz" };

const char *SHARPMZ_ASPECT_RATIO[]       = { "4:3", "16:9" };
const char *SHARPMZ_SCANDOUBLER_FX[]     = { "None", "HQ2x", "CRT 25%", "CRT 50%", "CRT 75%" };
const char *SHARPMZ_VRAMWAIT_MODE[]      = { "Off", "On" };
const char *SHARPMZ_VRAMDISABLE_MODE[]   = { "Enabled", "Disabled" };
const char *SHARPMZ_GRAMDISABLE_MODE[]   = { "Enabled", "Disabled" };
const char *SHARPMZ_GRAM_BASEADDR[]      = { "0x00", "0x08", "0x10", "0x18", "0x20", "0x28", "0x30", "0x38", "0x40", "0x48", "0x50", "0x58", "0x60", "0x68", "0x70", "0x78",
                                             "0x80", "0x88", "0x90", "0x98", "0xA0", "0xA8", "0xB0", "0xB8", "0xC0", "0xC8", "0xD0", "0xD8", "0xE0", "0xE8", "0xF0", "0xF8"  };
const char *SHARPMZ_PCG_MODE[]           = { "Off", "ROM", "RAM" };
const char *SHARPMZ_TAPE_AUTO_SAVE[]     = { "Disabled",  "Enabled" };
const char *SHARPMZ_DEBUG_ENABLE[]       = { "Off", "On" };
const char *SHARPMZ_DEBUG_LEDS[]         = { "Off", "On" };
const char *SHARPMZ_DEBUG_LEDS_BANK[]    = { "T80",    "I/O",      "IOCTL",    "Config",   "MZ80C I",  "MZ80C II", "MZ80B I", "MZ80B II" };
const char *SHARPMZ_DEBUG_LEDS_SUBBANK[] = { "Auto",   "A7-0",     "A15-8",    "DI",       "Signals",  "",         "",        "", 
                                             "Auto",   "Video",    "PS2Key",   "Signals",  "CMT 1",    "CMT 2",    "CMT 3",   "CMT 4",
                                             "Auto",   "A23-16",   "A15-8",    "A7-0",     "Signals",  "",         "",        "",
                                             "Auto",   "Config 1", "Config 2", "Config 3", "Config 4", "Config 5", "",        "",
                                             "Auto",   "CS 1",     "CS 2",     "CS 3",     "INT/RE",   "Clk",      "",        "",
                                             "Auto",   "",         "",         "",         "",         "",         "",        "",
                                             "Auto",   "CS 1",     "CS 2",     "MEM EN",   "INT",      "KEYB",     "PPIA",    "PPIB",
                                             "Auto",   "PPIC",     "",         "",         "",         "",         "",        "",
                                           };
const char *SHARPMZ_DEBUG_CPUFREQ[]      = { "Normal",      "1MHz", "100KHz", "10KHz", "5KHz", "1KHz", "500Hz", "100Hz", "50Hz", "10Hz", "5Hz", "2Hz", "1Hz", "0.5Hz", "0.2Hz", "0.1Hz" };
const char *SHARPMZ_DEBUG_LEDS_SMPFREQ[] = { "CPU",         "1MHz", "100KHz", "10KHz", "5KHz", "1KHz", "500Hz", "100Hz", "50Hz", "10Hz", "5Hz", "2Hz", "1Hz", "0.5Hz", "0.2Hz", "0.1Hz" };
const char *SHARPMZ_MEMORY_BANK[]        = { "SysROM",      "SysRAM",      "KeyMap",      "VRAM",      "CMTHDR",       "CMTDATA",       "CGROM",      "CGRAM",      "All" };
const char *SHARPMZ_MEMORY_BANK_FILE[]   = { "sysrom.dump", "sysram.dump", "keymap.dump", "vram.dump", "cmt_hdr.dump", "cmt_data.dump", "cgrom.dump", "cgram.dump", "all_memory.dump" };
const char *SHARPMZ_TAPE_TYPE[]          = { "N/A", "M/code", "MZ80 Basic", "MZ80 Data", "MZ700 Data", "MZ700 Basic", "N/A" };
const char *SHARPMZ_HELPTEXT[]           = { "Welcome to the Sharp MZ Series!  Use the cursor keys to navigate the menus.  Use space bar or enter to select an item.  Press Esc or F12 to exit the menus. ",
                                          0
                                        };
const char *SHARPMZ_FILE_FILTERS[]       = { "*.MZF", "*.MTI", "*.MZT", "*.*" };


// Prototypes.
//
void       EMZReleaseMenuMemory(void);
void       EMZReleaseDirMemory(void);
void       EMZSetupMenu(char *, char *, enum FONTS);
void       EMZSetupDirList(char *, char *, enum FONTS);
void       EMZAddToMenu(uint8_t, uint8_t, char *, enum MENUTYPES, enum MENUSTATE, t_menuCallback, enum MENUCALLBACK, t_choiceCallback);
int16_t    EMZDrawMenu(int16_t, uint8_t, enum MENUMODE);
void       EMZRefreshMenu(void);
void       EMZRefreshFileList(void);
void       EMZMainMenu(void);
void       EMZTapeStorageMenu(enum ACTIONMODE);
void       EMZMachineMenu(enum ACTIONMODE);
void       EMZDisplayMenu(enum ACTIONMODE);
void       EMZSystemMenu(enum ACTIONMODE);
void       EMZAbout(enum ACTIONMODE);
void       EMZRomManagementMenu(enum ACTIONMODE);
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
void       EMZReset(unsigned long, unsigned long);
uint8_t    EMZInit(uint8_t);

void       EMZLoadDirectToRAM(enum ACTIONMODE);
void       EMZLoadDirectToRAMSet(char *);
void       EMZQueueTape(enum ACTIONMODE);
void       EMZQueueTapeSet(char *);    
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

void       EMZTapeQueuePushFile(char *);
char       *EMZTapeQueuePopFile(void);
char       *EMZTapeQueueAPSSSearch(char);
char       *EMZNextTapeQueueFilename(char);
void       EMZClearTapeQueue(void);
void       EMZChangeCMTMode(enum ACTIONMODE);
short      EMZLoadTapeToRAM(const char *, unsigned char);
short      EMZSaveTapeFromCMT(const char *);

// Menu choice helper functions, increment to next choice.
void       EMZNextMachineModel(enum ACTIONMODE);
void       EMZNextCPUSpeed(enum ACTIONMODE);
void       EMZNextAudioSource(enum ACTIONMODE);
void       EMZNextAudioVolume(enum ACTIONMODE);
void       EMZNextAudioMute(enum ACTIONMODE);

// Getter/Setter methods!
void       EMZSetMenuRowPadding(uint8_t);
void       EMZSetMenuFont(enum FONTS);
void       EMZSetRowColours(enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR, enum COLOUR);  
void       EMZGetMenuBoundaries(int16_t *, int16_t *, int16_t *, int16_t *, int16_t *);
uint16_t   EMZGetMenuColumnWidth(void);

const char *EMZGetMachineModelChoice(void);
char       *EMZGetMachineTitle(void);
short      EMZGetMachineGroup(void);
const char *EMZGetMachineModelChoice(void);
const char *EMZGetCPUSpeedChoice(void);
const char *EMZGetAudioSourceChoice(void);
const char *EMZGetAudioVolumeChoice(void);
const char *EMZGetAudioMuteChoice(void);
const char *EMZGetCMTModeChoice(void);
const char *EMZGetDisplayTypeChoice(void);
const char *EMZGetDisplayOutputChoice(void);
const char *EMZGetVRAMModeChoice(void);
const char *EMZGetGRAMModeChoice(void);
const char *EMZGetVRAMWaitModeChoice(void);
const char *EMZGetPCGModeChoice(void);
const char *EMZGetAspectRatioChoice(void);
const char *EMZGetScanDoublerFXChoice(void);
const char *EMZGetLoadDirectFileFilterChoice(void);
const char *EMZGetQueueTapeFileFilterChoice(void);
const char *EMZGetTapeAutoSaveChoice(void);
const char *EMZGetTapeSaveFilePathChoice(void);
const char *EMZGetMonitorROM40Choice(void);
const char *EMZGetMonitorROM80Choice(void);
const char *EMZGetCGROMChoice(void);
const char *EMZGetKeyMappingROMChoice(void);
const char *EMZGetUserROMChoice(void);
const char *EMZGetFloppyDiskROMChoice(void);

void       EMZNextCMTMode(enum ACTIONMODE);
void       EMZNextDisplayType(enum ACTIONMODE);
void       EMZNextDisplayOutput(enum ACTIONMODE);
void       EMZNextVRAMMode(enum ACTIONMODE);
void       EMZNextGRAMMode(enum ACTIONMODE);
void       EMZNextVRAMWaitMode(enum ACTIONMODE);
void       EMZNextPCGMode(enum ACTIONMODE);
void       EMZNextAspectRatio(enum ACTIONMODE);
void       EMZNextScanDoublerFX(enum ACTIONMODE);
void       EMZNextLoadDirectFileFilter(enum ACTIONMODE);
void       EMZNextQueueTapeFileFilter(enum ACTIONMODE);
void       EMZNextTapeAutoSave(enum ACTIONMODE);
void       EMZNextMonitorROM40(enum ACTIONMODE);
void       EMZNextMonitorROM80(enum ACTIONMODE);
void       EMZNextCGROM(enum ACTIONMODE);
void       EMZNextKeyMappingROM(enum ACTIONMODE);
void       EMZNextUserROM(enum ACTIONMODE);
void       EMZNextFloppyDiskROM(enum ACTIONMODE);




#ifdef __cplusplus
}
#endif
#endif // EMUMZ_H
