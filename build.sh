#!/bin/bash
#========================================================================================================
# NAME
#     build.sh -  Shell script to build a ZPU/K64F/M68K program or OS.
#
# SYNOPSIS
#     build.sh [-CIOoMBAsTZdxh]
#
# DESCRIPTION
#
# OPTIONS
#     -C <CPU>      = Small, Medium, Flex, Evo, EvoMin, K64F, M68K - defaults to Evo.
#     -I <iocp ver> = 0 - Full, 1 - Medium, 2 - Minimum, 3 - Tiny (bootstrap only)
#     -O <os>       = zputa, zos
#     -o <os ver>   = 0 - Standalone, 1 - As app with IOCP Bootloader,
#                     2 - As app with tiny IOCP Bootloader, 3 - As app in RAM 
#     -M <size>     = Max size of the boot ROM/BRAM (needed for setting Stack).
#     -B <addr>     = Base address of <os>, default -o == 0 : 0x00000 else 0x01000 
#     -A <addr>     = App address of <os>, default 0x0C000
#     -N <size>     = Required size of heap
#     -n <size>     = Required size of application heap
#     -S <size>     = Required size of stack
#     -s <size>     = Required size of application stack
#     -a <size>     = Maximum size of an app, defaults to (BRAM SIZE - App Start Address - Stack Size) 
#                     if the App Start is located within BRAM otherwise defaults to 0x10000.
#     -T            = TranZPUter specific build, adds initialisation and setup code.
#     -Z            = Sharp MZ series ZPU build, zOS runs as an OS host on Sharp MZ hardware.
#     -d            = Debug mode.
#     -x            = Shell trace mode.
#     -h            = This help screen.
#
# EXAMPLES
#     build.sh -O zputa -B 0x00000 -A 0x50000
#
# EXIT STATUS
#      0    The command ran successfully
#
#      >0    An error ocurred.
#
#EndOfUsage <- do not remove this line
#========================================================================================================
# History:
#          v1.00         : Initial version (C) P. Smart January 2019.
#          v1.10         : Changes to better calculate mode and addresses and setup linker scripts.
#          v1.11         : Added CPU as it is clear certain features must be disabled in the original
#                          CPU's, ie. small where the emulated mul and div arent working as they should.
#          v1.2          : Added more zOS logic and the K64F processor target for the tranZPUter board.
#          v1.21         : Additional changes to manage heap.
#          v1.22         : Added code to build the libraries.
#          v1.23         : Added flags for Sharp MZ series specific build.
#          v1.24         : Added M68000 support
#========================================================================================================
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#========================================================================================================

