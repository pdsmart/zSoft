### Using IOCP

The I/O Control Program (IOCP) is basically a bootloader, it can operate standalone or as the first stage in booting an application. Depending on the configuration it can provide a low level monitor via the serail
connection for basic memory inspection and bootstrapping.

At the time of writing the following functionality and memory maps have been defined in the build.sh script and within the parameterisation of the IOCP /zOS /ZPUTA /RTL but any other is possible by adjusting the parameters.

 - Tiny - IOCP is the smallest size possible to boot from SD Card. It is useful for a SoC configuration where there is limited BRAM and the applications loaded from the SD card would potentially run in external RAM.
 - Minimum - As per tiny but adds: print IOCP version, interrupt handler, boot message and SD error messages. 
 - Medium - As per small but adds: command line processor adding the commands below, a timer for auto boot so that it can be disabled before expiry by pressing a key

    | Command | Description                                |
    | ------- | ------------------------------------------ |
    | 1       | Boot Application in Application area BRAM  |
    | 4       | Dump out BRAM (boot) memory                |
    | 5       | Dump out Stack memory                      |
    | 6       | Dump out application RAM                   |
    | C       | Clear Application area of BRAM             |
    | c       | Clear Application RAM                      |
    | d       | List the SD Cards directory                |
    | R       | Reset the system and boot as per power on  |
    | h       | Print out help on enabled commands         |
    | i       | Prints version information                 |

 - Full - As medium but adds additional commands below.

    | Command | Description                                |
    | ------- | ------------------------------------------ |
    | 2       | Upload to BRAM application area, in binary format, from serial port |
    | 3       | Upload to RAM, in binary format, from serial port |
    | i       | Print detailed SoC configuration           |


## Technical Detail

This section aims to provide some of the inner detail of the IOCP bootloader to date. A lot of this information can be seen in the zOS/ZPUTA documents but the aim is to provide finer detail
on IOCP here. 


### Memory Organisation


##### IOCP Memory Map


The IOCP has been defined with 4 possible memory maps depending on the memory model chosen at compile time. The currently defined memory maps for IOCP are as follows:-


![IOCP Memory Map](../images/IOCPMemoryMap.png)


IOCP can be configured to be the first stage loader for zOS/ZPUTA or any other suitably compiled program or it can be enhanced to be the actual end application. zOS/ZPUTA can also be 
compiled 'standalone' such that IOCP isnt needed, booting themselves directly as the power on firmware. 

zOS and ZPUTA can be built to use an SD card where they can have applets (portions of their functionality created as dedicated executables on the SD card) or in the case of ZPUTA, standalone 
with all functionality inbuilt. The latter is used when there is limited memory or a set of loadable programs is desired.

<br>


## Software Build

This section shows how to make a basic IOCP build.  The start point for building the IOCP (or zOS/ ZPUTA/ Applications) is the 'build.sh' script which contains all the logic required.
This program started off as knowledge dumps and enhanced as the software progressed. It would be nice, eventually, to redesign this script as it is pushing into BASH shell language what
really should be coded in Python or Java, but it works for now in building this software.


## Paths

For ease of reading, the following shortnames refer to the corresponding path in this chapter.

