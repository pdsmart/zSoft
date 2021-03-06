;--------------------------------------------------------------------------------------------------------
;
; Name:            ramcrt0.s
; Created:         June 2019
; Author(s):       Philip Smart
; Description:     zOS/ZPUTA startup code.
;
;                  This is the ZPU assembly language startup code for zOS/ZPUTA when the firmware runs
;                  in RAM. It defines the standard routines to clear and initialise memory along with
;                  methods to manipulate the stack to enable a zOS/ZPU APPlication to call functionality
;                  within the firmware. 
;                  Unlike the original ZPU crt0 this map does not require the low memory vector table
;                  and the soft-instructions as these are assumed to be in place by a bootstrap application
;                  such as IOCP.
;
; Credits:         
; Copyright:       (c) 2019-20 Philip Smart <philip.smart@net2net.org>
;
; History:         June 2019   - Initial script based on ZPU linker script.
;
;--------------------------------------------------------------------------------------------------------
; This source file is free software: you can redistribute it and#or modify
; it under the terms of the GNU General Public License as published
; by the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This source file is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http:#www.gnu.org;licenses;>.
;--------------------------------------------------------------------------------------------------------

        .file    "ramcrt0.s"
        .include "zpu_macros.s"
    
        ; This section is for API entry points which are used by external sub-processes to invoke functionality within this program.
        .section ".fixed_vectors","a"
    
        ; Define the location where the pointer to the interrupt handler is stored.
        .global _inthandler_fptr
        .set _inthandler_fptr, 0x00000400
    
        .global _start
_start:
        im  _stack                  ; Set stack address.
        popsp
        ;
        jmp _premain
        poppc

        .balign 32, 255

        ;--------------------------------
        ; ZPUTA API
        ;--------------------------------
        ;
        ; ZPUTA API calls for external sub-programs. Allows code snippets (apps) to use the logic built-in to ZPUTA, thus keeping
        ; the app code base small.
        ; This is a fixed size jump table which invokes the actual api code, per function, in the main .text
        ; section, allowing for optimizations.

        ; Break handler.
        jmp     _break

        ; putc and xprint calls
        ;
        jmp     __putchar
       ;jmp     _putc
        jmp     _fputc
        jmp     _puts
        jmp     _gets
        jmp     _fgets
        jmp     _fputs
        jmp     _xatoi
        jmp     _uxatoi
        jmp     _printf
       ;jmp     _xvprintf
        jmp     _sprintf
        jmp     _fprintf
        ;
        ; getc calls
        ;
        jmp     _getScreenWidth
        jmp     _getKey
        ;
        ; Util calls.
        ;
       ;jmp     _crc32_init
       ;jmp     _crc32_addword
        jmp     _get_dword
        jmp     _rtcSet
        jmp     _rtcGet
        ;
        ; FatFS System Calls.
        ;
        jmp     _f_open
        jmp     _f_close
        jmp     _f_read
        jmp     _f_write
        jmp     _f_lseek
        jmp     _f_truncate
        jmp     _f_sync
        jmp     _f_opendir
        jmp     _f_closedir
        jmp     _f_readdir
       ;jmp     _f_findfirst
       ;jmp     _f_findnext
        jmp     _f_mkdir
        jmp     _f_unlink
        jmp     _f_rename
        jmp     _f_stat
        jmp     _f_chmod
        jmp     _f_utime
        jmp     _f_chdir
        jmp     _f_chdrive
        jmp     _f_getcwd
        jmp     _f_getfree
        jmp     _f_getlabel
        jmp     _f_setlabel
       ;jmp     _f_forward
        jmp     _f_expand
        jmp     _f_mount
        jmp     _f_mkfs
       ;jmp     _f_fdisk
       ;jmp     _f_setcp
        jmp     _f_putc
       ;jmp     _f_puts
       ;jmp     _f_printf
        jmp     _f_gets
        ;
        ; Low level disk calls.
        ;
        jmp     _disk_read
        jmp     _disk_write
        jmp     _disk_ioctl
        ;
        ; Miscellaneous Calls.
        ;
        jmp     _getStrParam
        jmp     _getUintParam
        jmp     _set_serial_output
       ;jmp     _printBytesPerSec
        jmp     _printFSCode
        ;
        ; Memory management under OS control.
        ;
        jmp     _malloc
        jmp     _realloc
        jmp     _calloc
        jmp     _free

        ;--------------------------------
        ; End of ZPUTA API
        ;--------------------------------

        ;--------------------------------
        ; Main application startup.
        ;--------------------------------

        .section ".text","ax"

        .global _break;
