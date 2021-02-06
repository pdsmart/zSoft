/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzio.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing the realtime setting or reading of the host I/O
//                  ports.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         Nov 2020 - Initial write of the TranZPUter software.
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
#include "tzio.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.1"
#define VERSION_DATE         "08/12/2020"
#define APP_NAME             "TZIO"

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -o | --out <port>        Output to I/O address.\n");
    printf("  -i | --in <port>         Input from I/O address.\n");
    printf("\nOptions:-\n");
    printf("  -b | --byte              Byte value to send to the I/O port in the --out command, defaults to 0x00.\n");
    printf("  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter I/O domain.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzio --out 0xf8 --byte 0x10    # Setup the FPGA Video mode at address 0xf8.\n");
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
    uint32_t   ioAddr            = 0xFFFFFFFF;
    uint8_t    byte              = 0x00;
    int        argc              = 0;
    int        help_flag         = 0;
    int        mainboard_flag    = 0;
    int        verbose_flag      = 0;
    int        out_flag          = 0;
    int        in_flag           = 0;
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
        {"in",            required_argument, 0,   'i'},
        {"out",           required_argument, 0,   'o'},
        {"byte",          required_argument, 0,   'b'},
        {"mainboard",     no_argument,       0,   'm'},
        {"verbose",       no_argument,       0,   'v'},
        {0,               0,                 0,    0}
    };

    // Parse the command line options.
    //
    while((opt = getopt_long(argc, argv, ":hi:o:b:mv", long_options, &option_index)) != -1)  
    {  
        switch(opt)  
        {  
            case 'h':
                help_flag = 1;
                break;

            case 'm':
                mainboard_flag = 1;
                break;

            case 'o':
                out_flag = 1;
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(5);
                }
                ioAddr = (uint32_t)val;
                break;

            case 'i':
                in_flag = 1;
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(5);
                }
                ioAddr = (uint32_t)val;
                break;

            case 'b':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(6);
                }
                byte = (uint8_t)val;
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
    if(ioAddr == 0xFFFFFFFF)
    {
        printf("Please define the I/O address using --in <port> or --out <port>.\n");
        return(10);
    }
    if(out_flag == 0 && in_flag == 0)
    {
        printf("Please define a command, --help, --out or --in.\n");
        return(10);
    }
    if(ioAddr >= 0x10000)
    {
        printf("Host only has a 16bit port address, generally only lower 8 bits are used.\n");
        return(11);
    }
 
    // Action the request, output a byte or read a byte.
    if(out_flag)
    {
        writeZ80IO(ioAddr, byte, mainboard_flag);
    } else
    {
        byte = readZ80IO(ioAddr, mainboard_flag);
        printf("%02X\n", byte);
    }

    return(0);
}

#ifdef __cplusplus
}
#endif
