/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            mdiff.c
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
#include "mdiff.h"

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.1"
#define VERSION_DATE "10/04/2020"
#define APP_NAME     "MDIFF"

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
    char      *ptr = (char *)param1;
    long      startAddr;
    long      endAddr;
    long      cmpAddr;
    uint32_t  memAddr;
    int8_t    keyIn;

    if (!xatoi(&ptr, &startAddr))
    {
        printf("Illegal <start addr> value.\n");
    } else if (!xatoi(&ptr,  &endAddr))
    {
        printf("Illegal <end addr> value.\n");
    } else if (!xatoi(&ptr,  &cmpAddr))
    {
        printf("Illegal <cmp addr> value.\n");
    } else
    {
        printf("Comparing Memory from %08lx:%08lx with %08lx\n", startAddr, endAddr, cmpAddr);
        for(memAddr=startAddr; memAddr < endAddr; memAddr++, cmpAddr++)
        {
            if(*(uint8_t *)(cmpAddr) != *(uint8_t *)(memAddr))
            {
                printf("%08lx(%08x)->%08lx(%08x)\n", memAddr, *(uint8_t *)(memAddr), cmpAddr, *(uint8_t *)(cmpAddr));
            }

            // User abort (ESC), pause (Space) or all done?
            //
	    keyIn = getKey(0);
            if(keyIn == ' ')
            {
                do {
		    keyIn = getKey(0);
                } while(keyIn != ' ' && keyIn != 0x1b);
            }
            // Escape key pressed, exit with 0 to indicate this to caller.
            if (keyIn == 0x1b)
            {
                break;
            }
        }
    }

    return(0);
}

#ifdef __cplusplus
}
#endif
