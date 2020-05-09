/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            test.c
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
#include "test.h"

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.1"
#define VERSION_DATE "10/04/2020"
#define APP_NAME     "TEST"

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
    //uint32_t *pWordArray = malloc(2048);
    //uint16_t *pHwordArray = malloc(2048);
    //uint8_t  *pByteArray  = malloc(2048);
    static uint32_t wordArray[1024];
    static uint16_t hwordArray[1024];
    static uint8_t  byteArray[1024];
    static uint16_t idx;
    static uint32_t sum;
    long int        i, j, k, m;
    int32_t         idx1;
    uint32_t        idx2;

    puts("This is a test.\n");
    puts("Print another line.\n");
    printf("This is another test.\n");
    puts("All done\n");

    // Test the maths division.
    for (i = -10000; i < 10000; i += 8)
    {
        for (j = -10000; j < 10000; j += 11)
        {
            k = i / j;
            m = __divsi3 (i, j);
            if (k != m)
                printf ("fail %ld %ld %ld %ld\n", i, j, k, m);
        }
    }    

    // Test the mod, div and mul.
    for(idx1=-500; idx1 < 500; idx1++)
    {
	    printf("Result(%ld)=%ld %ld,%lu,%lu:Mul=%ld\n", idx1, (idx1/10), (idx1 % 10),((uint32_t)idx1/10), ((uint32_t)idx1 % 10), idx1 * 10);
	    printf("%ld, %ld\n", __divsi3(idx1,10), __udivsi3(idx1,10));
	    printf("%lu, %lu\n", __divsi3(idx1,10), __udivsi3(idx1,10));
    }
    for(idx2=0; idx2 < 500; idx2++)
    {
	    printf("Result(%ld)=%ld %ld, Mul:%ld\n", idx2, (int32_t)(idx2/10), (int32_t)(idx2 % 10), idx2 * 10);
	    printf("      (%lu)=%lu %lu\n", idx2, (uint32_t)(idx2/10), (uint32_t)(idx2 % 10));
	    printf("%ld, %ld\n", __divsi3(idx2,10), __udivsi3(idx2,10));
	    printf("%lu, %lu\n", __divsi3(idx2,10), __udivsi3(idx2,10));
    }    

    // These are just memory tests, the main test is in premain where a closer knit write operation of BSS fails, thus trying to debug.
    for(idx=0, sum=0; idx < 2048; idx++)
    {
        sum += wordArray[idx];
    }

    for(idx=0, sum=0; idx < 2048; idx++)
    {
        sum += hwordArray[idx];
    }

    for(idx=0, sum=0; idx < 2048; idx++)
    {
        sum += byteArray[idx];
    }

    return(0);
}

#ifdef __cplusplus
}
#endif
