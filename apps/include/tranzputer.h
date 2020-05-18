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
#define REFRESH_BYTE_COUNT   8                                           // This constant controls the number of bytes read/written to the z80 bus before a refresh cycle is needed.
#define FILL_RFSH_BYTE_CNT   256                                         // Number of bytes we can write before needing a full refresh for the DRAM.

// IO addresses on the tranZPUter or mainboard.
//
#define IO_TZ_CTRLLATCH      0x60


// SHarp MZ80A constants.
//
#define MZ_VID_RAM_ADDR      0xD000                                      // Start of Video RAM
#define MZ_VID_RAM_SIZE      2048                                        // Size of Video RAM.
#define MZ_VID_DFLT_BYTE     0x00                                        // Default character (SPACE) for video RAM.
#define MZ_ATTR_RAM_ADDR     0xD800                                      // On machines with the upgrade, the start of the Attribute RAM.
#define MZ_ATTR_RAM_SIZE     2048                                        // Size of the attribute RAM.
#define MZ_ATTR_DFLT_BYTE    0x07                                        // Default colour (White on Black) for the attribute.
#define MZ_SCROL_BASE        0xE200                                      // Base address of the hardware scroll registers.


// Pin Constants - Pins assigned at the hardware level to specific tasks/signals.
//
#define MAX_TRANZPUTER_PINS  47
#define Z80_MEM0_PIN         46
#define Z80_MEM1_PIN         47
#define Z80_MEM2_PIN         48
#define Z80_MEM3_PIN         49
#define Z80_MEM4_PIN         50
#define Z80_WR_PIN           10
#define Z80_RD_PIN           12
#define Z80_IORQ_PIN         8
#define Z80_MREQ_PIN         9
#define Z80_A0_PIN           39
#define Z80_A1_PIN           38
#define Z80_A2_PIN           37
#define Z80_A3_PIN           36
#define Z80_A4_PIN           35
#define Z80_A5_PIN           34
#define Z80_A6_PIN           33
#define Z80_A7_PIN           32
#define Z80_A8_PIN           31
#define Z80_A9_PIN           30
#define Z80_A10_PIN          29
#define Z80_A11_PIN          28
#define Z80_A12_PIN          27
#define Z80_A13_PIN          26
#define Z80_A14_PIN          25
#define Z80_A15_PIN          24
#define Z80_A16_PIN          23
#define Z80_A17_PIN          22
#define Z80_A18_PIN          21
#define Z80_D0_PIN           0
#define Z80_D1_PIN           1
#define Z80_D2_PIN           2
#define Z80_D3_PIN           3
#define Z80_D4_PIN           4
#define Z80_D5_PIN           5
#define Z80_D6_PIN           6
#define Z80_D7_PIN           7
#define Z80_WAIT_PIN         13
#define Z80_BUSACK_PIN       17
#define Z80_NMI_PIN          43
#define Z80_INT_PIN          44
#define CTL_RFSH_PIN         45
#define CTL_HALT_PIN         14
#define CTL_M1_PIN           20
#define CTL_BUSRQ_PIN        15
#define CTL_BUSACK_PIN       16
#define CTL_CLK_PIN          18
#define CTL_CLKSLCT_PIN      19

// Customised pin manipulation methods implemented as stripped down macros. The original had too much additional overhead with procedure call and validation tests,
// speed is of the essence for this project as pins change mode and value constantly.
//
#define pinLow(a)            *portClearRegister(pinMap[a]) = 1
#define pinHigh(a)           *portSetRegister(pinMap[a]) = 1
#define pinSet(a, b)         if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; }
#define pinGet(a)            *portInputRegister(pinMap[a])
#define pinInput(a)          { *portModeRegister(pinMap[a]) = 0; *ioPin[pinMap[a]] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS; }
#define pinOutput(a)         { *portModeRegister(pinMap[a]) = 1;\
                               *ioPin[pinMap[a]] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                               *ioPin[pinMap[a]] &= ~PORT_PCR_ODE; }
#define pinOutputSet(a,b)    { *portModeRegister(pinMap[a]) = 1;\
                               *ioPin[pinMap[a]] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);\
                               *ioPin[pinMap[a]] &= ~PORT_PCR_ODE;\
                               if(b) { *portSetRegister(pinMap[a]) = 1; } else { *portClearRegister(pinMap[a]) = 1; } }

