/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tcpu.c
// Created:         July 2019
// Author(s):       Philip Smart
// Description:     Standalone App for the zOS/ZPU test application.
//                  This program implements a loadable appliation which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         July 2019    - Initial framework creation.
//                  April 2020   - Updates to function with the K64F processor and zOS.
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
  #include <stdint.h>
  #include <string.h>
  #include "k64f_soc.h"
  #include <../../libraries/include/stdmisc.h>
#elif defined(__ZPU__)
  #include <stdint.h>
  #include <stdio.h>	    
  #include "zpu_soc.h"
  #include <stdlib.h>
  #include <stdmisc.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include "utils.h"
//
#if defined __ZPUTA__
  #include "zputa_app.h"
#elif defined __ZOS__
  #include "zOS_app.h"
#else
  #error OS not defined, use __ZPUTA__ or __ZOS__      
#endif
//
#include "app.h"
#include "tcpu.h"

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.1"
#define VERSION_DATE "10/04/2020"
#define APP_NAME     "TCPU"

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
  #if defined __ZPU__
    char      *ptr = (char *)param1;
    long      startAddr;
    long      endAddr;
    long      value;
    long      bitWidth;
    uint32_t  memAddr;
    uint32_t  addr;
    uint32_t  data;

    puts("TCPU Test Program\n");

    while(getserial_nonblocking() == -1)
    {
        addr = TCPU_ADDR;
        data = TCPU_DATA;


        printf("BUSRQI:%01x BUSACKI:%01x WAITI:%01x INTI:%01x NMII:%01x CLKI:%01x BUSRQO:%01x BUSACKO:%01x CTLCLRO:%01x CTLSET:%01x DATA:%04x, STATUS:%08lx\r",
                (addr >> 25) & 0x1, (addr >> 24) & 0x1, (addr >> 23) & 0x1, (addr >> 22) & 0x1, (addr >>21) & 0x1, (addr >> 20) & 0x1, (addr >> 19) & 0x1, (addr >> 18) & 0x1, (addr >> 17) & 0x1, (addr >> 16) & 0x1,
                (addr & 0xffff), data);

        TCPU_ADDR = 0x8000;
        TIMER_MILLISECONDS_DOWN = 5;
        while(TIMER_MILLISECONDS_DOWN > 0);
    }

//    if (!xatoi(&ptr, &startAddr))
//    {
//        printf("Illegal <start addr> value.\n");
//    } else if (!xatoi(&ptr,  &endAddr))
//    {
//        printf("Illegal <end addr> value.\n");
//    } else
//    {
//        if (!xatoi(&ptr, &value))
//        {
//            value = 0x00000000;
//        }
//        xatoi(&ptr,  &bitWidth);
//	if(bitWidth != 8 && bitWidth != 16 && bitWidth != 32)
//        {
//            bitWidth = 32;
//        }
//        bitWidth /= 8;
//
//        puts("Clearing Memory\n");
//        for(memAddr=startAddr; memAddr < endAddr; memAddr+=bitWidth)
//        {
//            switch(bitWidth)
//            {
//                case 1:
//                    *(uint8_t *)(memAddr) = value;
//                    break;
//                case 2:
//                    *(uint16_t *)(memAddr) = value;
//                    break;
//                case 4:
//                default:
//                    *(uint32_t *)(memAddr) = value;
//                    break;
//            }
//        }
//    }
  #elif defined __K64F__
    puts("This application needs completion for the Teensy 3.5 version of the tranZPUter.\n");
  #else
    #error "Target CPU not defined, use __ZPU__ or __K64F__"
  #endif

    return(0);
}

#ifdef __cplusplus
}
#endif
