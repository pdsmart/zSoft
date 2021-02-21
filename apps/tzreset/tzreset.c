/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzreset.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing a remote hardware reset of the tranZPUter
//                  board and host (not K64F).
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2020 - Initial write of the TranZPUter software.
//                  Feb 2021 - Replaced getopt_long with optparse as it is buggy and crashes.
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
	  
// Getopt_long is buggy so we use optparse.
#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include <optparse.h>

//
#include <app.h>
#include <tranzputer.h>
#include "tzreset.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.1"
#define VERSION_DATE         "21/02/2021"
#define APP_NAME             "TZRESET"

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -r | --reset             Perform a hardware reset.\n");
    printf("  -l | --load              Reload the default ROMS.\n");
    printf("  -m | --memorymode <val>  Set the startup memory mode.\n");
    printf("\nOptions:-\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzreset -r        # Resets the Z80 and associated tranZPUter logic..\n");
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
    int        argc              = 0;
    int        help_flag         = 0;
    int        load_flag         = 0;
    int        reset_flag        = 0;
    int        verbose_flag      = 0;
    int        opt; 
    uint8_t    memoryMode        = TZMM_ORIG;
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
    struct optparse options;
    static struct optparse_long long_options[] =
    {
        {"help",          'h',  OPTPARSE_NONE},
        {"load",          'l',  OPTPARSE_NONE},
        {"memorymode",    'm',  OPTPARSE_REQUIRED},
        {"reset",         'r',  OPTPARSE_NONE},
        {"verbose",       'v',  OPTPARSE_NONE},
        {0}
    };

    // Parse the command line options.
    //
    optparse_init(&options, argv);
    while((opt = optparse_long(&options, long_options, NULL)) != -1)  
    {  
        switch(opt)  
        {  
            case 'h':
                help_flag = 1;
                break;

            case 'l':
                load_flag = 1;
                break;

            case 'm':
                if(xatoi(&options.optarg, &val) == 0 || val >= 0x20)
                {
                    printf("Illegal numeric:%s\n", options.optarg);
                    return(1);
                }
                memoryMode = (uint8_t)val;
                break;

            case 'r':
                reset_flag = 1;
                break;

            case 'v':
                verbose_flag = 1;
                break;

            case '?':
                printf("%s: %s\n", argv[0], options.errmsg);
                return(1);
        }  
    } 

    // Validate the input.
    if(help_flag == 1 || reset_flag == 0)
    {
        usage();
        return(0);
    }

    // Reload the memory on the tranZPUter to boot default if load flag set.
    //
    if(load_flag)
    {
        hardResetTranZPUter();
    } else
    {
        // Call the reset method to do the hard work.
        //
        resetZ80(memoryMode);
    }

    return(0);
}

#ifdef __cplusplus
}
#endif
