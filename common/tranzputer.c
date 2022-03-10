/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tranzputer.c
// Created:         May 2020
// Version:         v1.1
// Author(s):       Philip Smart
// Description:     The TranZPUter library.
//                  This file contains methods which allow applications to access and control the
//                  tranZPUter board and the underlying Sharp MZ80A host.
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
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         v1.0 May 2020  - Initial write of the TranZPUter software.
//                  v1.1 July 2020 - Updated for v1.1 of the hardware, all pins have been routed on the PCB
//                                   to aid in decoding, for example, address lines are now all in Port C 11:0
//                                   for Z80 A11:A0 and Port A 15:12 for Z80 A15:12. This was done due to the
//                                   extra overhead required to piece together address and data lines in v1.0
//                                   where the PCB was routed for linear reading and routing.
//                  v1.2 July 2020 - Updates for the v2.1 tranZPUter board. I've used macro processing
//                                   to seperate v1+ and v2+ but I may well create two seperate directories
//                                   as both projects are updated. Alternatively I use git flow or similar, TBD!
//                  v1.3 Sep 2020  - Updates for the v2.2 tranZPUter board using an MK64FX512LLVQ 100 pin CPU
//                                   instead of the Teensy board. As the v2.2 continues on its own branch, all
//                                   references to previous boards and conditonal compilation have been removed
//                                   to make the code simpler to read.
//                  v1.4 Dec 2020  - With the advent of the tranZPUter SW-700 v1.3, major changes needed to
//                                   to be made. Some of the overhanging code from v1.0 of the original
//                                   tranZPUter SW had to be stripped out and direct 24bit addressing, as
//                                   opposed to memory mode configuration, was implemented. This added race
//                                   states as it was now much quicker so further tweaks using direct
//                                   register writes had to be made.
//                                   This version see's the advent of soft CPU's in hardware and the 
//                                   necessary support code added to this module.
//                                   Future boards will incorporate more memory/FPGA BRAM so the addressing
//                                   of external memory has been increased to 24bit (16Mbyte) and as
//                                   outlined above, now direct addressing.
//                  v1.5 Mar 2021  - Updates to merge the MZ-800 into the list of machines detected and 
//                                   handled by this module.
//                                   Found and fixed a major bug introduced in v1.4. The Z80 direction 
//                                   wasnt being set on occasion so an expected write wouldnt occur
//                                   which led to some interesting behaviour!
//                  v1.6 Apr 2021  - Fixed a SD directory cache bug when switching between directories.
//                                   The bug occurs due to an interaction between the heap management
//                                   and threads.
//                  v1.7 May 2021  - Changes to use 512K-1Mbyte Z80 Static RAM, build time configurable.
//                                   Added memory test routines to validate tranZPUter memory.
//                  v1.8 Oct 2021  - Major changes as the Sharp MZ Series FPGA Emulation is incorporated
//                                   within the tranZPUter. CPLD's are now coded on a 1:1 with a host
//                                   and support to allow CPLD emulation of a different host removed
//                                   as this is catered for by the Sharp MZ Series. 
//                                   MZ-2000 support added as the tranZPUter SW-700 now functions
//                                   on the MZ-2000 host hardware.
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
#include "diskio.h"
#include "utils.h"
#include <fonts.h>
#include <bitmaps.h>
#include <tranzputer.h>

// Bring in public declarations from emuMZ module (pseudo class).
#define EMUMZ_H
#include <emumz.h>

#ifndef __APP__        // Protected methods which should only reside in the kernel.

// Global scope variables used within the zOS kernel.
//
volatile uint32_t            *ioPin[MAX_TRANZPUTER_PINS];
uint8_t                      pinMap[MAX_TRANZPUTER_PINS];
uint32_t volatile            *ms;
static t_z80Control          z80Control;
static t_osControl           osControl;
static t_svcControl          svcControl;

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
static t_dispCodeMap dispCodeMap[] = {
    { 0xCC }, //  NUL '\0' (null character)     
    { 0xE0 }, //  SOH (start of heading)     
    { 0xF2 }, //  STX (start of text)        
    { 0xF3 }, //  ETX (end of text)          
    { 0xCE }, //  EOT (end of transmission)  
    { 0xCF }, //  ENQ (enquiry)              
    { 0xF6 }, //  ACK (acknowledge)          
    { 0xF7 }, //  BEL '\a' (bell)            
    { 0xF8 }, //  BS  '\b' (backspace)       
    { 0xF9 }, //  HT  '\t' (horizontal tab)  
    { 0xFA }, //  LF  '\n' (new line)        
    { 0xFB }, //  VT  '\v' (vertical tab)    
    { 0xFC }, //  FF  '\f' (form feed)       
    { 0xFD }, //  CR  '\r' (carriage ret)    
    { 0xFE }, //  SO  (shift out)            
    { 0xFF }, //  SI  (shift in)                
    { 0xE1 }, //  DLE (data link escape)        
    { 0xC1 }, //  DC1 (device control 1)     
    { 0xC2 }, //  DC2 (device control 2)     
    { 0xC3 }, //  DC3 (device control 3)     
    { 0xC4 }, //  DC4 (device control 4)     
    { 0xC5 }, //  NAK (negative ack.)        
    { 0xC6 }, //  SYN (synchronous idle)     
    { 0xE2 }, //  ETB (end of trans. blk)    
    { 0xE3 }, //  CAN (cancel)               
    { 0xE4 }, //  EM  (end of medium)        
    { 0xE5 }, //  SUB (substitute)           
    { 0xE6 }, //  ESC (escape)               
    { 0xEB }, //  FS  (file separator)       
    { 0xEE }, //  GS  (group separator)      
    { 0xEF }, //  RS  (record separator)     
    { 0xF4 }, //  US  (unit separator)       
    { 0x00 }, //  SPACE                         
    { 0x61 }, //  !                             
    { 0x62 }, //  "                          
    { 0x63 }, //  #                          
    { 0x64 }, //  $                          
    { 0x65 }, //  %                          
    { 0x66 }, //  &                          
    { 0x67 }, //  '                          
    { 0x68 }, //  (                          
    { 0x69 }, //  )                          
    { 0x6B }, //  *                          
    { 0x6A }, //  +                          
    { 0x2F }, //  ,                          
    { 0x2A }, //  -                          
    { 0x2E }, //  .                          
    { 0x2D }, //  /                          
    { 0x20 }, //  0                          
    { 0x21 }, //  1                          
    { 0x22 }, //  2                          
    { 0x23 }, //  3                          
    { 0x24 }, //  4                          
    { 0x25 }, //  5                          
    { 0x26 }, //  6                          
    { 0x27 }, //  7                          
    { 0x28 }, //  8                          
    { 0x29 }, //  9                          
    { 0x4F }, //  :                          
    { 0x2C }, //  ;                          
    { 0x51 }, //  <                          
    { 0x2B }, //  =                          
    { 0x57 }, //  >                          
    { 0x49 }, //  ?                          
    { 0x55 }, //  @
    { 0x01 }, //  A
    { 0x02 }, //  B
    { 0x03 }, //  C
    { 0x04 }, //  D
    { 0x05 }, //  E
    { 0x06 }, //  F
    { 0x07 }, //  G
    { 0x08 }, //  H
    { 0x09 }, //  I
    { 0x0A }, //  J
    { 0x0B }, //  K
    { 0x0C }, //  L
    { 0x0D }, //  M
    { 0x0E }, //  N
    { 0x0F }, //  O
    { 0x10 }, //  P
    { 0x11 }, //  Q
    { 0x12 }, //  R
    { 0x13 }, //  S
    { 0x14 }, //  T
    { 0x15 }, //  U
    { 0x16 }, //  V
    { 0x17 }, //  W
    { 0x18 }, //  X
    { 0x19 }, //  Y
    { 0x1A }, //  Z
    { 0x52 }, //  [
    { 0x59 }, //  \  '\\'
    { 0x54 }, //  ]
    { 0xBE }, //  ^
    { 0x3C }, //  _
    { 0xC7 }, //  `
    { 0x81 }, //  a
    { 0x82 }, //  b
    { 0x83 }, //  c
    { 0x84 }, //  d
    { 0x85 }, //  e
    { 0x86 }, //  f
    { 0x87 }, //  g
    { 0x88 }, //  h
    { 0x89 }, //  i
    { 0x8A }, //  j
    { 0x8B }, //  k
    { 0x8C }, //  l
    { 0x8D }, //  m
    { 0x8E }, //  n
    { 0x8F }, //  o
    { 0x90 }, //  p
    { 0x91 }, //  q
    { 0x92 }, //  r
    { 0x93 }, //  s
    { 0x94 }, //  t
    { 0x95 }, //  u
    { 0x96 }, //  v
    { 0x97 }, //  w
    { 0x98 }, //  x
    { 0x99 }, //  y
    { 0x9A }, //  z
    { 0xBC }, //  {
    { 0x80 }, //  |
    { 0x40 }, //  }
    { 0xA5 }, //  ~
    { 0xC0 }  //  DEL
};


// Dummy method for the interrupt Service Routines when disabled by current mode.
//
static void __attribute((naked, noinline)) irqPortE_dummy(void)
{
                                          // Save register we use.
    asm volatile("                        push     {r0-r1,lr}               \n");

                                          // Reset the interrupt, PORTE_ISFR <= PORTE_ISFR
    asm volatile("                        ldr      r0, =0x4004d0a0          \n"
                 "                        ldr      r1, [r0, #0]             \n"
                 "                        str      r1, [r0, #0]             \n");

                                          // Restore registers and exit.
    asm volatile("                        pop      {r0-r1,pc}               \n");

}
static void __attribute((naked, noinline)) irqPortD_dummy(void)
{
                                          // Save register we use.
    asm volatile("                        push     {r0-r1,lr}               \n");

                                          // Reset the interrupt, PORTE_ISFR <= PORTE_ISFR
    asm volatile("                        ldr      r0, =0x4004c0a0          \n"
                 "                        ldr      r1, [r0, #0]             \n"
                 "                        str      r1, [r0, #0]             \n");

                                          // Restore registers and exit.
    asm volatile("                        pop      {r0-r1,pc}               \n");

}
#if 0
static void __attribute((naked, noinline)) irqPortC_dummy(void)
{
                                          // Save register we use.
    asm volatile("                        push     {r0-r1,lr}               \n");

                                          // Reset the interrupt, PORTE_ISFR <= PORTE_ISFR
    asm volatile("                        ldr      r0, =0x4004b0a0          \n"
                 "                        ldr      r1, [r0, #0]             \n"
                 "                        str      r1, [r0, #0]             \n");

                                          // Restore registers and exit.
    asm volatile("                        pop      {r0-r1,pc}               \n");

}
#endif

// This method is called everytime an active irq triggers on Port E. For this design, this means the two IO CS
// lines, CTL_SVCREQ and TZ_SYSREQ. The SVCREQ is used when the Z80 requires a service, the SYSREQ is yet to
// be utilised.
//
//
static void __attribute((naked, noinline)) irqPortE(void)
{
                                          // Save register we use.
    asm volatile("                        push     {r0-r5,lr}               \n");

                                          // Reset the interrupt, PORTE_ISFR <= PORTE_ISFR
    asm volatile("                        ldr      r4, =0x4004d0a0          \n"
                 "                        ldr      r5, [r4, #0]             \n"
                 "                        str      r5, [r4, #0]             \n"

                                          // Is CTL_SVCREQ (E24) active (low), set flag and exit if it is.
                 "                        movs     r4, #1                   \n"
                 "                        tst      r5, #0x01000000          \n"
                 "                        beq      irqPortE_Exit            \n"
                 "                        strb     r4, %[val0]              \n" 

                 "              irqPortE_Exit:                              \n"

                                          : [val0] "+m" (z80Control.svcRequest),
                                            [val1] "+m" (z80Control.sysRequest) 
                                          :
                                          : "r4","r5","r7","r8","r9","r10","r11","r12"
                );
    asm volatile("                        pop      {r0-r5,pc}               \n");

    return;
}

// This method is called everytime an active irq triggers on Port D. For this design, this means the IORQ and RESET lines.
//
// Basic RESET detection.
//
static void __attribute((naked, noinline)) irqPortD(void)
{
                                          // This code is critical, if the Z80 is running at higher frequencies then very little time to capture it's requests.
    asm volatile("                        push     {r0-r3,lr}               \n");

                                          // Get the ISFR bit and reset.
    asm volatile("                        ldr      r1, =0x4004c0a0          \n"
                 "                        ldr      r0, [r1, #0]             \n"
                 "                        str      r0, [r1, #0]             \n"

                                          // Is Z80_RESET active, set flag and exit.
                 "                        movs     r1, #1                   \n"
                 "                        tst      r0, #0x0010              \n"
                 "                        beq      irqPortD_Exit            \n"
                 "                        strb     r1, %[val0]              \n"

                 "              irqPortD_Exit:                              \n"
            
                                          : [val0] "=m" (z80Control.resetEvent)
                                          : 
                                          : "r0","r1","r4","r5","r6","r7","r8","r9","r10","r11","r12");

                                          // Reset the interrupt, PORTD_ISFR <= PORTD_ISFR
    asm volatile("                        pop      {r0-r3,pc}               \n");

    return;
}

// Method to install the interrupt vector and enable it to capture Z80 memory/IO operations.
//
static void setupIRQ(void)
{
    __disable_irq();

    // Setup the interrupts according to the mode the tranZPUter is currently running under.
    switch(z80Control.hostType)
    {
        // For the MZ700 we need to enable IORQ to process the OUT statements the Z80 generates for memory mode selection. 
        case HW_MZ700:
            // Install the dummy method to be called when PortE triggers.
            _VectorsRam[IRQ_PORTE + 16] = irqPortE;

            // Install the method to be called when PortD triggers.
            _VectorsRam[IRQ_PORTD + 16] = irqPortD;
         
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
          
            // Setup the IRQ for Z80_IORQ.
            //installIRQ(Z80_IORQ, IRQ_MASK_FALLING);
           
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
          
            // Remove previous interrupts not needed in this mode.
            removeIRQ(Z80_IORQ);

            // Set relevant priorities to meet latency.
            NVIC_SET_PRIORITY(IRQ_PORTD, 0);
            NVIC_SET_PRIORITY(IRQ_PORTE, 16);
            break;
          
        // The MZ800 we setup for RESET and SVCREQ handling, everything else ignored.
        case HW_MZ800:
            // Install the dummy method to be called when PortE triggers.
            _VectorsRam[IRQ_PORTE + 16] = irqPortE;

            // Install the method to be called when PortD triggers.
            _VectorsRam[IRQ_PORTD + 16] = irqPortD;
         
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
          
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
          
            // Remove previous interrupts not needed in this mode.
            removeIRQ(Z80_IORQ);

            // Set relevant priorities to meet latency.
            NVIC_SET_PRIORITY(IRQ_PORTD, 0);
            NVIC_SET_PRIORITY(IRQ_PORTE, 16);
            break;


        // For the MZ80B we need to enable IORQ to process the OUT statements the Z80 generates for memory mode selection and I/O. 
        case HW_MZ80B:
            // Install the dummy method to be called when PortE triggers.
            _VectorsRam[IRQ_PORTE + 16] = irqPortE_dummy;
      
            // Install the method to be called when PortD triggers.
            _VectorsRam[IRQ_PORTD + 16] = irqPortD;
       
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
            
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
          
            // Remove previous interrupts not needed in this mode.
            removeIRQ(Z80_IORQ);

            // Set relevant priorities to meet latency.
            NVIC_SET_PRIORITY(IRQ_PORTD, 0);
            NVIC_SET_PRIORITY(IRQ_PORTE, 16);
            break;
           
        // The MZ2000 we setup for RESET and SVCREQ handling, everything else ignored.
        case HW_MZ2000:
            // Install the dummy method to be called when PortE triggers.
            _VectorsRam[IRQ_PORTE + 16] = irqPortE;
      
            // Install the method to be called when PortD triggers.
            _VectorsRam[IRQ_PORTD + 16] = irqPortD;
       
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
            
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
          
            // Remove previous interrupts not needed in this mode.
            removeIRQ(Z80_IORQ);

            // Set relevant priorities to meet latency.
            NVIC_SET_PRIORITY(IRQ_PORTD, 0);
            NVIC_SET_PRIORITY(IRQ_PORTE, 16);
            break;

        // If the tranZPUter is running as a Sharp MZ-80A then we dont need a complicated ISR, just enable the detection of IO requests on Port E.
        case HW_MZ80A:
        default:
            // Install the method to be called when PortE triggers.
            _VectorsRam[IRQ_PORTE + 16] = irqPortE;
            
            // Install the method to be called when PortD triggers.
            _VectorsRam[IRQ_PORTD + 16] = irqPortD;
           
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
            
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
          
            // Remove previous interrupts not needed in this mode.
            removeIRQ(Z80_IORQ);

            // Set relevant priorities to meet latency.
            NVIC_SET_PRIORITY(IRQ_PORTE, 0);
            NVIC_SET_PRIORITY(IRQ_PORTD, 16);
            break;
    }

    __enable_irq();
    return;
}

// Method to remove the IRQ vectors and disable them. 
// This method is called when the I/O processor goes 'silent', ie. not interacting with the host.
//
static void disableIRQ(void)
{
    __disable_irq();

    // Vectors to dummy routines.
    _VectorsRam[IRQ_PORTD + 16] = irqPortD_dummy;
    _VectorsRam[IRQ_PORTE + 16] = irqPortE_dummy;

    // Remove the interrupts.
    removeIRQ(CTL_SVCREQ);
    removeIRQ(Z80_IORQ);
    removeIRQ(Z80_RESET);

    __enable_irq();
    return;
}


