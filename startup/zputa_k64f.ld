/********************************************************************************************************
 *
 * Name:            zputa_k64f.tmpl
 * Created:         April 2020
 * Author(s):       Philip Smart
 * Description:     zputa linker map.
 *                  This is the linker map for the zputa testing firmware on the K64F processor as found
 *                  on the Teensy 3.5 board. It defines where the firmware is loaded, size and location.
 *                  Currently, as the K64F is fixed in terms of flash and memory, the parameters arent
 *                  dynamically changed by the build.sh script, although later use of the Teensy 3.6
 *                  may require this.
 *
 * Credits:         
 * Copyright:       (c) 2019-20 Philip Smart <philip.smart@net2net.org>
 *
 * History:         April 2020   - Initial script based on ZPU linker script.
 *
 ********************************************************************************************************
 * This source file is free software: you can redistribute it and#or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This source file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:#www.gnu.org;licenses;>.
 ********************************************************************************************************/

/* Linker file for GNU C Compiler */

/* Entry Point */
ENTRY(_VectorsFlash)

/* Specify the memory areas */
MEMORY
{
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x00080000
	RAM  (rwx) : ORIGIN = 0x2001A000, LENGTH = 0x00016000
}
/*
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
	RAM  (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 262136
*/

/* Define output sections */
SECTIONS
{
	.text : {
		. = 0;
		KEEP(*(.vectors))
		*(.startup*)
		/* TODO: does linker detect startup overflow onto flashconfig? */
		. = 0x400;
		KEEP(*(.flashconfig*))
        . = 0x420;
        KEEP(*(.zputavectors*))
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		KEEP(*(.init))
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} > FLASH = 0xFF

	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > FLASH
	_etext = .;

	.usbdescriptortable (NOLOAD) : {
		/* . = ORIGIN(RAM); */
		. = ALIGN(512);
		*(.usbdescriptortable*)
	} > RAM

	.dmabuffers (NOLOAD) : {
		. = ALIGN(4);
		*(.dmabuffers*)
	} > RAM

	.usbbuffers (NOLOAD) : {
		. = ALIGN(4);
		*(.usbbuffers*)
	} > RAM

	.data : AT (_etext) {
		. = ALIGN(4);
		_sdata = .; 
		*(.fastrun*)
		*(.data*)
		. = ALIGN(4);
		_edata = .; 
	} > RAM

	.noinit (NOLOAD) : {
		*(.noinit*)
	} > RAM

    /* Uninitialized data section */
	.bss : {
		. = ALIGN(4);
		_sbss = .;
        __START_BSS = .;
		__bss_start__ = .;
        *(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
		__bss_end__ = .;
        __END_BSS = .;
	} > RAM

    __heap_section_start__ = .;
    .heap 0x2001E000:
    {
        . = ALIGN(8);
		_ebss = .;
        /* __end__ = .; */
        /* PROVIDE(end = .); */
        __HeapBase = .;
        . += 0x00010000;
        __HeapLimit = .;
    } > RAM
    __heap_section_end__ = .;

    __stack_section_start__ = .;
    .stack 0x2002F000 :
    {
      . = ALIGN(8);
      . += 0x1000;
    } > RAM
    __stack_section_end__ = .;

    __StackTop   = .;
    __StackLimit = __StackTop - 0x1000;
    PROVIDE(__stack = __StackTop);
	_estack = ORIGIN(RAM) + LENGTH(RAM) - 8;
	_teensy_model_identifier = 0x1F;
}
