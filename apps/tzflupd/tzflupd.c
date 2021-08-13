/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzdump.c
// Created:         Jan 2021
// Author(s):       Philip Smart
// Description:     A TranZPUter helper utility to update the program in the K64F ARM Processor. This
//                  application takes a binary and flashes it into the K64F Flash Memory in order to 
//                  update zOS/ZPUTA or flash any other program (NB. flashing any other program may
//                  need an external OpenSDA programmer to reprogram the K64F with zOS).
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         Jan 2021 - Initial write of the TranZPUter software using NXP/Freescale flash
//                             driver source.
//                  Feb 2021 - Getopt too buggy with long arguments so replaced with optparse.
//                  Mar 2021 - Change sector size to K64F default and fixed some bugs.
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
//
// For the Freescale driver code, please see the license at the top of the driver source file.
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
//#include "utils.h"
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

#if defined(__K64F__)
#include <tranzputer_m.h>
#include "tzflupd.h"
#include "fsl_flash.h"
#endif

// Utility functions.
//#include <tools.c>

// Version info.
#define VERSION              "v1.2"
#define VERSION_DATE         "11/03/2021"
#define APP_NAME             "TZFLUPD"

// Global scope variables.
FATFS     diskHandle;
char      buffer[FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE];
uint8_t   FLASH_PROTECTION_SIGNATURE[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0xf9, 0xff, 0xff};

// Simple help screen to remmber how this utility works!!
//
void usage(void)
{
    printf("%s %s\n", APP_NAME, VERSION);
    printf("\nCommands:-\n");
    printf("  -h | --help              This help text.\n");
    printf("  -f | --file              Binary file to upload and flash into K64F.\n");
    printf("\nOptions:-\n");
    printf("  -d | --debug             Add debug steps to programming.\n");
    printf("  -v | --verbose           Output more messages.\n");

    printf("\nExamples:\n");
    printf("  tzflupd -f zOS_22012021_001.bin --verbose   # Upload and program the zOS_22012021_001.bin file into the K64F flash memory.\n");
}

// Method to initialise the SD card and mount the first partition. The file we need to upload into the K64F Flash RAM is stored on this card.
//
FRESULT initSDCard(void)
{
    // Locals.
    //
    FRESULT   result = FR_NOT_ENABLED;

    // Make a complete initialisation as we are using a new Fat FS instance.
    //
    if(!disk_initialize(0, 1))
    {
        sprintf(buffer, "0:");
        result = f_mount(&diskHandle, buffer, 0);
    }

    return(result);
}

