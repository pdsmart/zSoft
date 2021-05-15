/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzmtest.c
// Created:         July 2019
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility, allowing the realtime display of the tranZPUter
//                  or host mainboard memory. Code originates from the original zputa mtest application.
//
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2021    - Initial framework creation.
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
#include "app.h"
#include <tranzputer.h>
#include "tzmtest.h"

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.0"
#define VERSION_DATE "15/05/2021"
#define APP_NAME     "TZMTEST"

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
    printf("  -s | --size              Size of memory block to test (alternatively use --end).\n");
    printf("  -f | --fpga              Operations will take place in the FPGA memory. Default without this flag is to target the tranZPUter memory.\n");
    printf("  -i | --iter              Number of test iterations, default = 1.\n");
    printf("  -t | --test              Specify test as a bit value, bit 0 = R/W inc ascending test, 1 = R/W inc walking test, 2 = W ascending then R,\n");
    printf("                           bit 3 = W walking then R, bit 4 = echo and stick bit test.\n");
  //printf("  -w " --width             Specify memory width tests reqd as a bit value, bit 0 = 8 bit, bit 1 = 16 bit, bit 2 = 32 bit.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzmtest -a 0x000000 -s 0x20000   # Test 128K tranZPUter memory from 0x000000 to 0x020000.\n");

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
    uint32_t   iter              = 1;                // Default to 1 iteration.
    uint16_t   test              = 0x00FF;           // Default to all tests.
    uint16_t   width             = 0x0007;           // Default to all widths.
    uint32_t   testsToDo;
    uint8_t    retCode           = 0;
    int        argc              = 0;
    int        help_flag         = 0;
    int        fpga_flag         = 0;
    int        mainboard_flag    = 0;
  //int        mempage_flag      = 0;
    int        verbose_flag      = 0;
    int        opt; 
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
        {"start",         'a',  OPTPARSE_REQUIRED},
        {"end",           'e',  OPTPARSE_REQUIRED},
        {"size",          's',  OPTPARSE_REQUIRED},
        {"fpga",          'f',  OPTPARSE_NONE},
        {"mainboard",     'm',  OPTPARSE_NONE},
        {"iter",          'i',  OPTPARSE_REQUIRED},
        {"test",          't',  OPTPARSE_REQUIRED},
      //{"width",         'w',  OPTPARSE_REQUIRED},
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

            case 'f':
                fpga_flag = 1;
                break;

            case 'm':
                mainboard_flag = 1;
                break;

            case 'a':
                if(xatoi(&options.optarg, &val) == 0)
                {
                    printf("Illegal numeric (-a):%s\n", options.optarg);
                    return(5);
                }
                startAddr = (uint32_t)val;
                break;

            case 'e':
                if(xatoi(&options.optarg, &val) == 0)
                {
                    printf("Illegal numeric (-e):%s\n", options.optarg);
                    return(6);
                }
                endAddr = (uint32_t)val;
                break;

            case 's':
                if(xatoi(&options.optarg, &val) == 0)
                {
                    printf("Illegal numeric (-s):%s\n", options.optarg);
                    return(7);
                }
                memSize = (uint32_t)val;
                break;

            case 'i':
                if(xatoi(&options.optarg, &val) == 0)
                {
                    printf("Illegal numeric (-i):%s\n", options.optarg);
                    return(8);
                }
                iter = (uint32_t)val;
                break;

            case 't':
                if(xatoi(&options.optarg, &val) == 0)
                {
                    printf("Illegal numeric (-t):%s\n", options.optarg);
                    return(8);
                }
                test = (uint32_t)val;
                break;

          //case 'w':
          //    if(xatoi(&options.optarg, &val) == 0)
          //    {
          //        printf("Illegal numeric (-w):%s\n", options.optarg);
          //        return(8);
          //    }
          //    width = (uint32_t)val;
          //    break;

            case 'v':
                verbose_flag = 1;
                break;

            case '?':
                printf("%s: %s\n", argv[0], options.errmsg);
                return(1);
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
        endAddr = startAddr + 0x100;
    } else if(endAddr == 0xFFFFFFFF)
    {
        endAddr = startAddr + memSize;
    }
    if(endAddr < startAddr)
    {
        printf("Start Address must be greater than End Address.\n");
        return(11);
    }
    if(mainboard_flag == 1 && fpga_flag == 1)
    {
        printf("Please specify only one target, --mainboard, --fpga or default to tranZPUter memory.\n");
        return(12);
    }
    if(mainboard_flag == 1 && (startAddr > 0x10000 || endAddr > 0x10000))
    {
        printf("Mainboard only has 64K, please change the address or size.\n");
        return(13);
    }
    if(fpga_flag == 1 && (startAddr >= TZ_MAX_FPGA_MEM || endAddr > TZ_MAX_FPGA_MEM))
    {
        printf("FPGA only has a %dM window, please change the address or size.\n", TZ_MAX_FPGA_MEM/1024000);
        return(14);
    }
    if(mainboard_flag == 0 && fpga_flag == 0 && (startAddr >= TZ_MAX_Z80_MEM || endAddr > TZ_MAX_Z80_MEM))
    {
        printf("tranZPUter board only has %dK, please change the address or size.\n", TZ_MAX_Z80_MEM/1024);
        return(15);
    }

    // Setup the test/width indicator flag.
    testsToDo = (width << 16) | test;

    // A very simple test, this needs to be updated with a thorough bit pattern and location test.
    if(verbose_flag)
        printf( "Check memory addr 0x%08lX to 0x%08lX over %ld iteration(s).\n", startAddr, endAddr, iter );

    // Perform test for given number of iterations.
    for(uint32_t idx=0; idx < iter && retCode == 0; idx++)
    {
        if(testsToDo & 0x00010000)
        {
            retCode = testZ80Memory(startAddr,  endAddr, testsToDo, verbose_flag, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER);
        }
      //if(testsToDo & 0x00020000)
      //{
      //    test16bit(startAddr, endAddr, testsToDo);
      //}
      //if(testsToDo & 0x00040000)
      //{
      //    test32bit(startAddr, endAddr, testsToDo);
      //}
    }

    // Terminate output if in verbose mode.
    if(verbose_flag)
        printf("\n");

    // Indicate any failure if we are in silent mode.
    if(retCode != 0 && verbose_flag == 0)
        printf("Memory test failed with return code:%d, use --verbose flag for more detail.\n", retCode);

    return(0);
}

#ifdef __cplusplus
}
#endif