|  Short Name      | Path                                                                       | Description                                                                                       |
|------------------|----------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| \[\<ABS PATH>\]  | The path where this repository was extracted on your system.               |                                                                                                   |
| \<apps\>         | \[\<ABS PATH>\]/zsoft/apps                                                 | Application directory. All zOS/ZPUTA applications are located here along with their makefiles.                                                   |
| \<build\>        | \[\<ABS PATH>\]/zsoft/build                                                | A target output directory for the compiled software, ie. \<z-build\>/SD contains all files to be written to an SD card.                          |
| \<common\>       | \[\<ABS PATH>\]/zsoft/common                                               | Common C/C++ methods which are not assembled into a library.                                                                                     |
| \<libraries\>    | \[\<ABS PATH>\]/zsoft/libraries                                            | C/C++ libraries, usually part of a C/C++ installation but for embedded work, especially on the ZPU need to be created seperately.                |                                                                                                  |
| \<teensy3\>      | \[\<ABS PATH>\]/zsoft/teensy3                                              | The K64F based Teensy 3.5 software, some of which is used in the K64F version of zOS. Very rich libraries and can be easily added into a K64F programs. |                                                                                                  |
| \<include\>      | \[\<ABS PATH>\]/zsoft/include                                              | Common include header files.                                                                                                                     |                                                                                                  |
| \<startup\>      | \[\<ABS PATH>\]/zsoft/startup                                              | Embedded processor startup files, generally in Assembler. Templates and macros are used to create the correct targets memory model startup code. |                                                                                                  |
| \<iocp\>         | \[\<ABS PATH>\]/zsoft/iocp                                                 | The IO Control Program, my initial bootloader for bootstrapping an application on the ZPU.                                                       |                                                                                                  |
| \<zOS\>          | \[\<ABS PATH>\]/zsoft/zOS                                                  | The zOS source code, parameterised for the different target CPU's.                                                                               |                                                                                                  |
| \<zputa\>        | \[\<ABS PATH>\]/zsoft/zputa                                                | The ZPUTA source code, parameterised for the different target CPU's.                                                                             |                                                                                                  |
| \<rtl\>          | \[\<ABS PATH>\]/zsoft/rtl                                                  | Register Transfer Level files. These are generated memory definition and initialisation files required when building for a ZPU target project.   |                                                                                                  |
| \<docs\>         | \[\<ABS PATH>\]/zsoft/docs                                                 | Any relevant documentation for the software.                                                                                                     |                                                                                                  |
| \<tools\>        | \[\<ABS PATH>\]/zsoft/tools                                                | Tools to aid in the compilation and creation of target files.                                                                                    |     


## Tools

All development has been made under Linux, specifically Debian/Ubuntu. Besides the standard Linux buildchain, the following software is needed.