_break:
        breakpoint
        im    _break
        poppc ; infinite loop

        ;
        ; putc and xprint calls
        ;
        defapi  _putchar
       ;defapi  putc
        defapi  fputc
        defapi  puts
        defapi  gets
        defapi  fgets
        defapi  fputs
        defapi  xatoi
        defapi  uxatoi
        defapi  printf
       ;defapi  xvprintf
        defapi  sprintf
        defapi  fprintf
        ;
        ; getc calls
        ;
        defapi  getScreenWidth
        defapi  getKey
        ;
        ; Util calls.
        ;
       ;defapi  crc32_init
       ;defapi  crc32_addword
        defapi  get_dword
        defapi  rtcSet
        defapi  rtcGet
        ;
        ; FatFS System Calls.
        ;
        defapi  f_open
        defapi  f_close
        defapi  f_read
        defapi  f_write
        defapi  f_lseek
        defapi  f_truncate
        defapi  f_sync
        defapi  f_opendir
        defapi  f_closedir
        defapi  f_readdir
       ;defapi  f_findfirst
       ;defapi  f_findnext
        defapi  f_mkdir
        defapi  f_unlink
        defapi  f_rename
        defapi  f_stat
        defapi  f_chmod
        defapi  f_utime
        defapi  f_chdir
        defapi  f_chdrive
        defapi  f_getcwd
        defapi  f_getfree
        defapi  f_getlabel
        defapi  f_setlabel
       ;defapi  f_forward
        defapi  f_expand
        defapi  f_mount
        defapi  f_mkfs
       ;defapi  f_fdisk
       ;defapi  f_setcp
        defapi  f_putc
       ;defapi  f_puts
       ;defapi  f_printf
        defapi  f_gets
        ;
        ; Low level disk calls.
        ;
        defapi  disk_read
        defapi  disk_write
        defapi  disk_ioctl
        ;
        ; Miscellaneous Calls.
        ;
        defapi  getStrParam
        defapi  getUintParam
        defapi  set_serial_output
       ;defapi  printBytesPerSec
        defapi  printFSCode
        ;
        ; Memory management by OS.
        ;
        defapi  malloc
        defapi  realloc
        defapi  calloc
        defapi  free
        .global _restart
_restart:

        ; Define weak linkage for _premain, so that it can be overridden
        .weak _premain
_premain:
        ;    clear BSS data, then call main.

        im __bss_start__                          ; bssptr
.clearloop:
        loadsp 0                                  ; bssptr bssptr 
        im __bss_end__                            ; __bss_end__  bssptr bssptr
        ulessthanorequal                          ; (bssptr<=__bss_end__?) bssptr
        impcrel .done                             ; &.done (bssptr<=__bss_end__?) bssptr
        neqbranch                                 ; bssptr
        im 0                                      ; 0 bssptr
        loadsp 4                                  ; bssptr 0 bssptr
        loadsp 0                                  ; bssptr bssptr 0 bssptr
        im 4                                      ; 4 bssptr bssptr 0 bssptr
        add                                       ; bssptr+4 bssptr 0 bssptr
        storesp 12                                ; bssptr 0 bssptr+4
        store                                     ; (write 0->bssptr)  bssptr+4
        im .clearloop                             ; &.clearloop bssptr+4
        poppc                                     ; bssptr+4
.done:
        im _break                                 ; &_break bssptr+4
        storesp 4                                 ; &_break
        im main                                   ; &main &break
        poppc                                     ; &break

        ; NB! this is not an EMULATE instruction. It is a varargs fn.
        .global _syscall    
_syscall:
        syscall
        poppc
        
        .section ".rodata"
        .balign 4,0
_mask:
        .long 0x00ffffff
        .long 0xff00ffff
        .long 0xffff00ff
        .long 0xffffff00

        .section ".bss"
        .balign 4,0

        .global _memreg
_memreg:
        .long 0
        .long 0
        .long 0
        .long 0