PROG=${0##*/}
#PARAMS="`basename ${PROG} '.sh'`.params"
ARGS=$*

##############################################################################
# Load program specific variables
##############################################################################

# VERSION of this RELEASE.
#
VERSION="1.23"

# Constants.
BUILDPATH=`pwd`

# Temporary files.
TMP_DIR=/tmp
TMP_OUTPUT_FILE=${TMP_DIR}/tmpoutput_$$.log
TMP_STDERR_FILE=${TMP_DIR}/tmperror_$$.log

# Log mechanism setup.
#
LOG="/tmp/${PROG}_`date +"%Y_%m_%d"`.log"
LOGTIMEWIDTH=40
LOGMODULE="MAIN"

# Mutex's - prevent multiple threads entering a sensitive block at the same time.
#
MUTEXDIR="/var/tmp"

##############################################################################
# Utility procedures
##############################################################################

# Function to output Usage instructions, which is soley a copy of this script header.
#
function Usage
{
    # Output the lines at the start of this script from NAME to EndOfUsage
    cat $0 | nawk 'BEGIN {s=0} /EndOfUsage/ { exit } /NAME/ {s=1} { if (s==1) print substr( $0, 3 ) }'
    exit 1
}

# Function to output a message in Log format, ie. includes date, time and issuing module.
#
function Log
{
    DATESTR=`date "+%d/%m/%Y %H:%M:%S"`
    PADLEN=`expr ${LOGTIMEWIDTH} + -${#DATESTR} + -1 + -${#LOGMODULE} + -15`
    printf "%s %-${PADLEN}s %s\n" "${DATESTR} [$LOGMODULE]" " " "$*"
}

# Function to terminate the script after logging an error message.
#
function Fatal
{
    Log "ERROR: $*"
    Log "$PROG aborted"
    exit 2
}

# Function to output the Usage, then invoke Fatal to exit with a terminal message.
#
function FatalUsage
{
    # Output the lines at the start of this script from NAME to EndOfUsage
    cat $0 | nawk 'BEGIN {s=0} /EndOfUsage/ { exit } /NAME/ {s=1} { if (s==1) print substr( $0, 3 ) }'
    echo " "
    echo "ERROR: $*"
    echo "$PROG aborted"
    exit 3
}

# Function to output a message if DEBUG mode is enabled. Primarily to see debug messages should a
# problem occur.
#
function Debug
{
    if [ $DEBUGMODE -eq 1 ]; then
        Log "$*"
    fi
}

# Function to output a file if DEBUG mode is enabled.
#
function DebugFile
{
    if [ $DEBUGMODE -eq 1 ]; then
        cat $1
    fi
}

# Take an input value to be hex, validate it and format correctly.
function getHex
{
    local __resultvar=$2
    local inputHex=`echo $1 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Value:$1 is not hexadecimal."
    fi
    local decimal=`echo "16 i $inputHex p" | dc`
    local outputHex=`printf '0x%08X' "$((10#$decimal))"`
    eval $__resultvar="'$outputHex'"
}

# Add two hexadecimal values of the form result = 0xAAAAAA + 0xBBBBBB
function addHex
{
    local __resultvar=$3
    local inputHex1=`echo $1 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex1 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 1:$1/$inputHex1 is not hexadecimal."
    fi
    local inputHex2=`echo $2 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex2 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 2:$2/$inputHex2 is not hexadecimal."
    fi
    local sumDecimal=`echo "16 i $inputHex1 $inputHex2 + p" | dc`
    local outputHex=`printf '0x%08X' "$((10#$sumDecimal))"`
    eval $__resultvar="'$outputHex'"
}

# Subtract two hexadecimal values of the form result = 0xAAAAAA - 0xBBBBBB
function subHex
{
    local __resultvar=$3
    local inputHex1=`echo $1 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex1 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 1:$1/$inputHex1 is not hexadecimal."
    fi
    local inputHex2=`echo $2 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex2 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 2:$2/$inputHex2 is not hexadecimal."
    fi
    local sumDecimal=`echo "16 i $inputHex1 $inputHex2 - p" | dc`
    local outputHex=`printf '0x%08X' "$((10#$sumDecimal))"`
    eval $__resultvar="'$outputHex'"
}

# Function to round, via masking, a hex value with another value.
# ie. 0x20028932 & 0x2002F000 = 0x20028000
function roundHex
{
    local __resultvar=$3
    local inputHex1=`echo $1 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex1 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 1:$1/$inputHex1 is not hexadecimal."
    fi
    local inputHex2=`echo $2 | tr 'a-z' 'A-Z'|sed 's/0X//g'`
    TEST=$(( 16#$inputHex2 )) 2> /dev/null 
    if [ $? != 0 ]; then
        FatalUsage "Param 2:$2/$inputHex2 is not hexadecimal."
    fi
    local outputHex=`printf '0x%08X' "$((16#$inputHex1 & 16#$inputHex2))"`
    eval $__resultvar="'$outputHex'"
}


#########################################################################################
# Check enviroment
#########################################################################################

# Correct directory to start.
cd ${BUILDPATH}

# Preload default values into parameter/control variables.
#
DEBUGMODE=0;
TRACEMODE=0;
CPU=EVO;
OS=ZPUTA;
IOCP_VERSION=3;
BRAM_SIZE=0x10000;
getHex 0x01000 OS_BASEADDR;
getHex 0x0C000 APP_BASEADDR;
APP_LEN=0x02000;
APP_BOOTLEN=0x20;                 # Fixed size as this is the jump table to make calls within ZPUTA.
APP_HEAP_SIZE=0x1000;
APP_STACK_SIZE=0x400;
OS_HEAP_SIZE=0x4000;
OS_STACK_SIZE=0x1000;
TRANZPUTER=0
SHARPMZ=0
OSVER=2;

# Process parameters, loading up variables as necessary.
#
if [ $# -gt 0 ]; then
    while getopts ":hC:I:O:o:M:B:A:N:n:S:s:da:xTZ" opt; do
        case $opt in
            d)     DEBUGMODE=1;;
            C)     CPU=`echo ${OPTARG} | tr 'a-z' 'A-Z'`;;
            I)     IOCP_VERSION=${OPTARG};;
            O)     OS=`echo ${OPTARG} | tr 'a-z' 'A-Z'`;;
            o)     OSVER=${OPTARG};;
            M)     getHex ${OPTARG} BRAM_SIZE;;
            B)     getHex ${OPTARG} OS_BASEADDR;;
            A)     getHex ${OPTARG} APP_BASEADDR;;
            N)     getHex ${OPTARG} OS_HEAP_SIZE;;
            n)     getHex ${OPTARG} APP_HEAP_SIZE;;
            S)     getHex ${OPTARG} OS_STACK_SIZE;;
            s)     getHex ${OPTARG} APP_STACK_SIZE;;
            a)     getHex ${OPTARG} APP_LEN;;
            T)     TRANZPUTER=1;;
            Z)     SHARPMZ=1;;
            x)     set -x; TRACEMODE=1;;
            h)     Usage;;
           \?)     FatalUsage "Unknown option: -${OPTARG}";;
        esac
    done
    shift $(($OPTIND - 1 ))
fi

# Check the program to build is correct
if [ "${OS}" != "ZPUTA" -a "${OS}" != "ZOS" ]; then
    FatalUsage "Given <os> is not valid, must be 'zputa' or 'zos'"
fi
if [ "${OS}" = "ZPUTA" ]; then
    OSTYPE="__ZPUTA__"
else
    OSTYPE="__ZOS__"
fi
OSNAME=`echo ${OS} | tr 'A-Z' 'a-z'`

