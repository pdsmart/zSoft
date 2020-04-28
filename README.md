<br>
zSoft is the collective term for all the applications and operating systems developed for the ZPU processor. Originally the Z stood for ZPU but with the inclusion of additional CPU
architectures, it now means Zeta, the sixth Greek alphabet letter.

A few months ago the tranZPUterSW project was born as an offshoot from the tranZPUter in order to more rapidly forward the tranZPUter design via software development. This came in the shape
of the Teensy 3.5 development board by [PJRC.com](https://www.pjrc.com/store/teensy35.html) or more importantly the Freescale K64F processor, an ARM Cortex-M4 CPU. Adding this CPU required
quite a few changes to the source code to allow for two (or potentially more) different processing architectures but it makes the z-soft software more useable by 3rd parties should they
wish to use it in their own projects, especially given the plethora of ARM development boards available.

The zSoft collection currently includes:

#### zOS
zOS started life as an application to aid in testing the ZPU design as it evolved, originally given the name ZPUTA (ZPU Testing Application). Some of the FPGA development boards I had been
working with had limited memory (BRAM) and the application had to be made configurable to size it according to available memory. Once an SD card had been added into the hardware SoC, the
application advanced to become more of an Operating System on par with CPM/MSDOS storing applications on disk rather than being built into the application. Thus was born the idea for zOS, 
take ZPUTA, strip it down of unnecessary testing code and create an OS to be used at the core of the tranZPUter, tranZPUterSW, ZPU and Sharp MZ Emulator projects.

I am currently looking at blending in an RTOS to provide missing features such as scheduling but that is for another day, especially given the restriction which the ZPU GCC
compiler places on the project, ie C++98/C99 specifications!

#### ZPUTA
ZPUTA was the original test application which evolved into an OS and was split to form zOS. It will still carry on with its original purpose and provide functionality not available in 
zOS specifically useable for testing. It also has the capability of being built as a single program with most of the application applet functionality within it and so is useful in 
environments where there is no SD card. zOS will over time become more a refined OS for my own projects and ZPUTA will continue on for CPU/hardware testing. 

#### IOCP
Prior to ZPUTA, I created a bootstrap program called the IOCP (I/O Control Program) which operated at the lowest hardware levels and was able to boot more sophisticated programs, ie. ZPUTA.
The IOCP is size sensitive and was designed to bootstrap from any memory device and by including an SD card and the [Petit FatFS](http://elm-chan.org/fsw/ff/00index_p.html) it is able to
bootstrap off an SD card. IOCP was originally used to bootstrap ZPUTA, aiding development speed by just copying a ZPUTA kernel onto an SD card when a change was made but eventually I included
the low level code into ZPUTA and it was able to boot as the primary software without needing IOCP. IOCP still remains within the project to boot any application not needing ZPUTA or zOS
level of sophistication.

The following sections, selected on the navigation bar to the left, describe zOS, ZPUTA, IOCP and all the Application applets at their current states of evolution.
