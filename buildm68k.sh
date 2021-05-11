#!/bin/bash

# Not used: Target machine, used to select the right software for the SD card.
# TARGET=MZ-700
# TARGET=MZ-80A

M68K_SHARPMZ_BUILD=1
M68K_SHARPMZ_APPADDR=0x100000
M68K_SHARPMZ_APPSIZE=0x70000
M68K_SHARPMZ_HEAPSIZE=0x8000
M68K_SHARPMZ_STACKSIZE=0x3D80


# NB: When setting this variable, see lower section creating the SD card image which uses a hard coded value and will need updating.
ROOT_DIR=/dvlp/Projects/dev/github/
# NB: This clean out is intentionally hard coded as -fr is dangerous, if a variable failed to be set if could see your source base wiped out.
rm -fr /dvlp/Projects/dev/github/zSoft/SD/M68K/*

(
# Ensure the zOS target directories exist.
cd ${ROOT_DIR}/zSoft
mkdir -p SD/SharpMZ
mkdir -p SD/Dev
mkdir -p SD/M68K/ZOS

if [ "${M68K_SHARPMZ_BUILD}x" != "x" -a ${M68K_SHARPMZ_BUILD} = 1 ]; then
    echo "Building M68K for Sharp MZ"
    ./build.sh -C M68K -O zos -M 0x1FD80 -B 0x0000 -S ${M68K_SHARPMZ_STACKSIZE} -N ${M68K_SHARPMZ_HEAPSIZE} -A ${M68K_SHARPMZ_APPADDR} -a ${M68K_SHARPMZ_APPSIZE} -n 0x0000 -s 0x0000 -d -Z
    if [ $? != 0 ]; then
    	echo "Error building Sharp MZ Distribution..."
    	exit 1
    fi
    # Copy the BRAM ROM templates to the build area for the FPGA.
  #  cp rtl/TZSW_* ../tranZPUter/FPGA/SW700/v1.3/devices/sysbus/BRAM/
    # Copy the newly built files into the staging area ready for copying to an SD card.
  #  cp -r build/SD/* SD/SharpMZ/
    # The K64F needs a copy of the zOS ROM for the ZPU so that it can load it into the FPGA.
 #   cp build/SD/ZOS/* SD/K64F/ZOS/
fi

)
if [ $? != 0 ]; then
	exit 1
fi