// Method to restore the interrupt vector when the pin mode is changed and restored to default input mode.
//
static void restoreIRQ(void)
{
    __disable_irq();

    // Restore the interrupts according to the mode the tranZPUter is currently running under.
    switch(z80Control.hostType)
    {
        // For the MZ700 we need to enable IORQ to process the OUT statements the Z80 generates for memory mode selection. 
        case HW_MZ700:
            // Setup the IRQ for Z80_IORQ.
            //installIRQ(Z80_IORQ, IRQ_MASK_FALLING);
           
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);

            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
            break;
           
        // The MZ800 just needs to enable IORQ to process the OUT statements the Z80 generates for memory mode selection. 
        case HW_MZ800:
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);

            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
            break;

        // For the MZ80B we need to enable IORQ to process the OUT statements the Z80 generates for memory mode selection and I/O. 
        case HW_MZ80B:
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);

            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
            break;
           
        // The MZ2000 just re-enable the service request and reset interrupts.
        case HW_MZ2000:
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);

            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
            break;

        // If the tranZPUter is running as a Sharp MZ-80A then we dont need a complicated ISR, just enable the detection of IO requests on Port E.
        case HW_MZ80A:
        default:
            // Setup the IRQ for CTL_SVCREQ.
            installIRQ(CTL_SVCREQ, IRQ_MASK_LOW);
            
            // Setup the IRQ for Z80_RESET.
            installIRQ(Z80_RESET, IRQ_MASK_FALLING);
            break;
    }

    __enable_irq();
    return;
}

// Method to setup the pins and the pin map to power up default.
// The OS millisecond counter address is passed into this library to gain access to time without the penalty of procedure calls.
// Time is used for timeouts and seriously affects pulse width of signals when procedure calls are made.
//
void setupZ80Pins(uint8_t initK64F, volatile uint32_t *millisecondTick)
{
    // Locals.
    //
    static uint8_t firstCall = 1;

    // This method can be called more than once as a convenient way to return all pins to default. On first call though
    // the teensy library needs initialising, hence the static check.
    //
    if(firstCall == 1)
    {
        if(initK64F)
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
    pinMap[Z80_A19]     = Z80_A19_PIN;
    pinMap[Z80_A20]     = Z80_A20_PIN;
    pinMap[Z80_A21]     = Z80_A21_PIN;
    pinMap[Z80_A22]     = Z80_A22_PIN;
    pinMap[Z80_A23]     = Z80_A23_PIN;

    pinMap[Z80_D0]      = Z80_D0_PIN;
    pinMap[Z80_D1]      = Z80_D1_PIN;
    pinMap[Z80_D2]      = Z80_D2_PIN;
    pinMap[Z80_D3]      = Z80_D3_PIN;
    pinMap[Z80_D4]      = Z80_D4_PIN;
    pinMap[Z80_D5]      = Z80_D5_PIN;
    pinMap[Z80_D6]      = Z80_D6_PIN;
    pinMap[Z80_D7]      = Z80_D7_PIN;

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

    pinMap[CTL_SVCREQ]  = CTL_SVCREQ_PIN;
    pinMap[CTL_MBSEL]   = CTL_MBSEL_PIN;
    pinMap[CTL_BUSRQ]   = CTL_BUSRQ_PIN;
    pinMap[CTL_RFSH]    = CTL_RFSH_PIN;
    pinMap[CTL_HALT]    = CTL_HALT_PIN;
    pinMap[CTL_M1]      = CTL_M1_PIN;
    pinMap[CTL_WAIT]    = CTL_WAIT_PIN;
    pinMap[CTL_CLK]     = CTL_CLK_PIN;
    pinMap[CTL_BUSACK]  = CTL_BUSACK_PIN;

    // Now build the config array for all the ports. This aids in more rapid function switching as opposed to using
    // the pinMode and digitalRead/Write functions provided in the Teensy lib.
    //
    for(uint8_t idx=0; idx < MAX_TRANZPUTER_PINS; idx++)
    {
        ioPin[idx] = portConfigRegister(pinMap[idx]);

        // Set to input, will change as functionality dictates.
        if(idx != CTL_CLK && idx != CTL_BUSRQ && idx != CTL_WAIT && idx != CTL_MBSEL)
        {
            pinInput(idx);
        } else
        {
            if(idx == CTL_BUSRQ || idx == CTL_MBSEL || idx == CTL_WAIT)
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
    z80Control.svcControlAddr    = getServiceAddr();
    z80Control.refreshAddr       = 0x00;
    z80Control.disableRefresh    = 0;
    z80Control.runCtrlLatch      = readCtrlLatch();
    z80Control.holdZ80           = 0;
    z80Control.iplMode           = 1;
    z80Control.blockResetActions = 0;
    z80Control.ctrlMode          = Z80_RUN;
    z80Control.busDir            = TRISTATE;
    z80Control.hostType          = HW_MZ80A;
//    z80Control.machineMode       = MZ80A;
    z80Control.cpldVersion       = 1;

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
        z80Control.ioAddr            = 0;
        z80Control.ioEvent           = 0;
        z80Control.mz700.config      = 0x202;
        z80Control.ioData            = 0;
        z80Control.memorySwap        = 0;
        z80Control.crtMode           = 0;
        z80Control.scroll            = 0;

        // Setup the Interrupts for IORQ and MREQ.
        setupIRQ();
    }

    // Debug to quickly find out addresses of GPIO pins.
    #define GPIO_BITBAND_ADDR(reg, bit) (((uint32_t)&(reg) - 0x40000000) * 32 + (bit) * 4 + 0x42000000)
    //printf("%s, Set=%08lx, Clear=%08lx\n", "CTL_BUSRQ",  digital_pin_to_info_PGM[(CTL_BUSRQ_PIN)].reg,  digital_pin_to_info_PGM[(CTL_BUSRQ_PIN)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "Z80_WAIT",   digital_pin_to_info_PGM[(Z80_WAIT_PIN)].reg,   digital_pin_to_info_PGM[(Z80_WAIT_PIN)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "CTL_MBSEL",  digital_pin_to_info_PGM[(CTL_MBSEL)].reg,      digital_pin_to_info_PGM[(CTL_MBSEL)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "Z80_IORQ",   digital_pin_to_info_PGM[(Z80_IORQ_PIN)].reg,   digital_pin_to_info_PGM[(Z80_IORQ_PIN)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "Z80_MREQ",   digital_pin_to_info_PGM[(Z80_MREQ_PIN)].reg,   digital_pin_to_info_PGM[(Z80_MREQ_PIN)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "Z80_RD",     digital_pin_to_info_PGM[(Z80_RD_PIN)].reg,     digital_pin_to_info_PGM[(Z80_RD_PIN)].reg + 64 );
    //printf("%s, Set=%08lx, Clear=%08lx\n", "Z80_WR",     digital_pin_to_info_PGM[(Z80_WR_PIN)].reg,     digital_pin_to_info_PGM[(Z80_WR_PIN)].reg + 64 );
    return;
}

// Method to reset the Z80 CPU.
//
void resetZ80(uint8_t memoryMode)
{
    // Locals.
    //
    uint32_t startTime = *ms;

    // On the MZ-800, set the memory mode and disable default host logic by setting the mode to MZ800.
    //
    if(memoryMode == TZMM_MZ800)
    {
        // The first output is a dummy write which removes the original host configuration set when the CPLD cold starts. This is necessary to keep the machine original if
        // the K64F isnt present or autoboot is disabled. The second output ensures the memory management is set to the default state for an MZ-800.
        writeZ80IO(IO_TZ_CPLDCFG, HWMODE_MZ800 | MODE_VIDEO_MODULE_DISABLED | MODE_PRESERVE_CONFIG, TRANZPUTER);
        writeZ80IO(IO_TZ_MMIO4, 0, MAINBOARD);
    }

    // On MZ-2000 hosts the RESET is controlled by a gate array so we cannot assert our output. In order to reset we must use the 8255 output to force the gate array to
    // reset as it controls many signal states as well as the system reset signal.
    // UPDATED: CPLD updated to control the RESET line of the RESET switch thereby creating a true reset event. 8255 no longer used for reset.
    //
    if(z80Control.hostType == HW_MZ2000)
    {
        //writeZ80IO(IO_TZ_PPICTL, 0x06, MAINBOARD);
        writeZ80IO(IO_TZ_CPLDCMD, CPLD_RESET_HOST, MAINBOARD);
    }
    // Unknown boards do nothing as we dont know the underlying hardware configuration.
    else if(z80Control.hostType == HW_UNKNOWN)
    {
    }
    else
    {
        // Simply change the Z80_RESET pin to an output, assert low, reset high and reset to input to create the hardware reset event. On the original hardware the
        // reset pulse width is 90uS, the ms timer is only accurate to 100uS so we apply a 100uS pulse.
        //
        if(z80Control.hostType != HW_MZ2000 && z80Control.hostType != HW_UNKNOWN)
        {
            __disable_irq();
            pinOutputSet(Z80_RESET, LOW);
            for(volatile uint32_t pulseWidth=0; pulseWidth < DEFAULT_RESET_PULSE_WIDTH; pulseWidth++);
            pinHigh(Z80_RESET);
            pinInput(Z80_RESET);
            __enable_irq();
        }
    }

    // Set the memory mode to the one provided.
    //
    if(memoryMode != TZMM_MZ800)
    {
        setCtrlLatch(memoryMode);
    }

    // Wait a futher settling period before reinstating the interrupt.
    //
    while((*ms - startTime) < 400);

    // Restore the Z80 RESET IRQ as we have changed the pin mode.
    //
    restoreIRQ();

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

    // Is the Z80 currently being held? If it isnt, request access.
    if(z80Control.holdZ80 == 0)
    {
        // Set BUSRQ low which sets the Z80 BUSRQ low.
        pinLow(CTL_BUSRQ);
printf("Wait for low\n");
        // Set BUSRQ low and wait for a BUSACK or for the timeout period. If no response in the timeout period then the tranZPUter board/CPLD has locked up.
//        do {
//           // Wait 1ms or until BUSACK goes low.
//           for(uint32_t timer=*ms; timer == *ms && pinGet(CTL_BUSACK););

            // If BUSACK is still high, bring it inactive and loop as Z80_BUSACK may have gone low under FPGA control.
        //    if(pinGet(CTL_BUSACK))
        //    {
        //        pinHigh(CTL_BUSRQ);
        //    }
//       } 
      while(((*ms - startTime) < timeout && pinGet(CTL_BUSACK)));

        // If we timed out, deassert BUSRQ and return error.
        //
        if((*ms - startTime) >= timeout)
        {
printf("It is HIGH\n");
            pinHigh(CTL_BUSRQ);
            result = 1;
        } else
        {
printf("It is low\n");
            // Setup the bus ready for transactions, default to read.
            setupSignalsForZ80Access(READ);
   
            // Store the control latch state before we start modifying anything enabling a return to the pre bus request state.
            z80Control.runCtrlLatch = readCtrlLatchDirect(); 
        }
    } else
    {
        // Ensure signal direction is set to read as methods expect this state after a request for bus call.
        //
        setupSignalsForZ80Access(READ);
    }
    return(result);
}

// Method to request access to the main motherboard circuitry. This involves requesting the Z80 bus and then
// setting the CTL_MBSEL to high. The CPLD will enable the mainboard circuitry accordingly.
//
uint8_t reqMainboardBus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t            result = 0;

    // Ensure the CTL BUSACK signal high so we dont assert the mainboard BUSACK signal.
    pinHigh(CTL_MBSEL);

    // Request the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
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
// the CTL_MBSEL signal to low which disables (tri-states) the mainboard circuitry.
//
uint8_t reqTranZPUterBus(uint32_t timeout, enum TARGETS target)
{
    // Locals.
    //
    uint8_t  result = 0;

    // Requst the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
        // Now disable the mainboard by setting MBSEL low.
        pinLow(CTL_MBSEL);

        // Store the mode.
        z80Control.ctrlMode = TRANZPUTER_ACCESS;
      
        // For tranZPUter access, MEM4:0 should be set to TZMM_TZPU which allows a full 16MByte window in which to access the onboard RAM from the K64F processor.
        // For FPGA access, MEM4:0 should be set to TZMM_FPGA which allows a full 16MByte addressable window of resources inside the FPGA.
        z80Control.curCtrlLatch = (target == TRANZPUTER ? TZMM_TZPU : TZMM_FPGA);
    } else
    {
        printf("Failed to request TranZPUter Bus\n");
    }
    return(result);
}

// Method to set all the pins to be able to perform a transaction on the Z80 bus.
//
void setupSignalsForZ80Access(enum BUS_DIRECTION dir)
{
    // Address lines need to be outputs.
    setZ80AddrAsOutput();

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
    // Capture current time, bus should be released in the DEFAULT_BUSREQ_TIMEOUT period.
    uint32_t startTime = *ms;

    // Is the Z80 bus being held? If it isnt, release it otherwise do nothing.
    //
    if(z80Control.holdZ80 == 0)
    {
        // All address lines to inputs.
        //
        setZ80AddrAsInput();

        // If we were in write mode then set the data bus to input.
        //
        if(z80Control.busDir == WRITE)
        {
            // Same for data lines, revert to being inputs.
            setZ80DataAsInput();
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
        pinHigh(CTL_BUSRQ);
        pinHigh(CTL_MBSEL);
        
        // Store the mode.
        z80Control.ctrlMode = Z80_RUN;
           
        // Indicate bus direction.
        z80Control.busDir = TRISTATE;

        // Final check, wait for the CPLD to signal the bus has been released.
        while(((*ms - startTime) < DEFAULT_BUSREQ_TIMEOUT && !pinGet(CTL_BUSACK)));

        // Restore interrupt monitoring on pins.
        //
        restoreIRQ();
    } else
    {
        // If we are holding the Z80 bus, ensure correct state.
        pinHigh(CTL_MBSEL);
       
        // Store the mode.
        z80Control.ctrlMode = Z80_RUN;
    }
    return;
}


// Method to write a memory mapped byte onto the Z80 bus.
// As the original underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
// At time of testing, the code is fine with the 3.54MHz and 4MHz motherboards of the Sharp MZ-700 and MZ-80B.
//
uint8_t writeZ80Memory(uint32_t addr, uint8_t data, enum TARGETS target)
{
    // Locals.
    uint32_t          startTime = *ms;

    // Video RAM has a blanking circuit which causes the Z80 to enter a WAIT state. The K64F can only read the WAIT state and sometimes will miss it
    // so wait for the start of a blanking period and write.
    //
    //if(z80Control.ctrlMode == MAINBOARD_ACCESS && addr >= 0xD000 && addr < 0xE000)
    //{
    //    uint8_t blnkDet, blnkDetLast;
    //    blnkDet = 0x80;
    //    setZ80Direction(READ);
    //    do {
    //        blnkDetLast = blnkDet;
    //        blnkDet = readZ80Memory(0xE008) & 0x80;
    //    } while(!(blnkDet == 0x80 && blnkDetLast == 0x00));
    //    for(volatile uint32_t pulseWidth=0; pulseWidth < 10; pulseWidth++);
    //    setZ80Direction(WRITE);
    //    startTime = *ms;
    //}

    // Set the data and address on the bus.
    //
    setZ80Addr(addr);
    setZ80Data(data);

    // Different timing for uploading to the FPGA memory as it doesnt need the Z80 type transaction. Just create a pulse and the FPGA hardware
    // will translate it into a correct write signal.
    if(target == FPGA)
    {
        // Allow time for the address and data to settle then pulse the MREQ and WR line low.
        for(volatile uint32_t pulseWidth = 0; pulseWidth < 3; pulseWidth++);
        pinLow(Z80_MREQ);
        for(volatile uint32_t pulseWidth = 0; pulseWidth < 5; pulseWidth++);
        pinLow(Z80_WR);
        for(volatile uint32_t pulseWidth = 0; pulseWidth < 5; pulseWidth++);
        pinHigh(Z80_WR);
    }
    else
    {
        // Setup time before applying control signals.
        for(volatile uint32_t pulseWidth = 0; pulseWidth < 3; pulseWidth++);
        pinLow(Z80_MREQ);
        for(volatile uint32_t pulseWidth = 0; pulseWidth < 3; pulseWidth++);

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

            // On a K64F running at 120MHz this delay gives a pulsewidth of 760nS.
            for(volatile uint32_t pulseWidth=0; pulseWidth < 4; pulseWidth++);

            // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
            while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
        } else
        {
            // Start the write cycle, MREQ and WR go low.
            pinLow(Z80_WR);

            // On a K64F running at 120MHz this delay gives a pulsewidth of 760nS.
            // With the tranZPUter SW v2 boards, need to increase the write pulse width, alternatively wait until a positive edge on the CPU clock.
            for(volatile uint32_t pulseWidth=0; pulseWidth < 3; pulseWidth++);
        }
    }
    
    // Complete the write cycle.
    //
    pinHigh(Z80_WR);
    pinHigh(Z80_MREQ);

    return(0);
}

// Method to read a memory mapped byte from the Z80 bus.
// Keep to the Z80 timing diagram, but for better accuracy of matching the timing diagram this needs to be coded in assembler.
//
uint8_t readZ80Memory(uint32_t addr)
{
    // Locals.
    uint32_t startTime = *ms;
    uint8_t  data;

    // Video RAM has a blanking circuit which causes the Z80 to enter a WAIT state. The K64F can only read the WAIT state and sometimes will miss it
    // so wait for the start of a blanking period and read.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS && addr >= 0xD000 && addr < 0xE000)
    {
        uint8_t blnkDet, blnkDetLast;
        blnkDet = 0x80;
        do {
            blnkDetLast = blnkDet;
            blnkDet = readZ80Memory(0xE008) & 0x80;
        } while(!(blnkDet == 0x80 && blnkDetLast == 0x00));
        for(volatile uint32_t pulseWidth=0; pulseWidth < 10; pulseWidth++);
        startTime = *ms;
    }

    // Set the address on the bus and assert MREQ and RD.
    //
    setZ80Addr(addr);

    // Setup time before applying control signals.
    for(volatile uint32_t pulseWidth = 0; pulseWidth < 3; pulseWidth++);
    pinLow(Z80_MREQ);
    pinLow(Z80_RD);
    for(volatile uint32_t pulseWidth = 0; pulseWidth < 3; pulseWidth++);
  
    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // On a K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 4; pulseWidth++);

        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // On the tranZPUter v1.1, because of reorganisation of the signals, the time to process is less and so the pulse width under v1.0 is insufficient.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 4; pulseWidth++);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readZ80DataBus();

    // Complete the read cycle.
    //
    pinHigh(Z80_RD);
    pinHigh(Z80_MREQ);

    // Finally pass back the byte read to the caller.
    return(data);
}

// Method to write an array of values to Z80 memory.
//
uint8_t writeZ80Array(uint32_t addr, uint8_t *data, uint32_t size, enum TARGETS target)
{
    // Locals.
    uint32_t   nxtAddr = addr;
    uint8_t    *ptr    = data;
//printf("WZA: %08lx, %08lx\n", addr, data); 
    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation.
        //
        setZ80Direction(WRITE);

        // Loop through the array and write out the data to the next Z80 memory location.
        for(uint32_t idx=0; idx < size; idx++, nxtAddr++, ptr++)
        {
//printf("%08lx:%02x ", nxtAddr, *ptr);
//if(addr >= 0x300000 && addr < 0x300010)
//    delay(1);
            writeZ80Memory(nxtAddr, *ptr, target);
        }
//printf("\n");
    }

    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
        //
        releaseZ80();
    }
    return(0);
}

