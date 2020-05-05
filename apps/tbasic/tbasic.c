/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tbasic.c
// Created:         April 2029
// Author(s):       Miskatino (Miskatino Basic), RodionGork (TinyBasic), Philip Smart (zOS)
// Description:     Standalone App for the zOS/ZPU test application to provide a basic interpreter.
//                  This program implements a loadable appliation which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//                  Basic is a very useable language to make quick tests and thus I have ported the 
//                  TinyBasic/Miskatino fork to the ZPU/K64F and enhanced accordingly.
//
// Credits:         
// Copyright:       (C) Miskatino MiskatinoBasic -> 2018, (C) RodionGork TinyBasic-> 2017
//                  (c) 2019-2020 Philip Smart <philip.smart@net2net.org> zOS/ZPUTA Basic
//
// History:         April 2020   - Ported the Miskatino version to work on the ZPU and K64F, updatesdto
//                                 function with the K64F processor and zOS.
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
  #include <stdlib.h>
  #include <string.h>
  #include "k64f_soc.h"
  #define uint32_t __uint32_t
  #define uint16_t __uint16_t
  #define uint8_t  __uint8_t
  #define int32_t  __int32_t
  #define int16_t  __int16_t
  #define int8_t   __int8_t
#elif defined(__ZPU__)
  #include <stdint.h>
  #include "zpu_soc.h"
  #include <stdlib.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include "diskio.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "xprintf.h"
#include "utils.h"
#include "basic_main.h"
#include "basic_utils.h"
#include "basic_textual.h"
#include "basic_tokens.h"
#include "basic_extern.h"
//
#if defined __ZPUTA__
  #include "zputa_app.h"
#elif defined __ZOS__
  #include "zOS_app.h"
#else
  #error OS not defined, use __ZPUTA__ or __ZOS__      
#endif
//
#include "tbasic.h"


//#include <stdio.h>
//#include <stdlib.h>
//#include <signal.h>
//#include <termios.h>
//#include <unistd.h>
//#include <poll.h>
//#include <time.h>

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.0"
#define VERSION_DATE "10/04/2020"
#define APP_NAME     "TBASIC"

#define VARS_SPACE_SIZE 512
#define DATA_SPACE_SIZE 4096
#define LINE_SIZE 80

char extraCmdArgCnt[] = {2, 2, 0};
char extraFuncArgCnt[] = {1, 2};

static char* commonStrings = CONST_COMMON_STRINGS;
static char * parsingErrors = CONST_PARSING_ERRORS;
static short doExit = 0;

char dataSpace[DATA_SPACE_SIZE];
char lineSpace[LINE_SIZE * 3];

static FIL fCurrent;
static short idCurrent = 0;

short sysGetc(void)
{
    short keyIn;

    #if defined __K64F__
        keyIn = (short)usb_serial_getchar();
    #elif defined __ZPU__
        keyIn = (short)getserial_nonblocking();
    #else
        #error "Target CPU not defined, use __ZPU__ or __K64F__"
    #endif

    return keyIn;
}

void sysPutc(char c)
{
    xputc(c);
}

void sysEcho(char c)
{
    if (c == '\b') {
        sysPutc(c);
        sysPutc(' ');
    }
    sysPutc(c);
}

void sysPoke(unsigned long addr, uchar value)
{
    dataSpace[addr] = value;
}

uchar sysPeek(unsigned long addr)
{
    return dataSpace[addr];
}

numeric sysMillis(numeric div)
{
    numeric milliSec;

  #if defined __ZPU__
    milliSec = (numeric)TIMER_MILLISECONDS_UP;
  #elif defined __K64F__
    milliSec = (numeric)*G->millis;
  #else
    #error "Target CPU not defined, use __ZPU__ or __K64F__"
  #endif    
    return milliSec/div;
}

char translateInput(short c)
{
    if (c == -1) {
        c = 0;
    }
    return (char) (c & 0xFF);
}

