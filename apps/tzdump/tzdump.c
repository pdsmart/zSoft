/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzdump.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing the realtime display of the tranZPUter
//                  or host mainboard memory.
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
  #include <usb_serial.h>
  #include <core_pins.h>
  #include <Arduino.h>
  #include <getopt.h>
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
#include <app.h>
#include <tranzputer.h>
#include "tzdump.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.0"
#define VERSION_DATE         "15/05/2020"
#define APP_NAME             "TZDUMP"

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -a | --start             Start address.\n");
    printf("\nOptions:-\n");
    printf("  -e | --end               End address (alternatively use --size).\n");
    printf("  -s | --size              Size of memory block to dump (alternatively use --end).\n");
    printf("  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter memory.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzdump -a 0x000000 -s 0x200   # Dump tranZPUter memory from 0x000000 to 0x000200.\n");

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
    uint32_t   startAddr         = 0xFFFFFFFF;
    uint32_t   endAddr           = 0xFFFFFFFF;
    uint32_t   memSize           = 0xFFFFFFFF;
    int        argc              = 0;
    int        help_flag         = 0;
    int        mainboard_flag    = 0;
    int        verbose_flag      = 0;
    int        opt; 
    int        option_index      = 0; 
    long       val               = 0;
    char      *argv[20];
    char      *ptr               = strtok((char *)param1, " ");

    // Initialisation.

    // If the invoking command is given, add it to argv at the start.
    //
    if(param2 != 0)
    {
        argv[argc++] = (char *)param2;
    }

    // Now convert the parameter line into argc/argv suitable for getopt to use.
    while (ptr && argc < 20-1)
    {
        argv[argc++] = ptr;
        ptr = strtok(0, " ");
    }
    argv[argc] = 0;

    // Define parameters to be processed.
    static struct option long_options[] =
    {
        {"help",          no_argument,       0,   'h'},
        {"start",         required_argument, 0,   'a'},
        {"end",           required_argument, 0,   'e'},
        {"size",          required_argument, 0,   's'},
        {"mainboard",     no_argument,       0,   'm'},
        {"verbose",       no_argument,       0,   'v'},
        {0,               0,                 0,    0}
    };

    // Parse the command line options.
    //
    while((opt = getopt_long(argc, argv, ":hs:e:s:mv", long_options, &option_index)) != -1)  
    {  
        switch(opt)  
        {  
            case 'h':
                help_flag = 1;
                break;

            case 'm':
                mainboard_flag = 1;
                break;

            case 'a':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(5);
                }
                startAddr = (uint32_t)val;
                break;

            case 'e':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(6);
                }
                endAddr = (uint32_t)val;
                break;

            case 's':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(7);
                }
                memSize = (uint32_t)val;
                break;

            case 'v':
                verbose_flag = 1;
                break;

            case ':':  
                printf("Option %s needs a value\n", argv[optind-1]);  
                break;  
            case '?':  
                printf("Unknown option: %s, ignoring!\n", argv[optind-1]); 
                break;  
        }  
    } 

    // Validate the input.
    if(help_flag == 1)
    {
        usage();
        return(0);
    }
    if(startAddr == 0xFFFFFFFF)
    {
        printf("Please define the start address, size will default to 0x100.\n");
        return(10);
    }
    if(endAddr == 0xFFFFFFFF && memSize == 0xFFFFFFFF)
    {
        memSize = 0x100;
    } else if(memSize == 0xFFFFFFFF)
    {
        memSize = endAddr - startAddr;
    }
    if(mainboard_flag == 1 && (startAddr > 0x10000 || startAddr + memSize > 0x10000))
    {
        printf("Mainboard only has 64K, please change the address or size.\n");
        return(11);
    }
    if(mainboard_flag == 0 && (startAddr >= 0x80000 || startAddr + memSize > 0x80000))
    {
        printf("tranZPUter board only has 512K, please change the address or size.\n");
        return(12);
    }

    // Initialise the IO.
    setupZ80Pins(1, G->millis);

    // Call the dump utility to list out memory.
    //
    memoryDumpZ80(startAddr, memSize, startAddr, 32, mainboard_flag);

    return(0);
}

#ifdef __cplusplus
}
#endif