#define setZ80Data(a)        { pinSet(Z80_D7,  ((a >>  7) & 0x1)); pinSet(Z80_D6,  ((a >>  6) & 0x1));\
                               pinSet(Z80_D5,  ((a >>  5) & 0x1)); pinSet(Z80_D4,  ((a >>  4) & 0x1));\
                               pinSet(Z80_D3,  ((a >>  3) & 0x1)); pinSet(Z80_D2,  ((a >>  2) & 0x1));\
                               pinSet(Z80_D1,  ((a >>  1) & 0x1)); pinSet(Z80_D0,  ((a      ) & 0x1)); }
#define setZ80Addr(a)        { pinSet(Z80_A15, ((a >> 15) & 0x1)); pinSet(Z80_A14, ((a >> 14) & 0x1));\
                               pinSet(Z80_A13, ((a >> 13) & 0x1)); pinSet(Z80_A12, ((a >> 12) & 0x1));\
                               pinSet(Z80_A11, ((a >> 11) & 0x1)); pinSet(Z80_A10, ((a >> 10) & 0x1));\
                               pinSet(Z80_A9,  ((a >>  9) & 0x1)); pinSet(Z80_A8,  ((a >>  8) & 0x1));\
                               pinSet(Z80_A7,  ((a >>  7) & 0x1)); pinSet(Z80_A6,  ((a >>  6) & 0x1));\
                               pinSet(Z80_A5,  ((a >>  5) & 0x1)); pinSet(Z80_A4,  ((a >>  4) & 0x1));\
                               pinSet(Z80_A3,  ((a >>  3) & 0x1)); pinSet(Z80_A2,  ((a >>  2) & 0x1));\
                               pinSet(Z80_A1,  ((a >>  1) & 0x1)); pinSet(Z80_A0,  ((a      ) & 0x1)); }
#define setZ80RefreshAddr(a) { pinSet(Z80_A6,  ((a >>  6) & 0x1)); pinSet(Z80_A5,  ((a >>  5) & 0x1));\
                               pinSet(Z80_A4,  ((a >>  4) & 0x1)); pinSet(Z80_A3,  ((a >>  3) & 0x1));\
                               pinSet(Z80_A2,  ((a >>  2) & 0x1)); pinSet(Z80_A1,  ((a >>  1) & 0x1));\
                               pinSet(Z80_A0,  ((a      ) & 0x1)); }
#define readDataBus()        ( pinGet(Z80_D7) << 7 | pinGet(Z80_D6) << 6 | pinGet(Z80_D5) << 5 | pinGet(Z80_D4) << 4 |\
                               pinGet(Z80_D3) << 3 | pinGet(Z80_D2) << 2 | pinGet(Z80_D1) << 1 | pinGet(Z80_D0) )
//#define readCtrlLatch()      ( pinGet(Z80_A18) << 7  | pinGet(Z80_A17) << 6  | pinGet(Z80_A16) << 5  | pinGet(Z80_MEM4) << 4 |\
//                               pinGet(Z80_MEM3) << 3 | pinGet(Z80_MEM2) << 2 | pinGet(Z80_MEM1) << 1 | pinGet(Z80_MEM0) )
// Special case during development where the pins for the MEM4:1 are not connected.
#define readCtrlLatch()      ((pinGet(Z80_A18) << 7  | pinGet(Z80_A17) << 6  | pinGet(Z80_A16) << 5) & 0b11100000)
#define writeCtrlLatch(a)    { writeZ80IO(IO_TZ_CTRLLATCH, a); } 
#define readUpperAddr()      ((pinGet(Z80_A18) << 2  | pinGet(Z80_A17) << 1  | pinGet(Z80_A16)) & 0b00000111)
#define setZ80Direction(a)   { for(uint8_t idx=Z80_D0; idx <= Z80_D7; idx++) { if(a == WRITE) { pinOutput(idx); } else { pinInput(idx); } }; z80Control.busDir = a; }
#define reqZ80BusChange(a)   { if(a == MAINBOARD_ACCESS && z80Control.ctrlMode == TRANZPUTER_ACCESS) \
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


// Enumeration of the various pins on the project. These enums make it easy to refer to a signal and they are mapped
// to the actual hardware pin via the pinMap array.
// One of the big advantages is that a swath of pins, such as the address lines, can be switched in a tight loop rather than 
// individual pin assignments or clunky lists.
//
enum pinIdxToPinNumMap { 
    Z80_A0            = 0,
    Z80_A1            = 1,
    Z80_A2            = 2,
    Z80_A3            = 3,
    Z80_A4            = 4,
    Z80_A5            = 5,
    Z80_A6            = 6,
    Z80_A7            = 7,
    Z80_A8            = 8,
    Z80_A9            = 9,
    Z80_A10           = 10,
    Z80_A11           = 11,
    Z80_A12           = 12,
    Z80_A13           = 13,
    Z80_A14           = 14,
    Z80_A15           = 15,
    Z80_A16           = 16,
    Z80_A17           = 17,
    Z80_A18           = 18,

