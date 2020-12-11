#########################################################################################################
##
## Name:            Makefile
## Created:         July 2019
## Author(s):       Philip Smart
## Description:     App Makefile - Build an App for the ZPU Test Application (zputa) or the zOS 
##                                 operating system.
##                  This makefile builds an app which is stored on an SD card and called by ZPUTA/zOS
##                  The app is for testing some component where the code is not built into ZPUTA or 
##                  a user application for zOS.
##
## Credits:         
## Copyright:       (c) 2019-20 Philip Smart <philip.smart@net2net.org>
##
## History:         July 2019   - Initial Makefile created for template use.
##                  April 2020  - Added K64F as an additional target and resplit ZPUTA into zOS.
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

ifeq ($(__K64F__),1)
  TOOLSPATH        = $(CURDIR)/../../tools
  COMPILERPATH     = $(TOOLSPATH)/arm/bin
  BASE	           = $(abspath $(COMPILERPATH))/arm-none-eabi
else
  TOOLSPATH        = /opt/zpu
  COMPILERPATH     = $(TOOLSPATH)/bin
  BASE	           = zpu-elf
endif
CC                 = $(BASE)-gcc
CXX                = $(BASE)-g++
LD                 = $(BASE)-gcc
AS                 = $(BASE)-as
CP                 = $(BASE)-objcopy
DUMP               = $(BASE)-objdump
OBJCOPY            = $(BASE)-objcopy
SIZE               = $(BASE)-size

FS_SUBDIRS        := falloc fattr fcat fcd fclose fconcat fcp fdel fdir fdrive fdump finspect flabel fmkdir
FS_SUBDIRS        += fmkfs fopen fread frename fsave fseek fshowdir fstat ftime ftrunc fwrite fxtract
DISK_SUBDIRS      := ddump dstat
BUFFER_SUBDIRS    := bdump bedit bread bwrite bfill blen
MEM_SUBDIRS       := mclear mcopy mdiff mdump meb meh mew mperf msrch mtest
HW_SUBDIRS        := hr ht tcpu
TST_SUBDIRS       := dhry coremark
MISC_SUBDIRS      := help time
APP_SUBDIRS       := tbasic mbasic kilo ed
ifeq ($(__K64F__),1)
  ifeq ($(__TRANZPUTER__),1)
    TZPU_SUBDIRS  := tzpu tzload tzdump tzclear tzclk tzreset tzio
  endif
else
  TZPU_SUBDIRS    :=
endif

SUBDIRS           := $(FS_SUBDIRS) $(DISK_SUBDIRS) $(BUFFER_SUBDIRS) $(MEM_SUBDIRS) $(HW_SUBDIRS) $(TST_SUBDIRS) $(MISC_SUBDIRS) $(APP_SUBDIRS) $(TZPU_SUBDIRS)
BASEDIR            = ../..
TARGETS           := all clean install

# Our target.
$(TARGETS): $(SUBDIRS)
#
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS) $(MAKEFLAGS)

install:
.PHONY: $(TARGETS) $(SUBDIRS)
