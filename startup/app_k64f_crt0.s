########################################################################################################
#
# Name:            app_k64f_crt0.h
# Created:         April 2020
# Author(s):       Philip Smart
# Description:     ZPUTA/zOS application startup code.
#                  This is the assembler startup file of any C compiled APPlication for the ZPUTA and
#                  zOS programs. It specifies the entry points into ZPUTA/zOS for standard functions
#                  and also the stack manipulation to handle parameters and return codes for the
#                  C APplication (ie. ZPUTA/zOS Calls APP with parameters and APP returns a result code).
#
# Credits:         
# Copyright:       (c) 2019-20 Philip Smart <philip.smart@net2net.org>
#
# History:         April 2020   - Initial script based on zpu appcrt0.s.
#
########################################################################################################
# This source file is free software: you can redistribute it and#or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This source file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org;licenses;>.
########################################################################################################
            .file      "app_k64f_crt0.s"
            .include   "k64f_macros.s"

            .syntax    unified
            .arch      armv7-m
            .thumb

            .section   .text,"ax"

            .type      _start, function

            // Define symbols in C to be linked against this assembly startup code.
            .extern    __bss_section_begin__
            .extern    __bss_section_end__
            .extern    __data_section_begin__
            .extern    __data_section_end__
            .extern    __text_section_begin__
            .extern    __text_section_end__
            .extern    app
            .extern    G
            .extern    cfgSoC

            .global    _start
           // Save registers normally preserved in a C call.
_start:    stmdb       sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
           stmdb       sp!, {r0, r1, r2, r3};


            // Copy initialized data from text area to working RAM (data) area.
            ldr        r1, DATA_BEGIN
            ldr        r2, TEXT_END
            ldr        r3, DATA_END
            subs       r3, r3, r1                          // Length of initialized data
            beq        zerobss                             // Skip if none

copydata:   ldrb       r4, [r2], #1                        // Read byte from flash
            strb       r4, [r1], #1                        // Store byte to RAM
            subs       r3, r3, #1                          // Decrement counter
            bgt        copydata                            // Repeat until done

            // Zero uninitialized data (bss)
            ldr        r1, BSS_BEGIN
            ldr        r3, BSS_END
            subs       r3, r3, r1                          // Length of uninitialized data
            beq        exec_app                            // Skip if none

zerobss:    mov        r2, #0x00                           // Initialise with 0
clearbss:   strb       r2, [r1], #1                        // Store zero
            subs       r3, r3, #1                          // Decrement counter
            bgt        clearbss                            // Repeat until done

            // Call app(uint32_t param1, uint32_t param2)
exec_app:   ldmia.w    sp!, {r0, r1, r2, r3}               // Get back the parameters ready to pass to app()

            // Setup the G and cfgSoC variables using passed parameters.
            ldr        r4, =G                              // Setup the pointer for G in the app to G in zOS/ZPUTA.
            str        r2, [r4]
            ldr        r4, =cfgSoC                         // Setup the pointer for cfgSoC in the app to cfgSoC in zOS/ZPUTA.
            str        r3, [r4]
                                                           // r0 = param1, r1 = param2
            bl         app                                 // Call the application.
                                                           // r0 = return code.
            // Restore C call registers, load lr straight to PC, R0 = return code.
            ldmia.w    sp!, {r4, r5, r6, r7, r8, r9, sl, pc}

fail_loop:  b          fail_loop                           // The pop pc above should never fail, just in case the stack gets corrupted, stay in a loop for safety.

#
#// These are filled in by the linker
#    
TEXT_BEGIN: .word      __text_section_begin__
TEXT_END:   .word      __text_section_end__
DATA_BEGIN: .word      __data_section_begin__
DATA_END:   .word      __data_section_end__
BSS_BEGIN:  .word      __bss_section_begin__ 
BSS_END:    .word      __bss_section_end__
#
            #-----------------------------------------------
            # zOS/ZPUTA API Function Entry Points
            #-----------------------------------------------
            #
            # Entry points in the parent program, zOS/ZPUTA, where the relevvant named functions can be accessed. The
            # entry point in this table is used as the address for the specified function in the APPlication such that
            # a call to a library function in the APPlication is actually using code in zOS/ZPUTA at runtime thus minimising
            # code in the APPlication.
            #
            # This table doesnt consume space, just defines the addresses which will be called for the relevant function.
            #
            .equ funcNext,  0x04;
            .equ funcAddr,  OS_BASEADDR+0x420;
            defapifunc      break funcAddr
            #
            # putc and xprint calls.
            #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      putchar funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xputc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xfputc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xputs funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xgets funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xfgets funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xfputs funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xatoi funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      uxatoi funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xprintf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xvprintf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xsprintf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xfprintf funcAddr
            #
            # getc calls
            #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      usb_serial_getchar funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      usb_serial_getchar funcAddr
            #
            # Util calls
            #
          # .equ funcAddr,  funcAddr+funcNext;
          # defapifunc      crc32_init funcAddr
          # .equ funcAddr,  funcAddr+funcNext;
          # defapifunc      crc32_addword funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      get_dword funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      rtcSet funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      rtcGet funcAddr
            #
            #FatFS System Calls
            #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_open funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_close funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_read funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_write funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_lseek funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_truncate funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_sync funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_opendir funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_closedir funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_readdir funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_findfirst funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_findnext funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_mkdir funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_unlink funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_rename funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_stat funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_chmod funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_utime funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_chdir funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_chdrive funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_getcwd funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_getfree funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_getlabel funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_setlabel funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_forward funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_expand funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_mount funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_mkfs funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_fdisk funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_setcp funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_putc funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_puts funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      f_printf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      f_gets funcAddr
           #
           # Low level disk calls.
           #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      disk_read funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      disk_write funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      disk_ioctl funcAddr
           #
           # Miscellaneous calls.
           #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getStrParam funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getUintParam funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      set_serial_output funcAddr
        #   .equ funcAddr,  funcAddr+funcNext;
        #   defapifunc      printBytesPerSec funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      printFSCode funcAddr

    .end
