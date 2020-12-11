/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzload.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     A TranZPUter helper program, responsible for loading images off the SD card into 
//                  memory on the tranZPUter board or host mainboard./
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
#include "tzload.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.1"
#define VERSION_DATE         "10/12/2020"
#define APP_NAME             "TZLOAD"

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -d | --download <file>   File into which memory contents from the tranZPUter are stored.\n");
    printf("  -u | --upload   <file>   File whose contents are uploaded into the traZPUter memory.\n");
    printf("  -U | --uploadset <file>:<addr>,...,<file>:<addr>\n");
    printf("                           Upload a set of files at the specified locations. --mainboard specifies mainboard is target, default is tranZPUter.\n");
    printf("  -V | --video             The specified input file is uploaded into the video frame buffer or the specified output file is filled with the video frame buffer.\n");
    printf("\nOptions:-\n");
    printf("  -a | --addr              Memory address to read/write.\n");
    printf("  -l | --size              Size of memory block to read. This option is only used when reading tranZPUter memory, for writing, the file size is used.\n");
    printf("  -s | --swap              Read tranZPUter memory and store in <infile> then write out <outfile> to the same memory location.\n");
    printf("  -f | --fpga              Operations will take place in the FPGA memory. Default without this flag is to target the tranZPUter memory.\n");
    printf("  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter memory.\n");
    printf("  -z | --mzf               File operations are to process the file as an MZF format file, --addr and --size will override the MZF header values if needed.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzload --outfile monitor.rom -a 0x000000      # Load the file monitor.rom into the tranZPUter memory at address 0x000000.\n");

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
    uint32_t   memAddr           = 0xFFFFFFFF;
    uint32_t   memSize           = 0xFFFFFFFF;
    int        argc              = 0;
    int        help_flag         = 0;
    int        fpga_flag         = 0;
    int        mainboard_flag    = 0;
    int        mzf_flag          = 0;
    int        swap_flag         = 0;
    int        verbose_flag      = 0;
    int        video_flag        = 0;
    int        opt; 
    int        option_index      = 0; 
    int        uploadFNLen       = 0;
    int        downloadFNLen     = 0;
    int        uploadCnt         = 0;
    long       val;
    char      *argv[20];
    char      *uploadArr[20];
    char      *ptr               = strtok((char *)param1, " ");
    char       uploadFile[32];
    char       downloadFile[32];

    // Initialisation.
    uploadFile[0]  = '\0';
    downloadFile[0] = '\0';

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
        {"download",      required_argument, 0,   'd'},
        {"upload",        required_argument, 0,   'u'},
        {"uploadset",     required_argument, 0,   'U'},
        {"addr",          required_argument, 0,   'a'},
        {"size",          required_argument, 0,   'l'},
        {"fpga",          no_argument,       0,   'f'},
        {"mainboard",     no_argument,       0,   'm'},
        {"mzf",           no_argument,       0,   'z'},
        {"swap",          no_argument,       0,   's'},
        {"verbose",       no_argument,       0,   'v'},
        {"video",         no_argument,       0,   'V'},
        {0,               0,                 0,    0}
    };

    // Parse the command line options.
    //
    while((opt = getopt_long(argc, argv, ":ha:l:fmvU:Vzsd:u:", long_options, &option_index)) != -1)  
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

            case 's':
                swap_flag = 1;
                break;

            case 'a':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(6);
                }
                memAddr = (uint32_t)val;
                break;

            case 'l':
                if(xatoi(&argv[optind-1], &val) == 0)
                {
                    printf("Illegal numeric:%s\n", argv[optind-1]);
                    return(7);
                }
                memSize = (uint32_t)val;
                break;

            case 'd':
                // Data is read from the tranZPUter and stored in the given file.
                strcpy(downloadFile, argv[optind-1]);
                downloadFNLen = strlen(downloadFile);
                break;

            case 'u':
                // Data is uploaded from the given file into the tranZPUter.
                strcpy(uploadFile, argv[optind-1]);
                uploadFNLen = strlen(uploadFile);
                break;

            case 'U':
                // Extract an array of <file>:<addr>,... sets for upload.
                //
                ptr = strtok((char *)argv[optind-1], ",");
                while (ptr && uploadCnt < 20-1)
                {
                    uploadArr[uploadCnt++] = ptr;
                    ptr = strtok(0, ",");
                }
                if(uploadCnt == 0)
                {
                    printf("Upload set command should use format <file>:<addr>,...\n");
                    return(6);
                }
                break;

            case 'v':
                verbose_flag = 1;
                break;

            case 'V':
                video_flag = 1;
                break;

            case 'z':
                mzf_flag = 1;
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
    if(uploadCnt && (help_flag == 1 || uploadFNLen > 0 || downloadFNLen > 0 || swap_flag == 1 || video_flag == 1 || memAddr != 0xFFFFFFFF || memSize != 0xFFFFFFFF))
    {
        printf("Illegal combination of flags, --uploadset can only be used with --mainboard.\n");
        return(10);
    }
    if(video_flag == 1 && (help_flag == 1 || swap_flag == 1 || mainboard_flag == 1 || memAddr != 0xFFFFFFFF || memSize != 0xFFFFFFFF))
    {
        printf("Illegal combination of flags, --video can only be used with --upload, --download and --mainboard.\n");
        return(11);
    }

    // If the Video and Upload modes arent required, check other argument combinations.
    //
    if(uploadCnt == 0 && video_flag == 0)
    {
        if(help_flag == 1)
        {
            usage();
            return(0);
        }
        if( (uploadFNLen == 0 && downloadFNLen == 0)  ||
            (swap_flag == 0 && uploadFNLen > 0  && downloadFNLen > 0) )
        {
            if(swap_flag == 0)
            {
                printf("Input file or Output file (only one) needs to be specified.\n");
            }
            else
            {
                printf("Both an Input file and an Output file need to be specified for swap mode.\n");
            }
            return(15);     
        }
        if(swap_flag == 0 && downloadFNLen > 0 && memSize == 0xFFFFFFFF)
        {
            printf("Please define the size of memory you wish to read.\n");
            return(16);
        }
        if(mzf_flag == 1 && downloadFNLen > 0)
        {
            printf("MZF Format can currently only be used for file uploading.\n");
            return(17);
        }
        if(memAddr == 0xFFFFFFFF && mzf_flag == 0)
        {
            printf("Please define the target address.\n");
            return(18);
        }
    }
    if(uploadCnt == 0 && video_flag == 0)
    {
        if(mainboard_flag == 1 && mzf_flag == 0 && (memAddr > 0x10000 || memAddr + memSize > 0x10000))
        {
            printf("Mainboard only has 64K, please change the address and size.\n");
            return(19);
        }
        if(fpga_flag == 1 && mzf_flag == 0 && (memAddr >= 0x80000 || memAddr + memSize > 0x80000))
        {
            printf("FPGA only has a 512K window, please change the address or size.\n");
            return(20);
        }    
        if(mainboard_flag == 0 && fpga_flag == 0 && mzf_flag == 0 && (memAddr >= 0x80000 || memAddr + memSize > 0x80000))
        {
            printf("tranZPUter board only has 512K, please change the address and size.\n");
            return(21);
        }
    }

    // Initialise the IO.
    //setupZ80Pins(1, G->millis);

    // Bulk file upload command (used to preload a file set).
    //
    if(uploadCnt > 0)
    {
        // Process all the files in the upload list.
        //
        for(uint8_t idx=0; idx < uploadCnt; idx++)
        {
            strcpy(uploadFile, strtok((char *)uploadArr[idx], ":"));
            ptr = strtok(0, ",");
            if(xatoi(&ptr, &val) == 0)
            {
                printf("Illegal numeric in upload list:%s\n", ptr);
                return(30);
            }
            memAddr = (uint32_t)val;
         
            // Now we have the input file and the address where it should be loaded, call the load function.
            //
            if(mzf_flag == 0)
            {
                loadZ80Memory(uploadFile, 0, memAddr, 0, 0, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER, (idx == uploadCnt-1) ? 1 : 0);
            } else
            {
                loadMZFZ80Memory(uploadFile, memAddr,    0, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER, (idx == uploadCnt-1) ? 1 : 0);
            }
        }
    }
 
    // Standard read/write operation or swapping out memory?
    //
    else if(video_flag)
    {
        if(downloadFNLen > 0)
        {
            captureVideoFrame(SAVED, 0);
            saveVideoFrameBuffer(downloadFile, SAVED);
        } else
        {
            loadVideoFrameBuffer(uploadFile, SAVED);
            refreshVideoFrame(SAVED, 1, 0);
        }
    } 
    
    else
    {
        if(downloadFNLen > 0)
        {
            if(verbose_flag)
            {
                printf("Saving %s memory at address:%06lx into file:%s\n", (mainboard_flag == 1 ? "mainboard" : fpga_flag == 1 ? "fpga" : "tranZPUter"), memAddr, downloadFile);
            }
            saveZ80Memory(downloadFile, memAddr, memSize, 0, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER);
        }
        if(uploadFNLen > 0)
        {
            if(verbose_flag)
            {
                printf("Loading file:%s into the %s memory\n", uploadFile, (mainboard_flag == 1 ? "mainboard" : fpga_flag == 1 ? "fpga" : "tranZPUter"));
            }

            if(mzf_flag == 0)
            {
                loadZ80Memory(uploadFile, 0, memAddr, 0, 0, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER, 1);
            } else
            {
                loadMZFZ80Memory(uploadFile, memAddr,    0, mainboard_flag == 1 ? MAINBOARD : fpga_flag == 1 ? FPGA : TRANZPUTER, 1);
            }
        }
    }

    return(0);
}

#ifdef __cplusplus
}
#endif
