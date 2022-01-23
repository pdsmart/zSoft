/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            zOS.cpp
// Created:         January 2019 - April 2021
// Author(s):       Philip Smart
// Description:     ZPU and Teensy 3.5 (Freescale K64F) OS and test application.
//                  This program implements methods, tools, test mechanisms and performance analysers such
//                  that a ZPU/K64F cpu and their encapsulating SoC can be used, tested, debugged,
//                  validated and rated in terms of performance.
//
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//                  (c) 2013      ChaN, framework for the SD Card testing.
//
// History:         January 2019   - Initial script written for the STORM processor then changed to the ZPU.
//                  July 2019      - Addition of the Dhrystone and CoreMark tests.
//                  December 2019  - Tweaks to the SoC config and additional commands (ie. mtest).
//                  April 2020     - With the advent of the tranZPUter SW, forked ZPUTA into zOS and 
//                                   enhanced to work with both the ZPU and K64F for the original purpose
//                                   of testing but also now for end application programming using the 
//                                   features of zOS where applicable.
//                  July 2020      - Tweaks to accomodate v2.1 of the tranZPUter board.
//                  December 2020  - Updates to allow soft CPU functionality on the v1.3 tranZPUter SW-700
//                                   board.
//                  April 2021     - Bug fixes to the SD directory cache and better interoperability with
//                                   the MZ-800. Bug found which was introduced in December where the
//                                   Z80 direction wasnt always set correctly resulting in some strange
//                                   and hard to debug behaviour.
//                  May 2021       - Preparations to add the M68000 architecture.
//                                 - Updates to allow Z80 to access 1Mbyte static RAM.
//                  June 2021      - Tracking a very hard bug (enabling of the MZ80B emulation which wasnt
//                                   quite working would cause disk access to randomly fail. The FPGA
//                                   is isolated from the K64F and trying interrupts disabled yielded no
//                                   success. Still ongoing but in the interim I disabled/removed threads
//                                   which are normally the first port of call for strange behaviour
//                                   but it was seen that using them for running the tranzputer service
//                                   wasnt really needed as this could be based on a readline idle call.
//                  Oct 2021       - Extensions to support the MZ-2000 host and the Sharp MZ Series FPGA
//                                   Emulation.
//
// Notes:           See Makefile to enable/disable conditional components
//                  USELOADB              - The Byte write command is implemented in hw/sw so use it.
//                  USE_BOOT_ROM          - The target is ROM so dont use initialised data.
//                  MINIMUM_FUNTIONALITY  - Minimise functionality to limit code size.
//                  __ZPU__               - Target CPU is the ZPU
//                  __K64F__              - Target CPU is the K64F
//                  __SD_CARD__           - Add the SDCard logic.
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

