#########################################################################################################
##
## Name:            Makefile
## Created:         April 2020
## Author(s):       Philip Smart
## Description:     uM Library 
##                  This makefile builds the uM Library for the various CPUs in the zOS/ZPUTA scope.
##
## Credits:         (C) Original authors in component files, please see component files for (C) notices.
## Copyright:       (c) 2020 Philip Smart <philip.smart@net2net.org>, Make system and changes.
##
## History:         April 2020     - Initial Makefile created.
##
## Notes:           Optional component enables:
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
ifeq ($(__ZPU__),)
  BASEDIR             = $(CURDIR)/..
  TOOLSPATH           = $(BASEDIR)/tools
  COMPILERPATH        = $(TOOLSPATH)/arm/bin	
  BASE                = $(abspath $(COMPILERPATH))/arm-none-eabi
else
  BASEDIR             = $(CURDIR)/..
  TOOLSPATH           = /opt/zpu
  COMPILERPATH        = $(TOOLSPATH)/bin	
  BASE                = zpu-elf
endif
CC                    = $(BASE)-gcc
CXX                   = $(BASE)-g++
LD                    = $(BASE)-gcc
AS                    = $(BASE)-as
AR                    = $(BASE)-ar
CP                    = $(BASE)-objcopy
DUMP                  = $(BASE)-objdump
OBJCOPY               = $(BASE)-objcopy
SIZE                  = $(BASE)-size
RANLIB                = $(BASE)-ranlib

# Version etc.
TITLE                 = "uM Library"
COPYRIGHT             = "This make system and enhancements (C) P.D. Smart 2020, Version 1.0\n"
COPYRIGHT            += "Component files, (C) Various authors, please see component files." 

# Working directory to build object files.
BUILD_DIR             = $(abspath $(CURDIR)/build)

# Library component directories.
COMMON_DIR            = $(CURDIR)/../common
INCLUDE_DIR           = $(CURDIR)/../include
UMLIBC_DIR            = $(CURDIR)/umlibc
IMATH_DIR             = $(CURDIR)/imath
LIB_DIR               = $(CURDIR)//lib
LIB_INCLUDE_DIR       = $(CURDIR)/include