// Local memory dump routine for debug purposes.
//
#if 0
int dumpMemory(uint32_t memaddr, uint32_t memsize, uint32_t memwidth, uint32_t dispaddr, uint8_t dispwidth)
{
    uint8_t  displayWidth = dispwidth;;
    uint32_t pnt          = memaddr;
    uint32_t endAddr      = memaddr + memsize;
    uint32_t addr         = dispaddr;
    uint32_t i = 0;
    //uint32_t data;
    int8_t   keyIn;
    char c = 0;

    // If not set, calculate output line width according to connected display width.
    //
    if(displayWidth == 0)
    {
        switch(getScreenWidth())
        {
            case 40:
                displayWidth = 8;
                break;
            case 80:
                displayWidth = 16;
                break;
            default:
                displayWidth = 32;
                break;
        }
    }

    while (1)
    {
        printf("%08lX", addr); // print address
        printf(":  ");

        // print hexadecimal data
        for (i=0; i < displayWidth; )
        {
            switch(memwidth)
            {
                case 16:
                    if(pnt+i < endAddr)
                        printf("%04X", *(uint16_t *)(pnt+i));
                    else
                        printf("    ");
                        //printf("    ");
                    i+=2;
                    break;

                case 32:
                    if(pnt+i < endAddr)
                        printf("%08lX", *(uint32_t *)(pnt+i));
                    else
                        printf("        ");
                    i+=4;
                    break;

                case 8:
                default:
                    if(pnt+i < endAddr)
                        printf("%02X", *(uint8_t *)(pnt+i));
                    else
                        printf("  ");
                    i++;
                    break;
            }
            fputc((char)' ', stdout);
        }

        // print ascii data
        printf(" |");

        // print single ascii char
        for (i=0; i < displayWidth; i++)
        {
            c = (char)*(uint8_t *)(pnt+i);
            if ((pnt+i < endAddr) && (c >= ' ') && (c <= '~'))
                fputc((char)c, stdout);
            else
                fputc((char)' ', stdout);
        }

        puts("|");

        // Move on one row.
        pnt  += displayWidth;
        addr += displayWidth;

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
            return(0);
        }

        // End of buffer, exit the loop.
        if(pnt >= (memaddr + memsize))
        {
            break;
        }
    }

    // Normal exit, return -1 to show no key pressed.
    return(-1);
}
#endif


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
    int             argc              = 0;
    int             help_flag         = 0;
  //int             uploadFNLen       = 0;
    int             debug_flag        = 0;
    int             verbose_flag      = 0;
    int             opt; 
    int             updateFNLen       = 0;
  //long            val               = 0;
    char           *argv[20];
    char           *ptr               = strtok((char *)param1, " ");
    char            updateFile[32];
    uint32_t        fileSize          = 0;
    unsigned int    readSize          = 0;
    uint32_t        sizeToRead;
    uint32_t        bytesProcessed;
    flash_config_t  flashDriver;                                            // Flash driver Structure
    status_t        flashResult;                                            // Return code from each flash driver function
    FRESULT         fResult;                                                // Fat FS result.
    FIL             fileHandle;                                             // File Handle.

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
        {"help",          'h',               OPTPARSE_NONE},
        {"file",          'f',               OPTPARSE_REQUIRED},
        {"debug",         'd',               OPTPARSE_NONE},
        {"verbose",       'v',               OPTPARSE_NONE},
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
                strcpy(updateFile, options.optarg);
                updateFNLen = strlen(updateFile);                
                break;

            case 'd':
                debug_flag = 1;
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
    if(help_flag == 1)
    {
        usage();
        return(0);
    }
    if(updateFNLen == 0)
    {
        printf("Update file needs to be specified.\n");
        return(1);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------
    // As the kernel will be erased we need to assume kernel functionality so initialise kernel features, such as the SD card here as we use them locally.
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------
    if((fResult=initSDCard()) != FR_OK)
    {
        printf("ERROR: Failed to re-initialise the SD card, cannot continue.\n");   
        return(10);
    }

    // Initialise the flash driver.
    memset(&flashDriver, 0, sizeof(flash_config_t));
    flashResult = FLASH_Init(&flashDriver);
    if (kStatus_FLASH_Success != flashResult)
    {
        printf("Error: Failed to initialize Flash memory driver!\n");
        return(11);
    }
   
    // Try and open the source file.
    fResult = f_open(&fileHandle, updateFile, FA_OPEN_EXISTING | FA_READ);

    // Get the size of the file.
    if(fResult == FR_OK)
        fResult = f_lseek(&fileHandle, f_size(&fileHandle));
    if(fResult == FR_OK)
        fileSize = (uint32_t)f_tell(&fileHandle);
    if(fResult == FR_OK)
        fResult = f_lseek(&fileHandle, 0);    

    // Verify that the binary is a K64F program. Do this by comparing the protection area which is static and non changing.
    //
    fResult = f_lseek(&fileHandle, FLASH_PROTECTION_START_ADDR);
    if(fResult == FR_OK)
        fResult = f_read(&fileHandle, buffer, FLASH_PROTECTION_SIZE, &readSize);
    if(fResult == FR_OK)
    {
        for(int idx=0; idx < FLASH_PROTECTION_SIZE; idx++)
        {
            if(buffer[idx] != FLASH_PROTECTION_SIGNATURE[idx]) 
            {
                printf("Error: Update file doesnt look like a valid K64F program binary, aborting!\n");
                return(12);
            }
        }
        fResult = f_lseek(&fileHandle, 0);    
    }

    // If all ok, indicate file has been opened then prepare to read and flash.
    if(fResult == FR_OK)
    {
        // Indicate file, size and that it has been verified using the security flags.
        printf("%s %s\n\n", APP_NAME, VERSION);
        printf("Firmware update file: %s, size=%ld bytes\n\n", updateFile, fileSize);

        printf("********************************************************************************************************************\n");
        printf("Flash will now commence, no further output will be made until the flash is successfully programmed.\n");
        printf("If no further output is seen within 30 seconds, make a hard reset and verify the OS version. If the OS version hasnt\n");
        printf("changed, reissue this command.\n");
        printf("If device doesnt restart after a hard reset, use an OpenSDA or JTAG programmer to reprogram the OS.\n");
        printf("********************************************************************************************************************\n");

        // Slight delay to allow the output to flush to the user serial console.
        uint32_t startTime = *G->millis;
        while((*G->millis - startTime) < 1000) {};

        // Enter a loop, interrupts disabled, reading sector at a time from the SD card and flashing it into the Flash RAM.
        //
        __disable_irq();
        bytesProcessed = 0;
        do {
            sizeToRead = (fileSize-bytesProcessed) > FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE ? FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE : fileSize - bytesProcessed;
            fResult = f_read(&fileHandle, buffer, sizeToRead, &readSize);
            if (fResult || readSize == 0) break;   /* error or eof */                

            // If a sector isnt full, ie. last sector, pad with 0xFF.
            //
            if(readSize != FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE)
            {
                for(uint16_t idx=readSize; idx < FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE; idx++) { buffer[idx] = 0xFF; }
            }

            // Flash the sector into the correct location governed by the bytes already processed. We flash a K64F programming sector at a time with unused space set to 0xFF.
            //
            flashResult = FLASH_Erase(&flashDriver, bytesProcessed, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE, kFLASH_ApiEraseKey);
            // If no previous errors, program the next sector.
            if(flashResult == kStatus_FLASH_Success)
            {
                flashResult = FLASH_Program(&flashDriver, bytesProcessed, (uint32_t*)buffer, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);
            }

            // Update the address/bytes processed count.
            bytesProcessed += FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE;
        } while(bytesProcessed < fileSize && flashResult == kStatus_FLASH_Success);
        __enable_irq();

        // Verbose output.
        if(verbose_flag)
            printf("Bytes processed:%ld, exit status:%s\n", bytesProcessed, flashResult == kStatus_FLASH_Success ? "Success" : "Fail");

        // Success in programming, clear rest of flash RAM.
        if(flashResult == kStatus_FLASH_Success)
        {
            // Move to the next full sector as erase operates on sector boundaries.
            bytesProcessed = bytesProcessed + (bytesProcessed % FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);

            // Verbose output.
            if(verbose_flag)
                printf("Clearing remainder of flash:%ld bytes\n", ((uint32_t)flashDriver.PFlashTotalSize-bytesProcessed));

            // Erase remainder of the device, caters for previous image being larger than update.
            __disable_irq();
            flashResult = FLASH_Erase(&flashDriver, bytesProcessed, ((uint32_t)flashDriver.PFlashTotalSize-bytesProcessed), kFLASH_ApiEraseKey);
            __enable_irq();
        }

        // Any errors, report (assuming kernel still intact) and hang.
        if (flashResult != kStatus_FLASH_Success)
        {
            // This message may not be seen if the kernel has been wiped. put here in-case of error before erase.
            printf("Error: Failed to program new upgrade into Flash memory area!\n");
            printf("       Reset device. If device doesnt restart use an OpenSDA or JTAG programmer to reprogram.\n\n");
            while(1) {};
        }

        // Debug - place a message in an unused sector which can be checked to see if programming worked.
        if(debug_flag)
        {
            sprintf(buffer, "FLASH PROGRAMMING CHECK MESSAGE");
            for(uint16_t idx=strlen(buffer); idx < FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE; idx++) { buffer[idx] = 0x00; }
            __disable_irq();
            flashResult = FLASH_Erase(&flashDriver,   bytesProcessed+FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE, kFLASH_ApiEraseKey);
            flashResult = FLASH_Program(&flashDriver, bytesProcessed+FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE, (uint32_t*)buffer, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);
            __enable_irq();
            printf("Wrote check string at: %08lx\n", bytesProcessed+FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);
        }

        // Just in case we have output connectivity. If the update doesnt change too much then we should maintain connectivity with the USB.
        if(flashResult != kStatus_FLASH_Success)
        {
            printf("Error: Flash programming failed, addr:%ld, result:%ld\n", bytesProcessed, flashResult);
        }

        // Tidy up for exit.
        f_close(&fileHandle);
    } else
    {
        printf("Error: Failed to read update file:%s, aborting!\n", updateFile);
        return(13);
    }

    printf("Programming successful, please reset the device to activate update!\n");
    return(0);
}

#ifdef __cplusplus
}
#endif
