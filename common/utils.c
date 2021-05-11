////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            utils.c
// Created:         January 2019
// Author(s):       Philip Smart
// Description:     ZPU boottime utilities.
//                  A set of utilities to be used by ZPU applications which can assume that most C
//                  functionality is available, such as printf.
//
// Credits:         
// Copyright:       (c) 2019 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written.
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

#if defined __K64F__
  #include <stdio.h>
  #include <stdint.h>
  #include <stdlib.h>
  #include <string.h>
  #include "usb_serial.h"
  #include "k64f_soc.h"
  #include <../libraries/include/stdmisc.h>
#elif defined __ZPU__
  #include <stdint.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdmisc.h>
  #include "uart.h"
  #include "zpu_soc.h"
 #if defined __SHARPMZ__
  #include "sharpmz.h"
 #endif
#elif defined __M68K__
  #include <stdio.h>
//  #include <stdint.h>
//  #include <stdlib.h>
  #include <string.h>
#endif

#if defined(__SD_CARD__)
  #include "ff.h"
#endif
#include "utils.h"

#if defined(__ZPU__)
// Functions, in the absense of printf, to output a value as hex.
//  Nibble :- a single digit 0-f
void printnibble(uint8_t c)
{
    c&=0xf;
    if (c>9)
        fputc(c+'a'-10, stdout);
    else
        fputc(c+'0', stdout);
}
// Byte: 8 bits represented by 2 digits, <0-f><0-f>
void printhexbyte(uint8_t c)
{
    printnibble(c>>4);
    printnibble(c);
}
// Half Word: 16 bits represented by 4 digits.
void printhex(uint32_t c)
{
    printhexbyte((uint8_t)(c>>8));
    printhexbyte((uint8_t)(c));
}
// Word: 32 bits represented by 8 digits.
void printdhex(uint32_t c)
{
    printhexbyte((uint8_t)(c>>24));
    printhexbyte((uint8_t)(c>>16));
    printhexbyte((uint8_t)(c>>8));
    printhexbyte((uint8_t)(c));
}
#endif
#if defined(ABCD)
// Function to setup the CRC polynomial table prior to use.
//
static unsigned int crc32table[256];
unsigned int crc32_init(void)
{
    int j;
    unsigned int byte, crc, mask;

    for(byte = 0; byte <= 255; byte++)
    {
        crc = byte;
        for (j = 7; j >= 0; j--)
        {
           mask = -(crc & 1);
           crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        crc32table[byte] = crc;
    }

    // Starting value for CRC calculation.
    //
    return 0xFFFFFFFF;
}

// Function to add a word into the CRC sum.
//
unsigned int crc32_addword(unsigned int crc_in, unsigned int word)
{
   crc_in = (crc_in >> 8) ^ crc32table[(crc_in ^ ((word >> 24)&0xFF)) & 0xFF];
   crc_in = (crc_in >> 8) ^ crc32table[(crc_in ^ ((word >> 16)&0xFF)) & 0xFF];
   crc_in = (crc_in >> 8) ^ crc32table[(crc_in ^ ((word >>  8)&0xFF)) & 0xFF];
   crc_in = (crc_in >> 8) ^ crc32table[(crc_in ^ (word        &0xFF)) & 0xFF];

   return crc_in;
}
#endif

// Function to read a 32bit word from the active serial port.
//
unsigned int get_dword(void)
{
    unsigned int temp = 0;
    int idx;

    for(idx=0; idx < 4; idx++)
    {
        temp = (temp << 8) | (unsigned int)getKey(1);
    }

    return(temp);
}

// Method to parse a buffer and return a pointer to the first string encountered. The string will be null terminated
// and the callers pointer advanced to the next argument.
char *getStrParam(char **ptr)
{
    char    *paramptr = (*ptr);
    char    *spaceptr;
    uint8_t inQuotes = 0;

    // If no parameter available, exit.
    if(*ptr == 0x0)
        return NULL;

    // Find the end of the command and terminate it.
    while(*paramptr == ' ') paramptr++;
    if(*paramptr == '"') { paramptr++; inQuotes=1; }
    spaceptr = paramptr;
    if(inQuotes == 1)
    {
        while(*spaceptr != '"' && *spaceptr != 0x00) spaceptr++;
    } else
    {
        while(*spaceptr != ' ' && *spaceptr != 0x00) spaceptr++;
    }
    if(*spaceptr == ' ' || *spaceptr == '"') { (*spaceptr) = 0x00; spaceptr++; }

    // Callers pointer is advanced to the next argument or end of string.
    (*ptr) = spaceptr;

    // Return the pointer to the start of the argument.
    return(paramptr);
}

// Method to parse a buffer and extract a 32bit unsigned integer. The callers pointer is then
// advanced to the next argument.
// 0 is returned if any error encountered and the callers pointed remains unchanged.
uint32_t getUintParam(char **ptr)
{
    uint32_t result;

    // If no parameter available, exit.
    if(*ptr == 0x0 || !uxatoi(ptr, &result))
        return 0;

    return(result);
}

// Method to set the RTC.
//
uint8_t rtcSet(RTC *time)
{
    // Validate the incoming data.
    //
    if(time->month < 1 || time->month > 12) return(1);
    if(time->day < 1 || time->day > 31) return(2);
    if(time->hour > 23) return(3);
    if(time->min > 59) return(4);
    if(time->sec > 59) return(5);
    if(time->msec > 999) return(6);
    if(time->usec > 999) return(7);

  #if defined __ZPU__
    // Stop the clock, update the values and restart.
    RTC_CONTROL      = RTC_CTRL_HALT;
    RTC_YEAR         = time->year;
    RTC_MONTH        = time->month;
    RTC_DAY          = time->day;
    RTC_HOUR         = time->hour;
    RTC_MINUTE       = time->min;
    RTC_SECOND       = time->sec;
    RTC_MILLISECONDS = time->msec;
    RTC_MICROSECONDS = time->usec;
    RTC_CONTROL      = 0;
  #endif
  #if defined __K64F__
    dbg_puts("RTC Not yet implemented.\n");
  #endif

    // Success.
    return(0);
}

// Method to read from the RTC.
//
void rtcGet(RTC *time)
{
  #if defined __ZPU__
    // Read directly into the static RTC record.
    RTC_CONTROL   = RTC_CTRL_HALT;
    time->year    = RTC_YEAR;
    time->month   = RTC_MONTH;
    time->day     = RTC_DAY;
    time->hour    = RTC_HOUR;
    time->min     = RTC_MINUTE;
    time->sec     = RTC_SECOND;
    time->msec    = RTC_MILLISECONDS;
    time->usec    = RTC_MICROSECONDS;
    RTC_CONTROL   = 0;
  #endif
  #if defined __K64F__
    dbg_puts("RTC Not yet implemented.\n");
  #endif

    printf("%d/%d/%d %d:%d:%d.%d%d\n",time->year, time->month, time->day, time->hour, time->min, time->sec, time->msec, time->usec);
}

// Method to get a key from the input device (serial port/keyboard) according to the host for which this code is compiled.
// Input:  mode = 0 - No blocking, standard keyboard.
//                1 - blocking, standard keyboard.
//                2 - No blocking, ansi keyboard.       -- Sharp MZ build only.
//                3 - blocking, ansi keyboard.          -- Sharp MZ build only.
// Return: -1 = no key pressed.
//        ASCII value when key pressed.
//
int8_t getKey(uint8_t mode)
{
    int8_t  keyIn;

    do {
    #if defined __K64F__
        int usb_serial_getchar(void);
        keyIn = usb_serial_getchar();
    #elif defined __ZPU__
      #if defined __SHARPMZ__

    	keyIn = mzGetKey(mode);
      #else
        if(mode == 1 || mode == 3)
        {
            keyIn = getserial();
        } else
        {
            keyIn = getserial_nonblocking();
        }
      #endif
    #elif defined __M68K__
    #else
      #error "Target CPU not defined, use __ZPU__ or __K64F__"
    #endif
    } while(keyIn == -1 && (mode == 1 || mode == 3));

    return(keyIn);
}

// Method as above but non blocking.
//
int8_t getKeyNonBlocking(void)
{
    return(getKey(0));
}

#ifdef __cplusplus
    }
#endif