    Z80_D0            = 19,
    Z80_D1            = 20,
    Z80_D2            = 21,
    Z80_D3            = 22,
    Z80_D4            = 23,
    Z80_D5            = 24,
    Z80_D6            = 25,
    Z80_D7            = 26,

    Z80_MEM0          = 27,
    Z80_MEM1          = 28,
    Z80_MEM2          = 29,
    Z80_MEM3          = 30,
    Z80_MEM4          = 31,

    Z80_IORQ          = 32,
    Z80_MREQ          = 33,
    Z80_RD            = 34,
    Z80_WR            = 35,
    Z80_WAIT          = 36,
    Z80_BUSACK        = 37,

    Z80_NMI           = 38,
    Z80_INT           = 39,

    CTL_BUSACK        = 40,
    CTL_BUSRQ         = 41,
    CTL_RFSH          = 42,
    CTL_HALT          = 43,
    CTL_M1            = 44,
    CTL_CLK           = 45,
    CTL_CLKSLCT       = 46
};

// Possible control modes that the K64F can be in, do nothing where the Z80 runs normally, control the Z80 and mainboard, or control the Z80 and tranZPUter.
enum CTRL_MODE {
    Z80_RUN           = 0,
    TRANZPUTER_ACCESS = 1,
    MAINBOARD_ACCESS  = 2
};

// Possible bus directions that the K64F can setup for controlling the Z80.
enum BUS_DIRECTION {
    READ              = 0,
    WRITE             = 1,
    TRISTATE          = 2
};

// Possible video frames stored internally.
//
enum VIDEO_FRAMES {
    SAVED             = 0,
    WORKING           = 1
};

// Structure to maintain all the control and management variables so that the state of run is well known by any called method.
//
typedef struct {
    uint8_t            refreshAddr;                  // Refresh address for times when the K64F must issue refresh cycles on the Z80 bus.
    uint8_t            runCtrlLatch;                 // Latch value the Z80 is running with.
    uint8_t            curCtrlLatch;                 // Latch value set during tranZPUter access of the Z80 bus.
    uint8_t            videoRAM[2][2048];            // Two video memory buffer frames, allows for storage of original frame in [0] and working frame in [1].
    uint8_t            attributeRAM[2][2048];        // Two attribute memory buffer frames, allows for storage of original frame in [0] and working frame in [1].
    enum CTRL_MODE     ctrlMode;                     // Mode of control, ie normal Z80 Running, controlling mainboard, controlling tranZPUter.
    enum BUS_DIRECTION busDir;                       // Direction the bus has been configured for.
} t_z80Control;

// Application execution constants.
//


// References to variables within the main library code.
extern volatile uint32_t *ioPin[MAX_TRANZPUTER_PINS];
extern uint8_t           pinMap[MAX_TRANZPUTER_PINS];

// Prototypes.
//
void     yield(void);
void     setupPins(volatile uint32_t *);
uint8_t  reqZ80Bus(uint32_t);
void     relinquishZ80Bus(void);
uint8_t  reqMainboardBus(uint32_t);
uint8_t  reqTranZPUterBus(uint32_t);
void     setupSignalsForZ80Access(enum BUS_DIRECTION);
void     releaseZ80(void);
void     refreshZ80(void);
uint8_t  writeZ80Memory(uint16_t, uint8_t);
uint8_t  readZ80Memory(uint16_t);
uint8_t  writeZ80IO(uint16_t, uint8_t);
uint8_t  readZ80IO(uint16_t);
void     fillZ80Memory(uint32_t, uint32_t, uint8_t, uint8_t);
void     captureVideoFrame(enum VIDEO_FRAMES, uint8_t);
void     refreshVideoFrame(enum VIDEO_FRAMES, uint8_t, uint8_t);
FRESULT  loadVideoFrameBuffer(char *, enum VIDEO_FRAMES);
FRESULT  saveVideoFrameBuffer(char *, enum VIDEO_FRAMES);   
FRESULT  loadZ80Memory(char *, uint32_t, uint8_t, uint8_t);
FRESULT  saveZ80Memory(char *, uint32_t, uint32_t, uint8_t);
// Debug methods.
void     displaySignals(void);

#ifdef __cplusplus
}
#endif
#endif // TRANZPUTER_H