#     -C <CPU>      = Small, Medium, Flex, Evo, K64F - defaults to Evo.
# Check the CPU is correctly defined.
if [ "${CPU}" != "SMALL" -a "${CPU}" != "MEDIUM" -a "${CPU}" != "FLEX" -a "${CPU}" != "EVO" -a "${CPU}" != "EVOMIN" -a "${CPU}" != "K64F" -a "${CPU}" != "M68K" ]; then
    FatalUsage "Given <cpu> is not valid, must be one of 'Small', 'Medium', 'Flex', 'Evo', 'K64F', 'M68K'"
fi
if [ ${CPU} = "K64F" ]; then
    CPUTYPE="__K64F__"
elif [ ${CPU} = "M68K" ]; then
    CPUTYPE="__M68K__"
else
    CPUTYPE="__ZPU__"
fi

# Check the addresses for K64F, cannot use the default ZPU values.
if [ "${CPU}" = "K64F" -a "${OS_BASEADDR}" = "0x00001000" -a "${APP_BASEADDR}" = "0x0000C000" ]; then
    getHex 0x00000000 OS_BASEADDR;
    getHex 0x1fff0000 APP_BASEADDR;
fi

# Check IOCP_VERSION which has no meaning for the K64F/M68K processors.
if [ "${CPU}" = "K64F" -a "${IOCP_VERSION}" != "3" ]; then
    Fatal "-I < iocp ver> has no meaning for the K64F, there is no version or indeed no need for IOCP on this processor."
elif [ "${CPU}" = "M68K" -a "${IOCP_VERSION}" != "3" ]; then
    Fatal "-I < iocp ver> has no meaning for the M68K, there is no version or indeed no need for IOCP on this processor."
fi

# Check OSVER which has no meaning for the K64F/M68K processors.
if [ "${CPU}" = "K64F" -a "${OSVER}" != "2" ]; then
    Fatal "-o <os ver> has no meaning for the K64F, base is in Flash RAM and applications, if SD card enabled, are in RAM."
elif [ "${CPU}" = "M68K" -a "${OSVER}" != "2" ]; then
    Fatal "-o <os ver> has no meaning for the M68K, base is in Flash RAM and applications, if SD card enabled, are in RAM."
fi

# Setup any specific build options.
if [ ${TRANZPUTER} -eq 1 ]; then
    if [ "${CPU}" = "K64F" -a "${OS}" = "ZOS" ]; then 
        BUILDFLAGS="__TRANZPUTER__=1"
    fi
fi

# Setup any specific build options.
if [ ${SHARPMZ} -eq 1 ]; then
    if [[ "${CPU}" != "EVO" && "${CPU}" != "M68K" ]] || [[ "${OS}" != "ZOS" ]]; then
        Fatal "-Z only valid for zOS build on ZPU hardware."
    fi
    if [[ "${CPU}" = "EVO" || "${CPU}" = "M68K" ]] && [[ "${OS}" = "ZOS" ]]; then 
        BUILDFLAGS="__SHARPMZ__=1"
    fi
fi

# Clear out the build target directory.
rm -fr ${BUILDPATH}/build
mkdir -p ${BUILDPATH}/build
mkdir -p ${BUILDPATH}/build/SD

# Build message according to CPU Type
if [ "${CPUTYPE}" = "__ZPU__" ]; then
    Log "Building: ${OS}, OS_BASEADDR=${OS_BASEADDR}, APP_BASEADDR=${APP_BASEADDR} ..."
elif [ "${CPUTYPE}" = "__K64F__" ]; then
    Log "Building: ${OS} ..."
elif [ "${CPUTYPE}" = "__M68K__" ]; then
    Log "Building: ${OS} ..."
else
    Fatal "Internal error, unrecognised CPUTYPE at Build Info"
fi

# Stack start address (at the moment) is top of BRAM less 8 bytes (2 words), standard for the ZPU. There is
# no reason though why this cant be a fixed address determined by the developer in any memory location.
subHex ${BRAM_SIZE} 8 STACK_STARTADDR

# Build the libraries for this platform.
cd ${BUILDPATH}/libraries
make ${CPUTYPE}=1 CPU=${CPU} all
if [ $? != 0 ]; then
    Fatal "Aborting, failed to build the libraries for the ${CPUTYPE} processor."
fi

