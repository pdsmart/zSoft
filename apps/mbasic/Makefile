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
##                  23/4/2020   - Updated to merge mini basic, ed and command processor.
##
## Notes:           Optional component enables:
##                  USELOADB              - The Byte write command is implemented in hw#sw so use it.
##                  USE_BOOT_ROM          - The target is ROM so dont use initialised data.  ##                  MINIMUM_FUNTIONALITY  - Minimise functionality to limit code size.
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

APP_NAME       = mbasic
APP_DIR        = $(CURDIR)/..
APP_COMMON_DIR = $(CURDIR)/../common
COMMON_DIR     = $(CURDIR)../../common
BASEDIR        = ../../..

# Override values given by parent make for this application as its memory usage differs from the standard app.
ifeq ($(__K64F__),1)
override HEAPADDR  = 0x2000C000
override HEAPSIZE  = 0x0001BC00
override STACKADDR = 0x20027C00
override STACKSIZE = 0x00000400

# Modules making up Mbasic.
APP_C_SRC      = basic.c ed.c $(APP_COMMON_DIR)/sbrk.c $(COMMON_DIR)/readline.c $(APP_COMMON_DIR)/sysutils.c
UMM_C_SRC      =
CFLAGS         = -Os
CPPFLAGS       = -D__HEAPADDR__=$(HEAPADDR) -D__HEAPSIZE__=$(HEAPSIZE)
LDFLAGS        =
LIBS           = 
else
# Modules making up Mbasic.
APP_C_SRC      = basic.c ed.c $(APP_COMMON_DIR)/sbrk.c $(COMMON_DIR)/readline.c $(APP_COMMON_DIR)/sysutils.c
UMM_C_SRC      =
CFLAGS         = -Os
CPPFLAGS       = -D__HEAPADDR__=$(HEAPADDR) -D__HEAPSIZE__=$(HEAPSIZE)
LDFLAGS        = 
LIBS           =
endif

# Filter out the standard HEAP address and size, replacing with the ones required for this application.
# Useful for sub-makes
FILTER1        = $(filter-out $(filter HEAPADDR=%,$(MAKEFLAGS)), $(MAKEFLAGS))
FILTER2        = $(filter-out $(filter HEAPSIZE=%,$(FILTER1)),   $(FILTER1))
NEWMAKEFLAGS   = $(FILTER2) HEAPADDR=$(HEADADDR) HEAPSIZE=$(HEAPSIZE)

ifeq ($(__K64F__),1)
include        $(APP_DIR)/Makefile.k64f
else
# Mbasic needs 128K on the ZPU for some reason, 50% more than the Cortex-M4, so for small ZPU installations, we have to disable it!
#include        $(APP_DIR)/Makefile.zpu
clean:

all:

install:
endif
