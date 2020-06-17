/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzclk.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing the realtime change of the secondary CPU
//                  clock frequency. Access to the mainboard still occurs at the original mainboard
//                  frequency but when the secondary clock is eaabled, it is used when using tranZPUter
//                  onboard hardware such as the static RAM.
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
#include "tzclk.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.0"
#define VERSION_DATE         "08/06/2020"
#define APP_NAME             "TZCLK"

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -f | --freq              Desired CPU clock frequency.\n");
    printf("\nOptions:-\n");
    printf("  -e | --enable            Enable the secondary CPU clock.\n");
    printf("  -d | --disable           Disable the secondary CPU clock.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzclk --freq 4000000 --enable  # Set the secondary CPU clock frequency to 4MHz and enable its use on the tranZPUter board.\n");
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
    uint32_t   cpuFreq           = 0;
    uint32_t   actualFreq        = 0;
    int        argc              = 0;
    int        help_flag         = 0;
    int        enable_flag       = 0;
    int        disable_flag      = 0;
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
        {"freq",          required_argument, 0,   'f'},
        {"enable",        no_argument,       0,   'e'},
        {"disable",       no_argument,       0,   'd'},
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

            case 'e':
                enable_flag = 1;
                break;

            case 'd':
                disable_flag = 1;
                break;

            case 'f':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(5);
                }
                cpuFreq = (uint32_t)val;
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
    if(cpuFreq == 0)
    {
        printf("Please specify the CPU frequency with the --freq flag.\n");
        return(10);
    }
    if(enable_flag == 1 && disable_flag == 1)
    {
        printf("Illegal flag combination, cannot enable and disable the secondary CPU frequency at the same time.\n");
        return(12);
    }

    // Call the method to set the frequency and optionally enable/disable it on the tranZPUter board.
    actualFreq = setZ80CPUFrequency((float)cpuFreq, (enable_flag == 1 ? 1 : disable_flag == 1 ? 2 : 0));
    printf("Requested Frequency:%ldHz, Actual Frequency:%ldHz\n", cpuFreq, actualFreq);
    return(0);
}

#ifdef __cplusplus
}
#endif