void outputConstStr(char strId, char index, char* w)
{
    char* s;
    switch (strId)
    {
        case ID_COMMON_STRINGS:
            s = commonStrings;
            break;
        case ID_PARSING_ERRORS:
            s = parsingErrors;
            break;
        default:
            return;
    }
    while (index > 0)
    {
        while (*s++ != '\n')
        {
        }
        index -= 1;
    }
    while (*s != '\n')
    {
        if (w)
        {
            *(w++) = (*s++);
        } else 
        {
            sysPutc(*s++);
        }
    }
    if (w) 
    {
        *w = 0;
    }
}

static numeric power(numeric base, numeric exp)
{
    return exp < 1 ? 1 : base * power(base, exp - 1);
}

short extraCommandByHash(numeric h)
{
    switch (h)
    {
        case 0x036F: // POKE
            return CMD_EXTRA + 0;
        case 0x019C: // PIN - just prints argument values for test
            return CMD_EXTRA + 1;
        case 0x031A: // QUIT
            return CMD_EXTRA + 2;
        default:
            return -1;
    }
}

short extraFunctionByHash(numeric h) 
{
    switch (h)
    {
        case 0x0355: // PEEK
            return 0;
        case 0x06FC: // POWER - for test purpose
            return 1;
        default:
            return -1;
    }
}

void extraCommand(char cmd, numeric args[])
{
    switch (cmd)
    {
        case 0:
            sysPoke(args[0], args[1]);
            break;
        case 1:
            xprintf("PIN: %d,%d\n", args[0], args[1]);
            break;
        case 2:
            doExit = 1;
            break;
    }
}

numeric extraFunction(char cmd, numeric args[])
{
    switch (cmd)
    {
        case 0:
            return sysPeek(args[0]);
        case 1:
            return power(args[1], args[0]);
    }
    return 0;
}

char storageOperation(void* data, short size)
{
    UINT bytes;
    FRESULT fr;
    char fname[] = "TBASIC_0.dat";
    fname[7] += idCurrent;

    if (data == NULL)
    {
        if (idCurrent != 0)
        {
            f_close(&fCurrent);
        }
        idCurrent = 0;
        if (size != 0) 
        {
            idCurrent = abs(size);
            if(size > 0)
            {
                fr = f_open(&fCurrent, fname, FA_CREATE_ALWAYS | FA_WRITE);
                xprintf("Writing \"%s\"\n", fname);
            } else
            {
                fr = f_open(&fCurrent, fname, FA_OPEN_EXISTING | FA_READ);
                xprintf("Reading \"%s\"\n", fname);
            }

            if (fr != FR_OK)
            {
                printFSCode(fr);
                idCurrent = 0;
                return 0;
            }
        }
        return 1;
    }
    if (size > 0)
    {
        fr = f_write(&fCurrent, data, size, &bytes);
    } else 
    {
        fr = f_read(&fCurrent, data, -size, &bytes);
    }
    if(fr) { printFSCode(fr); return 0; } else { return 1; }
}

// Main entry and start point of a zOS/ZPUTA Application. Only 2 parameters are catered for and a 32bit return code, additional parameters can be added by changing the appcrt0.s
// startup code to add them to the stack prior to app() call.
//
// Return code for the ZPU is saved in _memreg by the C compiler, this is transferred to _memreg in zOS/ZPUTA in appcrt0.s prior to return.
// The K64F ARM processor uses the standard register passing conventions, return code is stored in R0.
//
uint32_t app(uint32_t param1, uint32_t param2)
{
    // Initialisation.
    //
  //char      *ptr = (char *)param1;
  //
    // Initialise the ZPU timer.
  #if defined __ZPU__
    TIMER_MILLISECONDS_UP = 0;
  #endif

    init(VARS_SPACE_SIZE, 80, sizeof(dataSpace) - VARS_SPACE_SIZE);
    while(doExit == 0)
    {
        lastInput = translateInput(sysGetc());
        dispatch();
    }
    return(0);
}

#ifdef __cplusplus
}
#endif