// Method to read an array of values from the Z80 memory.
//
uint8_t readZ80Array(uint32_t addr, uint8_t *data, uint32_t size, enum TARGETS target)
{
    // Locals.
    uint32_t   nxtAddr = addr;
    uint8_t    *ptr    = data;
    uint8_t    result  = 0;

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            result = reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            result = reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a read operation.
        //
        setZ80Direction(READ);

        // Loop through the given size reading byte at a time from the Z80 into the callers array.
        for(uint32_t idx=0; idx < size; idx++, nxtAddr++, ptr++)
        {
           *ptr = readZ80Memory(nxtAddr);
        }
    }

    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
        //
        releaseZ80();
    }
    return(result);
}

// Method to write a byte onto the Z80 I/O bus. This method is almost identical to the memory mapped method but are kept seperate for different
// timings as needed, the more code will create greater delays in the pulse width and timing.
//
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
//
uint8_t outZ80IO(uint32_t addr, uint8_t data)
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

        // On a K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        switch(z80Control.hostType)
        {
            case HW_MZ2000:
                for(volatile uint32_t pulseWidth=0; pulseWidth < 16; pulseWidth++);
                break;

            default:
                for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);
                break;
        }

        // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
        while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // Start the write cycle, WR go low.
        pinLow(Z80_WR);

        // With the tranZPUter SW v2 boards, need to increase the write pulse width as the latch is synchronous, alternatively wait until a positive edge on the CPU clock.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 8; pulseWidth++);
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
uint8_t inZ80IO(uint32_t addr)
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
        // On a K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        switch(z80Control.hostType)
        {
            case HW_MZ2000:
                for(volatile uint32_t pulseWidth=0; pulseWidth < 16; pulseWidth++);
                break;

            default:
                for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);
                break;
        }

        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // With the tranZPUter SW v2 boards, need to increase the read pulse width as the latch is synchronous, alternatively wait until a positive edge on the CPU clock.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 8; pulseWidth++);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readZ80DataBus();

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
        // and setting CTL_MBSEL high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_MBSEL);
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
        pinLow(CTL_MBSEL);
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
        // and setting CTL_MBSEL high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_MBSEL);
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
        pinLow(CTL_MBSEL);
    }
    return;
}

// Method to explicitly set the Memory model/mode of the tranZPUter.
//
void setCtrlLatch(uint8_t latchVal)
{
    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Gain control of the bus then set the latch value.
        //
        reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, TRANZPUTER);
    } else
    {
        // See if the bus needs changing.
        //
        reqZ80BusChange(TRANZPUTER_ACCESS);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(latchVal);
       
        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
            //
            releaseZ80();
        }
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
        // If the Z80 is in RUN mode, request the bus.
        // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
        //
        if(z80Control.ctrlMode == Z80_RUN)
        {
            // Gain control of the bus then set the latch value.
            //
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, TRANZPUTER);
        } else
        {
            // See if the bus needs changing.
            //
            reqZ80BusChange(TRANZPUTER_ACCESS);
        }

        // Gain control of the bus to change the CPU frequency latch.
        //
        if( z80Control.ctrlMode != Z80_RUN )
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
            outZ80IO((action == 1 || action == 3 ? IO_TZ_SETXMHZ : IO_TZ_SET2MHZ), 0);

            // Release the bus if it is not being held for further transations.
            //
            if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
            {
                // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
                //
                releaseZ80();
            }
         }
    }

    // Return the actual frequency set, this will be the nearest frequency the timers are capable of resolving.
    return(actualFreq);
}


// Method to write a value to a Z80 I/O address.
//
uint8_t writeZ80IO(uint32_t addr, uint8_t data, enum TARGETS target)
{
    // Locals.
 
    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation.
        //
        setZ80Direction(WRITE);

        // Output actual byte,
        outZ80IO(addr, data);
    }

    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
        //
        releaseZ80();
    }
    return(0);
}

// Method to read a value from a Z80 I/O address.
//
uint8_t readZ80IO(uint32_t addr, enum TARGETS target)
{
    // Locals.
    uint8_t result = 0;;

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            result = reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            result = reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a read operation.
        //
        setZ80Direction(READ);

        // Read actual byte,
        result = inZ80IO(addr);
    }

    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
        //
        releaseZ80();
    }
    return(result);
}

// Method to copy memory from the K64F to the Z80.
//
uint8_t copyFromZ80(uint8_t *dst, uint32_t src, uint32_t size, enum TARGETS target)
{
    // Locals.
    uint8_t    result = 0;

    // Sanity checks.
    //
    if((target == MAINBOARD && (src+size) > 0x10000) || (target == TRANZPUTER && (src+size) > TZ_MAX_Z80_MEM) || (target == FPGA && (src+size) > TZ_MAX_FPGA_MEM) )
        return(1);

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            result = reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            result = reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        for(uint32_t idx=0; idx < size; idx++)
        {
            // Perform a refresh on the main memory every 2ms.
            //
            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
         
            // And now read the byte and store.
            *dst = readZ80Memory((uint32_t)src);
            src++;
            dst++;
        }
    }
  
    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
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
uint8_t copyToZ80(uint32_t dst, uint8_t *src, uint32_t size, enum TARGETS target)
{
    // Locals.
    uint8_t    result = 0;

    // Sanity checks.
    //
    if((target == MAINBOARD && (dst+size) > 0x10000) || (target == TRANZPUTER && (dst+size) > TZ_MAX_Z80_MEM) || (target == FPGA && (dst+size) > TZ_MAX_FPGA_MEM) )
        return(1);

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            result = reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            result = reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation.
        //
        writeCtrlLatch(z80Control.curCtrlLatch);

        for(uint32_t idx=0; idx < size; idx++)
        {
            // Perform a refresh on the main memory every 2ms.
            //
            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
           
            // And now write the byte and to the next address!
            writeZ80Memory((uint32_t)dst, *src, target);
            src++;
            dst++;
        }
    }
 
    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    return(result);
}


// Method to fill memory under the Z80 control, either the mainboard or tranZPUter memory.
//
void fillZ80Memory(uint32_t addr, uint32_t size, uint8_t data, enum TARGETS target)
{
    // Locals.

    // Sanity checks.
    //
    if((target == MAINBOARD && (addr+size) > 0x10000) || (target == TRANZPUTER && (addr+size) > TZ_MAX_Z80_MEM) || (target == FPGA && (addr+size) > TZ_MAX_FPGA_MEM) )
        return;

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation (after setting the latch to starting value).
        //
        writeCtrlLatch(z80Control.curCtrlLatch);

        // Fill the memory but every RFSH_BYTE_CNT perform a DRAM refresh.
        //
        for(uint32_t idx=addr; idx < (addr+size); idx++)
        {
            if(idx % RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
            writeZ80Memory((uint32_t)idx, data, target);
        }
    }
  
    // Release the bus if it is not being held for further transations.
    //
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// A memory test method extracted from the base zputa code for use with the tranZPUter card.
//
uint8_t testZ80Memory(uint32_t start, uint32_t end, uint32_t testsToDo, int verbose, enum TARGETS target)
{
    // Locals.
    uint32_t       memPtr;
    uint32_t       memPtr2;
    unsigned long  count;
    unsigned long  count2;
    uint8_t        data;
    uint8_t        readBack;
    uint8_t        retCode = 0;
    uint32_t       errCnt = 0;


    // Sanity checks.
    //
    if((target == MAINBOARD && (start > 0x10000 || end > 0x10000)) || (target == TRANZPUTER && (start > TZ_MAX_Z80_MEM || end > TZ_MAX_Z80_MEM)) || (target == FPGA && (start > TZ_MAX_FPGA_MEM || end > TZ_MAX_FPGA_MEM)) )
        return(1);

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // Ensure we dont close the bus connection during the tests.
    z80Control.holdZ80 = 1;

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation (after setting the latch to starting value).
        //
        writeCtrlLatch(z80Control.curCtrlLatch);

        // Ascending value test write with readback.
        if(testsToDo & 0x00000001 && retCode == 0)
        {
            if(verbose)
                printf( "\rR/W 8bit ascending test pattern...    " );
            memPtr = start;
            data   = 0x00;
            count  = end - start;
            while( count-- && errCnt <= 20)
            {
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, data, target);
                setZ80Direction(READ);
                if( (readBack=readZ80Memory(memPtr)) != data )
                {
                    if(verbose)
                        printf( "\rError (8bit rwap) at 0x%08lX (%02x:%02x)\n", memPtr, readBack, data );
                    if(errCnt++ == 20)
                    {
                        if(verbose)
                            printf( "\rError count (8bit rwap) > 20, stopping test.\n");
                        retCode = 10;
                    }
                }
                memPtr++;
                data++;
                if( data >= 0xFF )
                    data = 0x00;
            }
        }
    
        // Walking value test write with readback.
        if(testsToDo & 0x00000002 && retCode == 0)
        {
            if(verbose)
                printf( "\rR/W 8bit walking test pattern...    " );
            memPtr = start;
            data   = 0x55;
            count  = end - start;
            errCnt = 0;
            while( count-- && errCnt <= 20)
            {
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, data, target);
                setZ80Direction(READ);
                if( (readBack=readZ80Memory(memPtr)) != data )
                {
                    if(verbose)
                        printf( "\rError (8bit rwwp) at 0x%08lX (%02x:%02x)\n", memPtr, readBack, data );
                    if(errCnt++ == 20)
                    {
                        if(verbose)
                            printf( "\rError count (8bit rwwp) > 20, stopping test.\n");
                        retCode = 20;
                    }
                }
                memPtr++;
                if( data == 0x55 )
                    data = 0xAA;
                else
                    data = 0x55;
            }
        }
    
        // Ascending value test write block then verify block.
        if(testsToDo & 0x00000004 && retCode == 0)
        {
            if(verbose)
                printf( "\rWrite 8bit ascending test pattern...    " );
            memPtr = start;
            data   = 0x00;
            count  = end - start;
            while( count-- )
            {
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, data, target);
                setZ80Direction(READ);
                if( (readBack=readZ80Memory(memPtr)) != data )
                {
                    if(verbose)
                        printf( "\rError (8bit wap) at 0x%08lX (%02x:%02x)\n", memPtr, readBack, data );
                    if(errCnt++ == 20)
                    {
                        if(verbose)
                            printf( "\rError count (8bit rwwp) > 20, stopping test.\n");
                        retCode = 30;
                    }
                }
                memPtr++;
                data++;
                if( data >= 0xFF )
                    data = 0x00;
            }
    
            if(verbose)
                printf( "\rRead 8bit ascending test pattern...     " );
            memPtr = start;
            data   = 0x00;
            count  = end - start;
            errCnt = 0;
            while( count-- && errCnt <= 20)
            {
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, data, target);
                setZ80Direction(READ);
                if( (readBack=readZ80Memory(memPtr)) != data )
                {
                    if(verbose)
                        printf( "\rError (8bit ap) at 0x%08lX (%02x:%02x)\n", memPtr, readBack, data );
                    if(errCnt++ == 20)
                    {
                        if(verbose)
                            printf( "\rError count (8bit ap) > 20, stopping test.\n");
                        retCode = 40;
                    }
                } 
                memPtr++;
                data++;
                if( data >= 0xFF )
                    data = 0x00;
            }
        }
    
        // Walking value test write block then verify block.
        if(testsToDo & 0x00000008 && retCode == 0)
        {
            if(verbose)
                printf( "\rWrite 8bit walking test pattern...    " );
            memPtr = start;
            data   = 0x55;
            count  = end - start;
            setZ80Direction(WRITE);
            while( count-- )
            {
                writeZ80Memory(memPtr, data, target);
                if( data == 0x55 )
                    data = 0xAA;
                else
                    data = 0x55;
                memPtr++;
            }
    
            if(verbose)
                printf( "\rRead 8bit walking test pattern...     " );
            memPtr = start;
            data   = 0x55;
            count  = end - start;
            errCnt = 0;
            setZ80Direction(READ);
            while( count-- && errCnt <= 20)
            {
                if( (readBack=readZ80Memory(memPtr)) != data )
                {
                    if(verbose)
                        printf( "\rError (8bit wp) at 0x%08lX (%02x:%02x)\n", memPtr, readBack, data );
                    if(errCnt++ == 20)
                    {
                        if(verbose)
                            printf( "\rError count (8bit wp) > 20, stopping test.\n");
                        retCode = 50;
                    }
                }
                memPtr++;
                if( data == 0x55 )
                    data = 0xAA;
                else
                    data = 0x55;
            }
        }
    
        // Echo and sticky bit test, more complex as the array has to be scanned for each set to see if any bit in the memory array
        // has been set with a write to 1 location or bits are staying stuck.
        if(testsToDo & 0x00000010 && retCode == 0)
        {
            if(verbose)
                printf( "\r8bit echo and sticky bit test...     " );
            memPtr = start;
            count  = end - start;
            errCnt = 0;
            fillZ80Memory(start, end - start, 0x00, target);
            while( count-- && errCnt <= 20)
            {
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, 0xFF, target);
    
                memPtr2 = start;
                count2  = end - start;
                while( count2-- && errCnt <= 20)
                {
                    setZ80Direction(READ);
                    readBack=readZ80Memory(memPtr2);
                    if( readBack != 0x00 && readBack != readZ80Memory(memPtr))
                    {
                        if(verbose)
                            printf( "\rError (8bit es) at 0x%08lx:0x%08lX (%02x:%02x)\n", memPtr, memPtr2, readBack, 0x00 );
                        setZ80Direction(WRITE);
                        writeZ80Memory(memPtr2, 0x00, target);
                        if(errCnt++ == 20)
                        {
                            if(verbose)
                                printf( "\rError count (8bit es) > 20, stopping test.\n");
                            retCode = 60;
                        }
                    }
                    memPtr2++;
                }
                setZ80Direction(WRITE);
                writeZ80Memory(memPtr, 0x00, target);
                memPtr++;
            }
        }
    }
     
    // Release the bus if it is not being held for further transations.
    //
    z80Control.holdZ80 = 1;
    if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    // Return completion code.
    return(retCode);
}

// A method to read the full video frame buffer from the Sharp MZ80A and store it in local memory (control structure). 
// No refresh cycles are needed as we grab the frame but between frames a full refresh is performed.
//
void captureVideoFrame(enum VIDEO_FRAMES frame, uint8_t noAttributeFrame)
{
    // Locals.

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Gain mainboard control.
        reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);

        // Ensure bus is set to write.
        setZ80Direction(WRITE);
    } else
    {
        // See if the bus needs changing.
        //
        reqZ80BusChange(MAINBOARD_ACCESS);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        // No need for refresh as we take less than 2ms time, just grab the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            z80Control.videoRAM[frame][idx] = readZ80Memory((uint32_t)idx+MZ_VID_RAM_ADDR);
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
                z80Control.attributeRAM[frame][idx] = readZ80Memory((uint32_t)idx+MZ_ATTR_RAM_ADDR);
            }

            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Restore the control latch to its original configuration.
            //
            setZ80Direction(WRITE);
            writeCtrlLatch(z80Control.runCtrlLatch);

            // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
            //
            releaseZ80();
        }
    }
    return;
}