#if defined __ZPU__
  #ifdef __cplusplus
  extern "C" {
  #endif
#endif

#if defined __K64F__
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "WProgram.h"
  #include "k64f_soc.h"
  #include <../libraries/include/stdmisc.h>
  #include <TeensyThreads.h>

#elif defined __ZPU__
  #include <stdint.h>
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdmisc.h>
  #include "zpu_soc.h"
  #include "uart.h"

//  #define     malloc     sys_malloc
//  #define     realloc    sys_realloc
//  #define     calloc     sys_calloc
//  #define     free       sys_free
//  void        *sys_malloc(size_t);            // Allocate memory managed by the OS.
//  void        *sys_realloc(void *, size_t);   // Reallocate a block of memory managed by the OS.
//  void        *sys_calloc(size_t, size_t);    // Allocate and zero a block of memory managed by the OS.
//  void        sys_free(void *);               // Free memory managed by the OS.
#elif defined __M68K__
  #include <stdint.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <../libraries/include/stdmisc.h>
  #include "m68k_soc.h"
#endif

#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include "diskio.h"
#if defined __K64F__ || defined __ZPU__
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include "utils.h"
#include "readline.h"
#include "zOS_app.h"     /* Header for definitions specific to apps run from zOS */
#include "zOS.h"

#if defined __TRANZPUTER__
  #include <tranzputer.h>
#endif

#if defined __SHARPMZ__
  #include <sharpmz.h>
#endif

#if defined(BUILTIN_TST_DHRYSTONE) && BUILTIN_TST_DHRYSTONE == 1
  #include <dhry.h>
#endif
#if defined(BUILTIN_TST_COREMARK) && BUILTIN_TST_COREMARK == 1
  #include <coremark.h>
#endif

// Version info.
#define VERSION      "v1.41"
#define VERSION_DATE "28/10/2021"
#define PROGRAM_NAME "zOS"

// Utility functions.
#include "tools.c"

// Method to process interrupts. This involves reading the interrupt status register and then calling
// the handlers for each triggered interrupt. A read of the interrupt controller clears the interrupt pending
// register so new interrupts will be processed after this method exits.
//
#if defined __ZPU__
void interrupt_handler()
{
    // Read the interrupt controller to find which devices caused an interrupt.
    //
    uint32_t intr = INTERRUPT_STATUS(INTR0);

    // Prevent additional interrupts.
    DisableInterrupts();

    dbg_puts("ZPU Interrupt Handler");

    if(INTR_IS_TIMER(intr))
    {
        dbg_puts("Timer interrupt");
    }
    if(INTR_IS_PS2(intr))
    {
        dbg_puts("PS2 interrupt");
    }
    if(INTR_IS_IOCTL_RD(intr))
    {
        dbg_puts("IOCTL RD interrupt");
    }
    if(INTR_IS_IOCTL_WR(intr))
    {
        dbg_puts("IOCTL WR interrupt");
    }
    if(INTR_IS_UART0_RX(intr))
    {
        dbg_puts("UART0 RX interrupt");
    }
    if(INTR_IS_UART0_TX(intr))
    {
        dbg_puts("UART0 TX interrupt");
    }
    if(INTR_IS_UART1_RX(intr))
    {
        dbg_puts("UART1 RX interrupt");
    }
    if(INTR_IS_UART1_TX(intr))
    {
        dbg_puts("UART1 TX interrupt");
    }

    // Enable new interrupts.
    EnableInterrupts();
}

// Method to initialise the timer.
//
void initTimer()
{
    dbg_puts("Setting up timer...");
    TIMER_INDEX(TIMER1) = 0;                // Set first timer
    TIMER_COUNTER(TIMER1) = 100000;         // Timer is prescaled to 100KHz
}

// Method to enable the timer.
//
void enableTimer()
{
    dbg_puts("Enabling timer...");
    TIMER_ENABLE(TIMER1) = 1;               // Enable timer 0
}
#endif

#if defined __K64F__
void interrupt_handler()
{
    // Enable new interrupts.
    EnableInterrupts();
}
#endif

// Method to read lines from an open and valid autoexec.bat file or from the command line.
//
static FIL     fAutoExec;
static uint8_t autoExecState = 0;
uint8_t getCommandLine(char *buf, uint8_t bufSize)
{
    // Locals.
    char              *ptr;
    uint8_t           result = 0;
    //FRESULT           fr;

    // Clear the buffer.
    memset(buf, 0x00, bufSize);

    // First invocation, try and open an autoexec.bat file.
    //
    if(autoExecState == 0)
    {
        // If we cant open an autoexec.bat file then disable further automated processing.
        //
        if(f_open(&fAutoExec, AUTOEXEC_FILE, FA_OPEN_EXISTING | FA_READ))
        {
            autoExecState = 2;
        } else
        {
            autoExecState = 1;
        }
    }
    if(autoExecState == 1)
    {
        if((ptr = f_gets(buf, bufSize, &fAutoExec)) != NULL)
        {
            puts(ptr);
        }
        else
        {
            f_close(&fAutoExec);
            autoExecState = 2;
        }
    } 

    // If no autoexec processed, use the command line.
    //
    if(autoExecState == 2)
    {
        // Standard line input from command line (UART).
      #if defined BUILTIN_READLINE
        #if defined __ZPU__
        readline((uint8_t *)buf, bufSize, 0, HISTORY_FILE_ZPU,  NULL);
        #elif defined __K64F__
        readline((uint8_t *)buf, bufSize, 1, HISTORY_FILE_K64F, tranZPUterControl);
        #elif defined __M68K__
        readline((uint8_t *)buf, bufSize, 0, HISTORY_FILE_M68K, NULL);
        #endif
      #else
        ptr = buf;
        gets(ptr, bufSize);
      #endif
    }

    return(result);
}

#if defined __TRANZPUTER__
// Method to monitor and control the tranZPUter board provided services as requested.
//
void tranZPUterControl(void)
{
    // Locals.
    uint8_t        ioAddr;

    // If a user reset event occurred, reload the default ROM set.
    //
    if(isZ80Reset())
    {
        // Reset tranZPUter board, set memory map and ROMS as necessary.
        //
        hardResetTranZPUter();

        // Clear reset event which caused this reload.
        clearZ80Reset();
    }

    // Has there been an IO instruction for a service request?
    //
    if(getZ80IO(&ioAddr) == 1)
    {
//printf("Activity on IO:%02x\n", ioAddr);
        switch(ioAddr)
        {
            // Service request. Actual data about the request is stored in the Z80 memory, so read the request and process.
            //
            case IO_TZ_SVCREQ:
                // Handle the service request.
                //
                processServiceRequest();
                break;

            default:
                break;
        }
    } else
    {
        // Idle time call the tranzputer service routine to handle non-event driven tasks.
        //
        TZPUservice();
    }
}
#endif

// Method to setup access to the SD card.
//
static uint8_t        diskInitialised = 0;
static uint8_t        fsInitialised   = 0;
#if defined(__SD_CARD__)
int setupSDCard(void)
{
    // Local variables.
    FRESULT           fr = FR_INVALID_DRIVE;
    char              buf[120];

    // Initialise the first disk if FS enabled as external commands depend on it.
    //
    fr = FR_NOT_ENABLED;
    if(!disk_initialize(0, 1))
    {
        sprintf(buf, "0:");
        fr = f_mount(&G.FatFs[0], buf, 0);
    }

    if(fr)
    {
        printf("Failed to initialise sd card 0, please init manually.\n");
    } else
    {
        // Indicate disk and filesystem are accessible.
        diskInitialised = 1;
        fsInitialised   = 1;
    }

    // Indicate result, FR_OK = SD card setup and ready, all other values SD card not ready.
    return(fr);
}
#endif

// Interactive command processor. Allow user to input a command and execute accordingly.
//
int cmdProcessor(void)
{
    // Local variables.
    char              *ptr;
    char              line[120];
    char              *cmdline;
    long              p1;
    long              p2;
    long              p3;
    uint32_t          up1;
    uint32_t          up2;
    uint32_t          memAddr;
  #if defined(__SD_CARD__)
    char              *src1FileName;
    uint8_t           trying          = 0;
    uint32_t          retCode         = 0xffffffff;
    FRESULT           fr;
    #if defined(BUILTIN_HW_TEST_TIMERS) && BUILTIN_HW_TEST_TIMERS == 1
    RTC               rtc;
    #endif
  #endif
    #if defined(BUILTIN_FS_CHANGETIME) && BUILTIN_FS_CHANGETIME == 1
    FILINFO           Finfo;
    #endif

    // Initialise any globals in the structure used to pass working variables to apps.
    G.Sector = 0;

    while(1)
    {
        // Prompt to indicate input required.
        printf("* ");
        ptr = line;
        getCommandLine(line, sizeof(line));

        // main functions
        switch(decodeCommand(&ptr))
        {
          #if defined(BUILTIN_MISC_SETTIME) && BUILTIN_MISC_SETTIME == 1
            // CMD_MISC_SETTIME [<year> <mon> <day> <hour> <min> <sec>]
            case CMD_MISC_SETTIME:
                if (xatoi(&ptr, &p1)) {
                    rtc.year = (uint16_t)p1;
                    xatoi(&ptr, &p1); rtc.month = (uint8_t)p1;
                    xatoi(&ptr, &p1); rtc.day = (uint8_t)p1;
                    xatoi(&ptr, &p1); rtc.hour = (uint8_t)p1;
                    xatoi(&ptr, &p1); rtc.min = (uint8_t)p1;
                    if (!xatoi(&ptr, &p1)) break;
                    rtc.sec = (uint8_t)p1;
                    rtc.msec = 0;
                    rtc.usec = 0;
                    rtcSet(&rtc);
                }

                rtcGet(&rtc);
                printf("%u/%u/%u %02u:%02u:%02u.%03u%03u\n", rtc.year, rtc.month, rtc.day, rtc.hour, rtc.min, rtc.sec, rtc.msec, rtc.usec);
                break;
          #endif
    
            // MEMORY commands
            //
          #if defined(BUILTIN_MEM_CLEAR) && BUILTIN_MEM_CLEAR == 1
            // Clear memory <start addr> <end addr> [<word>]
            case CMD_MEM_CLEAR:
                if (!xatoi(&ptr, &p1)) break;
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3))
                {
                    p3 = 0x00000000;
                }
                printf("Clearing....");
                for(memAddr=(uint32_t)p1; memAddr < (uint32_t)p2; memAddr+=4)
                {
                    *(uint32_t *)(memAddr) = (uint32_t)p3;
                }
                printf("\n");
                break;
          #endif

          #if defined(BUILTIN_MEM_COPY) && BUILTIN_MEM_COPY == 1
            // Copy memory <start addr> <end addr> <dst addr>
            case CMD_MEM_COPY:
                if (!xatoi(&ptr, &p1)) break;
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) break;
                printf("Copying...");
                for(memAddr=(uint32_t)p1; memAddr < (uint32_t)p2; memAddr++, p3++)
                {
                    *(uint8_t *)(p3) = *(uint8_t *)(memAddr);
                }
                printf("\n");
                break;
          #endif

          #if defined(BUILTIN_MEM_DIFF) && BUILTIN_MEM_DIFF == 1
            // Compare memory <start addr> <end addr> <compare addr>
            case CMD_MEM_DIFF:
                if (!xatoi(&ptr, &p1)) break;
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) break;
                printf("Comparing...");
                for(memAddr=(uint32_t)p1; memAddr < (uint32_t)p2; memAddr++, p3++)
                {
                    if(*(uint8_t *)(p3) != *(uint8_t *)(memAddr))
                    {
                        printf("%08lx(%08x)->%08lx(%08x)\n", memAddr, *(uint8_t *)(memAddr), p3, *(uint8_t *)(p3));
                    }
                }
                printf("\n");
                break;
          #endif

          #if defined(BUILTIN_MEM_DUMP) && BUILTIN_MEM_DUMP == 1
            // Dump memory, [<start addr>, [<end addr>], [<size>]]
            case CMD_MEM_DUMP:
                if (!xatoi(&ptr, &p1))
                {
                  #if defined __ZPU__
                    if(cfgSoC.implInsnBRAM)     { p1 = cfgSoC.addrInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p1 = cfgSoC.addrBRAM; }
                    else if(cfgSoC.implRAM)     { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implSDRAM)   { p1 = cfgSoC.addrSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p1 = cfgSoC.addrWBSDRAM; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #elif defined __K64F__
                    if(cfgSoC.implRAM)          { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implFRAM)    { p1 = cfgSoC.addrFRAM; }
                    else if(cfgSoC.implFRAMNV)  { p1 = cfgSoC.addrFRAMNV; }
                    else if(cfgSoC.implFRAMNVC) { p1 = cfgSoC.addrFRAMNVC; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #elif defined __M68K__
                    if(cfgSoC.implInsnBRAM)     { p1 = cfgSoC.addrInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p1 = cfgSoC.addrBRAM; }
                    else if(cfgSoC.implRAM)     { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implSDRAM)   { p1 = cfgSoC.addrSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p1 = cfgSoC.addrWBSDRAM; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #else
                    #error "Target CPU not defined, use __ZPU__, __K64F__ or __M68K__"
                  #endif
                }
                if (!xatoi(&ptr,  &p2))
                {
                  #if defined __ZPU__
                    if(cfgSoC.implInsnBRAM)     { p2 = cfgSoC.sizeInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p2 = cfgSoC.sizeBRAM; }
                    else if(cfgSoC.implRAM)     { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implSDRAM)   { p2 = cfgSoC.sizeSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p2 = cfgSoC.sizeWBSDRAM; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #elif defined __K64F__
                    if(cfgSoC.implRAM)          { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implFRAM)    { p2 = cfgSoC.sizeFRAM; }
                    else if(cfgSoC.implFRAMNV)  { p2 = cfgSoC.sizeFRAMNV; }
                    else if(cfgSoC.implFRAMNVC) { p2 = cfgSoC.sizeFRAMNVC; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #elif defined __M68K__
                    if(cfgSoC.implInsnBRAM)     { p2 = cfgSoC.sizeInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p2 = cfgSoC.sizeBRAM; }
                    else if(cfgSoC.implRAM)     { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implSDRAM)   { p2 = cfgSoC.sizeSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p2 = cfgSoC.sizeWBSDRAM; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #else
                    #error "Target CPU not defined, use __ZPU__, __K64F__ or __M68K__"
                  #endif
                }
                if (!xatoi(&ptr,  &p3) || (p3 != 8 && p3 != 16 && p3 != 32))
                {
                    p3 = 8;
                }
                printf("Dump Memory\n");
                memoryDump(p1, p2, p3, p1, 0);
                printf("\nComplete.\n");
                break;
          #endif

          #if defined(BUILTIN_MEM_EDIT_BYTES) && BUILTIN_MEM_EDIT_BYTES == 1
            // Edit memory with bytes, <addr> <byte> [<byte> .... <byte>]
            case CMD_MEM_EDIT_BYTES:
                if (!xatoi(&ptr, &p1)) break;
                if (xatoi(&ptr, &p2))
                {
                    do {
                        *(uint8_t *)(p1++) = (uint8_t)p2;
                    } while (xatoi(&ptr, &p2));
                    break;
                }
                for (;;)
                {
                    printf("%08lX %02X-", (uint32_t)p1, *(uint8_t *)p1);
                    fgets(line, sizeof line, stdin);
                    ptr = line;
                    if (*ptr == '.') break;
                    if (*ptr < ' ') { p1++; continue; }
                    if (xatoi(&ptr, &p2))
                    {
                        *(uint8_t *)(p1++) = (uint8_t)p2;
                    } else {
                        printf("???\n");
                    }
                }
                break;
          #endif
                            
          #if defined(BUILTIN_MEM_EDIT_HWORD) && BUILTIN_MEM_EDIT_HWORD == 1
            // Edit memory with half-words, <addr> <16bit h-word> [<h-word> .... <h-word>]
            case CMD_MEM_EDIT_HWORD:
                if (!uxatoi(&ptr, &up1)) break;
                if (uxatoi(&ptr, &up2))
                {
                    do {
                        *(uint16_t *)(up1) = (uint16_t)up2;
                        up1 += 2;
                    } while (uxatoi(&ptr, &up2));
                    break;
                }
                for (;;)
                {
                    printf("%08lX %04X-", (uint32_t)up1, *(uint16_t *)up1);
                    fgets(line, sizeof line, stdin);
                    ptr = line;
                    if (*ptr == '.') break;
                    if (*ptr < ' ') { up1 += 2; continue; }
                    if (uxatoi(&ptr, &up2))
                    {
                        *(uint16_t *)(up1) = (uint16_t)up2;
                        up1 += 2;
                    } else {
                        printf("???\n");
                    }
                }
                break;
          #endif
                        
          #if defined(BUILTIN_MEM_EDIT_WORD) && BUILTIN_MEM_EDIT_WORD == 1
            // Edit memory with words, <addr> <word> [<word> .... <word>]
            case CMD_MEM_EDIT_WORD:
                if (!uxatoi(&ptr, &up1)) break;
                if (uxatoi(&ptr, &up2))
                {
                    do {
                        *(uint32_t *)(up1) = up2;
                        up1 += 4;
                    } while (uxatoi(&ptr, &up2));
                    break;
                }
                for (;;)
                {
                    printf("%08lX %08lX-", up1, *(uint32_t *)(up1));
                    fgets(line, sizeof line, stdin);
                    ptr = line;
                    if (*ptr == '.') break;
                    if (*ptr < ' ') { up1 += 4; continue; }
                    if (uxatoi(&ptr, &up2))
                    {
                        *(uint32_t *)(up1) = up2;
                        up1 += 4;
                    } else {
                        printf("???\n");
                    }
                }
                break;
          #endif

          #if defined(BUILTIN_MEM_SRCH) && BUILTIN_MEM_SRCH == 1
            // Search memory for first occurrence of a word.
            case CMD_MEM_SRCH:
                if (!xatoi(&ptr, &p1))
                {
                  #if defined __ZPU__
                    if(cfgSoC.implInsnBRAM)     { p1 = cfgSoC.addrInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p1 = cfgSoC.addrBRAM; }
                    else if(cfgSoC.implRAM)     { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implSDRAM)   { p1 = cfgSoC.addrSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p1 = cfgSoC.addrWBSDRAM; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #elif defined __K64F__
                    if(cfgSoC.implRAM)          { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implFRAM)    { p1 = cfgSoC.addrFRAM; }
                    else if(cfgSoC.implFRAMNV)  { p1 = cfgSoC.addrFRAMNV; }
                    else if(cfgSoC.implFRAMNVC) { p1 = cfgSoC.addrFRAMNVC; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #elif defined __M68K__
                    if(cfgSoC.implInsnBRAM)     { p1 = cfgSoC.addrInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p1 = cfgSoC.addrBRAM; }
                    else if(cfgSoC.implRAM)     { p1 = cfgSoC.addrRAM; }
                    else if(cfgSoC.implSDRAM)   { p1 = cfgSoC.addrSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p1 = cfgSoC.addrWBSDRAM; }
                    else { p1 = cfgSoC.stackStartAddr - 512; }
                  #else
                    #error "Target CPU not defined, use __ZPU__, __K64F__ or __M68K__"
                  #endif
                }
                if (!xatoi(&ptr,  &p2))
                {
                  #if defined __ZPU__
                    if(cfgSoC.implInsnBRAM)     { p2 = cfgSoC.sizeInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p2 = cfgSoC.sizeBRAM; }
                    else if(cfgSoC.implRAM)     { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implSDRAM)   { p2 = cfgSoC.sizeSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p2 = cfgSoC.sizeWBSDRAM; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #elif defined __K64F__
                    if(cfgSoC.implRAM)          { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implFRAM)    { p2 = cfgSoC.sizeFRAM; }
                    else if(cfgSoC.implFRAMNV)  { p2 = cfgSoC.sizeFRAMNV; }
                    else if(cfgSoC.implFRAMNVC) { p2 = cfgSoC.sizeFRAMNVC; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #elif defined __M68K__
                    if(cfgSoC.implInsnBRAM)     { p2 = cfgSoC.sizeInsnBRAM; }
                    else if(cfgSoC.implBRAM)    { p2 = cfgSoC.sizeBRAM; }
                    else if(cfgSoC.implRAM)     { p2 = cfgSoC.sizeRAM; }
                    else if(cfgSoC.implSDRAM)   { p2 = cfgSoC.sizeSDRAM; }
                    else if(cfgSoC.implWBSDRAM) { p2 = cfgSoC.sizeWBSDRAM; }
                    else { p2 = cfgSoC.stackStartAddr + 8; }
                  #else
                    #error "Target CPU not defined, use __ZPU__, __K64F__ or __M68K__"
                  #endif
                }
                if (!xatoi(&ptr,  &p3))
                {
                    p3 = 0;
                }
                printf("Searching..\n");
                for(memAddr=(uint32_t)p1; memAddr < (uint32_t)p2; memAddr+=4)
                {
                    if(*(uint32_t *)(memAddr) == (uint32_t)p3)
                    {
                        printf("%08lx->%08lx\n", memAddr, *(uint32_t *)(memAddr));
                    }
                }
                printf("\n");
                break;
          #endif

          #if defined(BUILTIN_MEM_TEST) && BUILTIN_MEM_TEST == 1
            // Test memory, [<start addr> [<end addr>] [<iter>] [<test bitmap>]]
            case CMD_MEM_TEST:
                printf("Test Memory not-builtin\n");
                break;
          #endif

            // EXECUTION commands.
            case CMD_EXECUTE:
            {
                if (!xatoi(&ptr, &p1)) break;
                printf("Executing code @ %08lx ...\n", p1);
                void *jmpptr = (void *)p1;
                goto *jmpptr;
            }
            break;

            case CMD_CALL:
            {
                if (!xatoi(&ptr, &p1)) break;
                printf("Calling code @ %08lx ...\n", p1);
                int  (*func)(void) = (int (*)(void))p1;
                int retCode = func();
                if(retCode != 0)
                {
                    printf("Call returned code (%d).\n", retCode);
                }
            }
            break;

            // MISC commands
            case CMD_MISC_RESTART_APP:
                printf("Restarting application...\n");
              #if defined __ZPU__
                _restart();
              #endif
                break;

            // Reboot the ZPU to the cold start location.
            case CMD_MISC_REBOOT:
            {
                printf("Cold rebooting...\n");
                void *rbtptr = (void *)0x00000000;
                goto *rbtptr;
            }
            break;

          #if defined __SHARPMZ__
            // Clear the screen.
            //
            case CMD_MISC_CLS:
                mzClearScreen(3, 1);
                break;
               
            // Exit zOS and return to the Z80 host processor.
            //
            case CMD_MISC_Z80:
                mzSetZ80();
                break;
          #endif            

            // Configuration information
            case CMD_MISC_INFO:
                showSoCConfig();
                break;

           #if defined __ZPU__ || defined __K64F__
            // Test point - add code here when a test is needed on a kernel element then invoke after boot.
            case CMD_MISC_TEST:
                testRoutine();
                break;
           #endif

        #if defined(__SD_CARD__)
            // CMD_FS_CAT <name> - cat/output file
          #if defined(BUILTIN_FS_CAT) && BUILTIN_FS_CAT == 1
            case CMD_FS_CAT:
                fr = fileCat(getStrParam(&ptr));
                if(fr) { printFSCode(fr); } 
                break;
          #endif

            // CMD_FS_LOAD <name> <addr> - Load a file into memory
          #if defined(BUILTIN_FS_LOAD) && BUILTIN_FS_LOAD == 1
            case CMD_FS_LOAD:
                src1FileName = getStrParam(&ptr);
                memAddr = getUintParam(&ptr);
                fr = fileLoad(src1FileName, memAddr, 1);
                if(fr) { printFSCode(fr); } 
                break;
          #endif
        #endif
            
            // Unrecognised command, if SD card enabled, see if the command exists as an app. If it is an app, load and execute
            // otherwise throw error..
            case CMD_BADKEY:
            default:
                // Reset to start of line - if we match a command but it isnt built in, need to search for it on disk.
                //
                if(line[0] != '\0')
                {
                    // Duplicate the command line to pass it unmodified to the application.
                    cmdline = strndup(line, sizeof(line));
                    if(cmdline != NULL)
                    {
                #if defined(__SD_CARD__)
                        // Get the name of the command to try the various formats of using it.
                        ptr = cmdline;
                        src1FileName=getStrParam(&ptr);

                        if(diskInitialised && fsInitialised && strlen(src1FileName) < 16)
                        {
                            // The user normally just types the command, but it is possible to type the drive and or path and or extension, so cater
                            // for these possibilities by trial. An alternate way is to disect the entered command but I think this would take more code space.
                            trying = 1;
                            while(trying)
                            {
                                switch(trying)
                                {
                                    // Try formatting with all the required drive and path fields.
                                    case 1:
                                        sprintf(&line[40], "%d:\\%s\\%s.%s", APP_CMD_BIN_DRIVE, APP_CMD_BIN_DIR, src1FileName, APP_CMD_EXTENSION);
                                        break;
                                       
                                    // Try command as is.
                                    case 2:
                                        sprintf(&line[40], "%s", src1FileName);
                                        break;
                                         
                                    // Try command as is but with drive and bin dir.
                                    case 3: 
                                        sprintf(&line[40], "%d:\\%s\\%s", APP_CMD_BIN_DRIVE, APP_CMD_BIN_DIR, src1FileName);
                                        break;
                                       
                                    // Try command as is but with just drive.
                                    case 4: 
                                    default:
                                        sprintf(&line[40], "%d:\\%s", APP_CMD_BIN_DRIVE, src1FileName);
                                        break;
                                }
                                //                  command   Load addr          Exec addr          Mode of exec    Param1          Param2               Global struct   SoC Config struct 
                              #if defined __ZPU__
                                retCode = fileExec(&line[40], APP_CMD_LOAD_ADDR, APP_CMD_EXEC_ADDR, EXEC_MODE_CALL, (uint32_t)ptr,  (uint32_t)cmdline,   (uint32_t)&G,   (uint32_t)&cfgSoC);
                              #else
                                //printf("%s,%08lx,%08lx,%d,%s,%s\n", &line[40], APP_CMD_LOAD_ADDR, APP_CMD_EXEC_ADDR, EXEC_MODE_CALL, ptr, cmdline);
                                retCode = fileExec(&line[40], APP_CMD_LOAD_ADDR, APP_CMD_EXEC_ADDR, EXEC_MODE_CALL, (uint32_t)ptr,  (uint32_t)cmdline,   (uint32_t)&G,   (uint32_t)&cfgSoC);
                              #endif

                                if(retCode == 0xffffffff && trying <= 3)
                                {
                                    trying++;
                                } else
                                {
                                    trying = 0;
                                }
                            }
                        }
                        if(!diskInitialised || !fsInitialised || retCode == 0xffffffff)
                        {
                            printf("Bad command.\n");
                        }

                        // Free up the duplicated command line.
                        //
                        free(cmdline);
                #else
                        // Free up the duplicated command line.
                        //
                        free(cmdline);
                        printf("Unknown command!\n");
                #endif
                    } else
                    {
                        printf("Memory exhausted, cannot process command.\n");
                    }
                }
                break;

            // No input
            case CMD_NOKEY:
                break;
        }
    }
}

// Startup method of zOS, basic hardware initialisation before spawning the command processor and/or default application.
//
int main(int argc, char **argv)
{
    // Locals.
  #if defined __ZPU__ || defined __M68K__
    FILE osIO;
  #endif

    // Initialisation.
    //
    G.fileInUse = 0;

    // When zOS is the booted app or is booted by the tiny IOCP bootstrap, initialise hardware as it hasnt yet been done.
  #if defined __ZPU__
    #if defined(OS_BASEADDR) && (OS_BASEADDR == 0x0000 || OS_BASEADDR == 0x1000)
      // Setup the required baud rate for the UART. Once the divider is loaded, a reset takes place within the UART.
      UART_BRGEN(UART0) = BAUDRATEGEN(UART0, 115200, 115200);
      UART_BRGEN(UART1) = BAUDRATEGEN(UART1, 115200, 115200);

      // Enable the RX/TX units and enable FIFO mode.
      UART_CTRL(UART0)  = UART_TX_FIFO_ENABLE | UART_TX_ENABLE | UART_RX_FIFO_ENABLE | UART_RX_ENABLE;
      UART_CTRL(UART1)  = UART_TX_FIFO_ENABLE | UART_TX_ENABLE | UART_RX_FIFO_ENABLE | UART_RX_ENABLE;
    #endif
  #endif

    // For the K64F the millisecond timer is created by an interrupt every 1ms which updates a variable. This needs to be
    // exposed to the applications as linking in the teensy3 code creates too many dependencies.
    //
  #if defined __K64F__
    G.millis = &systick_millis_count;
  #endif

    // Setup the monitor serial port and the handlers to output/receive a character.
    //
  #if defined __K64F__
    Serial.begin(9600);

    // I/O is connected in the _read and_write methods withiin startup file mx20dx128.c.
    setbuf(stdout, NULL);

  #elif defined __SHARPMZ__
    // Setup the Input/Output streams to use the screen drivers.
    fdev_setup_stream(&osIO, mzPrintChar, mzGetChar, _FDEV_SETUP_RW);
    stdout = stdin = stderr = &osIO;

    // Initialise and clear screen.
    mzInit();

  #elif defined __ZPU__
    fdev_setup_stream(&osIO, uart_putchar, uart_getchar, _FDEV_SETUP_RW);
    stdout = stdin = stderr = &osIO;

  #elif defined __M68K__

  #endif

  #if defined __TRANZPUTER__
    // Setup the tranZPUter hardware ready for action!
    setupTranZPUter(0, VERSION, VERSION_DATE);
  #endif
 
    // Setup the configuration using the SoC configuration register if implemented otherwise the compiled internals.
    setupSoCConfig();

    // Ensure interrupts are disabled whilst setting up.
    DisableInterrupts();

    //printf("Setting up timer...\n");
    //TIMER_INDEX(TIMER1) = 0;                // Set first timer
    //TIMER_COUNTER(TIMER1) = 100000;         // Timer is prescaled to 100KHz
    //enableTimer();

    // Enable interrupts.
    SetIntHandler(interrupt_handler);

  #if defined __ZPU__
    //EnableInterrupt(INTR_TIMER | INTR_PS2 | INTR_IOCTL_RD | INTR_IOCTL_WR | INTR_UART0_RX | INTR_UART0_TX | INTR_UART1_RX | INTR_UART1_TX);
    //EnableInterrupt(INTR_UART0_RX | INTR_UART1_RX); // | INTR_TIMER);
  #endif

  #if defined(__SD_CARD__)
    setupSDCard();
  #endif

  #if defined __TRANZPUTER__
    // If the SD card is present and ready, initialise the tranZPUter logic dependent upon file storage.
    if(diskInitialised && fsInitialised)
    {
        // Setup memory on Z80 to default.
        loadTranZPUterDefaultROMS(CPUMODE_SET_Z80);

        // Cache initial directory.
        svcCacheDir(TZSVC_DEFAULT_MZF_DIR, MZF, 1);
      
        //No SD card found so setup tranZPUter accordingly.
        setupTranZPUter(1, NULL, NULL);
    } else
    {
        // No SD card found so setup tranZPUter accordingly.
        setupTranZPUter(9, NULL, NULL);
    }
  #endif

  #if defined __K64F__
    // Give time for the USB Serial Port to connect.
    delay(2000);
  #endif

    // Signon with version information.
    printVersion(1);

  #if defined __TRANZPUTER__
    // Complete tranZPUter setup.
    setupTranZPUter(8, NULL, NULL);
  #endif
  
    // Command processor. If it exits, then reset the CPU.
    cmdProcessor();

    // Reboot as it is not normal the command processor terminates.
    void *rbtptr = (void *)0x00000000;
    goto *rbtptr;
}

#if defined __ZPU__
  #ifdef __cplusplus
  }
  #endif
#endif