# Setup the bootloader if the OS is not standalone.
#
#if [ "${OSVER}" -ne 0 ]; then

    # IOCP is only needed by the ZPU family.
    if [ "${CPUTYPE}" = "__ZPU__" ]; then

        # Setup variables to meet the required IOCP configuration.
        #
        FUNCTIONALITY=${IOCP_VERSION} 
        if [ ${IOCP_VERSION} = 0 ]; then
            IOCP_BASEADDR=0x000000;
            IOCP_BOOTLEN=0x000400;
            IOCP_STARTADDR=0x000400;
            IOCP_LEN=0x003700;
            OS_SD_TARGET="BOOT.ROM"
        elif [ ${IOCP_VERSION} = 1 ]; then
            IOCP_BASEADDR=0x000000;
            IOCP_BOOTLEN=0x000400;
            IOCP_STARTADDR=0x000400;
            IOCP_LEN=0x002000;
            OS_SD_TARGET="BOOT.ROM"
        elif [ ${IOCP_VERSION} = 2 ]; then
            IOCP_BASEADDR=0x000000;
            IOCP_BOOTLEN=0x000400;
            IOCP_STARTADDR=0x000400;
            IOCP_LEN=0x002000;
            OS_SD_TARGET="BOOT.ROM"
        elif [ ${IOCP_VERSION} = 3 ]; then
            IOCP_BASEADDR=0x000000;
            IOCP_BOOTLEN=0x000400;
            IOCP_STARTADDR=0x000400;
            IOCP_LEN=0x001000;
            OS_SD_TARGET="BOOTTINY.ROM"
        else
            FatalUsage "Illegal IOCP Version."
        fi
        IOCP_SD_TARGET="IOCP_${FUNCTIONALITY}_${IOCP_BASEADDR}.bin"
        addHex ${IOCP_BASEADDR} ${IOCP_LEN} IOCP_APPADDR

        if [ $DEBUGMODE -eq 1 ]; then
            Log "IOCP_BASEADDR=${IOCP_BASEADDR}, IOCP_BOOTLEN=${IOCP_BOOTLEN}, IOCP_STARTADDR=${IOCP_STARTADDR}, IOCP_LEN=${IOCP_LEN}, OS_SD_TARGET=${OS_SD_TARGET}"
        fi

        Log "Building IOCP version - ${IOCP_VERSION}"
        cat ${BUILDPATH}/startup/iocp_bram.tmpl | sed -e "s/BOOTADDR/${IOCP_BASEADDR}/g"   \
                                                      -e "s/BOOTLEN/${IOCP_BOOTLEN}/g"     \
                                                      -e "s/IOCPSTART/${IOCP_STARTADDR}/g" \
                                                      -e "s/IOCPLEN/${IOCP_LEN}/g"         \
                                                      -e "s/STACK_SIZE/${STACK_SIZE}/g"    \
                                                      -e "s/STACK_ADDR/${STACK_STARTADDR}/g" > ${BUILDPATH}/startup/iocp_bram_${IOCP_BASEADDR}.ld
        cd ${BUILDPATH}/iocp
        make ${CPUTYPE}=1 clean
        if [ $? != 0 ]; then
            Fatal "Aborting, failed to clean IOCP build environment!"
        fi

        Log "make ${CPUTYPE}=1 IOCP_BASEADDR=${IOCP_BASEADDR} IOCP_APPADDR=${IOCP_APPADDR} FUNCTIONALITY=${FUNCTIONALITY} CPU=${CPU}"
        make ${CPUTYPE}=1 IOCP_BASEADDR=${IOCP_BASEADDR} IOCP_APPADDR=${IOCP_APPADDR} FUNCTIONALITY=${FUNCTIONALITY} CPU=${CPU}
        if [ $? != 0 ]; then
            Fatal "Aborting, failed to build IOCP!"
        fi
        cp ${BUILDPATH}/iocp/iocp.bin ${BUILDPATH}/build/${IOCP_SD_TARGET}
    fi
#fi

# Setup variables to meet the required zOS/ZPUTA configuration.
# 0 - Standalone, 1 - As app with IOCP Bootloader, 2 - As app with tiny IOCP Bootloader, 3 - As app in RAM 
# For CPU Type K64F all parameters are static.
if [ "${CPUTYPE}" = "__K64F__" ]; then
    OSBUILDSTR="${OSNAME}_k64f"
    OS_BOOTADDR=0x000000;
    OS_BASEADDR=0x000000;
    OS_BOOTLEN=0x000000;

elif [ "${CPUTYPE}" = "__ZPU__" ]; then
    if [ ${OSVER} = 0 ]; then
        OSBUILDSTR="${OSNAME}_standalone_boot_in_bram"
        OS_BOOTADDR=0x000000;
        OS_BASEADDR=0x000000;
        OS_BOOTLEN=0x000600;
    elif [ ${OSVER} = 1 ]; then
        OSBUILDSTR="${OSNAME}_with_iocp_in_bram"
        OS_BOOTADDR=${OS_BASEADDR};
        OS_BOOTLEN=0x000200;
    elif [ ${OSVER} = 2 ]; then
        OSBUILDSTR="${OSNAME}_with_tiny_iocp_in_bram"
        OS_BOOTADDR=${OS_BASEADDR};
        OS_BOOTLEN=0x000200;
    elif [ ${OSVER} = 3 ]; then
        OSBUILDSTR="${OSNAME}_as_app_in_ram"
        OS_BOOTADDR=${OS_BASEADDR};
        OS_BOOTLEN=0x000200;
    else
        FatalUsage "Illegal OS Version."
    fi 
elif [ "${CPUTYPE}" = "__M68K__" ]; then
    #OSBUILDSTR="${OSNAME}_m68k"
    OSBUILDSTR="${OSNAME}_standalone_boot_in_bram"
    OS_BOOTADDR=0x000000;
    OS_BASEADDR=0x000000;
    OS_BOOTLEN=0x000000;
fi

