/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tranzputer.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     The TranZPUter library.
//                  This file contains methods which allow applications to access and control the
//                  traZPUter board and the underlying Sharp MZ80A host.
//                  I had considered writing this module in C++ but decided upon C as speed is more
//                  important and C++ always adds a little additional overhead. 
//                  Even in C a fair bit of overhead is added by the compiler even after optimisation,
//                  thus the Interrupt Service Routines have been coded as inline assembler to gain
//                  extra cycles, one of the big problems is capturing the Z80 signals and data
//                  in order to action commands such as memory switch. Part of the problem is the
//                  non-uniform allocation of pins - part my problem in the hardware, part Teensy
//                  in the model of K64F and the pins available which means several GPIO registers
//                  must be read and pieced together to form a 16bit address bus and 8 bit data
//                  bus. Fine under normal situations but not ISR, an example is setting the memory 
//                  mode control switch - in C it takes 20uS, after stripping it down of nicities
//                  such as maps, this comes down to 13uS and further optimisations can be made in 
//                  assembler but 13uS is a long long time even for a 2MHz CPU and when cranked up
//                  to 8MHz it becomes a challenge!
//                 
//                  NB. This library is NOT thread safe. In zOS, one thread is running continually in
//                  this code but is suspended if zOS launches an application which will call this
//                  functionality.
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

#include "ff.h"            /* Declarations of FatFs API */
#include "utils.h"
#include <tranzputer.h>

#ifndef __APP__        // Protected methods which should only reside in the kernel.

// Global scope variables used within the zOS kernel.
//
volatile uint32_t *ioPin[MAX_TRANZPUTER_PINS];
uint8_t           pinMap[MAX_TRANZPUTER_PINS];
uint32_t volatile *ms;
t_z80Control      z80Control;
t_osControl       osControl;
t_svcControl      svcControl;

// Mapping table to map Sharp MZ80A Ascii to Standard ASCII.
//
static t_asciiMap asciiMap[] = {
    { 0x00 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x00 }, { 0x20 }, { 0x20 }, // 0x0F
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0x1F
    { 0x20 }, { 0x21 }, { 0x22 }, { 0x23 }, { 0x24 }, { 0x25 }, { 0x26 }, { 0x27 }, { 0x28 }, { 0x29 }, { 0x2A }, { 0x2B }, { 0x2C }, { 0x2D }, { 0x2E }, { 0x2F }, // 0x2F
    { 0x30 }, { 0x31 }, { 0x32 }, { 0x33 }, { 0x34 }, { 0x35 }, { 0x36 }, { 0x37 }, { 0x38 }, { 0x39 }, { 0x3A }, { 0x3B }, { 0x3C }, { 0x3D }, { 0x3E }, { 0x3F }, // 0x3F
    { 0x40 }, { 0x41 }, { 0x42 }, { 0x43 }, { 0x44 }, { 0x45 }, { 0x46 }, { 0x47 }, { 0x48 }, { 0x49 }, { 0x4A }, { 0x4B }, { 0x4C }, { 0x4D }, { 0x4E }, { 0x4F }, // 0x4F
    { 0x50 }, { 0x51 }, { 0x52 }, { 0x53 }, { 0x54 }, { 0x55 }, { 0x56 }, { 0x57 }, { 0x58 }, { 0x59 }, { 0x5A }, { 0x5B }, { 0x5C }, { 0x5D }, { 0x5E }, { 0x5F }, // 0x5F
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0x6F
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0x7F
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0x8F
    { 0x20 }, { 0x20 }, { 0x65 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x74 }, { 0x67 }, { 0x68 }, { 0x20 }, { 0x62 }, { 0x78 }, { 0x64 }, { 0x72 }, { 0x70 }, { 0x63 }, // 0x9F
    { 0x71 }, { 0x61 }, { 0x7A }, { 0x77 }, { 0x73 }, { 0x75 }, { 0x69 }, { 0x20 }, { 0x4F }, { 0x6B }, { 0x66 }, { 0x76 }, { 0x20 }, { 0x75 }, { 0x42 }, { 0x6A }, // 0XAF
    { 0x6E }, { 0x20 }, { 0x55 }, { 0x6D }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x6F }, { 0x6C }, { 0x41 }, { 0x6F }, { 0x61 }, { 0x20 }, { 0x79 }, { 0x20 }, { 0x20 }, // 0xBF
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0XCF
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0XDF
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, // 0XEF
    { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }, { 0x20 }  // 0XFF
};

// This method is called everytime an active irq triggers on Port E. For this design, this means the two IO CS
// lines, TZ_SVCREQ and TZ_SYSREQ. The SVCREQ is used when the Z80 requires a service, the SYSREQ is yet to
// be utilised.
//
//
#if DECODE_Z80_IO == 0 || DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
static void __attribute((naked, noinline)) irqPortE(void)
{
                                          // Save register we use.
    asm volatile("                        push     {r0-r5,lr}               \n");

                                          // Reset the interrupt, PORTE_ISFR <= PORTE_ISFR
    asm volatile("                        ldr      r4, =0x4004d0a0          \n"
                 "                        ldr      r5, [r4, #0]             \n"
                 "                        str      r5, [r4, #0]             \n"

                                          // Is TZ_SVCREQ (E10) active (low), set flag and exit if it is.
                 "                        movs     r4, #1                   \n"
                 "                        tst      r5, #0x400               \n"
                 "                        beq      ebr0                     \n"
                 "                        strb     r4, %[val0]              \n" 

                 "              ebr0:                                       \n"
                                          // Is TZ_SYSREQ (E11) active (low), set flag and exit if it is.
                 "                        tst      r5, #0x800               \n"
                 "                        beq      irqPortE_Exit            \n"
                 "                        strb     r4, %[val1]              \n"

                 "              irqPortE_Exit:                              \n"

                                          : [val0] "+m" (z80Control.svcRequest),
                                            [val1] "+m" (z80Control.sysRequest) 
                                          :
                                          : "r4","r5","r7","r8","r9","r10","r11","r12"
                );
    asm volatile("                        pop      {r0-r5,pc}               \n");

    return;
}
#endif

// An 8 bit helper function to write a value to the memory control latch during an interrupt. Should be coded in assembler but takes too long!!
//
uint8_t writeZ80io(uint8_t data)
{
    // Locals.

    // Control signals need to be output and deasserted.
    //
    *portModeRegister(Z80_IORQ_PIN) = 1;
    *portSetRegister(Z80_IORQ_PIN) = 1;
    *ioPin[Z80_IORQ] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_IORQ] &= ~PORT_PCR_ODE;
    //
    *portModeRegister(Z80_WR_PIN) = 1;
    *portSetRegister(Z80_WR_PIN) = 1;
    *ioPin[Z80_WR] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_WR] &= ~PORT_PCR_ODE;

    // Set the data and address on the bus. Address is hard coded as this routine is inteded to write to the memory mode latch.
    //
    *portModeRegister(Z80_A7_PIN) = 1;
    *portClearRegister(Z80_A7_PIN) = 1;
    *ioPin[Z80_A7] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A7] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A6_PIN) = 1;
    *portSetRegister(Z80_A6_PIN) = 1;
    *ioPin[Z80_A6] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A6] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A5_PIN) = 1;
    *portSetRegister(Z80_A5_PIN) = 1;
    *ioPin[Z80_A5] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A5] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A4_PIN) = 1;
    *portClearRegister(Z80_A4_PIN) = 1;
    *ioPin[Z80_A4] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A4] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A3_PIN) = 1;
    *portClearRegister(Z80_A3_PIN) = 1;
    *ioPin[Z80_A3] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A3] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A2_PIN) = 1;
    *portClearRegister(Z80_A2_PIN) = 1;
    *ioPin[Z80_A2] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A2] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A1_PIN) = 1;
    *portClearRegister(Z80_A1_PIN) = 1;
    *ioPin[Z80_A1] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A1] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_A0_PIN) = 1;
    *portClearRegister(Z80_A0_PIN) = 1;
    *ioPin[Z80_A0] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_A0] &= ~PORT_PCR_ODE;

    // Set the actual data onto D7 - D0
    *portModeRegister(Z80_D7_PIN) = 1;
    if((data >>  7) & 0x1) { *portSetRegister(Z80_D7_PIN) = 1; } else { *portClearRegister(Z80_D7_PIN) = 1; }
    *ioPin[Z80_D7] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D7] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D6_PIN) = 1;
    if((data >>  6) & 0x1) { *portSetRegister(Z80_D6_PIN) = 1; } else { *portClearRegister(Z80_D6_PIN) = 1; }
    *ioPin[Z80_D6] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D6] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D5_PIN) = 1;
    if((data >>  5) & 0x1) { *portSetRegister(Z80_D5_PIN) = 1; } else { *portClearRegister(Z80_D5_PIN) = 1; }
    *ioPin[Z80_D5] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D5] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D4_PIN) = 1;
    if((data >>  4) & 0x1) { *portSetRegister(Z80_D4_PIN) = 1; } else { *portClearRegister(Z80_D4_PIN) = 1; }
    *ioPin[Z80_D4] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D4] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D3_PIN) = 1;
    if((data >>  3) & 0x1) { *portSetRegister(Z80_D3_PIN) = 1; } else { *portClearRegister(Z80_D3_PIN) = 1; }
    *ioPin[Z80_D3] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D3] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D2_PIN) = 1;
    if((data >>  2) & 0x1) { *portSetRegister(Z80_D2_PIN) = 1; } else { *portClearRegister(Z80_D2_PIN) = 1; }
    *ioPin[Z80_D2] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D2] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D1_PIN) = 1;
    if((data >>  1) & 0x1) { *portSetRegister(Z80_D1_PIN) = 1; } else { *portClearRegister(Z80_D1_PIN) = 1; }
    *ioPin[Z80_D1] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D1] &= ~PORT_PCR_ODE;
    *portModeRegister(Z80_D0_PIN) = 1;
    if((data >>  0) & 0x1) { *portSetRegister(Z80_D0_PIN) = 1; } else { *portClearRegister(Z80_D0_PIN) = 1; }
    *ioPin[Z80_D0] = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    *ioPin[Z80_D0] &= ~PORT_PCR_ODE;

    // Start the write cycle, MREQ and WR go low.
    *portClearRegister(Z80_IORQ_PIN) = 1;
    *portClearRegister(Z80_WR_PIN) = 1;

    // Complete the write cycle.
    //
    *portSetRegister(Z80_IORQ_PIN) = 1;
    *portSetRegister(Z80_WR_PIN) = 1;

    // All lower address lines to inputs.
    //
    *portModeRegister(Z80_D7_PIN) = 0; *ioPin[Z80_D7] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D6_PIN) = 0; *ioPin[Z80_D6] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D5_PIN) = 0; *ioPin[Z80_D5] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D4_PIN) = 0; *ioPin[Z80_D4] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D3_PIN) = 0; *ioPin[Z80_D3] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D2_PIN) = 0; *ioPin[Z80_D2] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D1_PIN) = 0; *ioPin[Z80_D1] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_D0_PIN) = 0; *ioPin[Z80_D0] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;

    *portModeRegister(Z80_A7_PIN) = 0; *ioPin[Z80_A7] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A6_PIN) = 0; *ioPin[Z80_A6] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A5_PIN) = 0; *ioPin[Z80_A5] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A4_PIN) = 0; *ioPin[Z80_A4] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A3_PIN) = 0; *ioPin[Z80_A3] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A2_PIN) = 0; *ioPin[Z80_A2] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A1_PIN) = 0; *ioPin[Z80_A1] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_A0_PIN) = 0; *ioPin[Z80_A0] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;

    // All control signals to inputs.
    *portModeRegister(Z80_IORQ_PIN) = 0; *ioPin[Z80_IORQ] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
    *portModeRegister(Z80_WR_PIN) = 0; *ioPin[Z80_WR] = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;

    // Reset the IRQ triggers due to changing the mode of the pin which clears the trigger.
    uint32_t cfg;
    cfg = *ioPin[Z80_IORQ]; cfg &= ~0x000F0000; *ioPin[Z80_IORQ] = cfg; cfg |= IRQ_MASK_FALLING; *ioPin[Z80_IORQ] = cfg;
    //cfg = *ioPin[Z80_RESET]; cfg &= ~0x000F0000; *ioPin[Z80_RESET] = cfg; cfg |= IRQ_MASK_FALLING; *ioPin[Z80_RESET] = cfg;

    return(0);
}


