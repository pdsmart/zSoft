;--------------------------------------------------------------------------------------------------------
;
; Name:            zpu_macros.s
; Created:         June 2019
; Author(s):       Philip Smart
; Description:     ZPU Assembly Macros
;
;                  A set of macros to aid in writing assembly code for the ZPU.
;                  Macros are used widely for defining long IM loads or for API calls between an
;                  application and zOS/ZPUTA.
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
    .file "zpu_macros.s"

    ; Macro to generate Im instructions for a value upto 32bits.
	.macro fixedim value
			im \value
	.endm

    ; Macro to perform a subroutine call.
    .macro  jsr address
    
            im 8+0               ; save R0 - im pushes old TOS onto stack which becomes NOS
            load                 ;         - value retrieved from R0.
            im 8+4               ; save R1
            load
            im 8+8               ; save R2
            load
    
            fixedim \address
            call
            
            im 8+8
            store               ; restore R2
            im 8+4
            store               ; restore R1
            im 8+0
            store               ; restore R0
    .endm

    ; Macro to perform an api subroutine call.
    .macro  jsra address
            im      _memreg+12     ; Save the return address into the 4th memreg
            store 

            fixedim \address
            call
            
            im      _memreg+12     ; Retrieve the return address ready.
            load
    .endm

    ; Macro to perform an absolute jump.
	.macro  jmp address
			fixedim \address
			poppc
	.endm

	.macro fast_neg
	        not
            im 1
	        add
	.endm

    ; Macro to invoke a C library function.
    .macro cimpl funcname
            ; save R0
            im 8+0
            load
    
            ; save R1
            im 8+4
            load
    
            ; save R2
            im 8+8
            load
    
            loadsp 20
            loadsp 20
    
            fixedim \funcname
            call

            ; destroy arguments on stack
            storesp 0
            storesp 0    
     
            im 8+0
            load
    
            ; poke the result into the right slot
            storesp 24

            ; restore R2
            im 8+8
            store
    
            ; restore R1
            im 8+4
            store
    
            ; restore r0
            im 8+0
            store
    
            storesp 4
            poppc
    .endm

    .macro mult1bit
            ; create mask of lowest bit in A
            loadsp 8 ; A
            im 1
            and
            im -1
            add
            not
            loadsp 8 ; B
            and 
            add ; accumulate in C
    
            ; shift B left 1 bit
            loadsp 4 ; B
            addsp 0
            storesp 8 ; B
    
            ; shift A right 1 bit
            loadsp 8 ; A
            flip
            addsp 0
            flip
            storesp 12 ; A
    .endm

	.macro  defapi name
            ;.balign 16, 99 
            .global _\name
    _\name:
            im      _memreg+12     ; Save the return address into the 4th memreg
            store 

            im      \name
            call
            
            im      _memreg+12     ; Retrieve the return address ready.
            load
            im      OS_APPADDR+16
            poppc
	.endm

    .macro  defapifunc name offset
    	    .global \name;
            .set \name, \offset
    .endm