# For the ZPU, the start of the Boot loader, OS and application can change so need to calculate the Boot start and len, OS start and len, application Start and len.
if [ "${CPUTYPE}" = "__ZPU__" ]; then
    # Calculate the Start address of the OS. The OS has a Boot Address followed by a reserved space for microcode and hooks before the main OS code.
    addHex ${OS_BOOTLEN} ${OS_BASEADDR}           OS_STARTADDR

    # Calculate the Start address of the Application. An Application has a Boot Address, a reserved space for OS Hooks and then the application start.
    addHex ${APP_BOOTLEN} ${APP_BASEADDR}         APP_STARTADDR

    # Calculate the maximum Application length by subtracting the size of the BRAM - Application Start - Stack Space
    if [ "${APP_LEN}" = "" -a $(( 16#`echo ${APP_STARTADDR} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) -lt  $(( 16#`echo ${BRAM_SIZE} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) ]; then
        subHex ${BRAM_SIZE} ${APP_STARTADDR}      APP_LEN
        subHex ${APP_LEN} 0x20                    APP_LEN
    
    # If the APPLEN isnt set, give it a meaningful default.
    elif [ $(( 16#`echo ${APP_LEN} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) -eq 0 ]; then
        APP_LEN=0x10000;
    fi

    # Calculate the start of the Operating system code as the first section from the boot address is reserved.
    addHex ${OS_BOOTADDR} ${OS_BOOTLEN}           OS_STARTADDR

    # Calculate the length of the OS which is the start address of the App less the Boot address of the OS.
    subHex ${APP_BASEADDR} ${OS_BOOTADDR}         OS_LEN
    subHex ${OS_LEN} ${OS_BOOTLEN}                OS_LEN

    # Calculate the heap and stack vars.
    subHex ${BRAM_SIZE} 8                         OS_STACK_ENDADDR
    subHex ${OS_STACK_ENDADDR} ${OS_STACK_SIZE}   OS_STACK_STARTADDR
    subHex ${OS_STACK_STARTADDR} 4                OS_HEAP_ENDADDR
    subHex ${OS_HEAP_ENDADDR} ${OS_HEAP_SIZE}     OS_HEAP_STARTADDR

    # Stack start address for the APP. Normally this isnt used as the stack from zOS/ZPUTA is maintained, but available incase of
    # need for a local stack.
    addHex ${APP_STARTADDR} ${APP_LEN}            APP_ENDADDR
    #subHex ${APP_ENDADDR} ${OS_STACK_SIZE}        APP_ENDADDR
    subHex ${APP_ENDADDR} 8                       APP_STACK_ENDADDR
    subHex ${APP_STACK_ENDADDR} ${APP_STACK_SIZE} APP_STACK_STARTADDR

    # Calculate the heap start and end.
    subHex ${APP_STACK_STARTADDR} 4               APP_HEAP_ENDADDR
    subHex ${APP_HEAP_ENDADDR} ${APP_HEAP_SIZE}   APP_HEAP_STARTADDR

elif [ "${CPUTYPE}" = "__K64F__" ]; then

    # For the K64F the OS location and size is the 512K Flash RAM, this can be changed if a bootloader is installed before zOS.
    OS_STARTADDR=0x00000000
    addHex ${OS_STARTADDR} 0x00080000             OS_LEN

    # Calculate the heap, stack and RAM start address vars.
    OS_RAM_ENDADDR=0x20030000
    OS_RAM_MASK=0x3FFFF000
    OS_RAM_OSMEM=0x010000
    subHex ${OS_RAM_ENDADDR} 8                    OS_STACK_ENDADDR
    subHex ${OS_RAM_ENDADDR} ${OS_STACK_SIZE}     OS_STACK_STARTADDR
    roundHex ${OS_STACK_STARTADDR} ${OS_RAM_MASK} OS_STACK_STARTADDR
    subHex ${OS_STACK_STARTADDR} 4                OS_HEAP_ENDADDR
    subHex ${OS_HEAP_ENDADDR} ${OS_HEAP_SIZE}     OS_HEAP_STARTADDR
    roundHex ${OS_HEAP_STARTADDR} ${OS_RAM_MASK}  OS_HEAP_STARTADDR
    subHex ${OS_HEAP_STARTADDR} ${OS_RAM_OSMEM}   OS_RAM_STARTADDR
    roundHex ${OS_RAM_STARTADDR} ${OS_RAM_MASK}   OS_RAM_STARTADDR
    subHex ${OS_RAM_ENDADDR} ${OS_RAM_STARTADDR}  OS_RAM_LEN

    # For the K64F, the APP address start is as given on the command line or inbuilt default. The APPLEN is MAX RAM - APP Start Address.
    APP_STARTADDR=${APP_BASEADDR}
    APP_ENDADDR=${OS_RAM_STARTADDR}
    APP_RAM_MASK=0xFFFFF000
    subHex ${APP_ENDADDR} ${APP_BASEADDR}         APP_LEN

    # Stack start address for the APP. Normally this isnt used as the stack from zOS/ZPUTA is maintained, but available incase of
    # need for a local stack.
    subHex ${APP_ENDADDR} 8                       APP_STACK_ENDADDR
    subHex ${APP_STACK_ENDADDR} ${APP_STACK_SIZE} APP_STACK_STARTADDR
    roundHex ${APP_STACK_STARTADDR} ${APP_RAM_MASK} APP_STACK_STARTADDR

    # Calculate the heap start and end.
    subHex ${APP_STACK_STARTADDR} 0               APP_HEAP_ENDADDR
    subHex ${APP_HEAP_ENDADDR} ${APP_HEAP_SIZE}   APP_HEAP_STARTADDR
    roundHex ${APP_HEAP_STARTADDR} ${APP_RAM_MASK} APP_HEAP_STARTADDR

#subHex ${APP_STACK_STARTADDR} ${APP_STARTADDR} APP_STACK_STARTADDR
#    subHex ${APP_HEAP_STARTADDR} ${APP_STARTADDR} APP_HEAP_STARTADDR

elif [ "${CPUTYPE}" = "__M68K__" ]; then

    # Calculate the Start address of the OS. The OS has a Boot Address followed by a reserved space for microcode and hooks before the main OS code.
    addHex ${OS_BOOTLEN} ${OS_BASEADDR}           OS_STARTADDR

    # Calculate the Start address of the Application. An Application has a Boot Address, a reserved space for OS Hooks and then the application start.
    addHex ${APP_BOOTLEN} ${APP_BASEADDR}         APP_STARTADDR

    # Calculate the maximum Application length by subtracting the size of the BRAM - Application Start - Stack Space
    if [ "${APP_LEN}" = "" -a $(( 16#`echo ${APP_STARTADDR} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) -lt  $(( 16#`echo ${BRAM_SIZE} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) ]; then
        subHex ${BRAM_SIZE} ${APP_STARTADDR}      APP_LEN
        subHex ${APP_LEN} 0x20                    APP_LEN
    
    # If the APPLEN isnt set, give it a meaningful default.
    elif [ $(( 16#`echo ${APP_LEN} | tr 'a-z' 'A-Z'|sed 's/0X//g'` )) -eq 0 ]; then
        APP_LEN=0x10000;
    fi

    # Calculate the start of the Operating system code as the first section from the boot address is reserved.
    addHex ${OS_BOOTADDR} ${OS_BOOTLEN}           OS_STARTADDR

    # Calculate the length of the OS which is the start address of the App less the Boot address of the OS.
    subHex ${APP_BASEADDR} ${OS_BOOTADDR}         OS_LEN
    subHex ${OS_LEN} ${OS_BOOTLEN}                OS_LEN

    # Calculate the heap and stack vars.
    subHex ${BRAM_SIZE} 8                         OS_STACK_ENDADDR
    subHex ${OS_STACK_ENDADDR} ${OS_STACK_SIZE}   OS_STACK_STARTADDR
    subHex ${OS_STACK_STARTADDR} 4                OS_HEAP_ENDADDR
    subHex ${OS_HEAP_ENDADDR} ${OS_HEAP_SIZE}     OS_HEAP_STARTADDR

    # Stack start address for the APP. Normally this isnt used as the stack from zOS/ZPUTA is maintained, but available incase of
    # need for a local stack.
    addHex ${APP_STARTADDR} ${APP_LEN}            APP_ENDADDR
    #subHex ${APP_ENDADDR} ${OS_STACK_SIZE}        APP_ENDADDR
    subHex ${APP_ENDADDR} 8                       APP_STACK_ENDADDR
    subHex ${APP_STACK_ENDADDR} ${APP_STACK_SIZE} APP_STACK_STARTADDR

    # Calculate the heap start and end.
    subHex ${APP_STACK_STARTADDR} 4               APP_HEAP_ENDADDR
    subHex ${APP_HEAP_ENDADDR} ${APP_HEAP_SIZE}   APP_HEAP_STARTADDR

else
    Fatal "Internal error, unrecognised CPUTYPE at calcblock"
fi

Debug "OS_BASEADDR=${OS_BASEADDR}, OS_BOOTLEN=${OS_BOOTLEN}, OS_STARTADDR=${OS_STARTADDR}, OS_LEN=${OS_LEN}"
Debug "OS_RAM_STARTADDR=${OS_RAM_STARTADDR}, OS_RAM_ENDADDR=${OS_RAM_ENDADDR}, OS_RAM_LEN=${OS_RAM_LEN}, OS_RAM_MASK=${OS_RAM_MASK}, OS_RAM_OSMEM=${OS_RAM_OSMEM}"
Debug "OS_STACK_STARTADDR=${OS_STACK_STARTADDR}, OS_STACK_ENDADDR=${OS_STACK_ENDADDR}, OS_STACK_SIZE=${OS_STACK_SIZE}, OS_HEAP_STARTADDR=${OS_HEAP_STARTADDR}, OS_HEAP_ENDADDR=${OS_HEAP_ENDADDR} OS_HEAP_SIZE=${OS_HEAP_SIZE}"
Debug "APP_BASEADDR=${APP_BASEADDR}, APP_BOOTLEN=${APP_BOOTLEN}, APP_STARTADDR=${APP_STARTADDR}, APP_ENDADDR=${APP_ENDADDR}, APP_RAM_MASK=${APP_RAM_MASK}, APP_LEN=${APP_LEN}"
Debug "APP_STACK_STARTADDR=${APP_STACK_STARTADDR}, APP_STACK_ENDADDR=${APP_STACK_ENDADDR}, APP_STACK_SIZE=${APP_STACK_SIZE}, APP_HEAP_STARTADDR=${APP_HEAP_STARTADDR}, APP_HEAP_ENDADDR=${APP_HEAP_ENDADDR}, APP_HEAP_SIZE=${APP_HEAP_SIZE}"

# Build according to desired OS and target processor.
if [ "${OS}" = "ZPUTA" ]; then

    # Build the ZPUTA link script based on given and calculated values.
    Log "ZPUTA - ${OSBUILDSTR}"
    if [ "${CPUTYPE}" = "__ZPU__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zputa_zpu.tmpl
    elif [ "${CPUTYPE}" = "__ZPU__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zputa_k64f.tmpl
    elif [ "${CPUTYPE}" = "__M68K__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zputa_m68k.tmpl
    else
        Fatal "Internal error, unrecognised CPUTYPE at zputa cfg"
    fi
    cat ${TMPLFILE} | sed -e "s/BOOTADDR/${OS_BOOTADDR}/g"               \
                          -e "s/BOOTLEN/${OS_BOOTLEN}/g"                 \
                          -e "s/OS_START/${OS_STARTADDR}/g"              \
                          -e "s/OS_LEN/${OS_LEN}/g"                      \
                          -e "s/OS_RAM_STARTADDR/${OS_RAM_STARTADDR}/g"  \
                          -e "s/OS_RAM_LEN/${OS_RAM_LEN}/g"              \
                          -e "s/HEAP_SIZE/${OS_HEAP_SIZE}/g"             \
                          -e "s/HEAP_STARTADDR/${OS_HEAP_STARTADDR}/g"   \
                          -e "s/HEAP_ENDADDR/${OS_HEAP_ENDADDR}/g"   \
                          -e "s/STACK_SIZE/${OS_STACK_SIZE}/g"           \
                          -e "s/STACK_STARTADDR/${OS_STACK_STARTADDR}/g" \
                          -e "s/STACK_ENDADDR/${OS_STACK_ENDADDR}/g"     \
                          -e "s/STACK_ADDR/${OS_STACK_STARTADDR}/g" > ${BUILDPATH}/startup/${OSBUILDSTR}.ld

    cd ${BUILDPATH}/zputa
    make ${CPUTYPE}=1 ${OSTYPE}=1 ${BUILDFLAGS} clean
    if [ $? != 0 ]; then
        Fatal "Aborting, failed to clean ZPUTA build environment!"
    fi

    if [ "${CPUTYPE}" = "__ZPU__" ]; then
        Log "make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}"
        make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}
    elif [ "${CPUTYPE}" = "__K64F__" ]; then
        Log "make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}"
        make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}
    elif [ "${CPUTYPE}" = "__M68K__" ]; then
        Log "make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}"
        make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} ${BUILDFLAGS}
    else
        Fatal "Internal error, unrecognised CPUTYPE at ZPUTA Make"
    fi
    if [ $? != 0 ]; then
        Fatal "Aborting, failed to build ZPUTA!"
    fi
    cp ${BUILDPATH}/zputa/${OSBUILDSTR}.bin ${BUILDPATH}/build/SD/${OS_SD_TARGET}

elif [ "${OS}" = "ZOS" ]; then

    # Build the zOS link script based on given and calculated values.
    Log "zOS - ${OSBUILDSTR}"
    if [ "${CPUTYPE}" = "__ZPU__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zos_zpu.tmpl
    elif [ "${CPUTYPE}" = "__K64F__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zos_k64f.tmpl
    elif [ "${CPUTYPE}" = "__M68K__" ]; then
        TMPLFILE=${BUILDPATH}/startup/zos_m68k.tmpl
    else
        Fatal "Internal error, unrecognised CPUTYPE at zos cfg"
    fi
    cat ${TMPLFILE} | sed -e "s/BOOTADDR/${OS_BOOTADDR}/g"               \
                          -e "s/BOOTLEN/${OS_BOOTLEN}/g"                 \
                          -e "s/OS_START/${OS_STARTADDR}/g"              \
                          -e "s/OS_LEN/${OS_LEN}/g"                      \
                          -e "s/OS_RAM_STARTADDR/${OS_RAM_STARTADDR}/g"  \
                          -e "s/OS_RAM_LEN/${OS_RAM_LEN}/g"              \
                          -e "s/HEAP_SIZE/${OS_HEAP_SIZE}/g"             \
                          -e "s/HEAP_STARTADDR/${OS_HEAP_STARTADDR}/g"   \
                          -e "s/HEAP_ENDADDR/${OS_HEAP_ENDADDR}/g"   \
                          -e "s/STACK_SIZE/${OS_STACK_SIZE}/g"           \
                          -e "s/STACK_STARTADDR/${OS_STACK_STARTADDR}/g" \
                          -e "s/STACK_ENDADDR/${OS_STACK_ENDADDR}/g"     \
                          -e "s/STACK_ADDR/${OS_STACK_STARTADDR}/g" > ${BUILDPATH}/startup/${OSBUILDSTR}.ld

    cd ${BUILDPATH}/zOS
    make ${CPUTYPE}=1 ${OSTYPE}=1 clean
    if [ $? != 0 ]; then
        Fatal "Aborting, failed to clean zOS build environment!"
    fi
    if [ "${CPUTYPE}" = "__ZPU__" ]; then
        Log "make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}"
        make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}
    elif [ "${CPUTYPE}" = "__K64F__" ]; then
        Log "make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}"
        Log "make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}"
        make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}
    elif [ "${CPUTYPE}" = "__M68K__" ]; then
        Log "make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}"
        make ${OSBUILDSTR} ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} HEAPADDR=${OS_HEAP_STARTADDR} HEAPSIZE=${OS_HEAP_SIZE} STACKADDR=${OS_STACK_STARTADDR} STACKENDADDR=${OS_STACK_ENDADDR} STACKSIZE=${OS_STACK_SIZE} ${BUILDFLAGS}
    else
        Fatal "Internal error, unrecognised CPUTYPE at ZOS Make"
    fi
    if [ $? != 0 ]; then
        Fatal "Aborting, failed to build zOS!"
    fi

    # Also create a copy of the OS for use by the tranZPUter SW-700 project.
    mkdir -p ${BUILDPATH}/build/SD/ZOS
    if [ "${CPUTYPE}" = "__ZPU__" ]; then
        cp ${BUILDPATH}/zOS/${OSBUILDSTR}.bin ${BUILDPATH}/build/SD/${OS_SD_TARGET}
        cp ${BUILDPATH}/zOS/${OSBUILDSTR}.bin ${BUILDPATH}/build/SD/ZOS/ZOS.ROM
    elif [ "${CPUTYPE}" = "__K64F__" ]; then
        cp ${BUILDPATH}/zOS/main.bin ${BUILDPATH}/build/SD/${OS_SD_TARGET}
        cp ${BUILDPATH}/zOS/main.bin ${BUILDPATH}/build/SD/ZOS/ZOS.K64F.ROM
    elif [ "${CPUTYPE}" = "__M68K__" ]; then
        cp ${BUILDPATH}/zOS/main.bin ${BUILDPATH}/build/SD/${OS_SD_TARGET}
        cp ${BUILDPATH}/zOS/main.bin ${BUILDPATH}/build/SD/ZOS/ZOS.M68K.ROM
    else
        Fatal "Internal error, unrecognised CPUTYPE at ZOS Copy"
    fi
fi

# Build the apps and install into the build tree.
# For ZPU the location and size depends on FPGA resources so create a dynamic linker template. For K64F it is a static design but the heap size is variable so we also
# need to generate a dynamic linker template. This is now performed within the Makefile passing necessary variables to Make.
#
if [ "${CPUTYPE}" = "__ZPU__" ]; then
    TMPLFILE=${BUILDPATH}/startup/app_${OSNAME}_zpu.tmpl
    LDFILE=${BUILDPATH}/startup/app_zpu_${OS_BOOTADDR}_${APP_BASEADDR}.ld
elif [ "${CPUTYPE}" = "__K64F__" ]; then
    TMPLFILE=${BUILDPATH}/startup/app_${OSNAME}_k64f.tmpl
    LDFILE=${BUILDPATH}/startup/app_k64f_${OS_BOOTADDR}_${APP_BASEADDR}.ld
elif [ "${CPUTYPE}" = "__M68K__" ]; then
    TMPLFILE=${BUILDPATH}/startup/app_${OSNAME}_m68k.tmpl
    LDFILE=${BUILDPATH}/startup/app_m68k_${OS_BOOTADDR}_${APP_BASEADDR}.ld
else
    Fatal "Internal error, unrecognised CPUTYPE at LD tmpl setup"
fi

cd ${BUILDPATH}/apps
Log "make ${CPUTYPE}=1 ${OSTYPE}=1 ${BUILDFLAGS} clean"
make ${CPUTYPE}=1 ${OSTYPE}=1 ${BUILDFLAGS} clean
if [ $? != 0 ]; then
    Fatal "Aborting, failed to clean Apps build environment!"
fi
Log "make ${CPUTYPE} ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} TMPLFILE=${TMPLFILE} BASEADDR=${APP_BASEADDR} BASELEN=${APP_BOOTLEN} HEAPADDR=${APP_HEAP_STARTADDR} HEAPSIZE=${APP_HEAP_SIZE} STACKADDR=${APP_STACK_STARTADDR} STACKENDADDR=${APP_STACK_ENDADDR} STACKSIZE=${APP_STACK_SIZE} APPSTART=${APP_STARTADDR} APPSIZE=${APP_LEN} ${BUILDFLAGS}"
make ${CPUTYPE}=1 ${OSTYPE}=1 OS_BASEADDR=${OS_BOOTADDR} OS_APPADDR=${APP_BASEADDR} CPU=${CPU} TMPLFILE=${TMPLFILE} BASEADDR=${APP_BASEADDR} BASELEN=${APP_BOOTLEN} HEAPADDR=${APP_HEAP_STARTADDR} HEAPSIZE=${APP_HEAP_SIZE} STACKADDR=${APP_STACK_STARTADDR} STACKENDADDR=${APP_STACK_ENDADDR} STACKSIZE=${APP_STACK_SIZE} APPSTART=${APP_STARTADDR} APPSIZE=${APP_LEN} ${BUILDFLAGS}
if [ $? != 0 ]; then
    Fatal "Aborting, failed to build Apps!"
fi
mkdir -p bin
rm -f bin/*
Log "make ${CPUTYPE}=1 ${OSTYPE}=1 ${BUILDFLAGS} install"
make ${CPUTYPE}=1 ${OSTYPE}=1 ${BUILDFLAGS} install
if [ $? != 0 ]; then
    Fatal "Aborting, failed to install generated binaries!"
fi
cp -r ${BUILDPATH}/apps/bin ${BUILDPATH}/build/SD/bin
