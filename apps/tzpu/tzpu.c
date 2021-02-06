/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tzpu.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     The TranZPUter control program, responsible for booting up and configuring the
//                  underlying host, providing SD card services and interactive menus.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2020  - Initial write of the TranZPUter software.
//                  July 2020 - Another test, a bug with the VHDL on the v2.1 tranZPUter board, so 
//                              needed to output a constant memory mode. The bug turned out to be
//                              a delayed tri-stating of the bus.
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

#define TZPU_VERSION "1.0"

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
//
#include <app.h>
#include <tranzputer.h>
#include "tzpu.h"

// Utility functions.
#include <tools.c>

// Version info.
#define VERSION              "v1.1"
#define VERSION_DATE         "10/12/2020"
#define APP_NAME             "TZPU"

void testBus(void)
{
    printf("Requesting Z80 BUS and Mainboard access\n");
    if(reqMainboardBus(100) == 0)
    {
        setupSignalsForZ80Access(WRITE);
        uint8_t data = 0x00;
     //   while(digitalRead(Z80_WAIT) == 0);
        for(uint16_t idx=0xD000; idx < 0xD800; idx++)
        {
            writeZ80Memory(idx, data, MAINBOARD);
           // delay(1000000);
            data++;
        }
        releaseZ80();
    }
    else
    {
        printf("Failed to obtain the Z80 bus.\n");
    }
}

void testT80BusReqBug(void)
{
    printf("Repeating a bus request 100 times\n");
    while(true)
    {
        if(reqTranZPUterBus(100, TRANZPUTER) == 0)
        {
            setupSignalsForZ80Access(WRITE);
            uint8_t data = 0x07;
            writeZ80Memory(0x0060, data, TRANZPUTER);
            releaseZ80();
        }
        else
        {
            printf("Failed to obtain the Z80 bus.\n");
        }
    }
}

void testSixtyBug(void)
{
    printf("Repeating a write to 0x0060\n");
    while(true)
    {
        if(reqTranZPUterBus(100, TRANZPUTER) == 0)
        {
            setupSignalsForZ80Access(WRITE);
            uint8_t data = 0x07;
            writeZ80Memory(0x0060, data, TRANZPUTER);
            releaseZ80();
        }
        else
        {
            printf("Failed to obtain the Z80 bus.\n");
        }
    }
}

void testVRAMLocation(void)
{
    printf("Requesting Z80 BUS and Mainboard access\n");
    if(reqMainboardBus(100) == 0)
    {
        setupSignalsForZ80Access(READ);

        for(uint32_t idx=0; idx < 0xFFFFFFFF; idx++)
        {
            uint8_t dataV = readZ80Memory(0xD000);
            uint8_t dataA = readZ80Memory(0xD800);
            printf("%02x %02x\r", dataV, dataA);
        }
        releaseZ80();
    }
    else
    {
        printf("Failed to obtain the Z80 bus.\n");
    }
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
  //  char      *ptr = (char *)param1;
  //  char      *pathName;    
    uint32_t  retCode = 1;
    
    // Get name of file to load.
    //
    //pathName    = getStrParam(&ptr);
    //if(strlen(pathName) == 0)
    //{
    //    printf("Usage: tzpu <file>\n");
    //}
   // _init_Teensyduino_internal_();
   // setupZ80Pins(1, G->millis);
   //
    testVRAMLocation();
    testSixtyBug();

    printf("Loading Monitor ROM\n");
    loadZ80Memory("SA1510.rom", 0, 0x00000000, 0, 0, TRANZPUTER, 1);

    printf("Loading Floppy ROM\n");
    loadZ80Memory("1Z-013A.rom", 0, 0x0000F000, 0, 0, TRANZPUTER, 1);

    printf("Testing Display\n");
    testBus();
    //displaySignals();

    return(retCode);
}

#ifdef __cplusplus
}
#endif