|                                                           |                                                                                                                     |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
[ZPU GCC ToolChain](https://github.com/zylin/zpugcc)        | The GCC toolchain for ZPU development. Install into */opt* or similar common area.                                  |
[Arduino](https://www.arduino.cc/en/main/software)          |  The Arduino development environment, not really needed unless adding features to the K64F version of zOS from the extensive Arduino library. Not really needed, more for reference. |
[Teensyduino](https://www.pjrc.com/teensy/td_download.html) | The Teensy3 Arduino extensions to work with the Teensy3.5 board at the Arduino level. Not really needed, more for reference. |

IOCP hasnt yet been ported to the K64F as there seems little need for it, but for completeness and reference, the Teensy3.5/K64F the ARM compatible toolchain is stored in the repo within the
build tree.


## Build Tree

The software is organised into the following tree/folders:

| Folder    | Src File | Description                                                  |
| -------   | -------- | ------------------------------------------------------------ |
| apps      |          | The zOS/ZPUTA applications applets tree. These are separate standalone disk based applets which run under zOS/ZPUTA. <br/>All applets for zOS/ZPUTA are stored within this folder. |
| build     |          | Build tree output suitable for direct copy to an SD card.<br/> The initial bootloader and/or application as selected are compiled directly into a VHDL file for preloading in BRAM in the devices/sysbus/BRAM folder. |
| common    |          | Common C modules such as Elm Chan's excellent Fat FileSystem. |
| docs      |          | All documents relevant to this project.                       |
| libraries |          | Code libraries, soon to be populated with umlibc and FreeRTOS |
| rtl       |          | Register Transfer Level templates and generated files. These files contain the preloaded VHDL definition of a BRAM based RAM/ROM used with the ZPU.|
| teensy3   |          | Teensy 3.5 files required for building a K64F processor target. |
| include   |          | C Include header files.                                      |
| iocp      |          | A small bootloader/monitor application for initialization of the ZPU. Depending upon configuration this program can either boot an application from SD card or via the Serial Line and also provide basic tools such as memory examination. |
| startup   |          | Assembler and Linker files for generating ZPU applications. These files are critical for defining how GCC creates and links binary images as well as providing the micro-code for ZPU instructions not implemented in hardware. |
| tools     |          | Some small tools for converting binary images into VHDL initialization data. |
| zputa     |          | The ZPU Test Application. This is an application for testing the ZPU and the SoC components. It can either be built as a single image for pre-loading into a BRAM via VHDL or as a standalone application loaded by the IOCP bootloader from an SD card. The services it provides can either be embedded or available on the SD card as applets depending on memory restrictions. |
| zOS       |          | The z-Operating System. This is a light OS derived from ZPUTA with the aim of providing a framework, both for application building and runtime control, within a ZPU or K64F project. The OS provides disk based services, common hardware access API and additional features to run and service any application built for this enviroment. Applications can be started autonomously or interactively via the serial based VT100 console. |
|           | build.sh | Unix shell script to build IOCP, zOS, ZPUTA and Apps for a given design.<br> ![build.sh](../images/build_sh.png) |


### Build.sh

IOCP is built using the 'build.sh' script. This script encapsulates the Makefile system with it's plethora of flags and options. The synopsis of the script is below:
```bash
NAME
    build.sh -  Shell script to build a ZPU/K64F program or OS.

SYNOPSIS
    build.sh [-CIOoMBAsTZdxh]

DESCRIPTION

OPTIONS
    -C <CPU>      = Small, Medium, Flex, Evo, EvoMin, K64F - defaults to Evo.
    -I <iocp ver> = 0 - Full, 1 - Medium, 2 - Minimum, 3 - Tiny (bootstrap only)
    -O <os>       = zputa, zos
    -o <os ver>   = 0 - Standalone, 1 - As app with IOCP Bootloader,
                    2 - As app with tiny IOCP Bootloader, 3 - As app in RAM 
    -M <size>     = Max size of the boot ROM/BRAM (needed for setting Stack).
    -B <addr>     = Base address of <os>, default -o == 0 : 0x00000 else 0x01000 
    -A <addr>     = App address of <os>, default 0x0C000
    -N <size>     = Required size of heap
    -n <size>     = Required size of application heap
    -S <size>     = Required size of stack
    -s <size>     = Required size of application stack
    -a <size>     = Maximum size of an app, defaults to (BRAM SIZE - App Start Address - Stack Size) 
                    if the App Start is located within BRAM otherwise defaults to 0x10000.
    -T            = TranZPUter specific build, adds initialisation and setup code.
    -Z            = Sharp MZ series ZPU build, zOS runs as an OS host on Sharp MZ hardware.
    -d            = Debug mode.
    -x            = Shell trace mode.
    -h            = This help screen.

EXAMPLES
    build.sh -O zputa -B 0x00000 -A 0x50000

EXIT STATUS
     0    The command ran successfully

     >0    An error ocurred.
```

Sensible defaults are configured into the script, the overriding flags are described below.

| Flag&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;             | Description                                                                                                                         |
| ---------------------------------------------------------------------------------- | -----------                                                                                                                         |
| -C \<CPU\>       | The target CPU of the finished image. This flag is necessary As each ZPU has a different configuration and therefore the underlying Make needs to be directed into its choice of startup code and configuration. With the inclusion of the ARM based K64F this flag makes further sense.<br><br>The choices are:<br>*Small* - An absolute minimum ZPU hardware design with most instructions implemented in microcode.<br>*Medium* - An interim ZPU design using more hardware and better performance, majority of instructions are embedded in hardware.<br>*Flex* - A ZPU small based design but with many enhanced features, including the majority of instructions being embedded in hardware.<br>*Evo* - The latest evolution of the ZPU, all instructions are embedded in hardware, framework to add extended instructions and 2 stage cache to aid performance.<br>*K64F* - An ARM Cortex-M4 based processor from Freescale.<br>*Evo is the detault.* |
| -I \<iocp ver\>  | Set the memory model for the build. The memory models are described above.<br><br>The choices are:<br>*0*- Full<br>*1* - Medium<br>*2* - Minimum<br>*3* - Tiny (bootstrap only).<br>*Default is set to 3.* |
| -O \<os\>        | Set the target operating system.<br><br>The choices are:<br>*zputa* - The ZPU Test Application which also runs on the K64F.<br>*zos* - The zOS Operating System.<br>There is no default, if an OS is being built then this flag must be given. |
| -o \<os ver\>    | Set the operating system target model.<br><br>The choices are:<br>*0* - Standalone, the operating system is the startup firmware.<br>*1* - As app with IOCP Bootloader, the operating system is booted by IOCP from an SD card, IOCP remains memory resident.<br>*2* - As app with tiny IOCP Bootloader, the operating system is booted from SD card by the smallest IOCP possible, IOCP remains memory resident.<br>*3* - As app in RAM, the operating system is targetted to be loaded in external (to the FPGA) RAM, either static or dynamic, IOCP remains memory resident in BRAM.<br>*The default is 2.*<br>**This flag has no meaning for the K64F target.** |
| -M \<size\>      | Specify the maximum size of the boot ROM/BRAM (needed for setting Stack).<br>**This flag has no meaning on the K64F target.** |
| -B \<addr\>      | Base address of \<os\>.<br>*default: if -o == 0 : 0x00000 else 0x01000*<br>**This flag has no meaning on the K64F target.**                                                                        |
| -A \<addr\>      | App starting/load address for the given \<os\>.<br>*default 0x0C000 for ZPU, 0x1FFF0000 for K64F*                                                                                                |
| -N \<size\>      | Required size of \<os\> heap. The OS and application heaps are currently distinct. Should RTOS be integrated into zOS then this will change. |
| -n \<size\>      | Required size of application heap. The application has a seperate heap allocated and is managed by the application. |
| -S \<size\>      | Required size of stack. Normally the \<os\> manages the stack which is also used by an application. If the application is considered dangerous then it should be allocated a local stack. |
| -s \<size\>      | Required size of application stack. Normally this should not be required as the application shares the \<os\> stack. If the application is dangerous then set this value to allocate a local stack. |
| -a \<size\>      | Maximum size of an application.<br>*defaults to (BRAM SIZE - App Start Address - Stack Size) for the ZPU if the App Start is located within BRAM otherwise defaults to 0x10000, or 0x20030000 - Stack Size - Heap Size - 0x4000 for the K64F. If the App Start is located within BRAM otherwise defaults to 0x10000.*                                                              |
| -T               | TranZPUter specific build, adds initialisation and setup code. Build as an embedded OS for the tranZPUter board. |
| -Z               | Sharp MZ series ZPU build, zOS runs as an OS host on Sharp MZ hardware. Build as a host OS for the Sharp MZ series computer running a ZPU/NIOSII as the host processor. |
| -d               | Debug mode. Enable more verbose output on state and decisions made by the script. |
| -x               | Shell trace mode. A very low level, line by line trace of shell execution. Only used during debugging. |
| -h               | This help screen.                                                                                                                   |

<br>
To build a Tiny memory model IOCP bootloader for the Flex CPU with 32K BRAM and an application loading at 0x1000, you would issue the command:

```bash
<ABS PATH>/build.sh -C Flex -I 3 -M 0x8000 -B 0x1000
```

NB: App is a term only used when an \<os\> is deployed, ie. zOS + App, if you build a program to start at 0x1000 and place it onto the SD card as BOOTTINY.ROM then this is considered the bootstrapped program loading at Base Addr, not an application.

The bootstrapped filename is dependent upon the memory model chosen, this file must be present in the root directory of a FAT32 formatted SD card to be recognised and bootstrapped:
* BOOT.ROM - Memory models: Full, Medium, Minimum.
* TINYBOOT.ROM - Memory models: Tiny.



<br>
## Credits

Where I have used or based any component on a 3rd parties design I have included the original authors copyright notice within the headers or given due credit. All 3rd party software, to my knowledge and research, is open source and freely useable, if there is found to be any component with licensing restrictions, it will be removed from this repository and a suitable link/config provided.


<br>
## Licenses

The original ZPU uses the Free BSD license and such the Evo is also released under FreeBSD. SoC components and other developments written by me are currently licensed using the GPL. 3rd party components maintain their original copyright notices.

### The FreeBSD license
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted as representing official policies, either expressed or implied, of the this project.

### The Gnu Public License v3
 The source and binary files in this project marked as GPL v3 are free software: you can redistribute it and-or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 The source files are distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/.