// This method is called everytime an active irq triggers on Port D. For this design, this means the IORQ line.
//
// There are 3 versions of the same routine, originally using #if macro preprocessor statements but it became
// unwieldy. The purpose of the 3 version is:
// 0 = Basic IRQ just sets the reset flag if the user presses reset on the host.
// 1,2 = Captures I/O events, stores the address/data of the I/O command.
// 3 = MZ700 mode - this mode detects the MZ700 OUT commands and changes memory model. Unfortunately it doesnt work 100% due to another
// ISR occasionally delaying the activation of this routine which means we cannot apply a WAIT state and consequently the data on the Z80
// address bus has changed. Other than removing realtime clock and threads I cant see a way around it in software, will have to look at 
// a hardware solution.
//
// The method has been split into 3 versions depending on the configured mode. Mode 0 is just the basic I/O service request from the Z80 and
// reset detection, Mode 1 (mode 2 just stores the data on the data bus) registers and captures I/O and Memory mapped I/O events for later
// processing (which is subject to the same issue as the MZ700 mode, occasionally it doesnt get captured) and Mode 3 is the MZ700 mode.
//
#if DECODE_Z80_IO == 0
static void __attribute((naked, noinline)) irqPortD(void)
{
                                          // This code is critical, if the Z80 is running at higher frequencies then very little time to capture it's requests.
    asm volatile("                        push     {r0-r3,lr}               \n");

                                          // Get the ISFR bit and reset.
    asm volatile("                        ldr      r1, =0x4004c0a0          \n"
                 "                        ldr      r0, [r1, #0]             \n"
                 "                        str      r0, [r1, #0]             \n"

                                          // Is Z80_RESET active, set flag and exit.
                 "                        movs     r0, #1                   \n"
                 "                        strb     r0, %[val0]              \n"
            
                                          // Reset the interrupt, PORTD_ISFR <= PORTD_ISFR
                 "       irqPortD_Exit:                                     \n"
                 "                        pop      {r0-r3,pc}               \n"

                                          : [val0] "=m" (z80Control.resetEvent)
                                          : 
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");

    return;
}
#endif // DECODE_Z80_IO == 0
#if DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
static void __attribute((naked, noinline)) irqPortD(void)
{
                                          // This code is critical, AIT needs to be applied before end of the WAIT sample cycle, the 120Mhz K64F only just makes it. This is to ensure a good sample of the signals
                                          // then BUSRQ needs to be asserted in case of memory mode latch updates.
    asm volatile("                        push     {r0-r1}                  \n"
                 "                        ldr      r0, =0x43fe1114          \n"          // Z80_WAIT  0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r1, #1                   \n"
                 "                        str      r1, [r0,#0]              \n"

                                          // Save minimum number of registers, cycles matter as we need to capture the address and halt the Z80 whilst we decode it.
                 "                        pop      {r0-r1}                  \n"
                 "                        push     {r0-r8,lr}               \n"

                                          // Capture GPIO ports - this is necessary in order to make a clean capture and then decode.
                 "                        ldr      r0, =0x400ff010          \n"          // GPIOA_PDIR
                 "                        ldr      r4, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOB_PDIR
                 "                        ldr      r5, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOC_PDIR
                 "                        ldr      r6, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOD_PDIR
                 "                        ldr      r7, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOE_PDIR
                 "                        ldr      r8, [r0, #0]             \n"

                                          // De-assert Z80_WAIT, signals have been sampled and if it is kept low too long it will slow the host down and with BUSRQ, BUSACK will go high (a bug?)!!
                 "                        ldr      r0, =0x43fe1014          \n"          // Z80_WAIT 0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r3, #1                   \n"
                 "                        str      r3, [r0,#0]              \n"

                                          // If the IORQ line has gone high (another interrupt or slow system response) get out, cant work with incomplete data.
                 "                        tst      r7, #8                   \n"
                 "                        beq      irqPortD_Exit            \n"
                                          :
                                          :
                                          : "r0", "r4","r5","r6","r7","r8","r9","r10","r11","r12"
                );

    asm volatile(                         // Is Z80_RESET active, set flag and exit.
                 "              cbr:      tst      r7, #0x8000              \n"
                 "                        bne      cbr0                     \n"
                 "                        movs     r6, #1                   \n"
                 "                        strb     r6, %[val1]              \n"
                 "                        b        irqPortD_Exit            \n"
              
                                          // Is Z80_WR active, continue if it is as we consider IO WRITE cycles.
                 "              cbr0:     tst      r6, #16                  \n"
                 "                        beq      cbr1                     \n"
   
                                          // Is Z80_RD active, continue if it is as we consider IO READ cycles.
                 "                        tst      r6, #128                 \n"
                 "                        bne      irqPortD_Exit            \n"
                 "              cbr1:                                       \n"

                                          : [val1] "=m" (z80Control.resetEvent)
                                          : 
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");

                                          // Convert lower 8 address bits into a byte and store.
    asm volatile("                        lsrs     r0, r5, #4               \n"          // (z80Control.portB >> 4)&0x80)
                 "                        and.w    r0, r0, #128             \n"
                 "                        lsrs     r1, r8, #18              \n"          // (z80Control.portE >> 18)&0x40)
                 "                        and.w    r1, r1, #64              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r8, #20              \n"          // (z80Control.portE >> 20)&0x20)
                 "                        and.w    r1, r1, #32              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #4               \n"          // (z80Control.portC >> 4)&0x10)
                 "                        and.w    r1, r1, #16              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #6               \n"          // (z80Control.portC >> 6)&0x08)
                 "                        and.w    r1, r1, #8               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #8               \n"          // (z80Control.portC >> 8)&0x04)
                 "                        and.w    r1, r1, #4               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #10              \n"          // (z80Control.portC >> 10)&0x02)
                 "                        and.w    r1, r1, #2               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #17              \n"          // (z80Control.portA >> 17)&0x01)
                 "                        and.w    r1, r1, #1               \n"
                 "                        orrs     r0, r1                   \n"

                                          // Store the address for later processing..
                 "                        strb     r0, %[val0]              \n" 
                 "                        mov      r8, r0                   \n"          // Addr in R8
                 
                                          : [val0] "=m" (z80Control.ioAddr)
                                          :
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");

  #if DECODE_Z80_IO = 2
    // Convert data port bits into a byte and store.
    asm volatile("                        mov.w    r0, r7, lsl #5           \n"          // (z80Control.portD << 5)&0x80)
                 "                        and.w    r0, r0, #128             \n"
                 "                        lsls     r1, r7, #2               \n"          // (z80Control.portD << 2)&0x40)
                 "                        and.w    r1, r1, #64              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r7, #2               \n"          // (z80Control.portD >> 2)&0x20)
                 "                        and.w    r1, r1, #32              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #9               \n"          // (z80Control.portA >> 9)&0x10)
                 "                        and.w    r1, r1, #16              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #9               \n"          // (z80Control.portA >> 9)&0x08)
                 "                        and.w    r1, r1, #8               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsls     r1, r7, #2               \n"          // (z80Control.portD << 2)&0x04)
                 "                        and.w    r1, r1, #4               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r1, #16              \n"          // (z80Control.portB >> 16)&0x02)
                 "                        and.w    r1, r1, #2               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r1, #16              \n"          // (z80Control.portB >> 16)&0x01)
                 "                        and.w    r1, r1, #1               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        strb     r0, %[val0]              \n"
                 "                        mov      r7, r0                   \n"          // Data in R7

                                          : [val0] "=m" (z80Control.ioData)
                                          :
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");
  #endif

                                          // Process the IO request by setting the ioEvent flag as it wasnt an MZ700 memory switch request.
    asm volatile("                        movs     r4, #1                   \n"
                 "                        strb     r4, %[val2]              \n"

                                          : [val2]  "=m" (z80Control.ioEvent)
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
              
    asm volatile("       irqPortD_Exit:                                     \n"
                                          // Reset the interrupt, PORTD_ISFR <= PORTD_ISFR
                 "                        ldr      r3, =0x4004c0a0          \n"
                 "                        ldr      r2, [r3, #0]             \n"
                 "                        str      r2, [r3, #0]             \n"
                 "                        pop      {r0-r8,pc}               \n"
                                          :
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");

    return;
}
#endif // DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2

#if DECODE_Z80_IO == 3
static void __attribute((naked, noinline)) irqPortD(void)
{
                                          // This code is critical, WAIT needs to be applied before end of the WAIT sample cycle, the 120Mhz K64F only just makes it. This is to ensure a good sample of the signals
                                          // then BUSRQ needs to be asserted in case of memory mode latch updates.
    asm volatile("                        push     {r0-r1}                  \n");

    asm volatile("                        ldr      r0, =0x43fe1114          \n"          // Z80_WAIT  0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r1, #1                   \n"
                 "                        str      r1, [r0,#0]              \n"
                
                                          // Save minimum number of registers, cycles matter as we need to capture the address and halt the Z80 whilst we decode it.
                 "                        pop      {r0-r1}                  \n"
                 "                        push     {r0-r8,lr}               \n"

                                          // Get the triggering interrupt and reset, PORTD_ISFR <= PORTD_ISFR
                 "                        ldr      r0, =0x4004c0a0          \n"
                 "                        ldr      r4, [r0, #0]             \n"
                 "                        str      r4, [r0, #0]             \n"
                );

    asm volatile(                         // Is Z80_RESET active, set flag and exit.
                 "                        tst      r4, #0x8000              \n"
                 "                        beq      irqd0                    \n"
                 "                        movs     r6, #1                   \n"
                 "                        strb     r6, %[val1]              \n"
                 "                        b        irqPortD_Exit            \n"

                                          : [val1] "=m" (z80Control.resetEvent)
                                          : 
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");

    asm volatile(                         // If the IORQ line is high (another interrupt triggered us or slow system response) get out, cant work with incomplete data.
                 "              irqd0:    tst      r4, #8                   \n"
                 "                        beq      irqPortD_Exit            \n"
                );

                                          // Capture GPIO ports - this is necessary in order to make a clean capture and then decode.
    asm volatile("                        ldr      r0, =0x400ff010          \n"          // GPIOA_PDIR
                 "                        ldr      r4, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOB_PDIR
                 "                        ldr      r5, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOC_PDIR
                 "                        ldr      r6, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOD_PDIR
                 "                        ldr      r7, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOE_PDIR
                 "                        ldr      r8, [r0, #0]             \n"

                                          // De-assert Z80_WAIT, signals have been sampled and if it is kept low too long it will slow the host down and with BUSRQ active during a WAIT, BUSACK will go high (a bug?)!!
                 "                        ldr      r0, =0x43fe1014          \n"          // Z80_WAIT 0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r1, #1                   \n"
                 "                        str      r1, [r0,#0]              \n"
               
                                          // Assert BUSRQ so we keep the Z80 from moving to the next instruction incase we need to update the memory latch.
                 "                        ldr      r0, =0x43fe1100          \n"          // CTL_BUSRQ 0x43fe1100 clear, 0x43fe1000 set
                 "                        movs     r1, #1                   \n"
                 "                        str      r1, [r0,#0]              \n"

                                          :
                                          :
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");
  
    asm volatile(                         // Is TZ_SVCREQ (E10) active (low), set flag and exit if it is.
                 "                        movs     r0, #1                   \n"
                 "                        tst      r8, #0x400               \n"
                 "                        bne      irqd1                    \n"
                 "                        strb     r0, %[val0]              \n" 
                 "                        b        irqPortD_Exit            \n"

                                          // Is TZ_SYSREQ (E11) active (low), set flag and exit if it is.
                 "              irqd1:    tst      r8, #0x800               \n"
                 "                        bne      irqd2                    \n"
                 "                        strb     r0, %[val1]              \n"
                 "                        b        irqPortD_Exit            \n"

                                          : [val0] "+m" (z80Control.svcRequest),
                                            [val1] "+m" (z80Control.sysRequest) 
                                          :
                                          : "r0","r4","r5","r7","r8","r9","r10","r11","r12");
   
    asm volatile(                         // Is Z80_WR active, continue if it is as we consider IO WRITE cycles.
                 "              irqd2:    tst      r6, #16                  \n"
                 "                        beq      irqd3                    \n"
   
                                          // Is Z80_RD active, continue if it is as we consider IO READ cycles.
                 "                        tst      r6, #128                 \n"
                 "                        bne      irqPortD_Exit            \n"
                 "              irqd3:                                      \n"
                );

 #if 0
                                          // Now save the GPIO values into the structure. Seperate statements to get around the optimizer trying to use 5 scratch registers.
    asm volatile("                        str      r4, %[val1]              \n" : [val1] "=m" (z80Control.portA) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r5, %[val2]              \n" : [val2] "=m" (z80Control.portB) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r6, %[val3]              \n" : [val3] "=m" (z80Control.portC) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r7, %[val4]              \n" : [val4] "=m" (z80Control.portD) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r8, %[val5]              \n" : [val5] "=m" (z80Control.portE) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
  #endif

    // Convert lower 8 address bits into a byte and store.
    asm volatile("                        lsrs     r0, r5, #4               \n"          // (z80Control.portB >> 4)&0x80)
                 "                        and.w    r0, r0, #128             \n"
                 "                        lsrs     r1, r8, #18              \n"          // (z80Control.portE >> 18)&0x40)
                 "                        and.w    r1, r1, #64              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r8, #20              \n"          // (z80Control.portE >> 20)&0x20)
                 "                        and.w    r1, r1, #32              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #4               \n"          // (z80Control.portC >> 4)&0x10)
                 "                        and.w    r1, r1, #16              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #6               \n"          // (z80Control.portC >> 6)&0x08)
                 "                        and.w    r1, r1, #8               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #8               \n"          // (z80Control.portC >> 8)&0x04)
                 "                        and.w    r1, r1, #4               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r6, #10              \n"          // (z80Control.portC >> 10)&0x02)
                 "                        and.w    r1, r1, #2               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #17              \n"          // (z80Control.portA >> 17)&0x01)
                 "                        and.w    r1, r1, #1               \n"
                 "                        orrs     r0, r1                   \n"

                                          // Store the address for later processing..
                 "                        strb     r0, %[val0]              \n" 
                 "                        mov      r8, r0                   \n"          // Addr in R8
                 
                                          : [val0] "=m" (z80Control.ioAddr)
                                          :
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");
 #if 0
    // Convert data port bits into a byte and store.
    asm volatile("                        mov.w    r0, r7, lsl #5           \n"          // (z80Control.portD << 5)&0x80)
                 "                        and.w    r0, r0, #128             \n"
                 "                        lsls     r1, r7, #2               \n"          // (z80Control.portD << 2)&0x40)
                 "                        and.w    r1, r1, #64              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r7, #2               \n"          // (z80Control.portD >> 2)&0x20)
                 "                        and.w    r1, r1, #32              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #9               \n"          // (z80Control.portA >> 9)&0x10)
                 "                        and.w    r1, r1, #16              \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r4, #9               \n"          // (z80Control.portA >> 9)&0x08)
                 "                        and.w    r1, r1, #8               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsls     r1, r7, #2               \n"          // (z80Control.portD << 2)&0x04)
                 "                        and.w    r1, r1, #4               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r1, #16              \n"          // (z80Control.portB >> 16)&0x02)
                 "                        and.w    r1, r1, #2               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        lsrs     r1, r1, #16              \n"          // (z80Control.portB >> 16)&0x01)
                 "                        and.w    r1, r1, #1               \n"
                 "                        orrs     r0, r1                   \n"
                 "                        strb     r0, %[val0]              \n"
                 "                        mov      r7, r0                   \n"          // Data in R7

                                          : [val0] "=m" (z80Control.ioData)
                                          :
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");
  #endif

    // MZ700 memory mode switch?
    //         0x0000:0x0FFF     0xD000:0xFFFF
    // 0xE0 =  DRAM
    // 0xE1 =                    DRAM
    // 0xE2 =  MONITOR
    // 0xE3 =                    Memory Mapped I/O
    // 0xE4 =  MONITOR           Memory Mapped I/O
    // 0xE5 =                    Inhibit
    // 0xE6 =                    Return to state prior to 0xE5
    //
    // Must be in range 0xE0-0xE6 to be an MZ700 operation.
    asm volatile("                    cmp.w     r8, #224                    \n"
                 "                    blt       irqd20                      \n"
                 "                    cmp.w     r8, #230                    \n"
                 "                    bgt       irqd20                      \n"
                 "                                                          \n"
                 "                        ldr      r6, %[val1]              \n"          // Retrieve the config value for the MZ700.
                 "                        tst      r6, #0x40000             \n"
                 "                        bne      irqd16                   \n"          // For locked mode, only service E4, E5 & E6.
                 "                        and      r6, r6, #0xFFFFFF00      \n"          // Clear out the memoryMode[current[ as a new value will be stored.

                                          // 0xE0
                 "              irqd12:   cmp.w    r8, #224                 \n"          // R8 = address location of OUT command on Z80.
                 "                        bne      irqd13                   \n"
                 "                        orr      r6, #65536               \n"          // mode[16] = 1
                 "                        b        irqd11x                  \n"

                                          // 0xE1
                 "              irqd13:   cmp.w    r8, #225                 \n"
                 "                        bne      irqd14                   \n" 
                 "                        orr      r6, #131072              \n"          // mode[17] = 1
                 "                        b        irqd11x                  \n"

                                          // 0xE2
                 "              irqd14:   cmp.w    r9, #226                 \n"
                 "                        bne      irqd15                   \n"
                 "                        bic      r6, #65536               \n"          // mode[16] = 0
                 "                        b        irqd11x                  \n"

                                          // 0xE3
                 "              irqd15:   cmp.w    r8, #227                 \n"
                 "                        bne      irqd16                   \n"
                 "                        bic      r6, #131072              \n"          // mode[17] = 0

                                          // if(z80Control.mz700.mode[17:16] == '00')
                 "              irqd11x:  tst      r6, #0x30000             \n"
                 "                        bne      irqd12x                  \n"
                 "                        orr      r6,#2                    \n"          // memoryMode = 2
                 "                        b        irqd19                   \n"

                                          // if(z80Control.mz700.mode[17:16] == '10')
                 "              irqd12x:  tst      r6, #0x20000             \n"
                 "                        beq      irqd13x                  \n"
                 "                        tst      r6, #0x10000             \n"
                 "                        bne      irqd14x                  \n"
                 "                        orr      r6, #11                  \n"          // memoryMode = 11
                 "                        b        irqd19                   \n"
                 
                                          // if(z80Control.mz700.mode[17:16] == '01')
                 "              irqd13x:  orr      r6, #10                  \n"          // memoryMode = 10
                 "                        b        irqd19                   \n"

                                          // if(z80Control.mz700.mode[17:16] == '11)
                 "              irqd14x:  orr      r6, #12                  \n"          // memoryMode = 12
                 "                        b        irqd19                   \n"

                                          // 0xE4 - Reset to default.
                 "              irqd16:   cmp.w    r8, #228                 \n"
                 "                        bne      irqd17                   \n"
                 "                        mov      r6, #0x002               \n"          // mode[17:16] = '00', mode[inhibit] = '0',  memoryMode[current] = 2, memoryMode[old] = 2
                 "                        b        irqd19                   \n"

                                          // 0xE5 - Lock the region D000-FFFF by setting memory mode to 13.
                 "              irqd17:   cmp.w    r8, #229                 \n"
                 "                        bne      irqd18                   \n"
                 "                        orr      r6, #0x40000             \n"          // mode[inhibit] = 1 
                 "                        and      r5, r6, #0x0000FF00      \n"          // Look at previous memory mode and use to decide the mode we lock into.
                 "                        cmp      r5, #0xB00               \n"
                 "                        bne      irqd17x                  \n"
                 "                        orr      r6, #13                  \n"          // memoryMode[current] = 13 - Monitor ROM at 0000:0FFF
                 "                        b        irqd19                   \n"
                 "              irqd17x:  orr      r6, #14                  \n"          // memoryMode[current] = 14 - System RAM at 0000:0FFF
                 "                        b        irqd19                   \n"

                                          // 0xE6 - Unlock the region D000-FFF by returning the memory mode to original.
                 "              irqd18:   cmp.w    r8, #230                 \n"
                 "                        bne      irqd2                    \n"
                 "                        and      r6, #0xFFFBFFFF          \n"          // mode[inhibit] = 0
                 "                        and      r5, r6, #0x0000FF00      \n"
                 "                        lsrs     r5, r5, #8               \n"
                 "                        orr      r6, r5                   \n"          // memoryMode[current] = memoryMode[old]

                                          // Store the changed value back to the control structure.
                 "              irqd19:   lsls     r8, r8, #24 \n"
                 "                        and      r6, r6, #0x00FFFFFF \n"
                 "                        orr      r6, r8 \n"
                 "                        and      r5, r6, #0x000000FF      \n"
                 "                        lsls     r5, r5, #8               \n"
                 "                        and      r6, r6, #0xFFFF00FF      \n"
                 "                        orr      r6, r5                   \n"          // memoryMode[old] = memoryMode[current]
                 "                        str      r6, %[val1]              \n"

                                          // Output to variables.
                                          : [val1]  "=m" (z80Control.mz700.config)
                                          // Input from variables.
                                          :
                                          : "r5","r6","r7","r8","r9","r10","r11","r12");

                  // Easier at this point to make a C call than to code in assembler a write to the Z80 IO bus.
                  //
                  writeZ80io((uint8_t)z80Control.mz700.config);
              
                                          // After writing out the data, jump to exit, we dont set the ioEvent flag as the event has been processed already.
    asm volatile("                        b        irqPortD_Exit            \n"

                                          // Process the IO request by setting the ioEvent flag as it wasnt an MZ700 memory switch request.
                 "              irqd20:   movs     r4, #1                   \n"
                 "                        strb     r4, %[val2]              \n"

                                          : [val2]  "+m" (z80Control.ioEvent)
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
              
    asm volatile("       irqPortD_Exit:                                     \n"
                                          // De-assert Z80_WAIT, on a valid run it will have already been de-asserted, but certain events such as reset exit immediately so need to de-assert on exit.
                 "                        ldr      r4, =0x43fe1014          \n"          // Z80_WAIT 0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r5, #1                   \n"
                 "                        str      r5, [r4,#0]              \n"

                                          // De-assert BUSRQ nothing more to do.
                 "                        ldr      r4, =0x43fe1000          \n"          // CTL_BUSRQ 0x43fe1100 clear, 0x43fe1000 set
                 "                        movs     r5, #1                   \n"
                 "                        str      r5, [r4,#0]              \n"
             
                                          // Restore registers, all done.
                 "                        pop      {r0-r8,pc}               \n"
                                          :
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");

    return;
}
#endif // DECODE_Z80_IO == 3

// This method is called everytime an active irq triggers on Port C. For this design, this means the MREQ line.
// Store all the ports as they are needed to capture the Address and Data of the asserted z80 I/O transaction.
//
static void __attribute((naked, noinline)) irqPortC(void)
{
                                          // This code is critical, AIT needs to be applied before end of the WAIT sample cycle, the 120Mhz K64F only just makes it. This is to ensure a good sample of the signals
                                          // then BUSRQ needs to be asserted in case of memory mode latch updates.
    asm volatile("                        push     {r0-r1}                  \n");
    asm volatile("                        ldr      r0, =0x43fe1114          \n"          // Z80_WAIT  0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r1, #1                   \n"
                 "                        str      r1, [r0,#0]              \n"

                                          // Save minimum number of registers, cycles matter as we need to capture the address and halt the Z80 whilst we decode it.
                 "                        pop      {r0-r1}                  \n"
                 "                        push     {r0-r8,lr}               \n"
               
                                          // Get the triggering interrupt and reset, PORTC_ISFR <= PORTC_ISFR
                 "                        ldr      r3, =0x4004b0a0          \n"
                 "                        ldr      r2, [r3, #0]             \n"
                 "                        str      r2, [r3, #0]             \n"
                );

                                          // Capture GPIO ports - this is necessary in order to make a clean capture and then decode.
    asm volatile("                        ldr      r0, =0x400ff010          \n"          // GPIOA_PDIR
                 "                        ldr      r4, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOB_PDIR
                 "                        ldr      r5, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOC_PDIR
                 "                        ldr      r6, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOD_PDIR
                 "                        ldr      r7, [r0, #0]             \n"
                 "                        add.w    r0, #64                  \n"          // GPIOE_PDIR
                 "                        ldr      r8, [r0, #0]             \n"

                                          // De-assert Z80_WAIT, signals have been sampled and if it is kept low too long it will slow the host down and with BUSRQ, BUSACK will go high (a bug?)!!
                 "                        ldr      r0, =0x43fe1014          \n"          // Z80_WAIT 0x43fe1114 clear, 0x43fe1014 set
                 "                        movs     r3, #1                   \n"
                 "                        str      r3, [r0,#0]              \n"
                                          :
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12"
                );

  #if DECODE_Z80_IO >= 2
                                          // Now save the GPIO values into the structure. Seperate statements to get around the optimizer trying to use 5 scratch registers.
    asm volatile("                        str      r4, %[val1]              \n" : [val1] "=m" (z80Control.portA) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r5, %[val2]              \n" : [val2] "=m" (z80Control.portB) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r6, %[val3]              \n" : [val3] "=m" (z80Control.portC) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r7, %[val4]              \n" : [val4] "=m" (z80Control.portD) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
    asm volatile("                        str      r8, %[val5]              \n" : [val5] "=m" (z80Control.portE) : : "r4","r5","r6","r7","r8","r9","r10","r11","r12");
  #endif

    asm volatile(
                                          // Is Z80_WR active, continue if it is as we consider IO WRITE cycles.
                 "                        tst      r6, #16                  \n"
                 "                        beq      cbrx                     \n"
   
                                          // Is Z80_RD active, continue if it is as we consider IO READ cycles.
                 "                        tst      r6, #128                 \n"
                 "                        bne      irqPortC_Exit            \n"
                 "                                                          \n"
                                          // Is Z80_RFSH active, exit if so as we dont want to consider refresh cycles.
                 "                        tst      r5, #0x800000            \n"
                 "                        beq      irqPortC_Exit            \n"

                                          // Is Z80_M1 active, exit if so as we dont want to consider instruction cycles.
                 "                        tst      r7, #0x20                \n"
                 "                        beq      irqPortC_Exit            \n"
                 "              cbrx:                                       \n"
                );


  #if DECODE_Z80_IO >= 2

                                          // Is the memory address in the 0xE000 region?
                                          // Only interested in certain locations (memory mapped IO), exit if not in range.
    asm volatile("                        lsrs     r3, r8, #11              \n"       // (z80Control.portE >> 11)&0x8000)
                 "                        and      r3, r3, #32768           \n"
                 "                        lsls     r2, r4, #9               \n"        // (z80Control.portA << 9)&0x4000)
                 "                        and      r2, r2, #16384           \n"
                 "                        orrs     r3, r2                   \n"
                 "                        lsrs     r2, r4, #1               \n"        // (z80Control.portA >> 1)&0x2000)
                 "                        and      r2, r2, #8192            \n"
                 "                        orrs     r3, r2                   \n"
                 "                        lsrs     r2, r4, #3               \n"        // (z80Control.portA >> 3)&0x1000)
                 "                        and      r2, r2, #4096            \n"
                 "                        orrs     r3, r2                   \n"
                 "                        lsrs     r2, r4, #5               \n"        // (z80Control.portA >> 5)&0x0800)
                 "                        and      r2, r2, #2048            \n"
                 "                        orrs     r3, r2                   \n"
                 "                        cmp      r3, #57344               \n"        // == 0xE000
                 "                        bne      irqPortC_Exit            \n"
                );

                                          // Get the lower part of the address into R0 which we compare with the various registers and update.
                                          //
    asm volatile("                        mov      r3, r5, lsr #8           \n"    // (z80Control.portB >> 8)&0x0400)
                 "                        and      r3, r3, #1024            \n"
                 "                        mov      r2, r5, lsr #10          \n"   // (z80Control.portB >> 10)&0x0200)
                 "                        and      r2, r2, #512             \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r5, lsr #2           \n"    // (z80Control.portB >> 2)&0x0100)
                 "                        and      r2, r2, #256             \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r5, lsr #4           \n"    // (z80Control.portB >> 2)&0x0080)
                 "                        and      r2, r2, #128             \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r8, lsr #18          \n"   // (z80Control.portE >> 2)&0x0040)
                 "                        and      r2, r2, #64              \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r8, lsr #20          \n"   // (z80Control.portE >> 2)&0x0020)
                 "                        and      r2, r2, #32              \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r6, lsr #4           \n"    // (z80Control.portC >> 4)&0x0010)
                 "                        and      r2, r2, #16              \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r6, lsr #6           \n"    // (z80Control.portC >> 6)&0x0008)
                 "                        and      r2, r2, #8               \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r6, lsr #8           \n"    // (z80Control.portC >> 8)&0x0004)
                 "                        and      r2, r2, #4               \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r6, lsr #8           \n"    // (z80Control.portC >> 10)&0x0002)
                 "                        and      r2, r2, #2               \n"
                 "                        orr      r3, r2                   \n"
                 "                        mov      r2, r4, lsr #17          \n"   // (z80Control.portA >> 17)&0x0001)
                 "                        and      r2, r2, #1               \n"
                 "                        orr      r3, r2                   \n"

                                          ::: "r2","r3","r4","r5","r7","r8","r9","r10","r11","r12");

    asm volatile(                         // A memory swap event.
                 "                        movw     r1," XSTR(MZ_MEMORY_SWAP) "\n"
                 "                        cmp      r0, r1                   \n"
                 "                        bne      br0                      \n"
                 "                        movs     r2, #1                   \n"
                 "                        b.n      br1                      \n"
                 "              br0:                                        \n"
                                          // A memory reset event.
                 "                        movw     r1, " XSTR(MZ_MEMORY_RESET) "\n"
                 "                        cmp      r0, r1                   \n"
                 "                        bne      br2                      \n"
                 "                        movs     r2, #0                   \n"
                 "              br1:                                        \n"
                                          // Store to memorySwap
                 "                        strb     r2, %[val0]              \n"
                 "                        b.n      irqPortC_Exit            \n"
                 "              br2:                                        \n"
                                          // A CRT to normal mode event.
                 "                        movw     r1, " XSTR(MZ_CRT_NORMAL) "\n"
                 "                        cmp      r0, r1                   \n"
                 "                        bne      br3                      \n"
                 "                        movs     r2, #0                   \n"
                 "                        b.n      br4                      \n"
                 "              br3:                                        \n"
                                          // A CRT to inverse mode event.
                 "                        movw     r1, " XSTR(MZ_CRT_INVERSE) "\n"
                 "                        cmp      r0, r1                   \n"
                 "                        bne      br5                      \n"
                 "                        movs     r2, #0                   \n"
                 "              br4:                                        \n"
                                          // Store to crtMode.
                 "                        strb     r2, %[val1]              \n" 
                 "                        b.n      irqPortC_Exit            \n"
                 "              br5:                                        \n"
                                          // Memory address in SCROLL region?
                 "                        sub.w    r1, r0, " XSTR(MZ_SCROL_END - MZ_SCROL_BASE) "\n"
                 "                        cmp      r1, #255                 \n"
                 "                        bhi.n    irqPortC_Exit            \n"
                 "                        strb     r0, %[val2]              \n" 

                                          : [val0] "+m" (z80Control.memorySwap),
                                            [val1] "+m" (z80Control.crtMode),
                                            [val2] "+m" (z80Control.scroll)
                                          :
                                          : "r2","r3","r4","r5","r7","r8","r9","r10","r11","r12");
  #endif

    asm volatile("       irqPortC_Exit:                                     \n"
                 "                        pop      {r0-r8,pc}               \n"
                                          :
                                          :
                                          : "r4","r5","r6","r7","r8","r9","r10","r11","r12");

    return;
}


// Method to install the interrupt vector and enable it to capture Z80 memory/IO operations.
//
static void setupIRQ(void)
{
    __disable_irq();
  #if DECODE_Z80_IO == 0 || DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
    // Install the method to be called when PortE triggers.
    _VectorsRam[IRQ_PORTE + 16] = irqPortE;
  #endif

    // Install the method to be called when PortD triggers.
    _VectorsRam[IRQ_PORTD + 16] = irqPortD;
   
    // Install the method to be called when PortC triggers.
    _VectorsRam[IRQ_PORTC + 16] = irqPortC;
    __enable_irq();

  #if DECODE_Z80_IO == 0 || DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
    // Setup the IRQ for TZ_SVCREQ.
    installIRQ(TZ_SVCREQ, IRQ_MASK_FALLING);
    
    // Setup the IRQ for TZ_SYSREQ.
    installIRQ(TZ_SYSREQ, IRQ_MASK_FALLING);
  #endif

  #if DECODE_Z80_IO >= 2
    // Setup the IRQ for Z80_MREQ.
 //   installIRQ(Z80_MREQ, IRQ_MASK_RISING);
  #endif
   
  #if DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2 || DECODE_Z80_IO == 3
    // Setup the IRQ for Z80_IORQ.
    installIRQ(Z80_IORQ, IRQ_MASK_FALLING);
  #endif
   
    // Setup the IRQ for Z80_RESET.
    installIRQ(Z80_RESET, IRQ_MASK_FALLING);

    // Setup priorities, service request is the highest followed by IORQ.
    //
  #if DECODE_Z80_IO == 0 || DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
    NVIC_SET_PRIORITY(IRQ_PORTE, 0);
    NVIC_SET_PRIORITY(IRQ_PORTD, 16);
  #else
    NVIC_SET_PRIORITY(IRQ_PORTD, 0);
  #endif
}

// Method to restore the interrupt vector when the pin mode is changed and restored to default input mode.
//
static void restoreIRQ(void)
{
  #if DECODE_Z80_IO == 0 || DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2
    // Setup the IRQ for TZ_SVCREQ.
    installIRQ(TZ_SVCREQ, IRQ_MASK_FALLING);
    
    // Setup the IRQ for TZ_SYSREQ.
    installIRQ(TZ_SYSREQ, IRQ_MASK_FALLING);
  #endif

  #if DECODE_Z80_IO >= 2
    // Setup the IRQ for Z80_MREQ.
 //   installIRQ(Z80_MREQ, IRQ_MASK_FALLING);
  #endif
  
  #if DECODE_Z80_IO == 1 || DECODE_Z80_IO == 2 || DECODE_Z80_IO == 3
    // Setup the IRQ for Z80_IORQ.
    installIRQ(Z80_IORQ, IRQ_MASK_FALLING);
  #endif
   
    // Setup the IRQ for Z80_RESET.
    installIRQ(Z80_RESET, IRQ_MASK_FALLING);
}

// Method to setup the pins and the pin map to power up default.
// The OS millisecond counter address is passed into this library to gain access to time without the penalty of procedure calls.
// Time is used for timeouts and seriously affects pulse width of signals when procedure calls are made.
//
void setupZ80Pins(uint8_t initTeensy, volatile uint32_t *millisecondTick)
{
    // Locals.
    //
    static uint8_t firstCall = 1;

    // This method can be called more than once as a convenient way to return all pins to default. On first call though
    // the teensy library needs initialising, hence the static check.
    //
    if(firstCall == 1)
    {
        if(initTeensy)
            _init_Teensyduino_internal_();

        // Setup the pointer to the millisecond tick value updated by K64F interrupt.
        ms = millisecondTick;
    }

    // Setup the map of a loop useable array with its non-linear Pin Number.
    //
    pinMap[Z80_A0]      = Z80_A0_PIN;
    pinMap[Z80_A1]      = Z80_A1_PIN;
    pinMap[Z80_A2]      = Z80_A2_PIN;
    pinMap[Z80_A3]      = Z80_A3_PIN;
    pinMap[Z80_A4]      = Z80_A4_PIN;
    pinMap[Z80_A5]      = Z80_A5_PIN;
    pinMap[Z80_A6]      = Z80_A6_PIN;
    pinMap[Z80_A7]      = Z80_A7_PIN;
    pinMap[Z80_A8]      = Z80_A8_PIN;
    pinMap[Z80_A9]      = Z80_A9_PIN;
    pinMap[Z80_A10]     = Z80_A10_PIN;
    pinMap[Z80_A11]     = Z80_A11_PIN;
    pinMap[Z80_A12]     = Z80_A12_PIN;
    pinMap[Z80_A13]     = Z80_A13_PIN;
    pinMap[Z80_A14]     = Z80_A14_PIN;
    pinMap[Z80_A15]     = Z80_A15_PIN;
    pinMap[Z80_A16]     = Z80_A16_PIN;
    pinMap[Z80_A17]     = Z80_A17_PIN;
    pinMap[Z80_A18]     = Z80_A18_PIN;

    pinMap[Z80_D0]      = Z80_D0_PIN;
    pinMap[Z80_D1]      = Z80_D1_PIN;
    pinMap[Z80_D2]      = Z80_D2_PIN;
    pinMap[Z80_D3]      = Z80_D3_PIN;
    pinMap[Z80_D4]      = Z80_D4_PIN;
    pinMap[Z80_D5]      = Z80_D5_PIN;
    pinMap[Z80_D6]      = Z80_D6_PIN;
    pinMap[Z80_D7]      = Z80_D7_PIN;

    pinMap[Z80_MEM0]    = Z80_MEM0_PIN;
    pinMap[Z80_MEM1]    = Z80_MEM1_PIN;
    pinMap[Z80_MEM2]    = Z80_MEM2_PIN;
    pinMap[Z80_MEM3]    = Z80_MEM3_PIN;
    pinMap[Z80_MEM4]    = Z80_MEM4_PIN;

    pinMap[Z80_IORQ]    = Z80_IORQ_PIN;
    pinMap[Z80_MREQ]    = Z80_MREQ_PIN;
    pinMap[Z80_RD]      = Z80_RD_PIN;
    pinMap[Z80_WR]      = Z80_WR_PIN;
    pinMap[Z80_WAIT]    = Z80_WAIT_PIN;
    pinMap[Z80_BUSACK]  = Z80_BUSACK_PIN;

    pinMap[Z80_NMI]     = Z80_NMI_PIN;
    pinMap[Z80_INT]     = Z80_INT_PIN;
    pinMap[Z80_RESET]   = Z80_RESET_PIN;
    pinMap[MB_SYSCLK]   = SYSCLK_PIN;
    pinMap[TZ_BUSACK]   = TZ_BUSACK_PIN;
    pinMap[TZ_SVCREQ]   = TZ_SVCREQ_PIN;
    pinMap[TZ_SYSREQ]   = TZ_SYSREQ_PIN;

    pinMap[CTL_BUSACK]  = CTL_BUSACK_PIN;
    pinMap[CTL_BUSRQ]   = CTL_BUSRQ_PIN;
    pinMap[CTL_RFSH]    = CTL_RFSH_PIN;
    pinMap[CTL_HALT]    = CTL_HALT_PIN;
    pinMap[CTL_M1]      = CTL_M1_PIN;
    pinMap[CTL_CLK]     = CTL_CLK_PIN;
    pinMap[CTL_CLKSLCT] = CTL_CLKSLCT_PIN;

    // Now build the config array for all the ports. This aids in more rapid function switching as opposed to using
    // the pinMode and digitalRead/Write functions provided in the Teensy lib.
    //
    for(uint8_t idx=0; idx < MAX_TRANZPUTER_PINS; idx++)
    {
        ioPin[idx] = portConfigRegister(pinMap[idx]);

        // Set to input, will change as functionality dictates.
        if(idx != CTL_CLK && idx != CTL_BUSRQ && idx != CTL_BUSACK && idx != Z80_WAIT)
        {
            pinInput(idx);
        } else
        {
            if(idx == CTL_BUSRQ || idx == CTL_BUSACK || idx == Z80_WAIT)
            {
                pinOutputSet(idx, HIGH);
            } else
            {
                // Setup the alternative clock frequency on the CTL_CLK pin.
                //
                analogWriteFrequency(CTL_CLK_PIN, 3580000);
                analogWrite(CTL_CLK_PIN, 128);
            }
        }
    }

    // Initialise control structure.
    z80Control.svcControlAddr = getServiceAddr();
    z80Control.refreshAddr    = 0x00;
    z80Control.disableRefresh = 0;
    z80Control.runCtrlLatch   = readCtrlLatch(); 
    z80Control.ctrlMode       = Z80_RUN;
    z80Control.busDir         = TRISTATE;

    // Final part of first call initialisation, now that the pins are configured, setup the interrupts.
    //
    if(firstCall == 1)
    {
        firstCall = 0;

        // Control structure elements only in zOS.
        //
        z80Control.resetEvent        = 0;
        z80Control.svcRequest        = 0;
        z80Control.sysRequest        = 0;
      #if DECODE_Z80_IO >= 1
        z80Control.ioAddr            = 0;
        z80Control.ioEvent           = 0;
        z80Control.mz700.config      = 0x202;
      #endif
      #if DECODE_Z80_IO >= 2
        z80Control.ioData            = 0;
        z80Control.memorySwap        = 0;
        z80Control.crtMode           = 0;
        z80Control.scroll            = 0;
      #endif

        // Setup the Interrupts for IORQ and MREQ.
        setupIRQ();
    }

    // Debug to quickly find out addresses of GPIO pins.
    //#define GPIO_BITBAND_ADDR(reg, bit) (((uint32_t)&(reg) - 0x40000000) * 32 + (bit) * 4 + 0x42000000)
    //printf("%08lx, %08lx, %08lx, %08lx\n", GPIO_BITBAND_ADDR(CORE_PIN15_PORTREG, CORE_PIN15_BIT), CORE_PIN15_PORTREG, digital_pin_to_info_PGM[(CTL_BUSRQ_PIN)].reg + 64,digital_pin_to_info_PGM[(CTL_BUSRQ_PIN)].reg + 32 );
    //printf("%08lx, %08lx, %08lx, %08lx\n", GPIO_BITBAND_ADDR(CORE_PIN13_PORTREG, CORE_PIN13_BIT), CORE_PIN13_PORTREG, digital_pin_to_info_PGM[(Z80_WAIT_PIN)].reg + 64,digital_pin_to_info_PGM[(Z80_WAIT_PIN)].reg + 32 );
    //printf("%08lx, %08lx, %08lx, %08lx\n", GPIO_BITBAND_ADDR(CORE_PIN16_PORTREG, CORE_PIN16_BIT), CORE_PIN16_PORTREG, digital_pin_to_info_PGM[(CTL_BUSACK_PIN)].reg + 64,digital_pin_to_info_PGM[(CTL_BUSACK_PIN)].reg +32 );

    return;
}

// Method to reset the Z80 CPU.
//
void resetZ80(void)
{
    // Locals.
    //
    uint32_t startTime = *ms;

    // Simply change the Z80_RESET pin to an output, assert low, reset high and reset to input to create the hardware reset event. On the original hardware the
    // reset pulse width is 90uS, the ms timer is only accurate to 100uS so we apply a 100uS pulse.
    //
    __disable_irq();
    pinOutputSet(Z80_RESET, LOW);
    for(volatile uint32_t pulseWidth=0; pulseWidth < 100; pulseWidth++);
    pinHigh(Z80_RESET);
    pinInput(Z80_RESET);
    __enable_irq();

    // Wait a futher settling period before reinstating the interrupt.
    //
    while((*ms - startTime) < 400);

    // Restore the Z80 RESET IRQ as we have changed the pin mode.
    //
    installIRQ(Z80_RESET, IRQ_MASK_FALLING);

    // Clear the event flag as it is set 
    //z80Control.resetEvent = 0;
}

// Method to request the Z80 Bus. This halts the Z80 and sets all main signals to tri-state.
// Return: 0 - Z80 Bus obtained.
//         1 - Failed to obtain the Z80 bus.
uint8_t reqZ80Bus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t  result = 0;
    uint32_t startTime = *ms;

    // Set BUSRQ low which sets the Z80 BUSRQ low.
    pinLow(CTL_BUSRQ);

    // Wait for the Z80 to acknowledge the request.
    while((*ms - startTime) < timeout && pinGet(Z80_BUSACK));

    // If we timed out, deassert BUSRQ and return error.
    //
    if((*ms - startTime) >= timeout)
    {
        pinHigh(CTL_BUSRQ);
        result = 1;
    }
    
    // Store the control latch state before we start modifying anything enabling a return to the pre bus request state.
    z80Control.runCtrlLatch = readCtrlLatch(); 

    return(result);
}

// Method to request access to the main motherboard circuitry. This involves requesting the Z80 bus and then
// setting the Z80_RD and Z80_WR signals to low, this maps to a ENABLE_BUS signal which ensures that one half of
// the Mainboard BUSACK request signal is disabled (BUSACK to the motherboard has the opposite meaning, when active
// the mainboard is tri-stated and signals do not pass from th e Z80 and local memory to the mainboard). The other
// half of the mainboard request signal is disabled by setting CTL_BUSACK high.
//
uint8_t reqMainboardBus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t            result = 0;

    // Ensure the CTL BUSACK signal high so we dont assert the mainboard BUSACK signal.
    pinHigh(CTL_BUSACK);

    // Requst the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
        // Now request the mainboard by setting BUSACK high and Z80_RD/Z80_WR low.
        pinOutput(Z80_RD);
        pinOutput(Z80_WR);

        // A special mode in the FlashRAM decoder detects both RD and WR being low at the same time, this is not feasible under normal Z80 operating conditions, but what it signals
        // here is to raise ENABLE_BUS which ensures that BUSACK on the mainboard is deasserted. This is for the case where the Z80 may be running in tranZPUter memory with the
        // mainboard disabled.
        pinLow(Z80_RD);
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 1; pulseWidth++);

        // Immediately return the RD/WR to HIGH to complete the ENABLE_BUS latch action.
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);

        // Store the mode.
        z80Control.ctrlMode = MAINBOARD_ACCESS;

        // For mainboard access, MEM4:0 should be 0 so no activity is made to the tranZPUter circuitry except the control latch.
        z80Control.curCtrlLatch = TZMM_ORIG;
    } else
    {
        printf("Failed to request Mainboard Bus\n");
    }

    return(result);
}

// Method to request the local tranZPUter bus. This involves making a Z80 bus request and when relinquished, setting
// the CTL_BUSACK signal to low which disables (tri-states) the mainboard circuitry.
//
uint8_t reqTranZPUterBus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t  result = 0;

    // Ensure the CTL BUSACK signal high so we dont assert the mainboard BUSACK signal.
    pinHigh(CTL_BUSACK);

    // Requst the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
        // Now disable the mainboard by setting BUSACK low.
        pinLow(CTL_BUSACK);
       
        // Store the mode.
        z80Control.ctrlMode = TRANZPUTER_ACCESS;
      
        // For tranZPUter access, MEM4:0 should be TZMM[0-7] so no activity is made to the mainboard circuitry.
        z80Control.curCtrlLatch = TZMM_TZPU0;
    }

    return(result);
}

// Method to set all the pins to be able to perform a transaction on the Z80 bus.
//
void setupSignalsForZ80Access(enum BUS_DIRECTION dir)
{
    // Address lines (apart from the upper bank address lines A18:16) need to be outputs.
    for(uint8_t idx=Z80_A0; idx <= Z80_A15; idx++)
    {
        pinOutput(idx);
    }
    pinInput(Z80_A16);       // The upper address bits can only be changed via the 273 latch IC which requires a Z80 IO Write transaction.
    pinInput(Z80_A17);
    pinInput(Z80_A18);

    // Control signals need to be output and deasserted.
    //
    pinOutputSet(Z80_IORQ, HIGH);
    pinOutputSet(Z80_MREQ, HIGH);
    pinOutputSet(Z80_RD,   HIGH);
    pinOutputSet(Z80_WR,   HIGH);

    // Additional control lines need to be outputs and deasserted. These lines are for the main motherboard and not used on the tranZPUter.
    //
    pinOutputSet(CTL_HALT, HIGH);
    pinOutputSet(CTL_RFSH, HIGH);
    pinOutputSet(CTL_M1,   HIGH);

    // Setup bus direction.
    //
    setZ80Direction(dir);
    return;
}

// Method to release the Z80, set all signals to input and disable BUSRQ.
//
void releaseZ80(void)
{
    // All address lines to inputs, upper A18:16 are always defined as inputs as they can only be read by the K64F, they are driven by a
    // latch on the tranZPUter board.
    //
    for(uint8_t idx=Z80_A0; idx <= Z80_A15; idx++)
    {
        pinInput(idx);
    }

    // If we were in write mode then set the data bus to input.
    //
    if(z80Control.busDir == WRITE)
    {
        // Same for data lines, revert to being inputs.
        for(uint8_t idx=Z80_D0; idx <= Z80_D7; idx++)
        {
            pinInput(idx);
        }
    }

    // All control signals to inputs.
    pinInput(CTL_HALT);
    pinInput(CTL_RFSH);
    pinInput(CTL_M1);
    pinInput(Z80_IORQ);
    pinInput(Z80_MREQ);
    pinInput(Z80_RD);
    pinInput(Z80_WR);

    // Finally release the Z80 by deasserting the CTL_BUSRQ signal.
    pinHigh(CTL_BUSACK);
    pinHigh(CTL_BUSRQ);
    
    // Store the mode.
    z80Control.ctrlMode = Z80_RUN;
       
    // Indicate bus direction.
    z80Control.busDir = TRISTATE;

    // Restore interrupt monitoring on pins.
    //
    restoreIRQ();
    return;
}


// Method to write a memory mapped byte onto the Z80 bus.
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
//
uint8_t writeZ80Memory(uint16_t addr, uint8_t data)
{
    // Locals.
    uint32_t          startTime = *ms;
    volatile uint32_t pulseWidth;

    // Set the data and address on the bus.
    //
    setZ80Addr(addr);
    setZ80Data(data);

    // Setup time before applying control signals.
    for(pulseWidth = 0; pulseWidth < 5; pulseWidth++);
    pinLow(Z80_MREQ);

    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // If WAIT has been asserted, loop. Set a timeout to prevent a total lockup. Wait shouldnt exceed 100mS, it it does there is a
        // hardware fault and components such as DRAM will lose data due to no refresh.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);

        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
        while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);
    }

    // Hold time for the WR signal before clearing it.
    for(pulseWidth = 0; pulseWidth < 5; pulseWidth++);

    // Complete the write cycle.
    //
    pinHigh(Z80_WR);
    pinHigh(Z80_MREQ);
    return(0);
}

// Method to read a memory mapped byte from the Z80 bus.
// Keep to the Z80 timing diagram, but for better accuracy of matching the timing diagram this needs to be coded in assembler.
//
uint8_t readZ80Memory(uint16_t addr)
{
    // Locals.
    uint32_t startTime = *ms;
    uint8_t  data;

    // Set the address on the bus and assert MREQ and RD.
    //
    setZ80Addr(addr);
    pinLow(Z80_MREQ);
    pinLow(Z80_RD);
  
    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
        
        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 1; pulseWidth++);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readDataBus();

    // Complete the read cycle.
    //
    pinHigh(Z80_RD);
    pinHigh(Z80_MREQ);

    // Finally pass back the byte read to the caller.
    return(data);
}


// Method to write a byte onto the Z80 I/O bus. This method is almost identical to the memory mapped method but are kept seperate for different
// timings as needed, the more code will create greater delays in the pulse width and timing.
//
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
//
uint8_t writeZ80IO(uint16_t addr, uint8_t data)
{
    // Locals.
    uint32_t startTime = *ms;

    // Set the data and address on the bus.
    //
    setZ80Addr(addr);
    setZ80Data(data);
    pinLow(Z80_IORQ);

    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // If WAIT has been asserted, loop. Set a timeout to prevent a total lockup. Wait shouldnt exceed 100mS, it it does there is a
        // hardware fault and components such as DRAM will lose data due to no refresh.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);

        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
        while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);
    }

    // Complete the write cycle.
    //
    pinHigh(Z80_WR);
    pinHigh(Z80_IORQ);
    return(0);
}

// Method to read a byte from the Z80 I/O bus. This method is almost identical to the memory mapped method but are kept seperate for different
// timings as needed, the more code will create greater delays in the pulse width and timing.
//
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
uint8_t readZ80IO(uint16_t addr)
{
    // Locals.
    uint32_t startTime = *ms;
    uint8_t  data;

    // Set the address on the bus and assert MREQ and RD.
    //
    setZ80Addr(addr);
    pinLow(Z80_IORQ);
    pinLow(Z80_RD);
  
    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readDataBus();

    // Complete the read cycle.
    //
    pinHigh(Z80_RD);
    pinHigh(Z80_IORQ);

    // Finally pass back the byte read to the caller.
    return(data);
}

// Method to perform a refresh cycle on the z80 mainboard bus to ensure dynamic RAM contents are maintained during extended bus control periods.
//
// Under normal z80 processing a refresh cycle is issued every instruction during the T3/T4 cycles. As we arent reading instructions but just reading/writing
// then the refresh cycles are made after a cnofigurable set number of bus transactions (ie. 128 read/writes). This has to occur when either of the busses
// are under K64F control so a call to this method should be made frequently.
// 
void refreshZ80(void)
{
    // Locals.
    volatile uint8_t idx = 0;

    // Check to see if a refresh operation is needed.
    //
    if(z80Control.disableRefresh == 1)
        return;

    // Set 7 bits on the address bus.
    setZ80RefreshAddr(z80Control.refreshAddr);

    // If we are controlling the tranZPUter bus then a switch to the mainboard needs to be made first.
    //
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Quick way to gain mainboard access, force an enable of the Z80_BUSACK on the mainboard by setting RD/WR low, this fires an enable pulse at the 279 RS Flip Flop
        // and setting CTL_BUSACK high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_BUSACK);
    }

    // Assert Refresh.
    pinLow(CTL_RFSH);
    pinLow(Z80_MREQ);
    idx++;              // Increase the pulse width of the MREQ signal.
    pinHigh(Z80_MREQ);
    pinHigh(CTL_RFSH);

    // Restore access to tranZPUter bus if this was the original mode.
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Restore tranZPUter bus control.
        pinLow(CTL_BUSACK);
    }
   
    // Increment refresh address to complete.
    z80Control.refreshAddr++;
    z80Control.refreshAddr &= 0x7f;
    return;
}

