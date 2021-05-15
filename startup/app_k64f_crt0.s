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
            .extern    app                                 // Main app entry point.
            .extern    G                                   // Global shared variables with OS.
            .extern    cfgSoC                              // Configuration structure to define CPU and hardware.
            .extern    stdin                               // Standard input file stream.
            .extern    stdout                              // Standard output file stream.
            .extern    stderr                              // Standard error file stream.

            .global    _start
           // Save registers normally preserved in a C call.
_start:     stmdb       sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
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

            // Reminder, ARM calling convention in C allocation table.
            //
            // Register  Synonym  Contents?    Purpose
            //   r0                   N    argument/results
            //   r1                   N    argument/results
            //   r2                   N    argument/results
            //   r3                   N    argument/results
            //   r4                   Y    local variable
            //   r5                   Y    local variable
            //   r6                   Y    local variable
            //   r7                   Y    local variable
            //   r8                   Y    local variable
            //   r9                   Y    depends on platform standard
            //   r10                  Y    local variable
            //   r11      fp          Y    frame pointer/local variable
            //   r12      ip          N    intra-procedure-call scratch
            //   r13      sp          Y    stack pointer
            //   r14      lr          N    link register
            //   r15      pc          N    program counter
            // 4-> parameters are passed on the stack starting at sp. If values are pushed prior to accessing the parameters, they need to be read at the correct stack
            // offset. ie. In this routine, 12 registers are pushed, 48 bytes in total, so any access to parameter 4 (first parameter on stack) is at #48.
            //
            // NB: To force a hard fault to see stack trace, issue instruction .word 0xffffffff inline.

            // Call app(uint32_t param1, uint32_t param2)
exec_app:   ldr.w      r6, [sp, #48]                       // Get stdin, stdout, stderr file descriptors passed on stack.
            ldr        r4, =stdin                          // Setup the pointer for stdin in the app to stdin in zOS/ZPUTA.
            str        r6, [r4]

            ldr.w      r6, [sp, #52]
            ldr        r4, =stdout                         // Setup the pointer for stdout in the app to stdout in zOS/ZPUTA.
            str        r6, [r4]

            ldr.w      r6, [sp, #56]
            ldr        r4, =stderr                         // Setup the pointer for stderr in the app to stderr in zOS/ZPUTA.
            str        r6, [r4]

            // Setup the G and cfgSoC variables using passed parameters.
            ldmia.w    sp!, {r0, r1, r2, r3}               // Get back the parameters ready to pass to app()
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
            defapifunc      putc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      fputc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      puts funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      gets funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      fgets funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      fputs funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      xatoi funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      uxatoi funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      printf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      sprintf funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      fprintf funcAddr
            #
            # getc calls
            #
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getScreenWidth funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getKey funcAddr
            #
            # Util calls
            #
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      crc32_init funcAddr
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      crc32_addword funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      get_dword funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      rtcSet funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      rtcGet funcAddr
            #
            # FatFS System Calls
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
           #.equ funcAddr,  funcAddr+funcNext;
           #defapifunc      printBytesPerSec funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      printFSCode funcAddr

            # Memory management calls.
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      sys_malloc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      sys_realloc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      sys_calloc funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      sys_free funcAddr

            # tranZPUter kernel methods.
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      setupZ80Pins              funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      resetZ80                  funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      reqZ80Bus                 funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      reqMainboardBus           funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      reqTranZPUterBus          funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      setupSignalsForZ80Access  funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      releaseZ80                funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      writeZ80Memory            funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      readZ80Memory             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      writeZ80IO                funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      readZ80IO                 funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      refreshZ80                funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      refreshZ80AllRows         funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      setCtrlLatch              funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      setZ80CPUFrequency        funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      copyFromZ80               funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      copyToZ80                 funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      fillZ80Memory             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      testZ80Memory             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      captureVideoFrame         funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      refreshVideoFrame         funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      loadVideoFrameBuffer      funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      saveVideoFrameBuffer      funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getVideoFrame             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getAttributeFrame         funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      loadZ80Memory             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      loadMZFZ80Memory          funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      saveZ80Memory             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      memoryDumpZ80             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      isZ80Reset                funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      isZ80MemorySwapped        funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      getZ80IO                  funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      clearZ80Reset             funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      hardResetTranZPUter       funcAddr
            .equ funcAddr,  funcAddr+funcNext;
            defapifunc      convertSharpFilenameToAscii funcAddr
    .end
