#########################################################################################################
##
## Name:            Makefile
## Created:         July 2019
## Author(s):       Philip Smart
## Description:     zOS - zpu Operating System
##                  This makefile builds the zOS application the purpose of which is to provide test,
##                  monitoring tools and an execution environment for applications for the ZPU and
##                  devices within the SoC. 
##                  zOS more recently upgraded to handle the K64F on the Teensy 3.5 board which is used
##                  as part of the tranZPUter SW project.
##
## Credits:         
## Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
##
## History:         July 2019   - Initial Makefile created for template use.
##                  April 2020  - Split from the latest ZPUTA and added K64F logic to support the
##                                tranZPUter SW board.
##                  April 2021  - Added M68000
##
## Notes:           Optional component enables:
##                  USELOADB              - The Byte write command is implemented in hw#sw so use it.
##                  USE_BOOT_ROM          - The target is ROM so dont use initialised data.
##                  MINIMUM_FUNTIONALITY  - Minimise functionality to limit code size.
##
#########################################################################################################
## This source file is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This source file is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#########################################################################################################

ZOS_DIR        = .
ifeq ($(__K64F__),1)
include        $(ZOS_DIR)/Makefile.k64f
else ifeq ($(__ZPU__),1)
include        $(ZOS_DIR)/Makefile.zpu
else ifeq ($(__M68K__),1)
include        $(ZOS_DIR)/Makefile.m68k
else
$(error Please run using __K64F__=1,  __ZPU__=1 or __M68K__=1 according to target processor)
endif