// Method to perform a full row refresh on the dynamic DRAM. This method writes out a full 7bit address so that all rows are refreshed.
//
void refreshZ80AllRows(void)
{
    // Locals.
    volatile uint8_t idx;

    // Check to see if a refresh operation is needed.
    //
    if(z80Control.disableRefresh == 1)
        return;

    // If we are controlling the tranZPUter bus then a switch to the mainboard needs to be made first.
    //
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Quick way to gain mainboard access, force an enable of the Z80_BUSACK on the mainboard by setting RD/WR low, this fires an enable pulse at the 279 RS Flip Flop
        // and setting CTL_BUSACK high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_BUSACK);
    }

    // Loop through all 7 bits of refresh rows.
    idx = 0;
    while(idx < 0x80)
    {
        // Set 7 bits on the address bus.
        setZ80RefreshAddr(idx);

        // Assert Refresh.
        pinLow(CTL_RFSH);
        pinLow(Z80_MREQ);
        idx++;
        pinHigh(Z80_MREQ);
        pinHigh(CTL_RFSH);
    }

    // Restore access to tranZPUter bus if this was the original mode.
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Restore tranZPUter bus control.
        pinLow(CTL_BUSACK);
    }
    return;
}

// Method to explicitly set the Memory model/mode of the tranZPUter.
//
void setCtrlLatch(uint8_t latchVal)
{
    // Gain control of the bus then set the latch value.
    //
    if(reqTranZPUterBus(100) == 0)
    {
        // Setup the pins to perform a write operation.
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(latchVal);
        releaseZ80();
     }
     return;  
}

