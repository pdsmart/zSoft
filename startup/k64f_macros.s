#--------------------------------------------------------------------------------------------------------
#
# Name:            k64f_macros.s
# Created:         June 2019
# Author(s):       Philip Smart
# Description:     K64F Assembly Macros
#
#                  A set of macros to aid in writing assembly code for the ARM based K64F processor.
#                  Macros are used widely for defining API calls between an application and zOS/ZPUTA.
#
# Credits:         
# Copyright:       (c) 2019-20 Philip Smart <philip.smart@net2net.org>
#
# History:         June 2019   - Initial script based on ZPU linker script.
#
#--------------------------------------------------------------------------------------------------------
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
#--------------------------------------------------------------------------------------------------------
             .file "k64f_macros.s"

#	.macro  defapi name
#            ;.balign 16, 99 
#            .global _\name
#    _\name:
#            im      _memreg+12     ; Save the return address into the 4th memreg
#            store 
#
#            im      \name
#            call
#            
#            im      _memreg+12     ; Retrieve the return address ready.
#            load
#            im      OS_APPADDR+16
#            poppc
#	.endm

    .macro  defapifunc name offset
    	    .global \name;
            .set \name, \offset + 1
    .endm