// Method to refresh the video frame buffer on the Sharp MZ80A with the data held in local memory.
//
void refreshVideoFrame(enum VIDEO_FRAMES frame, uint8_t scrolHome, uint8_t noAttributeFrame)
{
    // Locals.

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the Z80 BUS to remain under the tranZPUter control for multiple transactions.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Gain mainboard control.
        reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
    } else
    {
        // See if the bus needs changing.
        //
        reqZ80BusChange(MAINBOARD_ACCESS);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a write operation.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // No need for refresh as we take less than 2ms time, just write the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            writeZ80Memory((uint32_t)idx+MZ_VID_RAM_ADDR, z80Control.videoRAM[frame][idx], MAINBOARD);
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
                writeZ80Memory((uint32_t)idx+MZ_ATTR_RAM_ADDR, z80Control.attributeRAM[frame][idx], MAINBOARD);
            }
       
            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // If the Scroll Home flag is set, this means execute a read against the hardware scoll register to restore the position 0,0 to location MZ_VID_RAM_ADDR.
        if(scrolHome)
        {
            setZ80Direction(READ);
            readZ80Memory((uint32_t)MZ_SCROL_BASE);
        }

        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Restore the control latch to its original configuration.
            //
            setZ80Direction(WRITE);
            writeCtrlLatch(z80Control.runCtrlLatch);

            // Release bus, we dont interfere with the control latch as I/O isnt affected by memory management.
            //
            releaseZ80();
        }
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
FRESULT loadZ80Memory(const char *src, uint32_t fileOffset, uint32_t addr, uint32_t size, uint32_t *bytesRead, enum TARGETS target, uint8_t releaseBus)
{
    // Locals.
    //
    FIL           File;
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
printf("Loading file(%s,%08lx,%08lx)\n", src, addr, size);   
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
            // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
            if(target == TRANZPUTER || target == FPGA)
            {
                reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
            } else
            {
                reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
            }
            // If successful, setup the control pins for upload mode.
            //
            if(z80Control.ctrlMode != Z80_RUN)
            {
                // Setup the pins to perform a write operation.
                //
                setZ80Direction(WRITE);
               
                // Setup the control latch to the required starting configuration.
                writeCtrlLatch(z80Control.curCtrlLatch);
            }
        } else
        {
            // See if the bus needs changing.
            //
            enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
            reqZ80BusChange(newMode);
        }

        // If we have taken control of the bus, commence upload.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // If the host hardware is an MZ-700/MZ-800 and we are loading into main memory, issue memory management commands to page in all the DRAM.
            //
            if(target == MAINBOARD && addr <= 0x1000 && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
            {
                // Then page in the lower DRAM bank.
                outZ80IO(IO_TZ_MMIO0, 0);
            }
            if(target == MAINBOARD && (addr >= 0xD000 || addr+size >= 0xD000) && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
            {
                // Then page in the upper DRAM bank.
                outZ80IO(IO_TZ_MMIO1, 0);
            }

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
                    // At the halfway mark perform a full refresh.
                    if(idx == (SECTOR_SIZE/2)) 
                    {
                        refreshZ80AllRows();
                    }
                   
                    // And now write the byte and to the next address!
                    writeZ80Memory((uint32_t)memPtr, buf[idx], target);
                    memPtr++;
                }
                loadSize += readSize;
            } while(loadSize < size);
          
            // Reset the memory management for MZ-700/MZ-800.
            //
            if(target == MAINBOARD && addr <= 0x1000 && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
            {
                // Restore lower memory bank to BIOS.
                outZ80IO(IO_TZ_MMIO2, 0);
            }
            if(target == MAINBOARD && (addr >= 0xD000 || addr+size >= 0xD000) && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
            {
                // Restore upper memory bank to VRAM etc.
                outZ80IO(IO_TZ_MMIO3, 0);
            }
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

    // Return number of bytes read if caller provided a variable.
    //
    if(bytesRead != NULL)
    {
        *bytesRead = loadSize;
    }

    // If requested or an error occurs, then release the Z80 bus as no more uploads will be taking place in this batch.
    //
    if(z80Control.holdZ80 == 0 && (releaseBus == 1 || fr0))
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return(fr0 ? fr0 : FR_OK);    
}


// Method to load an MZF format file from the SD card directly into the tranZPUter static RAM, FPGA or mainboard RAM.
// If the load address is specified then it overrides the MZF header value, otherwise load addr is taken from the header.
//
FRESULT loadMZFZ80Memory(const char *src, uint32_t addr, uint32_t *bytesRead, uint8_t hdrOnly, enum TARGETS target, uint8_t releaseBus)
{
    // Locals.
    FIL           File;
    unsigned int  readSize;
    uint32_t      addrOffset = SRAM_BANK0_ADDR;
    uint32_t      cmtHdrAddr = MZ_CMT_ADDR;
    t_svcDirEnt   mzfHeader;
    FRESULT       fr0;

//printf("z80Control 2a:\n svcControlAddr=%08lx\nrefreshAddr=%02x\ndisableRefresh=%02x\nrunCtrlLatch=%02x\ncurCtrlLatch=%02x\nholdZ80=%02x\nctrlMode=%02x\nbusDir=%02x\nhostType=%02x\nmachineMode=%02x\nresetEvent=%02x\nsvcRequest=%02x\n",
//        z80Control.svcControlAddr, z80Control.refreshAddr, z80Control.disableRefresh, z80Control.runCtrlLatch, z80Control.curCtrlLatch, z80Control.holdZ80, z80Control.ctrlMode, z80Control.busDir, z80Control.hostType,
//        z80Control.machineMode, z80Control.resetEvent, z80Control.svcRequest );
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

        // Setup bank in which to load the header/data. Default is bank 0, different host hardware uses different banks.
        //
        if(target == TRANZPUTER && z80Control.hostType == HW_MZ800)
        {
            addrOffset = SRAM_BANK6_ADDR;
        } 
        else if(target == TRANZPUTER && z80Control.hostType == HW_MZ2000)
        {
            addrOffset = SRAM_BANK6_ADDR;
        }
      
        // Save the header into the CMT area for reference, some applications expect it. If the load address is below 1200H this could be wiped out, the code below stores a copy
        // in the service record sector on exit for this purpose. The caller needs to check the service record and if the Load Address is below >= 1200H use the CMT header else
        // use the service sector.
        //
        // NB: This assumes TZFS is running and made this call. Ignore for MZ2000 when in IPL mode as the header isnt needed.
        //
        if(z80Control.hostType != HW_MZ2000 || (z80Control.hostType == HW_MZ2000 && z80Control.iplMode == 0))
        {
            copyToZ80(addrOffset+cmtHdrAddr, (uint8_t *)&mzfHeader, MZF_HEADER_SIZE, target);
        }

        if(hdrOnly == 0)
        {
            // Now obtain the parameters.
            //
            if(addr == 0xFFFFFFFF)
            {
                // If the address needs to be determined from the header yet the header is below the RAM, set the buffer to 0x1200 and let the caller sort it out.
                if(mzfHeader.loadAddr > 0x1000)
                    addr = mzfHeader.loadAddr;
                else
                    addr = 0x1200;
            }

            // Look at the attribute byte, if it is >= 0xF8 then it is a special tranZPUter binary object requiring loading into a seperate memory bank.
            // The attribute & 0x07 << 16 specifies the memory bank in which to load the image.
            if(mzfHeader.attr >= 0xF8)
            {
                addr += ((mzfHeader.attr & 0x07) << 16);
                //printf("CPM: Addr=%08lx, Size=%08lx\n", addr, mzfHeader.fileSize);
            } else
            {
                addr += addrOffset;
            }

            // Ok, load up the file into Z80 memory.
            fr0 = loadZ80Memory(src, MZF_HEADER_SIZE, addr, (mzfHeader.attr >= 0xF8 ? 0 : mzfHeader.fileSize), bytesRead, target, releaseBus);

            // If the load address was below 0x11D0, the lowest free point in the original memory map then the load is said to be in lower DRAM. In this case the CMT header wont be available
            // so load the header into the service sector as well so the caller can determine the load state.
            memcpy((uint8_t *)&svcControl.sector, (uint8_t *)&mzfHeader, MZF_HEADER_SIZE);
        }
    }
    return(fr0 ? fr0 : FR_OK);    
}