// Method to change the secondary CPU frequency and optionally enable/disable it.
// Input: frequency = desired frequency in Hertz.
//        action    = 0 - take no action, just change frequency, 1 - set and enable the secondary CPU frequency, 2 - set and disable the secondary CPU frequency,
//                    3 - enable the secondary CPU frequency, 4 - disable the secondary CPU frequency
// Output: actual set frequency in Hertz.
//
uint32_t setZ80CPUFrequency(float frequency, uint8_t action)
{
    // Locals.
    //
    uint32_t actualFreq = 0;

    // Setup the alternative clock frequency on the CTL_CLK pin.
    //
    if(action < 3)
    {
        actualFreq=analogWriteFrequency(CTL_CLK_PIN, frequency);
        analogWrite(CTL_CLK_PIN, 128);
    }

    // Process action, enable, disable or do nothing (just freq change).
    //
    if(action > 0)
    {
        // Gain control of the bus to change the CPU frequency latch.
        //
        if(reqTranZPUterBus(100) == 0)
        {
            // Setup the pins to perform a write operation.
            //
            setupSignalsForZ80Access(WRITE);
            writeZ80IO((action == 1 || action == 3 ? IO_TZ_SETXMHZ : IO_TZ_SET2MHZ), 0);
            releaseZ80();
         }
    }

    // Return the actual frequency set, this will be the nearest frequency the timers are capable of resolving.
    return(actualFreq);
}


// Method to copy memory from the K64F to the Z80.
//
uint8_t copyFromZ80(uint8_t *dst, uint32_t src, uint32_t size, uint8_t mainBoard)
{
    // Locals.
    uint8_t    result = 0;
    uint8_t    upperAddrBits = 0;

    // Sanity checks.
    //
    if((mainBoard == 1 && (src+size) > 0x10000) || (mainBoard == 0 && (src+size) > 0x80000))
        return(1);

    // Request the correct bus.
    //
    if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        for(uint32_t idx=0; idx < size && result == 0; idx++)
        {
            // If the address changes the upper address bits, update the latch to reflect the change.
            if((uint8_t)(src >> 16) != upperAddrBits)
            {
                // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                setZ80Direction(WRITE);
                upperAddrBits = (uint8_t)(src >> 16);
                writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
                setZ80Direction(READ);
            }

            // Perform a refresh on the main memory every 2ms.
            //
            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
         
            // And now read the byte and store.
            *dst = readZ80Memory((uint16_t)src);
            src++;
            dst++;
        }
       
        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    return(result);
}