# ANSI library.
UMLIBC_ANSI_C_SRC     = $(wildcard $(UMLIBC_DIR)/ansi/*.c)
UMLIBC_ANSI_CPP_SRC   = $(wildcard $(UMLIBC_DIR)/ansi/*.cpp)
UMLIBC_ANSI_SRC       = $(UMLIBC_ANSI_C_SRC:.c=.o)  $(UMLIBC_ANSI_CPP_SRC:.cpp=.o)
UMLIBC_ANSI_OBJS      = $(foreach src,$(UMLIBC_ANSI_SRC), $(BUILD_DIR)/$(src))

# MATH library.
UMLIBC_MATH_C_SRC     = $(wildcard $(UMLIBC_DIR)/math/*.c)
UMLIBC_MATH_CPP_SRC   = $(wildcard $(UMLIBC_DIR)/math/*.cpp)
UMLIBC_MATH_SRC       = $(UMLIBC_MATH_C_SRC:.c=.o)  $(UMLIBC_MATH_CPP_SRC:.cpp=.o)
UMLIBC_MATH_OBJS      = $(foreach src,$(UMLIBC_MATH_SRC), $(BUILD_DIR)/$(src))

# MATHF library.
UMLIBC_MATHF_C_SRC    = $(wildcard $(UMLIBC_DIR)/mathf/*.c)
UMLIBC_MATHF_CPP_SRC  = $(wildcard $(UMLIBC_DIR)/mathf/*.cpp)
UMLIBC_MATHF_SRC      = $(UMLIBC_MATHF_C_SRC:.c=.o)  $(UMLIBC_MATHF_CPP_SRC:.cpp=.o)
UMLIBC_MATHF_OBJS     = $(foreach src,$(UMLIBC_MATHF_SRC), $(BUILD_DIR)/$(src))

# MISC library.
UMLIBC_MISC_C_SRC     = $(wildcard $(UMLIBC_DIR)/misc/*.c)
UMLIBC_MISC_CPP_SRC   = $(wildcard $(UMLIBC_DIR)/misc/*.cpp)
UMLIBC_MISC_SRC       = $(UMLIBC_MISC_C_SRC:.c=.o)  $(UMLIBC_MISC_CPP_SRC:.cpp=.o)
UMLIBC_MISC_OBJS      = $(foreach src,$(UMLIBC_MISC_SRC), $(BUILD_DIR)/$(src))

# STDIO library.
UMLIBC_STDIO_C_SRC    = $(wildcard $(UMLIBC_DIR)/stdio/*.c)
UMLIBC_STDIO_CPP_SRC  = $(wildcard $(UMLIBC_DIR)/stdio/*.cpp)
UMLIBC_STDIO_SRC      = $(UMLIBC_STDIO_C_SRC:.c=.o)  $(UMLIBC_STDIO_CPP_SRC:.cpp=.o)
UMLIBC_STDIO_OBJS     = $(foreach src,$(UMLIBC_STDIO_SRC), $(BUILD_DIR)/$(src))

# Integer mathmatics for CRT0 and startup code library.
IMATH_C_SRC           = $(wildcard $(IMATH_DIR)/*.c)
IMATH_CPP_SRC         = $(wildcard $(IMATH_DIR)/*.cpp)
IMATH_SRC             = $(IMATH_C_SRC:.c=.o)  $(IMATH_CPP_SRC:.cpp=.o)
IMATH_OBJS            = $(foreach src,$(IMATH_SRC), $(BUILD_DIR)/$(src))

# Build time flags dependent on CPU target.
CFLAGS                = -I. -I$(UMLIBC_DIR)/include -I$(COMMON_DIR) -I$(INCLUDE_DIR) -I$(TEENSY35_DIR)
ifeq ($(__ZPU__),)
  CFLAGS             +=  -D__K64F__
else
  CFLAGS             +=  -D__ZPU__
endif

# Linker flags.
LDFLAGS               = -static

# Library name is suffixed by CPU.
#
ifeq ($(__ZPU__),)
  LIBTRG              = -k64f
else
  LIBTRG              = -zpu
endif


# Suffixes where interested in for this project.
#
.SUFFIXES:
.SUFFIXES: .o .c .h

# All, ie: all libraries to be built after a clean (necessary due to multi cpu platforms).
#
all:		Begin \
		    DoClean \
		    libumansi \
			libummath \
			libummathf \
			libumstdio \
			libummisc \
			libimath \
			DoInstall \
			End

# Targets for individual build or Any specific handling for library.
libumansi:	libumansi.a 

libummath: libummath.a
	
libummathf:	libummathf.a
	
libumstdio:	libumstdio.a
	
libummisc:	#libummisc.a

libimath:	libimath2.a


# How to clean up the directory... make it look pretty!
#
clean:      Begin \
            DoClean \
            End

# How to perform an installation of the resultant software.
#
install:    Begin \
            DoInstall \
            End
#
# Pre-make start sequence.
#
Begin:
	@echo $(TITLE)
	@echo $(COPYRIGHT)
	@echo
	@echo "Operation commencing @ `date`"
	@echo

#
# Post-make completion sequence.
#
End:
	@echo
	@echo "Completed @ `date`"

# Perform all cleanup operations to ensure future builds occur with
# completeness.
#
DoClean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.bak $(BUILD_DIR)/*$(LIBTRG).a $(BUILD_DIR)/*.BAK $(BUILD_DIR)/*.sav $(BUILD_DIR)/core
	rm -fr $(BUILD_DIR)/srv

# Perform installation of software as per spec.
#
DoInstall:
	mkdir -p $(LIB_DIR)
	mv $(CURDIR)/*.a $(LIB_DIR)/
	mkdir -p $(LIB_INCLUDE_DIR)
	cp -r $(UMLIBC_DIR)/include/* $(LIB_INCLUDE_DIR)/

# Build the UniX Library.
#
libumansi.a:	$(UMLIBC_ANSI_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

libummath.a:	$(UMLIBC_MATH_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

libummathf.a:	$(UMLIBC_MATHF_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

libumstdio.a:	$(UMLIBC_STDIO_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

libummisc.a:	$(UMLIBC_MISC_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

libimath2.a:	$(IMATH_OBJS)
	$(AR) rcsv $@ $+
	mv $(@F) `basename $(@F) .a`$(LIBTRG).a

$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.cpp Makefile
	@mkdir -p "$(dir $@)"
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/src/%.o: %.cpp Makefile
	@mkdir -p "$(dir $@)"
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(COMMON_DIR)/%.c Makefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(DHRY_DIR)/%.c Makefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(FATFS_DIR)/%.c Makefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(PFS_DIR)/%.c Makefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(OFLAGS)  -o $@ -c $<

$(BUILD_DIR)/%.o: %.s
	@mkdir -p "$(dir $@)"
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: $(STARTUP_DIR)/%.s
	@mkdir -p "$(dir $@)"
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# compiler generated dependency info
-include $(OBJS:.o=.d)