// Method to read a section of the tranZPUter/FPGA/mainboard memory and store it in an SD file.
//
FRESULT saveZ80Memory(const char *dst, uint32_t addr, uint32_t size, t_svcDirEnt *mzfHeader, enum TARGETS target)
{
    // Locals.
    //
    FIL           File;
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
      
            // If the Z80 is in RUN mode, request the bus.
            // This mechanism allows for the load command to leave the BUS under the tranZPUter control for multiple transactions.
            // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
            // may occur.
            //
            if(z80Control.ctrlMode == Z80_RUN)
            {
                // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
                if(target == TRANZPUTER || target == FPGA)
                {
                    reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
                } else
                {
                    reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
                }

                // If successful, setup the control pins for upload mode.
                //
                if(z80Control.ctrlMode != Z80_RUN)
                {
                    // Setup the pins to perform a write operation.
                    //
                    setZ80Direction(WRITE);
                   
                    // Setup the control latch to the required starting configuration.
                    writeCtrlLatch(z80Control.curCtrlLatch);
                }
            } else
            {
                // See if the bus needs changing.
                //
                enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
                reqZ80BusChange(newMode);
            }

            // If we have bus control, complete the task,
            //
            if( z80Control.ctrlMode != Z80_RUN )
            {
                // Setup the pins to perform a read operation.
                //
                setZ80Direction(READ);
    
                // Loop, reading a sector worth of data (or upto limit remaining) from the Z80 tranZPUter RAM or mainboard RAM and writing it into the open SD card file.
                //
                saveSize = 0;
                for (;;) {
    
                    // Work out how many bytes to write in the sector then fetch from the Z80.
                    sizeToWrite = (size-saveSize) > SECTOR_SIZE ? SECTOR_SIZE : size - saveSize;
                    for(unsigned int idx=0; idx < sizeToWrite; idx++)
                    {
                        // At the halfway mark perform a full refresh.
                        if(idx == (SECTOR_SIZE/2))
                        {
                            refreshZ80AllRows();
                        }
    
                        // And now read the byte and to the next address!
                        buf[idx] = readZ80Memory((uint32_t)memPtr);
                        memPtr++;
                    }
    
                    // Wrap disk write with two full refresh periods to counter for the amount of time taken to write to disk.
                    refreshZ80AllRows();
                    fr0 = f_write(&File, buf, sizeToWrite, &writeSize);
                    refreshZ80AllRows();
                    saveSize += writeSize;

                    if (fr0 || writeSize < sizeToWrite || saveSize >= size) break;       // error, disk full or range written.
                }
    
                // Release the bus if it is not being held for further transations.
                //
                if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
                {
                    // Restore the control latch to its original configuration.
                    //
                    setZ80Direction(WRITE);
                    writeCtrlLatch(z80Control.runCtrlLatch);
                    releaseZ80();
                }
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
int memoryDumpZ80(uint32_t memaddr, uint32_t memsize, uint32_t dispaddr, uint8_t dispwidth, uint8_t memCtrl, enum TARGETS target)
{
    uint8_t  data;
    int8_t   keyIn         = 0;
    uint32_t pnt           = memaddr;
    uint32_t endAddr       = memaddr + memsize;
    uint32_t addr          = dispaddr;
    uint32_t i             = 0;
    char     c             = 0;

    // Sanity checks.
    //
    if((target == MAINBOARD && (memaddr+memsize) > 0x10000) || (target == TRANZPUTER && (memaddr+memsize) > TZ_MAX_Z80_MEM) || (target == FPGA && (memaddr+memsize) > TZ_MAX_FPGA_MEM))
        return(1);

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the load command to leave the BUS under the tranZPUter control for multiple transactions.
    // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
    // may occur.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the board according to the target flag, target = MAINBOARD then the mainboard is controlled otherwise the tranZPUter board.
        if(target == TRANZPUTER || target == FPGA)
        {
            reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, target);
        } else
        {
            reqMainboardBus(DEFAULT_BUSREQ_TIMEOUT);
        }

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        enum CTRL_MODE newMode = (target == MAINBOARD) ? MAINBOARD_ACCESS : TRANZPUTER_ACCESS;
        reqZ80BusChange(newMode);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform a read operation.
        //
        setZ80Direction(READ);
        
        // If the host hardware is an MZ-700/MZ-800 and memCtrl has been set then we need to issue memory management commands to page in all the DRAM.
        //
        if(target == MAINBOARD && memCtrl == 1 && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
        {
            // First reset memory map.
            outZ80IO(IO_TZ_MMIO4, 0);

            // Then page in the DRAM banks.
            inZ80IO(IO_TZ_MMIO1);
            outZ80IO(IO_TZ_MMIO0, 0);
            outZ80IO(IO_TZ_MMIO1, 0);
        }

        while (1)
        {
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
            keyIn = getKey(0);
            if(keyIn == ' ')
            {
                do {
                    // Keep refreshing the Z80 DRAM whilst we wait for a key.
                    //
                    refreshZ80();

                    keyIn = getKey(0);
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
       
        // Reset the memory management for MZ-700/MZ-800.
        //
        if(target == MAINBOARD && memCtrl == 1 && (z80Control.hostType == HW_MZ700 || z80Control.hostType == HW_MZ800))
        {
            // Bring back the monitor ROM.
            outZ80IO(IO_TZ_MMIO2, 0);
            outZ80IO(IO_TZ_MMIO3, 0);
        }
       
        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Restore the control latch to its original configuration.
            //
            writeCtrlLatch(z80Control.runCtrlLatch);
            releaseZ80();
        }
    }

    // Normal exit, return -1, escape exit return 0.
    return(keyIn == 0x1b ? 0 : -1);
}

// Method to clear the host video display. To cater for differences in architecture, the machine model is given and processed accordingly.
//
void clsHost(void)
{
    switch(z80Control.hostType)
    {
        case HW_MZ2000:
            writeZ80IO(IO_TZ_PIOA, 0xD3, MAINBOARD);
            fillZ80Memory(MZ_VID_RAM_ADDR, MZ_VID_RAM_SIZE, 0x00, MAINBOARD);
            writeZ80IO(IO_TZ_PIOA, 0x13, MAINBOARD);
            break;

        case HW_MZ700:
            fillZ80Memory(MZ_VID_RAM_ADDR, MZ_VID_RAM_SIZE, 0x00, MAINBOARD);
            fillZ80Memory(MZ_ATTR_RAM_ADDR, MZ_ATTR_RAM_SIZE, VMATTR_FG_WHITE|VMATTR_BG_BLUE, MAINBOARD);
            break;

        case HW_MZ80A:
        default:
            fillZ80Memory(MZ_VID_RAM_ADDR, MZ_VID_RAM_SIZE, 0x00, MAINBOARD);
            break;
    }
    return;
}

// Method to print to the host display.
// 
void printfHost(uint8_t x, uint8_t y, char *format, ...)
{
    // Locals.
    //
    va_list   args;
    uint8_t   tmpbuf[1024];
    uint32_t  tmpbuflen;
    uint32_t  addr;

    // Sanity check.
    if(x > MZ_VID_MAX_COL || y > MZ_VID_MAX_ROW)
        return;

    // Setup variable arguments.
    va_start(args, format);

    // Expand the given string into a physical byte buffer.
    vsprintf(tmpbuf, format, args);

    // Store length as Sharp code uses 0x00 as SPACE character so strlen isnt useful when converting.
    tmpbuflen = strlen(tmpbuf);

    // Process any machine specific differences in display addressing.
    switch(z80Control.hostType)
    {
        case HW_MZ2000:
            addr = MZ_VID_RAM_ADDR + (y * MZ_VID_MAX_COL) + x;
            writeZ80IO(IO_TZ_PIOA, 0xD3, MAINBOARD);
            copyToZ80(addr, &tmpbuf, tmpbuflen, MAINBOARD);
            writeZ80IO(IO_TZ_PIOA, 0x13, MAINBOARD);
            break;

        case HW_MZ80A:
        case HW_MZ700:
        case HW_MZ800:
            // Translate ASCII to Sharp ASCII.
            for(uint32_t idx=0; idx < tmpbuflen; idx++)
            {
                tmpbuf[idx] = dispCodeMap[tmpbuf[idx]].dispCode;
            }
            addr = MZ_VID_RAM_ADDR + (y * MZ_VID_MAX_COL) + x;
            copyToZ80(addr, &tmpbuf, tmpbuflen, MAINBOARD);
            break;

        default:
            addr = MZ_VID_RAM_ADDR + (y * MZ_VID_MAX_COL) + x;
            copyToZ80(addr, &tmpbuf, tmpbuflen, MAINBOARD);
            break;
    }

    // End variable argument processing.
    va_end(args);
    return;
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
    // Return the value which would have been updated in the interrupt routine.
    return(z80Control.memorySwap == 1);
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
    if(z80Control.ioEvent == 1)
    {
        z80Control.ioEvent = 0;
        printf("I/O:%2x\n", z80Control.ioAddr);
    } else
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

// Helper method to convert a Sharp ASCII filename to one acceptable for use in the FAT32 filesystem.
//
void convertToFAT32FileNameFormat(char *dst)
{
    // Go through the given filename and change any characters which FAT32 doesnt support. This is necessary as the Sharp filenaming convention allows
    // for almost any character!
    //
    for(int idx=0; idx < strlen(dst); idx++)
    {
        if(dst[idx] == '/')
        {
            dst[idx] = '-';
        }
    }
    return;
}

//////////////////////////////////////////////////////////////
// tranZPUter i/f methods for zOS - handling and control    //
//                                                          //
//////////////////////////////////////////////////////////////

// Method to load a BIOS into the tranZPUter and configure for a particular host type.
//
//uint8_t loadBIOS(const char *biosFileName, uint8_t machineMode, uint32_t loadAddr)
uint8_t loadBIOS(const char *biosFileName, uint32_t loadAddr)
{
    // Locals.
    uint8_t result = FR_OK;
   
    // Load up the given BIOS into tranZPUter memory.
    if((result=loadZ80Memory(biosFileName, 0, loadAddr, 0, 0, TRANZPUTER, 1)) != FR_OK)
    {
        printf("Error: Failed to load %s into tranZPUter memory.\n", biosFileName);
    } else
    {
        // Change frequency to default.
        setZ80CPUFrequency(0, 4);

        // Set the machine mode according to BIOS loaded.
       // z80Control.machineMode    = machineMode;

        // setup the IRQ's according to this machine.
        setupIRQ();
    }
    return(result);
}

// A method to perform a reset on the tranZPUter board, it detects the operating mode (ie. hard or soft cpu) and performs
// the necessary initialisation accordingly.
//
void hardResetTranZPUter(void)
{
    // Locals.
    uint8_t  cpuConfig;
printf("Hard Z80 Reset\n");

    // Firstly, a small delay to allow the underlying hardware to initialise.
    //
//    delay(1000);
    
    // Next, ascertain what CPU we are using, soft or hard. If the read value is illegal default to Z80.
    //
    cpuConfig =  readZ80IO(IO_TZ_CPUCFG, TRANZPUTER);
    if(cpuConfig == 0xff) cpuConfig = CPUMODE_IS_Z80;
    cpuConfig &= CPUMODE_IS_SOFT_MASK;
  
    // If this host is an MZ-2000 with distinct boot and run modes, query the hardware to find the current mode as a reset in run mode doesnt require a hard reset.
    if(z80Control.hostType == HW_MZ2000)
    {
        z80Control.iplMode = readZ80IO(IO_TZ_CPLDSTATUS, TRANZPUTER) & 0x01;
printf("IPL Mode:%d\n", z80Control.iplMode);
    }
  
    // Load up the default ROMS in IPL mode (normal mode for all MZ's except MZ-80B/2000/2200/2500).
    if(z80Control.iplMode && !z80Control.blockResetActions)
        loadTranZPUterDefaultROMS(cpuConfig);
   
    // Setup the Interrupts for IORQ and MREQ.
    setupIRQ();

    // Reset any block action request.
    z80Control.blockResetActions = 0;

    return;
}

// Method to load TZFS with an optional BIOS.
//
//FRESULT loadTZFS(char *biosFile, uint8_t machineMode, uint32_t loadAddr)
FRESULT loadTZFS(char *biosFile, uint32_t loadAddr)
{
    // Locals.
    FRESULT    result = 0;

    if(biosFile)
    {
        result = loadBIOS(biosFile, loadAddr);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0,          MZ_UROM_ADDR,        0x1800, 0, TRANZPUTER, 1) != FR_OK))
    {
        printf("Error: Failed to load bank 1 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x1800, MZ_BANKRAM_ADDR+0x10000, 0x1000, 0, TRANZPUTER, 1) != FR_OK))
    {
        printf("Error: Failed to load page 2 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x2800, MZ_BANKRAM_ADDR+0x20000, 0x1000, 0, TRANZPUTER, 1) != FR_OK))
    {
        printf("Error: Failed to load page 3 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    if(!result && (result=loadZ80Memory((const char *)MZ_ROM_TZFS, 0x3800, MZ_BANKRAM_ADDR+0x30000, 0x1000, 0, TRANZPUTER, 1) != FR_OK))
    {
        printf("Error: Failed to load page 4 of %s into tranZPUter memory.\n", MZ_ROM_TZFS);
    }
    return(result);
}

// Method to load the default ROMS into the tranZPUter RAM ready for start.
// If the autoboot flag is set, perform autoboot by wiping the STACK area
// of the SA1510 - this has the effect of JP 00000H.
//
void loadTranZPUterDefaultROMS(uint8_t cpuConfig)
{
    // Locals.
    FRESULT    result = 0;

    // Set CPU and hold soft CPU clock if configured.
    writeZ80IO(IO_TZ_CPUCFG, cpuConfig & CPUMODE_IS_SOFT_MASK, TRANZPUTER);
printf("CPUCONFIG=%02x\n", cpuConfig);
    // Processing according to CPU. EVO needs its own zOS loading.
    switch(cpuConfig)
    {
        case CPUMODE_IS_T80:
        case CPUMODE_IS_Z80:
        default:
            // Now load the default BIOS into memory for the host type.
            switch(z80Control.hostType)
            {
                case HW_MZ700:
                    result = loadTZFS(MZ_ROM_1Z_013A_40C, MZ_MROM_ADDR);
                    break;

                case HW_MZ800:
                    // The MZ-800 uses a composite ROM containing the modified BIOS of the MZ-700 (1Z_013B), the IPL of the MZ-800 (9Z_504M), the CGROM for video text output
                    // and the BASIC IOCS, a common code area for BASIC,
                    //
                    // First we load the MZ-700 compatible BIOS in page 0 to support TZFS when activated.
                    result = loadTZFS(MZ_ROM_1Z_013A_40C, MZ_MROM_ADDR);

                    // Next we load the MZ-800 BIOS in page 7.
                    if(!result)
                    {
                        printf("Loading 1Z_013B\n");
                        //result = loadBIOS(MZ_ROM_1Z_013B,      MZ800, MZ_800_MROM_ADDR);
                        result = loadBIOS(MZ_ROM_1Z_013B,      MZ_800_MROM_ADDR);
                    }

                    // Load up the CGROM into RAM.
                    //printf("Loading CGROM\n");
                    //result = loadBIOS(MZ_ROM_800_CGROM,   MZ800, MZ_800_CGROM_ADDR);

                    // Next we load the modified 9Z-504M - modified to add an option to start TZFS.
                    if(!result)
                    {
                        printf("Loading 9Z_504M\n");
                        //result = loadBIOS(MZ_ROM_9Z_504M,      MZ800,  MZ_800_IPL_ADDR);
                        result = loadBIOS(MZ_ROM_9Z_504M,      MZ_800_IPL_ADDR);
                    }
                   
                    // Finally we load the common IOCS.
                    if(!result)
                    {
                        printf("Loading BASIC IOCS\n");
                        //result = loadBIOS(MZ_ROM_800_IOCS,     MZ800,  MZ_800_IOCS_ADDR);
                        result = loadBIOS(MZ_ROM_800_IOCS,     MZ_800_IOCS_ADDR);
                    }
                    break;

                case HW_MZ80B:
                    //result = loadBIOS(MZ_ROM_MZ80B_IPL,        MZ80B,  MZ_MROM_ADDR);
                    result = loadBIOS(MZ_ROM_MZ80B_IPL,        MZ_MROM_ADDR);
                    break;

                case HW_MZ2000:
                    // Load up the IPL BIOS at least try even if the CGROM failed.
                    printf("Loading IPL\n");
                    //result = loadBIOS(MZ_ROM_MZ2000_IPL_TZPU,  MZ2000, MZ_MROM_ADDR);
                    result = loadBIOS(MZ_ROM_MZ2000_IPL_TZPU,  MZ_MROM_ADDR);
                    if(result != FR_OK)
                    {
                        printf("Error: Failed to load IPL ROM %s into tranZPUter memory.\n", MZ_ROM_MZ2000_IPL_TZPU);
                    } else
                    {
                        // Load up the MZ2000 CGROM into the FPGA.
                        printf("Loading CGROM\n");
                       
                        // Enable the FPGA.
                        writeZ80IO(IO_TZ_CPLDCFG, 0x80 | MODE_VIDEO_MODULE_ENABLED | HWMODE_MZ2000, TRANZPUTER);

                        // Update the CGROM
                        result=loadZ80Memory((const char *)MZ_ROM_MZ2000_CGROM, 0, MZ_VID_CGROM_ADDR, 0x0800, 0, FPGA, 1);
                        if(result != FR_OK)
                        {
                            printf("Error: Failed to load CGROM %s into FPGA video memory.\n", MZ_ROM_MZ2000_CGROM);
                        }
                        
                        // Disable the FPGA.
                        writeZ80IO(IO_TZ_CPLDCFG, 0x80 | HWMODE_MZ2000, TRANZPUTER);
                      
                        // Make sure the video mode in the FPGA is correct (defaults to MZ700).
                        writeZ80IO(IO_TZ_VMCTRL, VMMODE_MZ2000, TRANZPUTER);
                    }
                    break;

                case HW_MZ80A:
                case HW_UNKNOWN:
                default:
                    result = loadTZFS(MZ_ROM_SA1510_40C, MZ_MROM_ADDR);
                    break;
            }
            break;

        case CPUMODE_IS_ZPU_EVO:
            printf("CPU Type = EVO\n");
           
            // Actual load, set the limit to the size of BRAM even though the ROM image will generally be smaller.
            if(!result && (result=loadZ80Memory((const char *)MZ_ROM_ZPU_ZOS, 0,      MZ_ZOS_ADDR,          0x20000, 0, FPGA, 1) != FR_OK))
            {
                printf("Error: Failed to load zOS %s into ZPU FPGA memory.\n", MZ_ROM_ZPU_ZOS);
            }
            break;

        case CPUMODE_IS_EMU_MZ:
            printf("Sharp MZ Series Emulation Active\n");
         
            // Start the emulation.
            EMZRun();
            break;
    }

    // If all was ok loading the roms, complete the startup.
    //
    if(!result)
    {
        // If the CPU is a Z80 then autoboot if necessary.
        //
        if(cpuConfig == CPUMODE_IS_Z80)
        {
            // Check to see if autoboot is needed.
            osControl.tzAutoBoot = testTZFSAutoBoot();
    
            // If autoboot flag set, force a restart to the ROM which will call User ROM startup code.
            if(osControl.tzAutoBoot)
            {
                if(z80Control.hostType == HW_MZ800)
                {
                    // On the MZ-800, once all firmware loaded, set the memory mode to MZ-800 and reset to enable processing on the tranZPUter memory rather than host memory.
                    resetZ80(TZMM_MZ800);
                }
                // MZ-2000 stays in original mode, user will start TZFS via key press in modified IPL bios.
                else if(z80Control.hostType == HW_MZ2000)
                {
                    resetZ80(TZMM_MZ2000);
                }
                else
                {
                    // Set the memory model to BOOT so we can bootstrap TZFS.
                    resetZ80(TZMM_BOOT);
                }
            } else
            {
                setCtrlLatch(TZMM_ORIG);
            }

        }
        // For the T80 we just issue a soft reset.
        else if(cpuConfig == CPUMODE_IS_T80)
        {
            writeZ80IO(IO_TZ_CPUCFG, cpuConfig | CPUMODE_CLK_EN | CPUMODE_RESET_CPU, TRANZPUTER);
        }
        // For the ZPU Evo we just issue a soft reset. Code could be consolidated with above IF statement but intentionally seperate
        // as the Evo design progresses and has differing requirements.
        else if(cpuConfig == CPUMODE_IS_ZPU_EVO)
        {
            writeZ80IO(IO_TZ_CPUCFG, cpuConfig | CPUMODE_CLK_EN | CPUMODE_RESET_CPU, TRANZPUTER);
        }
        // For the Sharp MZ Series Emulations we just issue a soft reset so that the T80 starts processing the ROM contents.
        else if(cpuConfig == CPUMODE_IS_EMU_MZ)
        {
printf("Writing CPUCFG:%02x\n", cpuConfig | CPUMODE_CLK_EN | CPUMODE_RESET_CPU);
            writeZ80IO(IO_TZ_CPUCFG, cpuConfig | CPUMODE_CLK_EN | CPUMODE_RESET_CPU, TRANZPUTER);
        } else
        {
printf("HERE 10\n");
            // Set the memory model to BOOT so we can bootstrap TZFS.
            resetZ80(TZMM_ORIG);
        }

        // No longer need refresh on the mainboard as all operations are in static RAM or FPGA.
        z80Control.disableRefresh = 1;
    } else
    {
        printf("Firmware load failure\n");
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

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the load command to leave the BUS under the tranZPUter control for multiple transactions.
    // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
    // may occur.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the tranZPUter bus as the service register is inside the CPLD.
        result = reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, TRANZPUTER);

        // If successful, setup the control pins for upload mode.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Setup the pins to perform a write operation.
            //
            setZ80Direction(WRITE);
           
            // Setup the control latch to the required starting configuration.
            writeCtrlLatch(z80Control.curCtrlLatch);
        }
    } else
    {
        // See if the bus needs changing.
        //
        if(z80Control.ctrlMode == MAINBOARD_ACCESS)
        {
            reqZ80BusChange(TRANZPUTER_ACCESS);
        }
    }

    // Request the tranZPUter bus.
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Update the memory location.
        //
        result=writeZ80Memory(z80Control.svcControlAddr+TZSVC_RESULT_OFFSET, status, TRANZPUTER);

        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Restore the control latch to its original configuration.
            //
            writeCtrlLatch(z80Control.runCtrlLatch);
            releaseZ80();
        }

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
void svcSetDefaults(enum FILE_TYPE type)
{
    // Set according to the type of file were working with.
    //
    switch(type)
    {
        case CAS:
            // If there is no directory path, use the inbuilt default.
            if(svcControl.directory[0] == '\0')
            {
                strcpy((char *)svcControl.directory, TZSVC_DEFAULT_CAS_DIR);
            }
            // If there is no wildcard matching, use default.
            if(svcControl.wildcard[0] == '\0')
            {
                strcpy((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD);
            }
            break;

        case BAS:
            // If there is no directory path, use the inbuilt default.
            if(svcControl.directory[0] == '\0')
            {
                strcpy((char *)svcControl.directory, TZSVC_DEFAULT_BAS_DIR);
            }
            // If there is no wildcard matching, use default.
            if(svcControl.wildcard[0] == '\0')
            {
                strcpy((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD);
            }
            break;

        case MZF:
        default:
            // If there is no directory path, use the inbuilt default.
            if(svcControl.directory[0] == '\0')
            {
                strcpy((char *)svcControl.directory, TZSVC_DEFAULT_MZF_DIR);
            }
   
            // If there is no wildcard matching, use default.
            if(svcControl.wildcard[0] == '\0')
            {
                strcpy((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD);
            }
            break;
    }
    return;
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
//         fileName    - MZF fileName, either CR or NUL terminated or TZSVC_FILENAME_SIZE chars long.
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
            if((np - fileName) == TZSVC_FILENAME_SIZE) return 1;

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
    } while (infinite && nc != 0x00 && nc != 0x0d && (np - fileName) < TZSVC_FILENAME_SIZE);

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
uint8_t svcReadDir(uint8_t mode, enum FILE_TYPE type)
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
            svcReadDir(TZSVC_CLOSE, type);

        // Setup the defaults
        //
        svcSetDefaults(type);

        // Open the directory.
        result = f_opendir(&dirFp, (char *)&svcControl.directory);
        if(result == FR_OK)
        {
            // Reentrant call to actually read the data.
            //
            dirOpen   = 1; 
            dirSector = 0;
            result    = (FRESULT)svcReadDir(TZSVC_NEXT, type);
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
                result=svcReadDir(TZSVC_OPEN, type);
            }
            if(!result)
            {
                // Now get the sector by advancement.
                for(uint8_t idx=dirSector; idx < svcControl.dirSector && result == FR_OK; idx++)
                {
                    result=svcReadDir(TZSVC_NEXT, type);
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
            
                // Check to see if this is a valid file for the given type.
                const char *ext = strrchr(fno.fname, '.');
                if(type == MZF && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_MZF_EXT) != 0))
                    continue;
                if(type == BAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_BAS_EXT) != 0))
                    continue;
                if(type == CAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_CAS_EXT) != 0))
                    continue;
                if(type == ALL && !ext)
                    continue;
    
                // Sharp files need special handling, the file needs te opened and the Sharp filename read out, this is then returned as the filename.
                //
                if(type == MZF)
                {
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
                } else
                {
                    // Check to see if the file matches any given wildcard.
                    //
                    if(matchFileWithWildcard((char *)&svcControl.wildcard, fno.fname, 0, 0))
                    {
                        // If the type is ALL FORMATTED then output a truncated directory entry formatted for display.
                        //
                        if(type == ALLFMT)
                        {
                            // Get the address of the file extension.
                            char *ext = strrchr(fno.fname, '.');
                          
                            // Although not as efficient, maintain the use of the Sharp MZF header for normal directory filenames just use the extra space for increased filename size.
                            if(ext)
                            {
                                // Although not as efficient, maintain the use of the Sharp MZF header for normal directory filenames just use the extra space for increased filename size.
                                if((ext - fno.fname) > TZSVC_LONG_FMT_FNAME_SIZE-5)
                                {
                                    fno.fname[TZSVC_LONG_FMT_FNAME_SIZE-6] = '*';       // Place a '*' to show the filename was truncated.
                                    fno.fname[TZSVC_LONG_FMT_FNAME_SIZE-5] = 0x00;
                                }
                                *ext = 0x00;
                                ext++;

                                sprintf((char *)&dirBlock->dirEnt[idx].fileName, "%-*s.%3s", TZSVC_LONG_FMT_FNAME_SIZE-5, fno.fname, ext);
                            } else
                            {
                                fno.fname[TZSVC_LONG_FMT_FNAME_SIZE] =  0x00;
                                strncpy((char *)&dirBlock->dirEnt[idx].fileName, fno.fname, TZSVC_LONG_FMT_FNAME_SIZE);
                            }
                        } else
                        // All other types just output the filename upto the limit truncating as necessary.
                        {
                            fno.fname[TZSVC_LONG_FNAME_SIZE] =  0x00;
                            strncpy((char *)&dirBlock->dirEnt[idx].fileName, fno.fname, TZSVC_LONG_FNAME_SIZE);
                        }

                        // Set the attribute in the directory record to indicate this is a valid record.
                        dirBlock->dirEnt[idx].attr = 0xff;

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


// A method to find a file either using a Sharp MZ80A name, a standard filename or a number assigned to a directory listing.
// For the Sharp MZ80A it is a bit long winded as each file that matches the filename specification has to be opened and the MZF header filename 
// has to be checked. For standard files it is just a matter of matching the name. For both types a short cut is a number which is a files position
// in a directory obtained from a previous directory listing.
//
uint8_t svcFindFile(char *file, char *searchFile, uint8_t searchNo, enum FILE_TYPE type)
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
    svcSetDefaults(type);

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

            // Check to see if this is a valid file for the given type.
            const char *ext = strrchr(fno.fname, '.');
            if(type == MZF && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_MZF_EXT) != 0))
                continue;
            if(type == BAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_BAS_EXT) != 0))
                continue;
            if(type == CAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_CAS_EXT) != 0))
                continue;
            if(type == ALL && !ext)
                continue;
    
            // Sharp files need special handling, the file needs te opened and the Sharp filename read out, this is then used as the filename for matching.
            //
            if(type == MZF)
            {
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
            } else
            {
                // Check to see if the file matches any given wildcard. If we dont have a match loop to next directory entry.
                //
                if(matchFileWithWildcard((char *)&svcControl.wildcard, (char *)&fno.fname, 0, 0))
                {
                    // If a filename has been given, see if this file matches it.
                    if(searchFile != NULL)
                    {
                        // Check to see if the file matches the name given with wildcard expansion if needed.
                        //
                        if(matchFileWithWildcard(searchFile, (char *)&fno.fname, 0, 0))
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
uint8_t svcReadDirCache(uint8_t mode, enum FILE_TYPE type)
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
    svcSetDefaults(type);
   
    // Need to refresh cache directory?
    if(!osControl.dirMap.valid || strcasecmp((const char *)svcControl.directory, osControl.dirMap.directory) != 0 || osControl.dirMap.type != type)
        result=svcCacheDir((const char *)svcControl.directory, svcControl.fileType, 0);

    // If there is no cache revert to direct directory read.
    //
    if(!osControl.dirMap.valid || result)
    {
        result = svcReadDir(mode, type);
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
            result    = (FRESULT)svcReadDirCache(TZSVC_NEXT, type);
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
                if(matchFileWithWildcard((char *)&svcControl.wildcard, type == MZF ? (char *)&osControl.dirMap.mzfFile[dirEntry]->mzfHeader.fileName : (char *)osControl.dirMap.sdFileName[dirEntry], 0, 0))
                {
                    // Sharp files we copy the whole header into the entry.
                    //
                    if(type == MZF)
                    {
                        // Valid so store and find next entry.
                        //
                        memcpy((char *)&dirBlock->dirEnt[idx], (char *)&osControl.dirMap.mzfFile[dirEntry]->mzfHeader, TZSVC_CMPHDR_SIZE);
                    } else
                    {
                        // If the type is ALL FORMATTED then output a truncated directory entry formatted for display.
                        //
                        if(type == ALLFMT)
                        {
                            // Duplicate the cache entry, formatting is destructuve and less time neeed to duplicate than repair.
                            char *fname = strdup((char *)osControl.dirMap.sdFileName[dirEntry]);
                            if(fname != NULL)
                            {
                                // Get the address of the file extension.
                                char *ext = strrchr(fname, '.');
                          
                                // Although not as efficient, maintain the use of the Sharp MZF header for normal directory filenames just use the extra space for increased filename size.
                                if(ext)
                                {
                                    // Although not as efficient, maintain the use of the Sharp MZF header for normal directory filenames just use the extra space for increased filename size.
                                    if((ext - fname) > TZSVC_LONG_FMT_FNAME_SIZE-5)
                                    {
                                        fname[TZSVC_LONG_FMT_FNAME_SIZE-6] = '*';       // Place a '*' to show the filename was truncated.
                                        fname[TZSVC_LONG_FMT_FNAME_SIZE-5] = 0x00;
                                    }
                                    *ext = 0x00;
                                    ext++;
                                    sprintf((char *)&dirBlock->dirEnt[idx].fileName, "%-*s.%3s", TZSVC_LONG_FMT_FNAME_SIZE-5, fname, ext);
                                } else
                                {
                                    fname[TZSVC_LONG_FMT_FNAME_SIZE] = 0x00;
                                    strncpy((char *)&dirBlock->dirEnt[idx].fileName, fname, TZSVC_LONG_FMT_FNAME_SIZE);
                                }

                                // Release the duplicate memory.
                                free(fname);
                            } else
                            {
                                printf("Out of memory duplicating directory filename.\n");
                            }
                        } else
                        // All other types just output the filename upto the limit truncating as necessary.
                        {
                            osControl.dirMap.sdFileName[dirEntry][TZSVC_LONG_FNAME_SIZE] = 0x00;
                            strncpy((char *)&dirBlock->dirEnt[idx].fileName, (char *)osControl.dirMap.sdFileName[dirEntry],  TZSVC_LONG_FNAME_SIZE);
                        }

                        // Set the attribute in the directory record to indicate this is a valid record.
                        dirBlock->dirEnt[idx].attr = 0xff;
                    } 
                    idx++;
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
uint8_t svcFindFileCache(char *file, char *searchFile, uint8_t searchNo, enum FILE_TYPE type)
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
        result = svcFindFile(file, searchFile, searchNo, type);
    } else
    {
        // If we are searching on file number and there is no filter in place, see if it is valid and exit with data.
        if(searchNo != 0xFF && strcmp((char *)svcControl.wildcard, TZSVC_DEFAULT_WILDCARD) == 0)
        {
            if(searchNo < osControl.dirMap.entries && osControl.dirMap.mzfFile[searchNo]) // <- this is the same for standard files as mzfFile is a union for sdFileName
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
                if(matchFileWithWildcard((char *)&svcControl.wildcard, type == MZF ? (char *)&osControl.dirMap.mzfFile[idx]->mzfHeader.fileName : (char *)&osControl.dirMap.sdFileName[idx], 0, 0))
                {
                    // If a filename has been given, see if this file matches it.
                    if(searchFile != NULL)
                    {
                        // Check to see if the file matches the name given with wildcard expansion if needed.
                        //
                        if(matchFileWithWildcard(searchFile, type == MZF ? (char *)&osControl.dirMap.mzfFile[idx]->mzfHeader.fileName : (char *)&osControl.dirMap.sdFileName[idx], 0, 0))
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
            sprintf(file, "0:\\%s\\%s", osControl.dirMap.directory, type == MZF ? osControl.dirMap.mzfFile[idx]->sdFileName : osControl.dirMap.sdFileName[idx]);
        }
    }

    // Return 1 if file was found, 0 for all other cases.
    //
    return(result == FR_OK ? (found == 0 ? 0 : 1) : 0);
}

// Method to build up a cache of all the files on the SD card in a given directory along with any mapping to Sharp MZ80A headers if required.
// For Sharp MZ80A files this involves scanning each file to extract the MZF header and creating a map.
//
uint8_t svcCacheDir(const char *directory, enum FILE_TYPE type, uint8_t force)
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
    if(force == 0 && osControl.dirMap.valid && strcasecmp(directory, osControl.dirMap.directory) == 0 && osControl.dirMap.type == type)
        return(1);

    // Invalidate the map and free existing memory incase of errors.
    //
    osControl.dirMap.valid = 0;
    for(uint8_t idx=0; idx < osControl.dirMap.entries; idx++)
    {
        if(osControl.dirMap.type == MZF && osControl.dirMap.mzfFile[idx])
        {
            free(osControl.dirMap.mzfFile[idx]->sdFileName);
            free(osControl.dirMap.mzfFile[idx]);
            osControl.dirMap.mzfFile[idx] = 0;
        } else
        {
            free(osControl.dirMap.sdFileName[idx]);
            osControl.dirMap.sdFileName[idx] = 0;
        }
    }
    osControl.dirMap.entries = 0;
    osControl.dirMap.type = MZF;

    // Open the directory and extract all files.
    result = f_opendir(&dirFp, directory);
    if(result == FR_OK)
    {
        fileNo = 0;
        
        do {
            // Read an SD directory entry. If reading Sharp MZ80A files then open the returned SD file so that we can to read out the important MZF data for name matching.
            //
            result = f_readdir(&dirFp, &fno);

            // If an error occurs or we are at the end of the directory listing close the sector and pass back.
            if(result != FR_OK || fno.fname[0] == 0) break;

            // Check to see if this is a valid file for the given type.
            const char *ext = strrchr(fno.fname, '.');
            if(type == MZF && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_MZF_EXT) != 0))
                continue;
            if(type == BAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_BAS_EXT) != 0))
                continue;
            if(type == CAS && (!ext || strcasecmp(++ext, TZSVC_DEFAULT_CAS_EXT) != 0))
                continue;
            if(type == ALL && !ext)
                continue;

            // Sharp files need special handling, the file needs te opened and the Sharp filename read out, this is then used in the cache.
            //
            if(type == MZF)
            {
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
                    osControl.dirMap.mzfFile[fileNo] = (t_sharpToSDMap *)malloc(sizeof(t_sharpToSDMap));
                    if(osControl.dirMap.mzfFile[fileNo] != NULL)
                        osControl.dirMap.mzfFile[fileNo]->sdFileName = (uint8_t *)malloc(strlen(fno.fname)+1);

                    if(osControl.dirMap.mzfFile[fileNo] == NULL || osControl.dirMap.mzfFile[fileNo]->sdFileName == NULL)
                    {
                        printf("Out of memory cacheing directory:%s\n", directory);
                        for(uint8_t idx=0; idx <= fileNo; idx++)
                        {
                            if(osControl.dirMap.mzfFile[idx])
                            {
                                free(osControl.dirMap.mzfFile[idx]->sdFileName);
                                free(osControl.dirMap.mzfFile[idx]);
                                osControl.dirMap.mzfFile[idx] = 0;
                            }
                        }
                        result = FR_NOT_ENOUGH_CORE;
                    } else
                    {
                        // Copy in details into this maps node.
                        strcpy((char *)osControl.dirMap.mzfFile[fileNo]->sdFileName, fno.fname);
                        memcpy((char *)&osControl.dirMap.mzfFile[fileNo]->mzfHeader, (char *)&dirEnt, TZSVC_CMPHDR_SIZE);
                        fileNo++;
                    }
                }
            } else
            {
                // Cache this entry. The SD filename is dynamically allocated as it's size can be upto 255 characters for LFN names. The SD service header is based
                // on Sharp names which are fixed at 17 characters which shouldnt be a problem, just truncate the name as there is no support for longer names yet!
                osControl.dirMap.sdFileName[fileNo] = (uint8_t *)malloc(strlen(fno.fname)+1);

                if(osControl.dirMap.sdFileName[fileNo] == NULL)
                {
                    printf("Out of memory cacheing directory:%s\n", directory);
                    for(uint8_t idx=0; idx <= fileNo; idx++)
                    {
                        if(osControl.dirMap.sdFileName[idx])
                        {
                            free(osControl.dirMap.sdFileName[idx]);
                            osControl.dirMap.sdFileName[idx] = 0;
                        }
                    }
                    result = FR_NOT_ENOUGH_CORE;
                } else
                {
                    // Copy in details into this maps node.
                    strcpy((char *)osControl.dirMap.sdFileName[fileNo], fno.fname);
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
        // Save the filetype.
        osControl.dirMap.type = type;
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to open a file for reading and return requested sectors.
//
uint8_t svcReadFile(uint8_t mode, enum FILE_TYPE type)
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
            svcReadFile(TZSVC_CLOSE, type);

        // Setup the defaults
        //
        svcSetDefaults(type);

        if(type == CAS || type == BAS)
        {
            // Build the full filename from what has been provided.
            // Cassette and basic images, create filename as they are not cached.
            sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, svcControl.filename, (type == MZF ? TZSVC_DEFAULT_MZF_EXT : type == CAS ? TZSVC_DEFAULT_CAS_EXT : TZSVC_DEFAULT_BAS_EXT));
        }

        // Find the file using the given file number or file name.
        //
        if( (type == MZF && (svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo, type))) || type == CAS || type == BAS )
        {
            // Open the file, fqfn has the FQFN of the correct file on the SD drive.
            result = f_open(&File, fqfn, FA_OPEN_EXISTING | FA_READ);

            if(result == FR_OK)
            {
                // Reentrant call to actually read the data.
                //
                fileOpen   = 1; 
                fileSector = 0;
                result     = (FRESULT)svcReadFile(TZSVC_NEXT, type);
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

            // Place number of bytes read into the record for the Z80 to know where EOF is.
            //
            svcControl.loadSize = readSize;
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

// Method to create a file for writing and on subsequent calls write the data into that file.
//
uint8_t svcWriteFile(uint8_t mode, enum FILE_TYPE type)
{
    // Locals - dont use global as this is a seperate thread.
    //
    static FIL        File;
    static uint8_t    fileOpen   = 0;         // Seperate flag as their is no public way to validate that File is open and valid, the method in FatFS is private for this functionality.
    static uint8_t    fileSector = 0;         // Sector number being read.
    FRESULT           result    = FR_OK;
    unsigned int      readSize;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Request to createopen? Validate that we dont already have an open file and the create the file.
    if(mode == TZSVC_OPEN)
    {
        // Close if previously open.
        if(fileOpen == 1)
            svcWriteFile(TZSVC_CLOSE, type);

        // Setup the defaults
        //
        svcSetDefaults(type);
       
        // Build the full filename from what has been provided.
        sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, svcControl.filename, (type == MZF ? TZSVC_DEFAULT_MZF_EXT : type == CAS ? TZSVC_DEFAULT_CAS_EXT : TZSVC_DEFAULT_BAS_EXT));

        // Create the file, fqfn has the FQFN of the correct file on the SD drive.
        result = f_open(&File, fqfn, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);

        // If the file was opened, set the flag to enable writing.
        if(!result)
            fileOpen = 1;
    }
  
    // Write the next sector into the file.
    else if(mode == TZSVC_NEXT && fileOpen == 1)
    {
        // If the Z80 is requesting a non sequential sector then seek to the correct location prior to the write.
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
            // Write the required sector.
            result = f_write(&File, (char *)&svcControl.sector, svcControl.saveSize, &readSize);
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
    } else
    {
        printf("WARNING: svcWriteFile called with unknown mode:%d\n", mode);
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to load a file from SD directly into the tranZPUter memory.
//
uint8_t svcLoadFile(enum FILE_TYPE type)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    uint32_t          bytesRead;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Setup the defaults
    //
    svcSetDefaults(type);

    // MZF and MZF Headers are handled with their own methods as it involves looking into the file to determine the name and details.
    //
    if(type == MZF || type == MZFHDR)
    {
        // Tidy up the MZF filename suitable for file matching.
//        for(int idx=0; idx < MZF_FILENAME_LEN; idx++)
//        {
//            svcControl.filename[idx] = (svcControl.filename[idx] == 0x0d ? 0x00 : svcControl.filename[idx]);
//        }
//
        
        // Find the file using the given file number or file name.
        //
        if(svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo, MZF))
        {
            // Call method to load an MZF file.
            result = loadMZFZ80Memory(fqfn, (svcControl.loadAddr == 0xFFFF ? 0xFFFFFFFF : svcControl.loadAddr), 0, (type == MZFHDR ? 1 : 0), (svcControl.memTarget == 0 ? TRANZPUTER : MAINBOARD), 1);

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
    } else
    // Cassette images are for NASCOM/Microsoft Basic. The files are in NASCOM format so the header needs to be skipped.
    // BAS files are for human readable BASIC files.
    if(type == CAS || type == BAS)
    {
        // Build the full filename from what has been provided.
        sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, svcControl.filename, type == CAS ? TZSVC_DEFAULT_CAS_EXT : TZSVC_DEFAULT_BAS_EXT);

        // For the tokenised cassette, skip the header and load directly into the memory location provided. The load size given is the maximum size of file
        // and the loadZ80Memory method will only load upto the given size.
        //
        if((result=loadZ80Memory((const char *)fqfn, 0, svcControl.loadAddr, svcControl.loadSize, &bytesRead, TRANZPUTER, 1)) != FR_OK)
        {
            printf("Error: Failed to load CAS:%s into tranZPUter memory.\n", fqfn);
        } else
        {
            // Return the size of load in the service control record.
            svcControl.loadSize = (uint16_t) bytesRead;
        }
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to save a file from tranZPUter memory directly into a file on the SD card.
//
uint8_t svcSaveFile(enum FILE_TYPE type)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>
    char              asciiFileName[TZSVC_FILENAME_SIZE+1];
    uint32_t          addrOffset = SRAM_BANK0_ADDR;
    t_svcDirEnt       mzfHeader;

    // Setup the defaults
    //
    svcSetDefaults(type);
   
    // MZF are handled with their own methods as it involves looking into the file to determine the name and details.
    //
    if(type == MZF)
    {
        // Setup bank in which to load the header/data. Default is bank 0, different host hardware uses different banks.
        //
        if(svcControl.memTarget == 0 && z80Control.hostType == HW_MZ800)
        {
            addrOffset = SRAM_BANK6_ADDR;
        } 
        else if(svcControl.memTarget == 0 && z80Control.hostType == HW_MZ2000)
        {
            addrOffset = SRAM_BANK6_ADDR;
        }
      
        // Get the MZF header which contains the details of the file to save.
        copyFromZ80((uint8_t *)&mzfHeader, addrOffset + MZ_CMT_ADDR, MZF_HEADER_SIZE, TRANZPUTER);

        // Need to extract and convert the filename to create a file.
        //
        convertSharpFilenameToAscii(asciiFileName, (char *)mzfHeader.fileName, TZSVC_FILENAME_SIZE);

        // Build filename.
        //
        sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, asciiFileName, TZSVC_DEFAULT_MZF_EXT);

        // Convert any non-FAT32 characters prior to save.
        convertToFAT32FileNameFormat(fqfn);

        // Call the main method to save memory passing in the correct MZF details and header.
        result = saveZ80Memory(fqfn, (mzfHeader.loadAddr < MZ_CMT_DEFAULT_LOAD_ADDR-3 ? addrOffset+MZ_CMT_DEFAULT_LOAD_ADDR : addrOffset+mzfHeader.loadAddr), mzfHeader.fileSize, &mzfHeader, (svcControl.memTarget == 0 ? TRANZPUTER : MAINBOARD));
    } else
    // Cassette images are for NASCOM/Microsoft Basic. The files are in NASCOM format so the header needs to be skipped.
    // BAS files are for human readable BASIC files.
    if(type == CAS || type == BAS)
    {
        // Build the full filename from what has been provided.
        sprintf(fqfn, "0:\\%s\\%s.%s", svcControl.directory, svcControl.filename, type == CAS ? TZSVC_DEFAULT_CAS_EXT : TZSVC_DEFAULT_BAS_EXT);

        // Convert any non-FAT32 characters prior to save.
        convertToFAT32FileNameFormat(fqfn);
      
        // Call the main method to save memory passing in the correct details from the service record.
        result = saveZ80Memory(fqfn, svcControl.saveAddr, svcControl.saveSize, NULL, TRANZPUTER);
    }

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to erase a file on the SD card.
//
uint8_t svcEraseFile(enum FILE_TYPE type)
{
    // Locals - dont use global as this is a seperate thread.
    //
    FRESULT           result    = FR_OK;
    char              fqfn[FF_LFN_BUF + 13];  // 0:\12345678\<filename>

    // Setup the defaults
    //
    svcSetDefaults(MZF);

    // MZF are handled with their own methods as it involves looking into the file to determine the name and details.
    //
    if(type == MZF)
    {
        // Find the file using the given file number or file name.
        //
        if(svcFindFileCache(fqfn, (char *)&svcControl.filename, svcControl.fileNo, type))
        {
            // Call method to load an MZF file.
            result = f_unlink(fqfn);
        } else
        {
            result = FR_NO_FILE;
        }
    } else
    // Cassette images are for NASCOM/Microsoft Basic. The files are in NASCOM format so the header needs to be skipped.
    if(type == CAS)
    {
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
        //printf("Writing offset=%08lx\n", fileOffset);
        //for(uint16_t idx=0; idx < SECTOR_SIZE; idx++)
        //{
        //    printf("%02x ", svcControl.sector[idx]);
        //    if(idx % 32 == 0)
        //        printf("\n");
        //}
        //printf("\n");

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

// Method to read 1 sector from the SD disk directly, raw, and save it in the provided external RAM buffer.
// Inputs:
//     svcControl.vDriveNo    = Drive number to read from.
//     svcControl.sectorNo    = SD LBA Sector to read from. NB. This is 32 bit and overwrites fileNo/fileType to extend 16bit sectorNo to 32bit.
// Outputs:
//     svcControl.sector      = 1 sector of data read from the SD card.
//
uint8_t svcReadSDRaw(void)
{
    // Locals.
    FRESULT           result    = FR_OK;

    // Sanity checks.
    //
    if(svcControl.vDriveNo >= 3)
    {
        printf("svcReadSDRaw: Illegal input values: vDriveNo=%d\n", svcControl.vDriveNo);
        return(TZSVC_STATUS_FILE_ERROR);
    }
//printf("disk_read, drive=%d, sector=%08lx, sector number=%08lx\n", svcControl.vDriveNo, svcControl.sector, svcControl.sectorLBA);
    // Extract LBA sector number and perform physical read.
    result = disk_read(svcControl.vDriveNo, svcControl.sector, svcControl.sectorLBA, 1); 

    // Return values: 0 - Success : maps to TZSVC_STATUS_OK
    //                1 - Fail    : maps to TZSVC_STATUS_FILE_ERROR
    return(result == FR_OK ? TZSVC_STATUS_OK : TZSVC_STATUS_FILE_ERROR);
}

// Method to write 1 sector to the SD disk directly, raw, from the provided external RAM buffer.
// Inputs:
//     svcControl.vDriveNo    = Drive number to write to.
//     svcControl.sectorNo    = SD LBA Sector to write to. NB. This is 32 bit and overwrites fileNo/fileType to extend 16bit sectorNo to 32bit.
//     svcControl.sector      = 1 sector of data to write for writing onto the SD card.
//
uint8_t svcWriteSDRaw(void)
{
    // Locals.
    FRESULT           result    = FR_OK;

    // Sanity checks.
    //
    if(svcControl.vDriveNo >= 3)
    {
        printf("svcWriteSDRaw: Illegal input values: vDriveNo=%d\n", svcControl.vDriveNo);
        return(TZSVC_STATUS_FILE_ERROR);
    }

//printf("disk_write, drive=%d, sector=%08lx, sector number=%08lx\n", svcControl.vDriveNo, svcControl.sector, svcControl.sectorLBA);
    // Extract LBA sector number and perform physical write.
    result = disk_write(svcControl.vDriveNo, svcControl.sector, svcControl.sectorLBA, 1); 

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
    uint8_t  cpuConfig  = readZ80IO(IO_TZ_CPUCFG, TRANZPUTER) & CPUMODE_IS_SOFT_MASK;

    if(cpuConfig == CPUMODE_IS_Z80 || cpuConfig == CPUMODE_IS_T80)
    {
        // If in CPM mode then set the service address accordingly.
        if(memoryMode == TZMM_CPM || memoryMode == TZMM_CPM2)
            addr = TZSVC_CMD_STRUCT_ADDR_CPM;
      
        // If in MZ700 mode then set the service address accordingly.
        if(memoryMode == TZMM_MZ700_0 || memoryMode == TZMM_MZ700_2 || memoryMode == TZMM_MZ700_3 || memoryMode == TZMM_MZ700_4)
            addr = TZSVC_CMD_STRUCT_ADDR_MZ700;

        // If in MZ2000 mode then the service address differs according to boot state.
        if(memoryMode == TZMM_MZ2000)
        {
            // Get the machine state and set the service address accordingly.
            if(z80Control.iplMode)
            {
                addr = TZSVC_CMD_STRUCT_ADDR_MZ2000_IPL;
            }
            else
            {
                addr = TZSVC_CMD_STRUCT_ADDR_MZ2000_NST;
            }
        }
    } else
    {
        // zOS
        addr = TZSVC_CMD_STRUCT_ADDR_ZOS;
    }
printf("getServiceAddr:%02x,%02x,%02x,%01x,%08lx,%02x\n", z80Control.runCtrlLatch, z80Control.curCtrlLatch, memoryMode, cpuConfig, addr, svcControl.cmd);
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
    uint8_t    doExit          = 0;
    uint8_t    doReset         = 0;
    uint32_t   actualFreq;
    uint32_t   copySize        = TZSVC_CMD_STRUCT_SIZE;

    // If an emulation is active then a service request is an interrupt, call the emulation handler indicating an interrupt occurred.
    if(z80Control.emuMZactive)
    {
        EMZservice(1);
    }
  
    // A request requires multiple transactions on the Z80 bus, so to avoid multiple request/ack cycles, we gain access then hold it until the end of the request.
    //
    else if(reqZ80Bus(DEFAULT_BUSREQ_TIMEOUT) == 0)
    {
        z80Control.holdZ80 = 1;

        // If this host is an MZ-2000 with distinct boot and run modes, query the hardware to find the current mode.
        if(z80Control.hostType == HW_MZ2000)
        {
            z80Control.iplMode = readZ80IO(IO_TZ_CPLDSTATUS, TRANZPUTER) & 0x01;
        }

        // Update the service control record address according to memory mode.
        //
        z80Control.svcControlAddr = getServiceAddr();

        // Get the command and associated parameters.
        copyFromZ80((uint8_t *)&svcControl, z80Control.svcControlAddr, TZSVC_CMD_SIZE, z80Control.svcControlAddr > TZ_MAX_Z80_MEM ? FPGA : TRANZPUTER);

        // Need to get the remainder of the data for the write operations.
        if(svcControl.cmd == TZSVC_CMD_WRITEFILE || svcControl.cmd == TZSVC_CMD_NEXTWRITEFILE || svcControl.cmd == TZSVC_CMD_WRITESDDRIVE || svcControl.cmd == TZSVC_CMD_SD_WRITESECTOR)
        {
            copyFromZ80((uint8_t *)&svcControl.sector, z80Control.svcControlAddr+TZSVC_CMD_SIZE, TZSVC_SECTOR_SIZE, z80Control.svcControlAddr > TZ_MAX_Z80_MEM ? FPGA : TRANZPUTER);
        }

        // Check this is a valid request.
        if(svcControl.result == TZSVC_STATUS_REQUEST)
        {
            // Set status to processing. Z80 can use this to decide if the K64F received its request after a given period of time.
            setZ80SvcStatus(TZSVC_STATUS_PROCESSING);

            // Action according to command given.
            //
            switch(svcControl.cmd)
            {
                // Open a directory stream and return the first block.
                case TZSVC_CMD_READDIR:
                    status=svcReadDirCache(TZSVC_OPEN, svcControl.fileType);
                    break;

                // Read the next block in the directory stream.
                case TZSVC_CMD_NEXTDIR:
                    status=svcReadDirCache(TZSVC_NEXT, svcControl.fileType);
                    break;

                // Open a file stream and return the first block.
                case TZSVC_CMD_READFILE:
                    status=svcReadFile(TZSVC_OPEN, svcControl.fileType);
                    break;

                // Read the next block in the file stream.
                case TZSVC_CMD_NEXTREADFILE:
                    status=svcReadFile(TZSVC_NEXT, svcControl.fileType);
                    break;

                // Create a file for data write.
                case TZSVC_CMD_WRITEFILE:
                    status=svcWriteFile(TZSVC_OPEN, svcControl.fileType);
                    break;
                      
                // Write a block of data to an open file.
                case TZSVC_CMD_NEXTWRITEFILE:
                    status=svcWriteFile(TZSVC_NEXT, svcControl.fileType);
                    break;

                // Close an open dir/file.
                case TZSVC_CMD_CLOSE:
                    svcReadDir(TZSVC_CLOSE, svcControl.fileType);
                    svcReadFile(TZSVC_CLOSE, svcControl.fileType);
                    svcWriteFile(TZSVC_CLOSE, svcControl.fileType);
                   
                    // Only need to copy the command section back to the Z80 for a close operation.
                    //
                    copySize = TZSVC_CMD_SIZE;
                    break;

                // Load a file directly into target memory.
                case TZSVC_CMD_LOADFILE:
                    status=svcLoadFile(svcControl.fileType);
                    break;
                  
                // Save a file directly from target memory.
                case TZSVC_CMD_SAVEFILE:
                    status=svcSaveFile(svcControl.fileType);
                    refreshCacheDir = 1;
                    break;
                  
                // Erase a file from the SD Card.
                case TZSVC_CMD_ERASEFILE:
                    status=svcEraseFile(svcControl.fileType);
                    refreshCacheDir = 1;
                    break;

                // Change active directory. Do this immediately to validate the directory name given.
                case TZSVC_CMD_CHANGEDIR:
                    status=svcCacheDir((const char *)svcControl.directory, svcControl.fileType, 0);
                    break;

                // Load the 40 column version of the default host bios into memory.
                case TZSVC_CMD_LOAD40ABIOS:
                    //loadBIOS(MZ_ROM_SA1510_40C, MZ80A, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_SA1510_40C, MZ_MROM_ADDR);
                   
                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ80A)
                    {
                        // Change frequency to match Sharp MZ-80A
                        setZ80CPUFrequency(MZ_80A_CPU_FREQ, 1);
                    }
                    break;
                  
                // Load the 80 column version of the default host bios into memory.
                case TZSVC_CMD_LOAD80ABIOS:
                    //loadBIOS(MZ_ROM_SA1510_80C, MZ80A, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_SA1510_80C, MZ_MROM_ADDR);
                   
                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ80A)
                    {
                        // Change frequency to match Sharp MZ-80A
                        setZ80CPUFrequency(MZ_80A_CPU_FREQ, 1);
                    }
                    break;
                   
                // Load the 40 column MZ700 1Z-013A bios into memory for compatibility switch.
                case TZSVC_CMD_LOAD700BIOS40:
                    //loadBIOS(MZ_ROM_1Z_013A_40C, MZ700, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_1Z_013A_40C, MZ_MROM_ADDR);

                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ700)
                    {
                        // Change frequency to match Sharp MZ-700
                        setZ80CPUFrequency(MZ_700_CPU_FREQ, 1);
                    }
                    break;

                // Load the 80 column MZ700 1Z-013A bios into memory for compatibility switch.
                case TZSVC_CMD_LOAD700BIOS80:
                    //loadBIOS(MZ_ROM_1Z_013A_80C, MZ700, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_1Z_013A_80C, MZ_MROM_ADDR);
                  
                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ700)
                    {
                        // Change frequency to match Sharp MZ-700
                        setZ80CPUFrequency(MZ_700_CPU_FREQ, 1);
                    }
                    break;
                  
                // Load the MZ800 9Z-504M bios into memory for compatibility switch.
                case TZSVC_CMD_LOAD800BIOS:
                    //loadBIOS(MZ_ROM_9Z_504M,  MZ800, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_9Z_504M,  MZ_MROM_ADDR);

                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ800)
                    {
                        // Change frequency to match Sharp MZ-800
                        setZ80CPUFrequency(MZ_800_CPU_FREQ, 1);
                    }
                    break;
                   
                // Load the MZ-80B IPL ROM into memory for compatibility switch.
                case TZSVC_CMD_LOAD80BIPL:
                    //loadBIOS(MZ_ROM_MZ80B_IPL, MZ80B, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_MZ80B_IPL, MZ_MROM_ADDR);
                  
                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ80B)
                    {
                        // Change frequency to match Sharp MZ-80B
                        setZ80CPUFrequency(MZ_80B_CPU_FREQ, 1);
                    }
                    break;
                   
                // Load the MZ-2000 IPL ROM into memory for compatibility switch.
                case TZSVC_CMD_LOAD2000IPL:
                    //loadBIOS(MZ_ROM_MZ2000_IPL, MZ2000, MZ_MROM_ADDR);
                    loadBIOS(MZ_ROM_MZ2000_IPL, MZ_MROM_ADDR);
                  
                    // Set the frequency of the CPU if we are emulating the hardware.
                    if(z80Control.hostType != HW_MZ2000)
                    {
                        // Change frequency to match Sharp MZ-80B
                        setZ80CPUFrequency(MZ_2000_CPU_FREQ, 1);
                    }
                    break;

                // Load TZFS upon request. This service is for the MZ-80B/MZ-2000 which dont have a monitor BIOS installed and TZFS isnt loaded upon reset but rather through user request.
                case TZSVC_CMD_LOADTZFS:
                    switch(z80Control.hostType)
                    {
                        case HW_MZ80B:
                            break;

                        case HW_MZ2000:
                            // Load TZFS and a modified 1Z-013A MZ700 Monitor to provide an interactive TZFS session during IPL mode.
                            if(loadTZFS(MZ_ROM_1Z_013A_2000, MZ_MROM_ADDR) == FR_OK)
                            {
                                // Request a cold IPL start with no ROM load, this will invoke the loaded TZFS as the BOOT IPL.
                                z80Control.blockResetActions = 1;
                                writeZ80IO(IO_TZ_PPICTL, 0x06, MAINBOARD);
                            }
                            break;

                        // Default, ie. not a valid host, is to do nothing.
                        default:
                            break;
                    }
                    break;

                // Load the CPM CCP+BDOS from file into the address given.
                case TZSVC_CMD_LOADBDOS:
                    if((status=loadZ80Memory((const char *)osControl.lastFile, MZF_HEADER_SIZE, svcControl.loadAddr+0x40000, svcControl.loadSize, 0, TRANZPUTER, 1)) != FR_OK)
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
                    actualFreq = setZ80CPUFrequency(svcControl.cpuFreq * 1000, 1);
                    svcControl.cpuFreq = (uint16_t)(actualFreq / 1000);
                    break;

                // Switch the hardware to select the hard Z80 CPU. This involves the switch then a reset procedure.
                //
                case TZSVC_CMD_CPU_SETZ80:
                    //printf("Switch to Z80\n");
                    // Switch to hard Z80.
                    writeZ80IO(IO_TZ_CPUCFG, CPUMODE_SET_Z80, TRANZPUTER);

                    // Ensure the video is returned to default.
                    writeZ80IO(IO_TZ_VMCTRL, VMMODE_MZ700, TRANZPUTER);
                   
                    // Set a reset event so that the ROMS are reloaded and the Z80 set.
                    z80Control.resetEvent = 1;
                    break;

                // Switch the hardware to select the soft T80 CPU. This involves the switch.
                //
                case TZSVC_CMD_CPU_SETT80:
                    //printf("Switch to T80\n");
                    // Switch to soft T80 cpu.
                    writeZ80IO(IO_TZ_CPUCFG, CPUMODE_SET_T80, TRANZPUTER);
                  
                    // Set a reset event so that the ROMS are reloaded and a reset occurs.
                    z80Control.resetEvent = 1;
                    break;
                   
                // Switch the hardware to select the soft ZPU Evolution CPU. This involves the switch.
                //
                case TZSVC_CMD_CPU_SETZPUEVO:
                    //printf("Switch to EVO\n");
                    // Switch to soft T80 cpu.
                    writeZ80IO(IO_TZ_CPUCFG, CPUMODE_SET_ZPU_EVO, TRANZPUTER);
                  
                    // Set a reset event so that the ROMS are reloaded and a reset occurs.
                    z80Control.resetEvent = 1;
                    break;

                // Switch the hardware to select the Sharp MZ Series Emulations and load up the settings accordingly.
                // 
                case TZSVC_CMD_EMU_SETMZ80K:
                case TZSVC_CMD_EMU_SETMZ80C:
                case TZSVC_CMD_EMU_SETMZ1200:
                case TZSVC_CMD_EMU_SETMZ80A:
                case TZSVC_CMD_EMU_SETMZ700:
                case TZSVC_CMD_EMU_SETMZ1500:
                case TZSVC_CMD_EMU_SETMZ800:
                case TZSVC_CMD_EMU_SETMZ80B:
                case TZSVC_CMD_EMU_SETMZ2000:
                case TZSVC_CMD_EMU_SETMZ2200:
                case TZSVC_CMD_EMU_SETMZ2500:
                    // Initialise the emulation and OSD.
                    if(!EMZInit(z80Control.hostType, (uint8_t)(svcControl.cmd - TZSVC_CMD_EMU_SETMZ80K)))
                    {
                        // Switch to the emulation CPU (T80).
                        writeZ80IO(IO_TZ_CPUCFG, CPUMODE_SET_EMU_MZ, TRANZPUTER);

                        // Enable the emulator service methods to handle User OSD Menu, tape/floppy loading etc.
                        z80Control.emuMZactive = 1;

                        // Set a reset event so that the ROMS are reloaded and a reset occurs.
                        z80Control.resetEvent = 1;
                    } else
                    {
                        printf("Failed to init EmuMZ core\n");
                    }
                    break;

                // Raw direct access to the SD card. This request initialises the requested disk - if shared with this I/O processor then dont perform
                // physical initialisation.
                case TZSVC_CMD_SD_DISKINIT:
                    // No logic needed as the K64F initialises the underlying drive and the host accesses, via a mapping table, the 2-> partitions.
                    break;

                // Raw read access to the SD card.
                //
                case TZSVC_CMD_SD_READSECTOR:
//printf("Read Raw Start\n");
                    status=svcReadSDRaw();
//printf("Read Raw Exit\n");
                    break;

                // Raw write access to the SD card.
                //
                case TZSVC_CMD_SD_WRITESECTOR:
//printf("Write Raw Start\n");
                    status=svcWriteSDRaw();
//printf("Write Raw Exit\n");
                    break;

                // Command to exit from TZFS and return machine to original mode.
                case TZSVC_CMD_EXIT:
                    // Disable secondary frequency.
                    setZ80CPUFrequency(0, 4);

                    // Clear the stack and monitor variable area as DRAM wont have been refreshed so will contain garbage.
                    fillZ80Memory(MZ_MROM_STACK_ADDR, MZ_MROM_STACK_SIZE, 0x00, MAINBOARD);

                    // Set to refresh mode just in case the I/O processor performs any future action in silent mode!
                    z80Control.disableRefresh = 0;
                   
                    // Set a flag to perform the physical reset. This is necessary as the service routine holds the Z80 bus and there is an issue with the Z80 if you reset whilst
                    // the bus is being held (it sometimes misses the first instruction).
                    doExit = 1;
                    break;

                default:
                    printf("WARNING: Unrecognised command:%02x\n", svcControl.cmd);
                    status=TZSVC_STATUS_BAD_CMD;
                    break;
            }
        } else
        {
            // Indicate error condition.
            status=TZSVC_STATUS_BAD_REQ;
        }
  
        // Update the status in the service control record then copy it across to the Z80.
        //
        svcControl.result = status;
        copyToZ80(z80Control.svcControlAddr, (uint8_t *)&svcControl, copySize, z80Control.svcControlAddr > TZ_MAX_Z80_MEM ? FPGA : TRANZPUTER);

        // Need to refresh the directory? Do this at the end of the routine so the Sharp MZ80A isnt held up.
        if(refreshCacheDir)
            svcCacheDir((const char *)svcControl.directory, svcControl.fileType, 1);

        // Finally, return the memory management mode to original and release the Z80 Bus.
        writeCtrlLatch(z80Control.runCtrlLatch);
        z80Control.holdZ80 = 0;
        releaseZ80();
   
        // If the doExit flag is set it means the CPU should be set to original memory mode and reset.
        if(doExit == 1)
        {
            // Now reset the machine so everything starts as power on.
            resetZ80(TZMM_ORIG);
            z80Control.runCtrlLatch = TZMM_ORIG;

            // Disable the interrupts so no further service processing.
            disableIRQ();
        }
        // If the doReset force a reset of the CPU.
        if(doReset == 1)
        {
            // Reset the machine requested by earlier action.
            resetZ80(TZMM_BOOT);
            z80Control.runCtrlLatch = TZMM_BOOT;
        }
    } else
    {
        printf("Failed to request access to the Z80 Bus, cannot service request.\n");
    }
    return;
}

// Method for periodic task scheduling and execution. When the main process (used to be thread) is idle this method is invoked
// to service any non-event driven tasks.
//
void TZPUservice(void)
{
    // If the emulator is active, pass control to its periodic servicing task routine.
    //
    if(z80Control.emuMZactive == 1)
    {
        // Specify the service is non interrupt driven (arg = 0).
        EMZservice(0);
    }
}

// Method to test if the autoboot TZFS flag file exists on the SD card. If the file exists, set the autoboot flag.
//
uint8_t testTZFSAutoBoot(void)
{
    // Locals.
    uint8_t   result = 0;
    FIL       File;

    // Detect if the autoboot tranZPUter TZFS flag is set. This is a file called TZFSBOOT in the SD root directory.
    if(!f_open(&File, TZFS_AUTOBOOT_FLAG, FA_OPEN_EXISTING | FA_READ))
    {
        result = 1;
        f_close(&File);
    }

    return(result);
}

// Method to identify the type of host the tranZPUter SW is running on. Originally it was only the MZ80A but the scope has expanded
// to the MZ-700,  MZ-80B, MZ-800, MZ-2000 and potentially a plethora of other machines in the future.
//
void setHost(uint8_t printInfo)
{
    // Locals.
    uint8_t   cpldInfo;
    uint8_t   cpuInfo;

    // If the Z80 is in RUN mode, request the bus.
    // This mechanism allows for the load command to leave the BUS under the tranZPUter control for multiple transactions.
    // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
    // may occur.
    //
    if(z80Control.ctrlMode == Z80_RUN)
    {
        // Request the tranZPUter bus as the information registers are in the CPLD.
        reqTranZPUterBus(DEFAULT_BUSREQ_TIMEOUT, TRANZPUTER);
    } else
    {
        // See if the bus needs changing.
        //
        reqZ80BusChange(TRANZPUTER_ACCESS);
    }

    // If we have bus control, complete the task,
    //
    if( z80Control.ctrlMode != Z80_RUN )
    {
        // Setup the pins to perform an IO read operation.
        //
        setZ80Direction(READ);
       
        // Copy the ID value from the CPLD information register so that we can identify the host type.
        //
        cpldInfo = inZ80IO(IO_TZ_CPLDINFO);

        // Copy the ID value from the CPU information register to identify FPGA capabilities.
        //
        cpuInfo = inZ80IO(IO_TZ_CPUINFO);
      
        // Release the bus if it is not being held for further transations.
        //
        if(z80Control.holdZ80 == 0 && z80Control.ctrlMode != Z80_RUN)
        {
            // Release the bus to continue.
            //
            releaseZ80();
        }
    } else
    {
        if(printInfo) printf("Failed to access tranZPUter bus to read CPLDINFO, setting to UNKNOWN\n");
        cpldInfo = 0xFF;
    }

    // Setup the control variable to indicate type of host. This will affect processing of interrupts, BIOS loads etc.
    //
    switch(cpldInfo & 0x17)
    {
        case HW_MZ700:
            if(!printInfo)
            {
                z80Control.hostType       = HW_MZ700;
         //       z80Control.machineMode    = MZ700;
            } else
                printf("Host Type: MZ-700\n");
            break;

        case HW_MZ800:
            if(!printInfo)
            {
                z80Control.hostType       = HW_MZ800;
         //       z80Control.machineMode    = MZ800;
            } else
                printf("Host Type: MZ-800\n");
            break;

        case HW_MZ80B:
            if(!printInfo)
            {
                z80Control.hostType       = HW_MZ80B;
         //       z80Control.machineMode    = MZ80B;
            } else
                printf("Host Type: MZ-80B\n");
            break;

        case HW_MZ2000:
            if(!printInfo)
            {
                z80Control.hostType       = HW_MZ2000;
         //       z80Control.machineMode    = MZ2000;
            } else
                printf("Host Type: MZ-2000\n");
            break;

        case HW_MZ80A:
        case HW_MZ80K:
        case HW_MZ80C:
        case HW_MZ1200:
            if(!printInfo)
            {
                z80Control.hostType       = HW_MZ80A;
         //       z80Control.machineMode    = MZ80A;
            } else
                printf("Host Type: MZ-80A\n");
            break;

        default:
            z80Control.hostType           = HW_UNKNOWN;
         //   z80Control.machineMode        = UNKNOWN;
            break;
    }

    // Extract the CPLD version for any programming nuances required in older versions.
    z80Control.cpldVersion = (cpldInfo & CPLD_VERSION) >> 5;

    // Report on video hardware.
    //
    if(cpldInfo & VIDEO_FPGA)
    {
        if(printInfo)
            printf("FPGA video hardware detected.\n");
    }

    // Store and report on CPU hardware.
    //
    if(cpuInfo & CPUMODE_IS_SOFT_AVAIL)
    {
        z80Control.softcpuInfo = cpuInfo & CPUMODE_IS_SOFT_MASK;

        if(printInfo)
        {
            if(z80Control.softcpuInfo == CPUMODE_IS_Z80)
                printf("No soft CPU hardware, hard Z80 only.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_T80)
                printf("Soft T80 available.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_ZPU_EVO)
                printf("Soft ZPU available.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_EMU_MZ)
                printf("Sharp MZ Series FPGA Emulation available.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_BBB)
                printf("Unspecified soft BBB CPU available.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_CCC)
                printf("Unspecified soft CCC CPU available.\n");
            if(z80Control.softcpuInfo & CPUMODE_IS_DDD)
                printf("Unspecified soft DDD CPU available.\n");
        }
    }

    // Report on CPLD version.
    //
    if(printInfo)
        printf("CPLD Version: %d\n", z80Control.cpldVersion);

    return;
}

// Method to configure the hardware and events to operate the tranZPUter SW upgrade.
//
void setupTranZPUter(uint8_t stage, char *VERSION, char *VERSION_DATE)
{
    // Functionality can be split according to boot up stage of the processor using the 'stage' variable.
    //
    if(stage == 0)
    {
        // Setup the pins to default mode and configure IRQ's.
        setupZ80Pins(0, &systick_millis_count);
  
        // Check to see which host the tranZPUter is installed in.
        setHost(0);

        // Once the external Z80 interface has been enabled, make machine dependent interactions whilst we are initialising.
        switch(z80Control.hostType)
        {
            case HW_MZ2000:
                // Initialise the hardware and present a sign on message while the tranZPUter initialises.
                writeZ80IO(IO_TZ_PPICTL,  0x82, MAINBOARD);
                writeZ80IO(IO_TZ_PPIC,    0x58, MAINBOARD);
                writeZ80IO(IO_TZ_PPIA,    0xF7, MAINBOARD);
                writeZ80IO(IO_TZ_PPIA,    0xFF, MAINBOARD);
                writeZ80IO(IO_TZ_PIOCTLA, 0x0F, MAINBOARD);
                writeZ80IO(IO_TZ_PIOCTLB, 0xCF, MAINBOARD);
                writeZ80IO(IO_TZ_PIOCTLB, 0xFF, MAINBOARD);

                // Sign on message whilst we initialise tranZPUter hardware.
                clsHost();
                printfHost(4, 4, "tranZPUter SW-700 v1.3");
                printfHost(4, 7, "CPLD fw: v%d", z80Control.cpldVersion);
                printfHost(4, 7, "zOS:     %s (%s)", VERSION, VERSION_DATE);
                if(z80Control.softcpuInfo & CPUMODE_IS_EMU_MZ)
                    printfHost(4, 8, "emuMZ:   %s (%s)", EMZGetVersion(), EMZGetVersionDate());
                break;

            case HW_MZ700:
                // Sign on message whilst we initialise tranZPUter hardware.
                clsHost();
                printfHost(4, 4, "tranZPUter SW-700 v1.3");
                printfHost(4, 6, "CPLD fw: v%d", z80Control.cpldVersion);
                printfHost(4, 7, "zOS:     %s (%s)", VERSION, VERSION_DATE);
                if(z80Control.softcpuInfo & CPUMODE_IS_EMU_MZ)
                    printfHost(4, 8, "emuMZ:   %s (%s)", EMZGetVersion(), EMZGetVersionDate());
                break;

            default:
                break;
        }

        // Reset the interrupts so they take into account the current host rather than the default.
        setupIRQ();
  
        // Start off by clearing active memory banks, the AS6C4008 chip holds random values at power on.
        fillZ80Memory(0x000000, TZ_MAX_Z80_MEM, 0x00, TRANZPUTER);
    }
    else if(stage == 1)
    {
        // Release the bus to start loaded ROM.
        writeZ80IO(IO_TZ_CPLDCMD, CPLD_RELEASE_HOST_BUS, MAINBOARD);
    }
    else if(stage == 8)
    {
        // Make any final machine specific configurations prior to setup end.
        switch(z80Control.hostType)
        {
            case HW_MZ2000:
             // writeZ80IO(IO_TZ_PPICTL, 0x06, MAINBOARD);
                break;

            default:
                break;
        }

        // Print out host information.
        setHost(1);
    }
    // No SD found, so run with original ROM.
    else if(stage == 9)
    {
        // Set to original mode so the onboard ROM can be executed.
        //
        writeZ80IO(IO_TZ_CTRLLATCH, TZMM_ORIG, MAINBOARD);
       
        // Release the bus to start the code in the onboard ROM.
        writeZ80IO(IO_TZ_CPLDCMD, CPLD_RELEASE_HOST_BUS, MAINBOARD);
    }
   
    // Release Z80 if it is being held.
    if(z80Control.holdZ80 != 0)
    {
        z80Control.holdZ80 = 0;
        releaseZ80();
    }

    return;
}

// Test routine. Add code here to test an item within the kernel. Anything accessing hardware generally has to call the kernel as it doesnt have real access.
//
void testRoutine(void)
{
    // Locals.
    //
    uint32_t startTime = *ms;

    printf("Testing BUSRQ/BUSACK mechanism...\n");

    for(uint8_t idx=0; idx < 100; idx++)
    {
        // and continue for the timeout period. If no response in the timeout period then the tranZPUter board/CPLD have locked up.
        do {
            // Set BUSRQ low which sets the Z80 BUSRQ low.
            pinLow(CTL_BUSRQ);

            // Wait 1ms or until BUSACK goes low.
            for(uint32_t timer=*ms; timer == *ms && pinGet(CTL_BUSACK););

        } while(((*ms - startTime) < DEFAULT_BUSREQ_TIMEOUT && pinGet(CTL_BUSACK)));

        if((*ms - startTime) >= DEFAULT_BUSREQ_TIMEOUT)
        {
            printf("BUSACK didnt go low on Idx=%d\n", idx);
        }

        startTime = *ms;
        while(((*ms - startTime) < 500));
      
        // Finally release the Z80 by deasserting the CTL_BUSRQ signal.
        pinHigh(CTL_BUSRQ);
   
        // Final check, wait for the CPLD to signal the bus has been released.
        startTime = *ms;
        while(((*ms - startTime) < DEFAULT_BUSREQ_TIMEOUT && !pinGet(CTL_BUSACK)));

        if((*ms - startTime) >= DEFAULT_BUSREQ_TIMEOUT)
        {
            printf("BUSACK didnt go high on Idx=%d\n", idx);
        }
    }
    printf("Test done.\n");
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
    uint8_t  MBSEL = 0;
    uint8_t  ZBUSACK = 0;
    uint8_t  MBCLK = 0;
    uint8_t  HALT = 0;
    uint8_t  BUSACK = 0;

    setupZ80Pins(0, NULL);

    printf("Z80 Bus Signals:\r\n");
    while(1)
    {
        ADDR  = (pinGet(Z80_A23) & 0x1) << 23;
        ADDR  = (pinGet(Z80_A22) & 0x1) << 22;
        ADDR  = (pinGet(Z80_A21) & 0x1) << 21;
        ADDR  = (pinGet(Z80_A20) & 0x1) << 20;
        ADDR  = (pinGet(Z80_A19) & 0x1) << 19;
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
        MBSEL=pinGet(CTL_MBSEL);
        ZBUSACK=pinGet(Z80_BUSACK);
        MBCLK=pinGet(MB_SYSCLK);
        HALT=pinGet(CTL_HALT);
        BUSACK=pinGet(CTL_BUSACK);
    
        printf("\rADDR=%06lx %08x %3s %3s %3s %3s %3s %3s %2s %4s %4s %2s %2s %3s %3s %4s %4s", ADDR, DATA,
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
