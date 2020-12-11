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
#include <sharpmz.h>

#ifndef __APP__        // Protected methods which should only reside in the kernel.

// Global scope variables used within the zOS kernel.
//
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


int mzPrintChar(char c, FILE *stream)
{
    static uint32_t dispMemAddr = 0xE80000;
    static row = 0, col = 0;

    if(c == '\n')
    {
        if(++row > 24) row = 24;
        col = 0;
    }

    dispMemAddr = 0xE81000 + (row * 40) + col;
    *(uint8_t *)(dispMemAddr) = (char)dispCodeMap[c].dispCode;
    if(++col > 39)
    {
        if(++row > 24) row = 24;
        col = 0;
    }
    return(0);
}

int mzGetChar(FILE *stream)
{
    return(-1);
}


//////////////////////////////////////////////////////////////
// End of Sharp MZ i/f methods for zOS                      //
//////////////////////////////////////////////////////////////
#endif // Protected methods which reside in the kernel.

#if defined __APP__
#endif // __APP__

#ifdef __cplusplus
}
  #endif