// Method to copy memory to the K64F from the Z80.
//
uint8_t copyToZ80(uint32_t dst, uint8_t *src, uint32_t size, uint8_t mainBoard)
{
    // Locals.
    uint8_t    result = 0;
    uint8_t    upperAddrBits = 0;

    // Sanity checks.
    //
    if((mainBoard == 1 && (dst+size) > 0x10000) || (mainBoard == 0 && (dst+size) > 0x80000))
        return(1);

    // Request the correct bus.
    //
    if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
    {
        // Setup the pins to perform a write operation.
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        for(uint32_t idx=0; idx < size && result == 0; idx++)
        {
            // If the address changes the upper address bits, update the latch to reflect the change.
            if((uint8_t)(dst >> 16) != upperAddrBits)
            {
                // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                upperAddrBits = (uint8_t)(dst >> 16);
                writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
            }

            // Perform a refresh on the main memory every 2ms.
            //
            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
           
            // And now write the byte and to the next address!
            writeZ80Memory((uint16_t)dst, *src);
            src++;
            dst++;
        }
       
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    return(result);
}


// Method to fill memory under the Z80 control, either the mainboard or tranZPUter memory.
//
void fillZ80Memory(uint32_t addr, uint32_t size, uint8_t data, uint8_t mainBoard)
{
    // Locals.
    uint8_t  upperAddrBits = 0;

    if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // Fill the memory but every RFSH_BYTE_CNT perform a DRAM refresh.
        //
        for(uint32_t idx=addr; idx < (addr+size); idx++)
        {
            // If the address changes the upper address bits, update the latch to reflect the change.
            if((uint8_t)(idx >> 16) != upperAddrBits)
            {
                // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                upperAddrBits = (uint8_t)(idx >> 16);
                writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
            }

            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
            writeZ80Memory((uint16_t)idx, data);
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// A method to read the full video frame buffer from the Sharp MZ80A and store it in local memory (control structure). 
// No refresh cycles are needed as we grab the frame but between frames a full refresh is performed.
//
void captureVideoFrame(enum VIDEO_FRAMES frame, uint8_t noAttributeFrame)
{
    // Locals.

    if(reqMainboardBus(100) == 0)
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        // No need for refresh as we take less than 2ms time, just grab the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            z80Control.videoRAM[frame][idx] = readZ80Memory((uint16_t)idx+MZ_VID_RAM_ADDR);
        }

        // Perform a full row refresh to maintain the DRAM.
        refreshZ80AllRows();

        // If flag not set capture the attribute RAM. This is normally not present on a standard Sharp MZ80A only present on models
        // with the MZ80A Colour Board upgrade.
        //
        if(noAttributeFrame == 0)
        {
            // Same for the attribute frame, no need for refresh as we take 2ms or less, just grab it.
            for(uint16_t idx=0; idx < MZ_ATTR_RAM_SIZE; idx++)
            {
                z80Control.attributeRAM[frame][idx] = readZ80Memory((uint16_t)idx+MZ_ATTR_RAM_ADDR);
            }

            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// Method to refresh the video frame buffer on the Sharp MZ80A with the data held in local memory.
//
void refreshVideoFrame(enum VIDEO_FRAMES frame, uint8_t scrolHome, uint8_t noAttributeFrame)
{
    // Locals.

    if(reqMainboardBus(100) == 0)
    {
        // Setup the pins to perform a write operation.
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // No need for refresh as we take less than 2ms time, just write the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            writeZ80Memory((uint16_t)idx+MZ_VID_RAM_ADDR, z80Control.videoRAM[frame][idx]);
        }

        // Perform a full row refresh to maintain the DRAM.
        refreshZ80AllRows();
       
        // If flag not set write out to the attribute RAM. This is normally not present on a standard Sharp MZ80A only present on models
        // with the MZ80A Colour Board upgrade.
        //
        if(noAttributeFrame == 0)
        {
            // No need for refresh as we take less than 2ms time, just write the video frame.
            for(uint16_t idx=0; idx < MZ_ATTR_RAM_SIZE; idx++)
            {
                writeZ80Memory((uint16_t)idx+MZ_ATTR_RAM_ADDR, z80Control.attributeRAM[frame][idx]);
            }
       
            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // If the Scroll Home flag is set, this means execute a read against the hardware scoll register to restore the position 0,0 to location MZ_VID_RAM_ADDR.
        if(scrolHome)
        {
            setZ80Direction(READ);
            readZ80Memory((uint16_t)MZ_SCROL_BASE);
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// Method to load up the local video frame buffer from a file.
//
FRESULT loadVideoFrameBuffer(char *src, enum VIDEO_FRAMES frame)
{
    // Locals.
    //
    FIL           File;
    unsigned int  readSize;
    FRESULT       result;

    // Sanity check on filenames.
    if(src == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and open the source file.
    result = f_open(&File, src, FA_OPEN_EXISTING | FA_READ);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!result)
    {
        memset(z80Control.videoRAM[frame], MZ_VID_DFLT_BYTE, MZ_VID_RAM_SIZE);
        result = f_read(&File, z80Control.videoRAM[frame], MZ_VID_RAM_SIZE, &readSize);
        if (!result)
        {
            memset(z80Control.attributeRAM[frame], MZ_ATTR_DFLT_BYTE, MZ_ATTR_RAM_SIZE);
            result = f_read(&File, z80Control.attributeRAM[frame], MZ_ATTR_RAM_SIZE, &readSize);
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("File not found:%s\n", src);
    }

    return(result ? result : FR_OK);    
}

// Method to save the local video frame buffer into a file.
//
FRESULT saveVideoFrameBuffer(char *dst, enum VIDEO_FRAMES frame)
{
    // Locals.
    //
    FIL           File;
    unsigned int  writeSize;        
    FRESULT       result;

    // Sanity check on filenames.
    if(dst == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and create the destination file.
    result = f_open(&File, dst, FA_CREATE_ALWAYS | FA_WRITE);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!result)
    {
        // Write the entire framebuffer to the SD file, video then attribute.
        //
        result = f_write(&File, z80Control.videoRAM[frame], MZ_VID_RAM_SIZE, &writeSize);
        if (!result && writeSize == MZ_VID_RAM_SIZE) 
        {
            result = f_write(&File, z80Control.attributeRAM[frame], MZ_ATTR_RAM_SIZE, &writeSize);
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("Cannot create file:%s\n", dst);
    }

    return(result ? result : FR_OK);    
}

// Simple method to pass back the address of a video frame for local manipulation.
//
char *getVideoFrame(enum VIDEO_FRAMES frame)
{
    return((char *)&z80Control.videoRAM[frame]);
}

// Simple method to pass back the address of an attribute frame for local manipulation.
//
char *getAttributeFrame(enum VIDEO_FRAMES frame)
{
    return((char *)&z80Control.attributeRAM[frame]);
}

// Method to load a file from the SD card directly into the tranZPUter static RAM or mainboard RAM.
//
FRESULT loadZ80Memory(const char *src, uint32_t fileOffset, uint32_t addr, uint32_t size, uint8_t mainBoard, uint8_t releaseBus)
{
    // Locals.
    //
    FIL           File;
    uint8_t       upperAddrBits = 0;
    uint32_t      loadSize       = 0L;
    uint32_t      sizeToRead     = 0L;
    uint32_t      memPtr         = addr;
    unsigned int  readSize;
    unsigned char buf[SECTOR_SIZE];
    FRESULT       fr0;

    // Sanity check on filenames.
    if(src == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and open the source file.
    fr0 = f_open(&File, src, FA_OPEN_EXISTING | FA_READ);

    // If no size given get the file size.
    //
    if(size == 0)
    {
        if(!fr0)
            fr0 = f_lseek(&File, f_size(&File));
        if(!fr0)
            size = (uint32_t)f_tell(&File);
    }

    // Seek to the correct location.
    //
    if(!fr0)
        fr0 = f_lseek(&File, fileOffset);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        // If the Z80 is in RUN mode, request the bus.
        // This mechanism allows for the load command to leave the BUS under the tranZPUter control for upload of multiple files. 
        // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
        // may occur.
        //
        if(z80Control.ctrlMode == Z80_RUN)
        {
            // Request the board according to the mainboard flag, mainboard = 1 then the mainboard is controlled otherwise the tranZPUter board.
            if(mainBoard == 0)
            {
                reqTranZPUterBus(100);
            } else
            {
                reqMainboardBus(100);
            }

            // If successful, setup the control pins for upload mode.
            //
            if(z80Control.ctrlMode != Z80_RUN)
            {
                // Setup the pins to perform a write operation.
                //
                setupSignalsForZ80Access(WRITE);
               
                // Setup the control latch to the required starting configuration.
                writeCtrlLatch(z80Control.curCtrlLatch);
            }
        } else
        {
            // See if the bus needs changing.
            //
            enum CTRL_MODE newMode = (mainBoard == 0) ? TRANZPUTER_ACCESS : MAINBOARD_ACCESS;
            reqZ80BusChange(newMode);
        }

        // If we have taken control of the bus, commence upload.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Loop, reading a sector at a time from SD file and writing it directly into the Z80 tranZPUter RAM or mainboard RAM.
            //
            loadSize = 0;
            memPtr = addr;
            do {
                // Wrap a disk read with two full refresh periods to counter for the amount of time taken to read disk.
                refreshZ80AllRows();
                sizeToRead = (size-loadSize) > SECTOR_SIZE ? SECTOR_SIZE : size - loadSize;
                fr0 = f_read(&File, buf, sizeToRead, &readSize);
                refreshZ80AllRows();
                if (fr0 || readSize == 0) break;   /* error or eof */

                // Go through each byte in sector and send to Z80 bus.
                for(unsigned int idx=0; idx < readSize; idx++)
                {
                    // If the address changes the upper address bits, update the latch to reflect the change.
                    if((uint8_t)(memPtr >> 16) != upperAddrBits)
                    {
                        // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                        upperAddrBits = (uint8_t)(memPtr >> 16);
                        writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
                    }

                    // At the halfway mark perform a full refresh.
                    if(idx == (SECTOR_SIZE/2)) 
                    {
                        refreshZ80AllRows();
                    }
                   
                    // And now write the byte and to the next address!
                    writeZ80Memory((uint16_t)memPtr, buf[idx]);
                    memPtr++;
                }
                loadSize += readSize;
            } while(loadSize < size);
        } else
        {
            printf("Failed to request Z80 access.\n");
            fr0 = FR_INT_ERR;
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("File not found:%s\n", src);
    }

    // If requested or an error occurs, then release the Z80 bus as no more uploads will be taking place in this batch.
    //
    if(releaseBus == 1 || fr0)
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    return(fr0 ? fr0 : FR_OK);    
}


// Method to load an MZF format file from the SD card directly into the tranZPUter static RAM or mainboard RAM.
// If the load address is specified then it overrides the MZF header value, otherwise load addr is taken from the header.
//
FRESULT loadMZFZ80Memory(const char *src, uint32_t addr, uint8_t mainBoard, uint8_t releaseBus)
{
    // Locals.
    FIL           File;
    unsigned int  readSize;
    t_svcDirEnt   mzfHeader;
    FRESULT       fr0;

    // Sanity check on filenames.
    if(src == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and open the source file.
    fr0 = f_open(&File, src, FA_OPEN_EXISTING | FA_READ);

    // If no error occurred, read in the header.
    //
    if(!fr0)
        fr0 = f_read(&File, (char *)&mzfHeader, MZF_HEADER_SIZE, &readSize);

    // No errors, process.
    if(!fr0 && readSize == MZF_HEADER_SIZE)
    {
        // Firstly, close the file, no longer needed.
        f_close(&File);
      
        // Save the header into the CMT area for reference, some applications expect it.
        // This assumes the TZFS is running and the memory bank is 64K block 0.
        //
        copyToZ80(MZ_CMT_ADDR, (uint8_t *)&mzfHeader, MZF_HEADER_SIZE, 0);
printf("File:%s,attr=%02x,addr:%08lx\n", src, mzfHeader.attr, addr);
        // Now obtain the parameters.
        //
        if(addr == 0xFFFFFFFF)
            addr = mzfHeader.loadAddr;

        // Look at the attribute byte, if it is >= 0xF8 then it is a special tranZPUter binary object requiring loading into a seperate memory bank.
        // The attribute & 0x07 << 16 specifies the memory bank in which to load the image.
        if(mzfHeader.attr >= 0xF8)
        {
            addr += ((mzfHeader.attr & 0x07) << 16);
            printf("CPM: Addr=%08lx\n", addr);
        }

        // Ok, load up the file into Z80 memory.
        fr0 = loadZ80Memory(src, MZF_HEADER_SIZE, addr, 0, mainBoard, releaseBus);
    }

    return(fr0 ? fr0 : FR_OK);    
}


// Method to read a section of the tranZPUter/mainboard memory and store it in an SD file.
//
FRESULT saveZ80Memory(const char *dst, uint32_t addr, uint32_t size, t_svcDirEnt *mzfHeader, uint8_t mainBoard)
{
    // Locals.
    //
    FIL           File;
    uint8_t       upperAddrBits = 0;
    uint32_t      saveSize       = 0L;
    uint32_t      sizeToWrite;
    uint32_t      memPtr         = addr;
    unsigned int  writeSize      = 0;        
    unsigned char buf[SECTOR_SIZE];
    FRESULT       fr0;

    // Sanity check on filenames.
    if(dst == NULL || size == 0)
        return(FR_INVALID_PARAMETER);
    
    // Try and create the destination file.
    fr0 = f_open(&File, dst, FA_CREATE_ALWAYS | FA_WRITE);

    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        // If an MZF header has been passed, we are saving an MZF file, write out the MZF header first prior to the data.
        //
        if(mzfHeader)
        {
            fr0 = f_write(&File, (char *)mzfHeader, MZF_HEADER_SIZE, &writeSize);
        }

        if(!fr0)
        {
            if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
            {
                // Setup the pins to perform a read operation (after setting the latch to starting value).
                //
                setupSignalsForZ80Access(WRITE);
                writeCtrlLatch(z80Control.curCtrlLatch);
                setZ80Direction(READ);
    
                // Loop, reading a sector worth of data (or upto limit remaining) from the Z80 tranZPUter RAM or mainboard RAM and writing it into the open SD card file.
                //
                saveSize = 0;
                for (;;) {
    
                    // Work out how many bytes to write in the sector then fetch from the Z80.
                    sizeToWrite = (size-saveSize) > SECTOR_SIZE ? SECTOR_SIZE : size - saveSize;
                    for(unsigned int idx=0; idx < sizeToWrite; idx++)
                    {
                        // If the address changes the upper address bits, update the latch to reflect the change.
                        if((uint8_t)(memPtr >> 16) != upperAddrBits)
                        {
                            // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                            setZ80Direction(WRITE);
                            upperAddrBits = (uint8_t)(memPtr >> 16);
                            writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
                            setZ80Direction(READ);
                        }
    
                        // At the halfway mark perform a full refresh.
                        if(idx == (SECTOR_SIZE/2))
                        {
                            refreshZ80AllRows();
                        }
    
                        // And now read the byte and to the next address!
                        buf[idx] = readZ80Memory((uint16_t)memPtr);
                        memPtr++;
                    }
    
                    // Wrap disk write with two full refresh periods to counter for the amount of time taken to write to disk.
                    refreshZ80AllRows();
                    fr0 = f_write(&File, buf, sizeToWrite, &writeSize);
                    refreshZ80AllRows();
                    saveSize += writeSize;

                    if (fr0 || writeSize < sizeToWrite || saveSize >= size) break;       // error, disk full or range written.
                }
    
                // Restore the control latch to its original configuration.
                //
                setZ80Direction(WRITE);
                writeCtrlLatch(z80Control.runCtrlLatch);
                releaseZ80();
                printf("Saved %ld bytes, final address:%lx\n", saveSize, memPtr);
    
            } else
            {
                printf("Failed to request Z80 access.\n");
            }
        } else
        {
            printf("Failed to write the MZF header.\n");
        }
       
        // Close to sync files.
        f_close(&File);

    } else
    {
        printf("Cannot create file:%s\n", dst);
    }

    return(fr0 ? fr0 : FR_OK);    
}

// Function to dump out a given section of the Z80 memory on the tranZPUter board or mainboard.
//
int memoryDumpZ80(uint32_t memaddr, uint32_t memsize, uint32_t dispaddr, uint8_t dispwidth, uint8_t mainBoard)
{
    uint8_t  data;
    uint8_t  upperAddrBits = 0;
    int8_t   keyIn         = 0;
    uint32_t pnt           = memaddr;
    uint32_t endAddr       = memaddr + memsize;
    uint32_t addr          = dispaddr;
    uint32_t i             = 0;
    char     c             = 0;

    // Sanity checks.
    //
    if((mainBoard == 1 && (memaddr+memsize) > 0x10000) || (mainBoard == 0 && (memaddr+memsize) > 0x80000))
        return(1);

    // Request the correct bus.
    //
    if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        while (1)
        {
            // If the address changes the upper address bits, update the latch to reflect the change.
            if((uint8_t)(pnt >> 16) != upperAddrBits)
            {
                // Write the upper address bits to the 273 control latch to select the next memory model which allows access to the subsequent 64K bank.
                setZ80Direction(WRITE);
                upperAddrBits = (uint8_t)(pnt >> 16);
                writeCtrlLatch(TZMM_TZPU0 + upperAddrBits);
                setZ80Direction(READ);
            }

            // Print the current address.
            printf("%06lX", addr);
            printf(":  ");
    
            // print hexadecimal data
            for (i=0; i < dispwidth; i++)
            {
                if(pnt+i < endAddr)
                {
                    data = readZ80Memory(pnt+i);
                    printf("%02X", data);
                }
                else
                    printf("  ");

                fputc((char)' ', stdout);
            }
    
            // print ascii data
            printf(" |");
    
            // print single ascii char
            for (i=0; i < dispwidth; i++)
            {
                c = (char)readZ80Memory(pnt+i);
                if ((pnt+i < endAddr) && (c >= ' ') && (c <= '~'))
                    fputc((char)c,   stdout);
                else
                    fputc((char)' ', stdout);
            }
    
            puts("|");
    
            // Move on one row.
            pnt  += dispwidth;
            addr += dispwidth;

            // Refresh Z80 DRAM at end of each line.
            //
            refreshZ80();
    
            // User abort (ESC), pause (Space) or all done?
            //
            #if defined __K64F__
                keyIn = usb_serial_getchar();
            #elif defined __ZPU__
                keyIn = getserial_nonblocking();
            #else
                #error "Target CPU not defined, use __ZPU__ or __K64F__"
            #endif
            if(keyIn == ' ')
            {
                do {
                    // Keep refreshing the Z80 DRAM whilst we wait for a key.
                    //
                    refreshZ80();

                    #if defined __K64F__
                        keyIn = usb_serial_getchar();
                    #elif defined __ZPU__
                        keyIn = getserial_nonblocking();
                    #else
                        #error "Target CPU not defined, use __ZPU__ or __K64F__"
                    #endif
                } while(keyIn != ' ' && keyIn != 0x1b);
            }
            // Escape key pressed, exit with 0 to indicate this to caller.
            if (keyIn == 0x1b)
            {
                break;
            }
    
            // End of buffer, exit the loop.
            if(pnt >= (memaddr + memsize))
            {
                break;
            }
        }
       
        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    // Normal exit, return -1, escape exit return 0.
    return(keyIn == 0x1b ? 0 : -1);
}

///////////////////////////////////////////////////////
// Getter/Setter methods to keep z80Control private. //
///////////////////////////////////////////////////////

// Method to test if a reset event has occurred, ie. the user pressed the RESET button.
uint8_t isZ80Reset(void)
{
    // Return the value which would have been updated in the interrupt routine.
    return(z80Control.resetEvent == 1);
}

// Method to test to see if the main memory has been swapped from 0000-0FFF to C000-CFFF
uint8_t isZ80MemorySwapped(void)
{
#if DECODE_Z80_IO >= 2
    // Return the value which would have been updated in the interrupt routine.
    return(z80Control.memorySwap == 1);
#else
    return(0);
#endif
}

// Method to get an IO instruction event should one have occurred since last poll.
//
uint8_t getZ80IO(uint8_t *addr)
{
    // Locals.
    uint8_t retcode = 1;

    if(z80Control.svcRequest == 1)
    {
        *addr = IO_TZ_SVCREQ;
        z80Control.svcRequest = 0;
    } else
    if(z80Control.sysRequest == 1)
    {
        *addr = IO_TZ_SYSREQ;
        z80Control.sysRequest = 0;
    } else
  #if DECODE_Z80_IO >= 1
    if(z80Control.ioEvent == 1)
    {
        z80Control.ioEvent = 0;
        printf("I/O:%2x\n", z80Control.ioAddr);
    } else
  #endif
    {
        retcode = 0;
    }

    return(retcode);
}

// Method to clear a Z80 RESET event.
//
void clearZ80Reset(void)
{
    z80Control.resetEvent = 0;
}

// Method to convert a Sharp filename into an Ascii filename.
//
void convertSharpFilenameToAscii(char *dst, char *src, uint8_t size)
{
    // Loop through and convert each character via the mapping table.
    for(uint8_t idx=0; idx < size; idx++)
    {
        *dst = (char)asciiMap[(int)*src].asciiCode;
        src++;
        dst++;
    }
    // As the Sharp Filename does not always contain a terminator, set a NULL at the end of the string (size+1) so that it can be processed
    // by standard routines. This means dst must always be size+1 in length.
    //
    *dst = 0x00;
    return;
}

//////////////////////////////////////////////////////////////
// tranZPUter i/f methods for zOS - handling and control    //
//                                                          //
//////////////////////////////////////////////////////////////

// Method to load the default ROMS into the tranZPUter RAM ready for start.
// If the autoboot flag is set, perform autoboot by wiping the STACK area
// of the SA1510 - this has the effect of JP 00000H.
//
void loadTranZPUterDefaultROMS(void)
{
    // Locals.
    FRESULT  result;

    // Start off by clearing active memory banks, the AS6C4008 chip holds random values at power on.
    fillZ80Memory(0x000000, 0x10000, 0x00, 0); // TZFS and Sharp MZ80A mode.
    fillZ80Memory(0x040000, 0x20000, 0x00, 0); // CPM Mode.

    // Now load the necessary images into memory.
    if((result=loadZ80Memory((const char *)MZ_ROM_SA1510_40C, 0, MZ_MROM_ADDR, 0, 0, 1)) != FR_OK)
    {
        printf("Error: Failed to load %s into tranZPUter memory.\n", MZ_ROM_SA1510_40C);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0,      MZ_UROM_ADDR,            0x1800, 0, 1) != FR_OK))
    {
        printf("Error: Failed to load bank 1 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x1800, MZ_BANKRAM_ADDR+0x10000, 0x1000, 0, 1) != FR_OK))
    {
        printf("Error: Failed to load page 2 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x2800, MZ_BANKRAM_ADDR+0x20000, 0x1000, 0, 1) != FR_OK))
    {
        printf("Error: Failed to load page 3 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x3800, MZ_BANKRAM_ADDR+0x30000, 0x1000, 0, 1) != FR_OK))
    {
        printf("Error: Failed to load page 4 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }

    // If all was ok loading the roms, complete the startup.
    //
    if(!result)
    {
        // Set the memory model to BOOT so we can bootstrap TZFS.
        setCtrlLatch(TZMM_BOOT);

        // If autoboot flag set, force a restart to the ROM which will call User ROM startup code.
        if(osControl.tzAutoBoot)
        {
            delay(100);
            fillZ80Memory(MZ_MROM_STACK_ADDR, MZ_MROM_STACK_SIZE, 0x00, 1);
        }

        // No longer need refresh on the mainboard as all operations are in static RAM.
        z80Control.disableRefresh = 1;
    }
    return;
}

// Method to set the service status flag on the Z80 (and duplicated in the internal
// copy).
//
uint8_t setZ80SvcStatus(uint8_t status)
{
    // Locals
    uint8_t result = 1;

    // Request the tranZPUter bus.
    //
    if(reqTranZPUterBus(100) == 0)
    {
        // Setup the pins to perform a write operation.
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // Update the memory location.
        //
        result=writeZ80Memory(z80Control.svcControlAddr+TZSVC_RESULT_OFFSET, status);

        // Release the Z80.
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();

        // Update in-memory copy of the result variable.
        svcControl.result = status;
    } else
    {
        result = 1;
    }
    return(result);
}

// Simple method to set defaults in the service structure if not already set by the Z80.
//
void svcSetDefaults(void)
{
    // If there is no directory path, use the inbuilt default.
    if(svcControl.directory[0] == '\0')
    {
        strcpy((char *)svcControl.directory, TZSVC_DEFAULT_DIR);
    }
   
    // If there is no wildcard matching, use default.
    if(svcControl.wildcard[0] == '\0')
    {
        strcpy((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD);
    }
}

// Helper method for matchFileWithWildcard.
// Get the next character from the filename or pattern and modify it if necessary to match the Sharp character set.
static uint32_t getNextChar(const char** ptr)
{
    uint8_t chr;

    // Get a byte
    chr = (uint8_t)*(*ptr)++;

    // To upper ASCII char
    if(islower(chr)) chr -= 0x20;

    return chr;
}

// Match an MZF name with a given wildcard.
// This method originated from the private method in FatFS but adapted to work with MZF filename matching.
// Input:  wildcard    - Pattern to match
//         fileName    - MZF fileName, either CR or NUL terminated or MZF_FILENAME_LEN chars long.
//         skip        - Number of characters to skip due to ?'s
//         infinite    - Infinite search as * specified.
// Output: 0           - No match
//         1           - Match
//
static int matchFileWithWildcard(const char *pattern, const char *fileName, int skip, int infinite)
{
    const char *pp, *np;
    uint32_t   pc, nc;
    int        nm, nx;

    // Pre-skip name chars
    while (skip--)
    {
        // Branch mismatched if less name chars
        if (!getNextChar(&fileName)) return 0;
    }
    // (short circuit)
    if (*pattern == 0 && infinite) return 1;

    do {
        // Top of pattern and name to match
        pp = pattern; np = fileName;
        for (;;)
        {
            // Wildcard?
            if (*pp == '?' || *pp == '*')
            {
                nm = nx = 0;

                // Analyze the wildcard block
                do {
                    if (*pp++ == '?') nm++; else nx = 1;
                } while (*pp == '?' || *pp == '*');

                // Test new branch (recurs upto number of wildcard blocks in the pattern)
                if (matchFileWithWildcard(pp, np, nm, nx)) return 1;

                // Branch mismatched
                nc = *np; break;
            }
         
            // End of filename, Sharp filenames can be terminated with 0x00, CR or size. If we get to the end of the name then it is 
            // a match.
            //
            if((np - fileName) == MZF_FILENAME_LEN) return 1;

            // Get a pattern char 
            pc = getNextChar(&pp);

            // Get a name char 
            nc = getNextChar(&np);

            // Sharp uses null or CR to terminate a pattern and a filename, which is not determinate!
            if((pc == 0x00 || pc == 0x0d) && (nc == 0x00 || nc == 0x0d)) return 1;

            // Branch mismatched?
            if (pc != nc) break;

            // Branch matched? (matched at end of both strings)
            if (pc == 0) return 1;
        }

        // fileName++
        getNextChar(&fileName);

    /* Retry until end of name if infinite search is specified */
    } while (infinite && nc != 0x00 && nc != 0x0d && (np - fileName) < MZF_FILENAME_LEN);

    return 0;
}

// Method to open/read a directory listing. 
// This method opens a given directory on the SD card (the z80 provides the directory name or it defaults to MZF). The directory
// is read and a unique incrementing number is given to each entry, this number can be used in a later request to open the file to save on 
// name entry and matching.
// A basic pattern filter is applied to the results returned, ie A* will return only files starting with A.
// 
// The parameters are passed in the svcControl block.
//
uint8_t svcReadDir(uint8_t mode)
{
    // Locals - dont use global as this is a seperate thread.
    //
    static DIR        dirFp;
    static uint8_t    dirOpen   = 0;          // Seperate flag as their is no public way to validate that dirFp is open and valid, the method in FatFS is private for this functionality.
    static FILINFO    fno;
    static uint8_t    dirSector = 0;          // Virtual directory sector.
    FRESULT           result    = FR_OK;
    unsigned int      readSize;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    FIL               File;
    t_svcCmpDirBlock  *dirBlock = (t_svcCmpDirBlock *)&svcControl.sector;   

    // Request to open? Validate that we dont already have an open directory then open the requested one.
    if(mode == TZSVC_OPEN)
    {
        // Close if previously open.
        if(dirOpen == 1)
            svcReadDir(TZSVC_CLOSE);

        // Setup the defaults
        //
        svcSetDefaults();

        // Open the directory.
        result = f_opendir(&dirFp, (char *)&svcControl.directory);
        if(result == FR_OK)
        {
            // Reentrant call to actually read the data.
            //
            dirOpen   = 1; 
            dirSector = 0;
            result    = (FRESULT)svcReadDir(TZSVC_NEXT);
        }
    }

    // Read a block of directory entries into the z80 service buffer sector.
    else if(mode == TZSVC_NEXT && dirOpen == 1)
    {
        // If the Z80 is requesting a non sequential directory sector then we have to start from the beginning as each sector is built up not read.
        //
        if(dirSector != svcControl.dirSector)
        {
            // If the current sector is after the requested sector, rewind by re-opening.
            //
            if(dirSector < svcControl.dirSector)
            {
                result=svcReadDir(TZSVC_OPEN);
            }
            if(!result)
            {
                // Now get the sector by advancement.
                for(uint8_t idx=dirSector; idx < svcControl.dirSector && result == FR_OK; idx++)
                {
                    result=svcReadDir(TZSVC_NEXT);
                }
            }
        }

        // Proceed if no errors have occurred.
        //
        if(!result)
        {
            // Zero the directory entry block - unused entries will then appear as NULLS.
            memset(dirBlock, 0x00, TZSVC_SECTOR_SIZE);
    
            // Loop the required number of times to fill a sector full of entries.
            //
            uint8_t idx=0;
            while(idx < TZVC_MAX_CMPCT_DIRENT_BLOCK && result == FR_OK)
            {
                // Read an SD directory entry then open the returned SD file so that we can to read out the important MZF data which is stored in it as it will be passed to the Z80.
                //
                result = f_readdir(&dirFp, &fno);

                // If an error occurs or we are at the end of the directory listing close the sector and pass back.
                if(result != FR_OK || fno.fname[0] == 0) break;
            
                // Check to see if this is a valid MZF file.
                const char *ext = strrchr(fno.fname, '.');
                if(!ext || strcasecmp(++ext, TZSVC_DEFAULT_EXT) != 0)
                    continue;
    
                // Build filename.
                //
                sprintf(fqfn, "0:\\%s\\%s", svcControl.directory, fno.fname);
    
                // Open the file so we can read out the MZF header which is the information TZFS/CPM needs.
                //
                result = f_open(&File, fqfn, FA_OPEN_EXISTING | FA_READ);
    
                // If no error occurred, read in the header.
                //
                if(!result) result = f_read(&File, (char *)&dirBlock->dirEnt[idx], TZSVC_CMPHDR_SIZE, &readSize);
    
                // No errors, read the header.
                if(!result && readSize == TZSVC_CMPHDR_SIZE)
                {
                    // Close the file, no longer needed.
                    f_close(&File);
    
                    // Check to see if the file matches any given wildcard.
                    //
                    if(matchFileWithWildcard((char *)&svcControl.wildcard, (char *)&dirBlock->dirEnt[idx].fileName, 0, 0))
                    {
                        // Valid so find next entry.
                        idx++;
                    } else
                    {
                        // Scrub the entry, not valid.
                        memset((char *)&dirBlock->dirEnt[idx], 0x00, TZSVC_CMPHDR_SIZE);
                    }
                }
            }
        }

        // Increment the virtual directory sector number as the Z80 expects a sector of directory entries per call.
        if(!result)
            dirSector++;
    }
    // Close the currently open directory.
    else if(mode == TZSVC_CLOSE)
    {
        if(dirOpen)
            f_closedir(&dirFp);
        dirOpen = 0;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}


// A method to find a file either using the Sharp MZ80A name or a number assigned to a directory listing.
// It is a bit long winded as each file that matches the filename specification has to be opened and the MZF header filename 
// has to be checked. Cacheing would help here but wasteful in resources for number of times it would be called.
//
uint8_t svcFindFile(char *file, char *searchFile, uint8_t searchNo)
{
    // Locals
    uint8_t        fileNo    = 0;
    uint8_t        found     = 0;
    unsigned int   readSize;
    char           fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    FIL            File;
    FILINFO        fno;
    DIR            dirFp;
    FRESULT        result    = FR_OK;
    t_svcCmpDirEnt dirEnt;

    // Setup the defaults
    //
    svcSetDefaults();

    // Open the directory.
    result = f_opendir(&dirFp, (char *)&svcControl.directory);
    if(result == FR_OK)
    {
        fileNo = 0;
        
        do {
            // Read an SD directory entry then open the returned SD file so that we can to read out the important MZF data for name matching.
            //
            result = f_readdir(&dirFp, &fno);

            // If an error occurs or we are at the end of the directory listing close the sector and pass back.
            if(result != FR_OK || fno.fname[0] == 0) break;

            // Check to see if this is a valid MZF file.
            const char *ext = strrchr(fno.fname, '.');
            if(!ext || strcasecmp(++ext, TZSVC_DEFAULT_EXT) != 0)
                continue;

            // Build filename.
            //
            sprintf(fqfn, "0:\\%s\\%s", svcControl.directory, fno.fname);

            // Open the file so we can read out the MZF header which is the information TZFS/CPM needs.
            //
            result = f_open(&File, fqfn, FA_OPEN_EXISTING | FA_READ);

            // If no error occurred, read in the header.
            //
            if(!result) result = f_read(&File, (char *)&dirEnt, TZSVC_CMPHDR_SIZE, &readSize);

            // No errors, read the header.
            if(!result && readSize == TZSVC_CMPHDR_SIZE)
            {
                // Close the file, no longer needed.
                f_close(&File);
                
                // Check to see if the file matches any given wildcard. If we dont have a match loop to next directory entry.
                //
                if(matchFileWithWildcard((char *)&svcControl.wildcard, (char *)&dirEnt.fileName, 0, 0))
                {
                    // If a filename has been given, see if this file matches it.
                    if(searchFile != NULL)
                    {
                        // Check to see if the file matches the name given with wildcard expansion if needed.
                        //
                        if(matchFileWithWildcard(searchFile, (char *)&dirEnt.fileName, 0, 0))
                        {
                            found = 2;
                        }
                    }
          
                    // If we are searching on file number and the latest directory entry retrieval matches, exit and return the filename.
                    if(searchNo != 0xFF && fileNo == (uint8_t)searchNo)
                    {
                        found = 1;
                    } else
                    {
                        fileNo++;
                    }
                }
            }
        } while(!result && !found);

        // If the file was found, copy the FQFN into its buffer.
        //
        if(found)
        {
            strcpy(file, fqfn);
        }
    }

    // Return 1 if file was found, 0 for all other cases.
    //
    return(result == FR_OK ? (found == 0 ? 0 : 1) : 0);
}

// Method to read the current directory from the cache. If the cache is invalid resort to using the standard direct method.
//
uint8_t svcReadDirCache(uint8_t mode)
{
    // Locals - dont use global as this is a seperate thread.
    //
    static uint8_t    dirOpen     = 0;          // Seperate flag as their is no public way to validate that dirFp is open and valid, the method in FatFS is private for this functionality.
    static uint8_t    dirSector   = 0;          // Virtual directory sector.
    static uint8_t    dirEntry    = 0;          // Last cache entry number processed.
    FRESULT           result      = FR_OK;
    t_svcCmpDirBlock  *dirBlock = (t_svcCmpDirBlock *)&svcControl.sector;   

    // Setup the defaults
    //
    svcSetDefaults();

    // If there is no cache revert to direct directory read.
    //
    if(!osControl.dirMap.valid)
    {
        result = svcReadDir(mode);
    } else
    {
        // Request to open? No need with cache, just return next block.
        if(mode == TZSVC_OPEN)
        {
            // Reentrant call to actually read the data.
            //
            dirOpen   = 1; 
            dirSector = 0;
            dirEntry  = 0;
            result    = (FRESULT)svcReadDirCache(TZSVC_NEXT);
        }
    
        // Read a block of directory entries into the z80 service buffer sector.
        else if(mode == TZSVC_NEXT && dirOpen == 1)
        {
            // If the Z80 is requesting a non sequential directory sector then calculate the new cache entry position.
            //
            if(dirSector != svcControl.dirSector)
            {
                dirEntry = svcControl.dirSector * TZVC_MAX_CMPCT_DIRENT_BLOCK;
                dirSector = svcControl.dirSector;
                if(dirEntry > osControl.dirMap.entries)
                {
                    dirEntry = osControl.dirMap.entries;
                    dirSector = osControl.dirMap.entries / TZVC_MAX_CMPCT_DIRENT_BLOCK;
                }
            }
    
            // Zero the directory entry block - unused entries will then appear as NULLS.
            memset(dirBlock, 0x00, TZSVC_SECTOR_SIZE);
    
            // Loop the required number of times to fill a sector full of entries.
            //
            uint8_t idx=0;
            while(idx < TZVC_MAX_CMPCT_DIRENT_BLOCK && dirEntry < osControl.dirMap.entries && result == FR_OK)
            {
                // Check to see if the file matches any given wildcard.
                //
                if(matchFileWithWildcard((char *)&svcControl.wildcard, (char *)&osControl.dirMap.file[dirEntry]->mzfHeader.fileName, 0, 0))
                {
                    // Valid so store and find next entry.
                    //
                    memcpy((char *)&dirBlock->dirEnt[idx], (char *)&osControl.dirMap.file[dirEntry]->mzfHeader, TZSVC_CMPHDR_SIZE);
                    idx++;
                } else
                {
                    // Scrub the entry, not valid.
                    memset((char *)&dirBlock->dirEnt[idx], 0x00, TZSVC_CMPHDR_SIZE);
                }
                dirEntry++;
            }

            // Increment the virtual directory sector number as the Z80 expects a sector of directory entries per call.
            if(!result)
                dirSector++;
        }
        // Close the currently open directory.
        else if(mode == TZSVC_CLOSE)
        {
            dirOpen = 0;
        }
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// A method to find a file using the cached directory. If the cache is not available (ie. no memory) use the standard method.
//
uint8_t svcFindFileCache(char *file, char *searchFile, uint8_t searchNo)
{
    // Locals
    uint8_t        fileNo    = 0;
    uint8_t        found     = 0;
    uint8_t        idx       = 0;
    FRESULT        result    = FR_OK;

    // If there is no cache revert to direct search.
    //
    if(!osControl.dirMap.valid)
    {
        result = svcFindFile(file, searchFile, searchNo);
    } else
    {
        // If we are searching on file number and there is no filter in place, see if it is valid and exit with data.
        if(searchNo != 0xFF && strcmp((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD) == 0)
        {
            if(searchNo < osControl.dirMap.entries && osControl.dirMap.file[searchNo])
            {
                found = 1;
                idx   = searchNo;
            } else
            {
                result = FR_NO_FILE;
            }
        } else
        {
            do {
                // Check to see if the file matches any given wildcard. If we dont have a match loop to next directory entry.
                //
                if(matchFileWithWildcard((char *)&svcControl.wildcard, (char *)&osControl.dirMap.file[idx]->mzfHeader.fileName, 0, 0))
                {
                    // If a filename has been given, see if this file matches it.
                    if(searchFile != NULL)
                    {
                        // Check to see if the file matches the name given with wildcard expansion if needed.
                        //
                        if(matchFileWithWildcard(searchFile, (char *)&osControl.dirMap.file[idx]->mzfHeader.fileName, 0, 0))
                        {
                            found = 2;
                        }
                    }

                    // If we are searching on file number then see if it matches (after filter has been applied above).
                    if(searchNo != 0xFF && fileNo == (uint8_t)searchNo)
                    {
                        found = 1;
                    } else
                    {
                        fileNo++;
                    }
                }
                if(!found)
                {
                    idx++;
                }
            } while(!result && !found && idx < osControl.dirMap.entries);
        }

        // If the file was found, copy the FQFN into its buffer.
        //
        if(found)
        {
            // Build filename.
            //
            sprintf(file, "0:\\%s\\%s", osControl.dirMap.directory, osControl.dirMap.file[idx]->sdFileName);
        }
    }

    // Return 1 if file was found, 0 for all other cases.
    //
    return(result == FR_OK ? (found == 0 ? 0 : 1) : 0);
}

// Method to build up a cache of all the files on the SD card in a given directory along with the Sharp MZ80A header to which they map.
// This involves scanning each file to extract the MZF header and creating a map.
//
uint8_t svcCacheDir(const char *directory, uint8_t force)
{
    // Locals
    uint8_t        fileNo    = 0;
    unsigned int   readSize;
    char           fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    FIL            File;
    FILINFO        fno;
    DIR            dirFp;
    FRESULT        result    = FR_OK;
    t_svcCmpDirEnt dirEnt;

    // No need to cache directory if we have already cached it.
    if(force == 0 && osControl.dirMap.valid && strcasecmp(directory, osControl.dirMap.directory) == 0)
        return(1);

    // Invalidate the map and free existing memory incase of errors.
    //
    osControl.dirMap.valid = 0;
    for(uint8_t idx=0; idx < osControl.dirMap.entries; idx++)
    {
        if(osControl.dirMap.file[idx])
        {
            free(osControl.dirMap.file[idx]->sdFileName);
            free(osControl.dirMap.file[idx]);
            osControl.dirMap.file[idx] = 0;
        }
    }
    osControl.dirMap.entries = 0;

    // Open the directory and extract all files.
    result = f_opendir(&dirFp, directory);
    if(result == FR_OK)
    {
        fileNo = 0;
        
        do {
            // Read an SD directory entry then open the returned SD file so that we can to read out the important MZF data for name matching.
            //
            result = f_readdir(&dirFp, &fno);

            // If an error occurs or we are at the end of the directory listing close the sector and pass back.
            if(result != FR_OK || fno.fname[0] == 0) break;

            // Check to see if this is a valid MZF file.
            const char *ext = strrchr(fno.fname, '.');
            if(!ext || strcasecmp(++ext, TZSVC_DEFAULT_EXT) != 0)
                continue;

            // Build filename.
            //
            sprintf(fqfn, "0:\\%s\\%s", directory, fno.fname);

            // Open the file so we can read out the MZF header which is the information TZFS/CPM needs.
            //
            result = f_open(&File, fqfn, FA_OPEN_EXISTING | FA_READ);

            // If no error occurred, read in the header.
            //
            if(!result) result = f_read(&File, (char *)&dirEnt, TZSVC_CMPHDR_SIZE, &readSize);

            // No errors, read the header.
            if(!result && readSize == TZSVC_CMPHDR_SIZE)
            {
                // Close the file, no longer needed.
                f_close(&File);

                // Cache this entry. The SD filename is dynamically allocated as it's size can be upto 255 characters for LFN names. The Sharp name is 
                // fixed at 17 characters as you cant reliably rely on terminators and the additional data makes it a constant 32 chars long.
                osControl.dirMap.file[fileNo] = (t_sharpToSDMap *)malloc(sizeof(t_sharpToSDMap));
                osControl.dirMap.file[fileNo]->sdFileName = (uint8_t *)malloc(strlen(fno.fname)+1);

                if(osControl.dirMap.file[fileNo] == NULL || osControl.dirMap.file[fileNo]->sdFileName == NULL)
                {
                    printf("Out of memory cacheing directory:%s\n", directory);
                    for(uint8_t idx=0; idx <= fileNo; idx++)
                    {
                        if(osControl.dirMap.file[idx])
                        {
                            free(osControl.dirMap.file[idx]->sdFileName);
                            free(osControl.dirMap.file[idx]);
                            osControl.dirMap.file[idx] = 0;
                        }
                    }
                    result = FR_NOT_ENOUGH_CORE;
                } else
                {
                    // Copy in details into this maps node.
                    strcpy((char *)osControl.dirMap.file[fileNo]->sdFileName, fno.fname);
                    memcpy((char *)&osControl.dirMap.file[fileNo]->mzfHeader, (char *)&dirEnt, TZSVC_CMPHDR_SIZE);
                    fileNo++;
                }
            }
        } while(!result && fileNo < TZSVC_MAX_DIR_ENTRIES);
    }

    // Success?
    if(result == FR_OK && (fno.fname[0] == 0 || fileNo == TZSVC_MAX_DIR_ENTRIES))
    {
        // Validate the cache.
        osControl.dirMap.valid = 1;
        osControl.dirMap.entries = fileNo;
        strcpy(osControl.dirMap.directory, directory);
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to open a file for reading and return requested sectors.
//
uint8_t svcReadFile(uint8_t mode)
{
    // Locals - dont use global as this is a seperate thread.
    //
    static FIL        File;
    static uint8_t    fileOpen   = 0;         // Seperate flag as their is no public way to validate that File is open and valid, the method in FatFS is private for this functionality.
    static uint8_t    fileSector = 0;         // Sector number being read.
    FRESULT           result    = FR_OK;
    unsigned int      readSize;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Find the required file.
    // Request to open? Validate that we dont already have an open file then find and open the file.
    if(mode == TZSVC_OPEN)
    {
        // Close if previously open.
        if(fileOpen == 1)
            svcReadFile(TZSVC_CLOSE);

        // Setup the defaults
        //
        svcSetDefaults();

        // Find the file using the given file number or file name.
        //
        if(svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo))
        {
            // Open the file, fqfn has the FQFN of the correct file on the SD drive.
            result = f_open(&File, fqfn, FA_OPEN_EXISTING | FA_READ);

            if(result == FR_OK)
            {
                // Reentrant call to actually read the data.
                //
                fileOpen   = 1; 
                fileSector = 0;
                result     = (FRESULT)svcReadFile(TZSVC_NEXT);
            }
        }
    }
  
    // Read the next sector from the file.
    else if(mode == TZSVC_NEXT && fileOpen == 1)
    {
        // If the Z80 is requesting a non sequential sector then seek to the correct location prior to the read.
        //
        if(fileSector != svcControl.fileSector)
        {
            result = f_lseek(&File, (svcControl.fileSector * TZSVC_SECTOR_SIZE));
            fileSector = svcControl.fileSector;
        }

        // Proceed if no errors have occurred.
        //
        if(!result)
        {
            // Read the required sector.
            result = f_read(&File, (char *)&svcControl.sector, TZSVC_SECTOR_SIZE, &readSize);
        }

        // Move onto next sector.
        fileSector++;
    }
   
    // Close the currently open file.
    else if(mode == TZSVC_CLOSE)
    {
        if(fileOpen)
            f_close(&File);
        fileOpen = 0;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to load a file from SD directly into the tranZPUter memory.
//
uint8_t svcLoadFile(void)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Setup the defaults
    //
    svcSetDefaults();

    // Find the file using the given file number or file name.
    //
    if(svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo))
    {
        // Call method to load an MZF file.
        result = loadMZFZ80Memory(fqfn, 0xFFFFFFFF, 0, 1);

        // Store the filename, used in reload or immediate saves.
        //
        osControl.lastFile = (uint8_t *)realloc(osControl.lastFile, strlen(fqfn)+1);
        if(osControl.lastFile == NULL)
        {
            printf("Out of memory saving last file name, dependent applications (ie. CP/M) wont work!\n");
            result = FR_NOT_ENOUGH_CORE;
        } else
        {
            strcpy((char *)osControl.lastFile, fqfn);
        }
    } else
    {
        result = FR_NO_FILE;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to save a file from tranZPUter memory directly into a file on the SD card.
//
uint8_t svcSaveFile(void)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    char              asciiFileName[MZF_FILENAME_LEN+1];
    t_svcDirEnt       mzfHeader;

    // Setup the defaults
    //
    svcSetDefaults();

    // Get the MZF header which contains the details of the file to save.
    copyFromZ80((uint8_t *)&mzfHeader, MZ_CMT_ADDR, MZF_HEADER_SIZE, 0);

    // Need to extract and convert the filename to create a file.
    //
    convertSharpFilenameToAscii(asciiFileName, (char *)mzfHeader.fileName, MZF_FILENAME_LEN);

    // Build filename.
    //
    sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, asciiFileName, TZSVC_DEFAULT_EXT);

    // Call the main method to save memory passing in the correct MZF details and header.
    result = saveZ80Memory(fqfn, (mzfHeader.loadAddr < MZ_CMT_DEFAULT_LOAD_ADDR-3 ? MZ_CMT_DEFAULT_LOAD_ADDR : mzfHeader.loadAddr), mzfHeader.fileSize, &mzfHeader, 0);

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}


// Method to erase a file on the SD card.
//
uint8_t svcEraseFile(void)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Setup the defaults
    //
    svcSetDefaults();

    // Find the file using the given file number or file name.
    //
    if(svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo))
    {
        // Call method to load an MZF file.
        result = f_unlink(fqfn);
    } else
    {
        result = FR_NO_FILE;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to add a SD disk file as a CP/M disk drive for read/write by CP/M.
//
uint8_t svcAddCPMDrive(void)
{
    // Locals
    char           fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    FRESULT        result    = FR_OK;
    // Sanity checks.
    //
    if(svcControl.fileNo >= CPM_MAX_DRIVES)
        return(TZSVC_STATUS_FILE_ERROR);

    // Disk already allocated? May be a reboot or drive reassignment so free up the memory to reallocate.
    //
    if(osControl.cpmDriveMap.drive[svcControl.fileNo] != NULL)
    {
        if(osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName != NULL)
        {
            free(osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName);
            osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName = 0;
        }
        free(osControl.cpmDriveMap.drive[svcControl.fileNo]);
        osControl.cpmDriveMap.drive[svcControl.fileNo] = 0;
    }

    // Build filename for the drive.
    //
    sprintf(fqfn, CPM_DRIVE_TMPL, svcControl.fileNo);
    osControl.cpmDriveMap.drive[svcControl.fileNo] = (t_cpmDrive *)malloc(sizeof(t_cpmDrive));
    if(osControl.cpmDriveMap.drive[svcControl.fileNo] == NULL)
    {
        printf("Out of memory adding CP/M drive:%s\n", fqfn);
        result = FR_NOT_ENOUGH_CORE;
    } else
    {
        osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName = (uint8_t *)malloc(strlen(fqfn)+1);
        if(osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName == NULL)
        {
            printf("Out of memory adding filename to CP/M drive:%s\n", fqfn);
            result = FR_NOT_ENOUGH_CORE;
        } else
        {
            strcpy((char *)osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName, fqfn);
          
            // Open the file to verify it exists and is valid, also to assign the file handle.
            //
            result = f_open(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File, (char *)osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);

            // If no error occurred, read in the header.
            //
            if(!result)
            {
                osControl.cpmDriveMap.drive[svcControl.fileNo]->lastTrack = 0;
                osControl.cpmDriveMap.drive[svcControl.fileNo]->lastSector = 0;
            } else
            {
                // Error opening file so free up and release slot, return error.
                free(osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName);
                osControl.cpmDriveMap.drive[svcControl.fileNo]->fileName = 0;
                free(osControl.cpmDriveMap.drive[svcControl.fileNo]);
                osControl.cpmDriveMap.drive[svcControl.fileNo] = 0;
                result = FR_NOT_ENOUGH_CORE;
            }
        }
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to read one of the opened, attached CPM drive images according to the Track and Sector provided.
// Inputs:
//     svcControl.trackNo  = Track to read from.
//     svcControl.sectorNo = Sector to read from.
//     svcControl.fileNo   = CPM virtual disk number, which should have been attached with the svcAddCPMDrive method.
// Outputs:
//     svcControl.sector   = 512 bytes read from file.
//
uint8_t svcReadCPMDrive(void)
{
    // Locals.
    FRESULT           result    = FR_OK;
    uint32_t          fileOffset;
    unsigned int      readSize;

    // Sanity checks.
    //
    if(svcControl.fileNo >= CPM_MAX_DRIVES || osControl.cpmDriveMap.drive[svcControl.fileNo] == NULL)
    {
        printf("svcReadCPMDrive: Illegal input values: fileNo=%d, driveMap=%08lx\n", svcControl.fileNo,  (uint32_t)osControl.cpmDriveMap.drive[svcControl.fileNo]);
        return(TZSVC_STATUS_FILE_ERROR);
    }

    // Calculate the offset into the file.
    fileOffset = ((svcControl.trackNo * CPM_SECTORS_PER_TRACK) + svcControl.sectorNo) * SECTOR_SIZE;

    // Seek to the correct location as directed by the track/sector.
    result = f_lseek(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File, fileOffset);
    if(!result)
        result = f_read(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File, (char *)svcControl.sector, SECTOR_SIZE, &readSize);
   
    // No errors but insufficient bytes read, either the image is bad or there was an error!
    if(!result && readSize != SECTOR_SIZE)
    {
        result = FR_DISK_ERR;
    } else
    {
        osControl.cpmDriveMap.drive[svcControl.fileNo]->lastTrack  = svcControl.trackNo;
        osControl.cpmDriveMap.drive[svcControl.fileNo]->lastSector = svcControl.sectorNo;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to write to one of the opened, attached CPM drive images according to the Track and Sector provided.
// Inputs:
//     svcControl.trackNo  = Track to write into.
//     svcControl.sectorNo = Sector to write into.
//     svcControl.fileNo   = CPM virtual disk number, which should have been attached with the svcAddCPMDrive method.
//     svcControl.sector   = 512 bytes to write into the file.
// Outputs:
//
uint8_t svcWriteCPMDrive(void)
{
    // Locals.
    FRESULT           result    = FR_OK;
    uint32_t          fileOffset;
    unsigned int      writeSize;

    // Sanity checks.
    //
    if(svcControl.fileNo >= CPM_MAX_DRIVES || osControl.cpmDriveMap.drive[svcControl.fileNo] == NULL)
    {
        printf("svcWriteCPMDrive: Illegal input values: fileNo=%d, driveMap=%08lx\n", svcControl.fileNo,  (uint32_t)osControl.cpmDriveMap.drive[svcControl.fileNo]);
        return(TZSVC_STATUS_FILE_ERROR);
    }

    // Calculate the offset into the file.
    fileOffset = ((svcControl.trackNo * CPM_SECTORS_PER_TRACK) + svcControl.sectorNo) * SECTOR_SIZE;

    // Seek to the correct location as directed by the track/sector.
    result = f_lseek(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File, fileOffset);
    if(!result)
    {
        printf("Writing offset=%08lx\n", fileOffset);
        for(uint16_t idx=0; idx < SECTOR_SIZE; idx++)
        {
            printf("%02x ", svcControl.sector[idx]);
            if(idx % 32 == 0)
                printf("\n");
        }
        printf("\n");

        result = f_write(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File, (char *)svcControl.sector, SECTOR_SIZE, &writeSize);
        if(!result)
        {
            f_sync(&osControl.cpmDriveMap.drive[svcControl.fileNo]->File);
        }
    }
  
    // No errors but insufficient bytes written, either the image is bad or there was an error!
    if(!result && writeSize != SECTOR_SIZE)
    {
        result = FR_DISK_ERR;
    } else
    {
        osControl.cpmDriveMap.drive[svcControl.fileNo]->lastTrack  = svcControl.trackNo;
        osControl.cpmDriveMap.drive[svcControl.fileNo]->lastSector = svcControl.sectorNo;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Simple method to get the service record address which is dependent upon memory mode which in turn is dependent upon software being run.
//
uint32_t getServiceAddr(void)
{
    // Locals.
    uint32_t addr       = TZSVC_CMD_STRUCT_ADDR_TZFS;
    uint8_t  memoryMode = readCtrlLatch();

    // Currently only CPM has a different service record address.
    if(memoryMode == TZMM_CPM || memoryMode == TZMM_CPM2)
        addr = TZSVC_CMD_STRUCT_ADDR_CPM;

    return(addr);
}

// Method to process a service request from the z80 running TZFS or CPM.
//
void processServiceRequest(void)
{
    // Locals.
    //
    uint8_t    refreshCacheDir = 0;
    uint8_t    status          = 0;
    uint32_t   copySize        = TZSVC_CMD_STRUCT_SIZE;

    // Update the service control record address according to memory mode.
    //
    z80Control.svcControlAddr = getServiceAddr();

    // Get the command and associated parameters.
    copyFromZ80((uint8_t *)&svcControl, z80Control.svcControlAddr, TZSVC_CMD_SIZE, 0);

    // Need to get the remainder of the data for the write operations.
    if(svcControl.cmd == TZSVC_CMD_WRITESDDRIVE)
    {
        copyFromZ80((uint8_t *)&svcControl.sector, z80Control.svcControlAddr+TZSVC_CMD_SIZE, TZSVC_SECTOR_SIZE, 0);
    }

    // Check this is a valid request.
    if(svcControl.result != TZSVC_STATUS_REQUEST)
        return;

    // Set status to processing. Z80 can use this to decide if the K64F received its request after a given period of time.
    setZ80SvcStatus(TZSVC_STATUS_PROCESSING);

    // Action according to command given.
    //
    switch(svcControl.cmd)
    {
        // Open a directory stream and return the first block.
        case TZSVC_CMD_READDIR:
            status=svcReadDirCache(TZSVC_OPEN);
            break;

        // Read the next block in the directory stream.
        case TZSVC_CMD_NEXTDIR:
            status=svcReadDirCache(TZSVC_NEXT);
            break;

        // Open a file stream and return the first block.
        case TZSVC_CMD_READFILE:
            status=svcReadDir(TZSVC_OPEN);
            break;

        // Read the next block in the file stream.
        case TZSVC_CMD_MEXTREADFILE:
            status=svcReadFile(TZSVC_NEXT);
            break;

        // Close an open dir/file.
        case TZSVC_CMD_CLOSE:
            svcReadDir(TZSVC_CLOSE);
            svcReadFile(TZSVC_CLOSE);
           
            // Only need to copy the command section back to the Z80 for a close operation.
            //
            copySize = TZSVC_CMD_SIZE;
            break;

        // Load a file directly into target memory.
        case TZSVC_CMD_LOADFILE:
            status=svcLoadFile();
            break;
        
        // Save a file directly from target memory.
        case TZSVC_CMD_SAVEFILE:
            status=svcSaveFile();
            refreshCacheDir = 1;
            break;
          
        // Erase a file from the SD Card.
        case TZSVC_CMD_ERASEFILE:
            status=svcEraseFile();
            refreshCacheDir = 1;
            break;

        // Change active directory. Do this immediately to validate the directory name given.
        case TZSVC_CMD_CHANGEDIR:
            status=svcCacheDir((const char *)svcControl.directory, 0);
            break;

        // Load the 40 column version of the SA1510 bios into memory.
        case TZSVC_CMD_LOAD40BIOS:
            if((status=loadZ80Memory((const char *)MZ_ROM_SA1510_40C, 0, MZ_MROM_ADDR, 0, 0, 1)) != FR_OK)
            {
                printf("Error: Failed to load %s into tranZPUter memory.\n", MZ_ROM_SA1510_40C);
            }
          
            // Change frequency to default.
            setZ80CPUFrequency(MZ_80A_CPU_FREQ, 2);
            break;
          
        // Load the 80 column version of the SA1510 bios into memory.
        case TZSVC_CMD_LOAD80BIOS:
            if((status=loadZ80Memory((const char *)MZ_ROM_SA1510_80C, 0, MZ_MROM_ADDR, 0, 0, 1)) != FR_OK)
            {
                printf("Error: Failed to load %s into tranZPUter memory.\n", MZ_ROM_SA1510_80C);
            }
           
            // Change frequency to default.
            setZ80CPUFrequency(MZ_80A_CPU_FREQ, 2);
            break;
           
        // Load the 40 column MZ700 1Z-013A bios into memory.
        case TZSVC_CMD_LOAD700BIOS40:
            if((status=loadZ80Memory((const char *)MZ_ROM_1Z_013A_40C, 0, MZ_MROM_ADDR, 0, 0, 1)) != FR_OK)
            {
                printf("Error: Failed to load %s into tranZPUter memory.\n", MZ_ROM_1Z_013A_40C);
            }

            // Change frequency to match Sharp MZ-700
            setZ80CPUFrequency(MZ_700_CPU_FREQ, 1);
            break;

        // Load the 80 column MZ700 1Z-013A bios into memory.
        case TZSVC_CMD_LOAD700BIOS80:
            if((status=loadZ80Memory((const char *)MZ_ROM_1Z_013A_80C, 0, MZ_MROM_ADDR, 0, 0, 1)) != FR_OK)
            {
                printf("Error: Failed to load %s into tranZPUter memory.\n", MZ_ROM_1Z_013A_80C);
            }
          
            // Change frequency to match Sharp MZ-700
            setZ80CPUFrequency(MZ_700_CPU_FREQ, 1);
            break;

        // Load the CPM CCP+BDOS from file into the address given.
        case TZSVC_CMD_LOADBDOS:
            if((status=loadZ80Memory((const char *)osControl.lastFile, MZF_HEADER_SIZE, svcControl.loadAddr+0x40000, svcControl.loadSize, 0, 1)) != FR_OK)
            {
                printf("Error: Failed to load BDOS:%s into tranZPUter memory.\n", (char *)osControl.lastFile);
            }
            break;

        // Add a CP/M disk to the system for read/write access by CP/M on the Sharp MZ80A.
        //
        case TZSVC_CMD_ADDSDDRIVE:
            status=svcAddCPMDrive();
            break;
         
        // Read an assigned CP/M drive sector giving an LBA address and the drive number.
        //
        case TZSVC_CMD_READSDDRIVE:
            status=svcReadCPMDrive();
            break;
          
        // Write a sector to an assigned CP/M drive giving an LBA address and the drive number.
        //
        case TZSVC_CMD_WRITESDDRIVE:
            status=svcWriteCPMDrive();
         
            // Only need to copy the command section back to the Z80 for a write operation.
            //
            copySize = TZSVC_CMD_SIZE;
            break;

        // Switch to the mainboard frequency (default).
        case TZSVC_CMD_CPU_BASEFREQ:
            setZ80CPUFrequency(0, 4);
            break;

        // Switch to the alternate frequency managed by the K64F counters.
        case TZSVC_CMD_CPU_ALTFREQ:
            setZ80CPUFrequency(0, 3);
            break;

        // Set the alternate frequency. The TZFS command provides the frequency in KHz so multiply up to Hertz before changing.
        case TZSVC_CMD_CPU_CHGFREQ:
            setZ80CPUFrequency(svcControl.cpuFreq * 1000, 1);
            break;

        default:
            break;
    }

    // Update the status in the service control record then copy it across to the Z80.
    //
    svcControl.result = status;
    copyToZ80(z80Control.svcControlAddr, (uint8_t *)&svcControl, copySize, 0);

    // Need to refresh the directory? Do this at the end of the routine so the Sharp MZ80A isnt held up.
    if(refreshCacheDir)
        svcCacheDir((const char *)svcControl.directory, 1);

    return;
}

// Method to test if the autoboot TZFS flag file exists on the SD card. If the file exists, set the autoboot flag.
//
uint8_t testTZFSAutoBoot(void)
{
    // Locals.
    uint8_t   result = 0;
    FIL       File;

    // Detect if the autoboot tranZPUter TZFS flag is set. This is a file called TZFSBOOT in the SD root directory.
    if(f_open(&File, "TZFSBOOT.FLG", FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        result = 1;
        f_close(&File);
    }

    return(result);
}

// Method to configure the hardware and events to operate the tranZPUter SW upgrade.
//
void setupTranZPUter(void)
{
    // Setup the pins to default mode and configure IRQ's.
    setupZ80Pins(0, &systick_millis_count);

    // Check to see if autoboot is needed.
    osControl.tzAutoBoot = testTZFSAutoBoot();
  
    // Ensure the machine is ready by performing a RESET.
    resetZ80();
}

//////////////////////////////////////////////////////////////
// End of tranZPUter i/f methods for zOS                    //
//////////////////////////////////////////////////////////////
#endif // Protected methods which reside in the kernel.


#if defined __APP__
// Dummy function to override a weak definition in the Teensy library. Currently the yield functionality is not
// needed within apps running on the K64F it is only applicable in the main OS.
//
void yield(void)
{
    return;
}
#endif // __APP__

#if defined __APP__ && defined __TZPU_DEBUG__
// Simple method to output the Z80 signals to the console - feel good factor. To be of real use the signals need to be captured against the system
// clock and the replayed, perhaps a todo!
//
void displaySignals(void)
{
    uint32_t ADDR = 0;
    uint8_t  DATA = 0;
    uint8_t  RD = 0;
    uint8_t  WR = 0;
    uint8_t  IORQ = 0;
    uint8_t  MREQ = 0;
    uint8_t  NMI = 0;
    uint8_t  INT = 0;
    uint8_t  M1 = 0;
    uint8_t  RFSH = 0;
    uint8_t  WAIT = 0;
    uint8_t  BUSRQ = 0;
    uint8_t  BUSACK = 0;
    uint8_t  ZBUSACK = 0;
    uint8_t  MBCLK = 0;
    uint8_t  HALT = 0;
    uint8_t  CLKSLCT = 0;
    uint8_t  latch = 0;

    setupZ80Pins(0, NULL);

    printf("Z80 Bus Signals:\r\n");
    while(1)
    {
        ADDR  = (pinGet(Z80_A18) & 0x1) << 18;
        ADDR |= (pinGet(Z80_A17) & 0x1) << 17;
        ADDR |= (pinGet(Z80_A16) & 0x1) << 16;
        ADDR |= (pinGet(Z80_A15) & 0x1) << 15;
        ADDR |= (pinGet(Z80_A14) & 0x1) << 14;
        ADDR |= (pinGet(Z80_A13) & 0x1) << 13;
        ADDR |= (pinGet(Z80_A12) & 0x1) << 12;
        ADDR |= (pinGet(Z80_A11) & 0x1) << 11;
        ADDR |= (pinGet(Z80_A10) & 0x1) << 10;
        ADDR |= (pinGet(Z80_A9)  & 0x1) << 9;
        ADDR |= (pinGet(Z80_A8)  & 0x1) << 8;
        ADDR |= (pinGet(Z80_A7)  & 0x1) << 7;
        ADDR |= (pinGet(Z80_A6)  & 0x1) << 6;
        ADDR |= (pinGet(Z80_A5)  & 0x1) << 5;
        ADDR |= (pinGet(Z80_A4)  & 0x1) << 4;
        ADDR |= (pinGet(Z80_A3)  & 0x1) << 3;
        ADDR |= (pinGet(Z80_A2)  & 0x1) << 2;
        ADDR |= (pinGet(Z80_A1)  & 0x1) << 1;
        ADDR |= (pinGet(Z80_A0)  & 0x1);
        DATA  = (pinGet(Z80_D7)  & 0x1) << 7;
        DATA |= (pinGet(Z80_D6)  & 0x1) << 6;
        DATA |= (pinGet(Z80_D5)  & 0x1) << 5;
        DATA |= (pinGet(Z80_D4)  & 0x1) << 4;
        DATA |= (pinGet(Z80_D3)  & 0x1) << 3;
        DATA |= (pinGet(Z80_D2)  & 0x1) << 2;
        DATA |= (pinGet(Z80_A1)  & 0x1) << 1;
        DATA |= (pinGet(Z80_D0)  & 0x1);
        RD=pinGet(Z80_RD);
        WR=pinGet(Z80_WR);
        MREQ=pinGet(Z80_MREQ);
        IORQ=pinGet(Z80_IORQ);
        NMI=pinGet(Z80_NMI);
        INT=pinGet(Z80_INT);
        M1=pinGet(CTL_M1);
        RFSH=pinGet(CTL_RFSH);
        WAIT=pinGet(Z80_WAIT);
        BUSRQ=pinGet(CTL_BUSRQ);
        BUSACK=pinGet(CTL_BUSACK);
        ZBUSACK=pinGet(Z80_BUSACK);
        MBCLK=pinGet(MB_SYSCLK);
        HALT=pinGet(CTL_HALT);
        CLKSLCT=pinGet(CTL_CLKSLCT);
        latch=readCtrlLatch();
    
        printf("\rADDR=%06lx %08x %02x %3s %3s %3s %3s %3s %3s %2s %4s %4s %2s %2s %3s %3s %4s %4s", ADDR, DATA, latch,
                                  (RD == 0 && MREQ == 0 && WR == 1 && IORQ == 1) ? "MRD" : "   ",
                                  (RD == 0 && IORQ == 0 && WR == 1 && MREQ == 1) ? "IRD" : "   ",
                                  (WR == 0 && MREQ == 0 && RD == 1 && IORQ == 1) ? "MWR" : "   ",
                                  (WR == 0 && IORQ == 0 && RD == 1 && MREQ == 1) ? "IWR" : "   ",
                                  (NMI == 0)     ? "NMI"   : "   ",
                                  (INT == 0)     ? "INT"   : "   ",
                                  (M1 == 0)      ? "M1"    : "  ",
                                  (RFSH == 0)    ? "RFSH"  : "    ",
                                  (WAIT == 0)    ? "WAIT"  : "    ",
                                  (BUSRQ == 0)   ? "BR"    : "  ",
                                  (BUSACK == 0)  ? "BA"    : "  ",
                                  (ZBUSACK == 0) ? "ZBA"   : "   ",
                                  (MBCLK == 1)   ? "CLK"   : "   ",
                                  (HALT == 0)    ? "HALT"  : "    ",
                                  (CLKSLCT == 0) ? "CLKS"  : "    "
              );
    }

    return;
}
#endif    // __TZPU_DEBUG__

#ifdef __cplusplus
}
  #endif
