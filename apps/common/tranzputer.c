/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            tranzputer.c
// Created:         May 2020
// Author(s):       Philip Smart
// Description:     The TranZPUter library.
//                  This file contains methods which allow applications to access and control the
//                  traZPUter board and the underlying Sharp MZ80A host.
//                  I had considered writing this module in C++ but decided upon C as speed is more
//                  important and C++ always adds a little additional overhead. Some of the methods need
//                  to be written as embedded assembler but this is for a later time when using the
//                  tranZPUter on faster motherboards.
// Credits:         
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2020 - Initial write of the TranZPUter software.
//
// Notes:           See Makefile to enable/disable conditional components
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This source file is free software: you can redistribute it and#or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__K64F__)
  #include <stdio.h>
  #include <ctype.h>
  #include <stdint.h>
  #include <string.h>
  #include <unistd.h>
  #include <stdarg.h>
  #include <Arduino.h>
  #include "k64f_soc.h"
  #include <../../libraries/include/stdmisc.h>
#elif defined(__ZPU__)
  #include <stdint.h>
  #include <stdio.h>
  #include "zpu_soc.h"
  #include <stdlib.h>
  #include <ctype.h>
  #include <stdmisc.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif

#include "ff.h"            /* Declarations of FatFs API */
#include "utils.h"
#include <tranzputer.h>


// Global scope variables, used by pin assignment and configuration macros. 
volatile uint32_t *ioPin[MAX_TRANZPUTER_PINS];
uint8_t           pinMap[MAX_TRANZPUTER_PINS];
uint32_t volatile *ms;
t_z80Control      z80Control;


// Dummy function to override a weak definition in the Teensy library. Currently the yield functionality is not
// needed within apps running on the K64F it is only applicable in the main OS.
//
void yield(void)
{
    return;
}

// Method to setup the pins and the pin map to power up default.
// The OS millisecond counter address is passed into this library to gain access to time without the penalty of procedure calls.
// Time is used for timeouts and seriously affects pulse width of signals when procedure calls are made.
//
void setupPins(volatile uint32_t *millisecondTick)
{
    // Locals.
    //
    static uint8_t firstCall = 0;

    // This method can be called more than once as a convenient way to return all pins to default. On first call though
    // the teensy library needs initialising, hence the static check.
    //
    if(firstCall == 0)
    {
        firstCall = 1;
        _init_Teensyduino_internal_();

        ms = millisecondTick;
    }

    // Setup the map of a loop useable array with its non-linear Pin Number.
    //
    pinMap[Z80_A0]      = Z80_A0_PIN;
    pinMap[Z80_A1]      = Z80_A1_PIN;
    pinMap[Z80_A2]      = Z80_A2_PIN;
    pinMap[Z80_A3]      = Z80_A3_PIN;
    pinMap[Z80_A4]      = Z80_A4_PIN;
    pinMap[Z80_A5]      = Z80_A5_PIN;
    pinMap[Z80_A6]      = Z80_A6_PIN;
    pinMap[Z80_A7]      = Z80_A7_PIN;
    pinMap[Z80_A8]      = Z80_A8_PIN;
    pinMap[Z80_A9]      = Z80_A9_PIN;
    pinMap[Z80_A10]     = Z80_A10_PIN;
    pinMap[Z80_A11]     = Z80_A11_PIN;
    pinMap[Z80_A12]     = Z80_A12_PIN;
    pinMap[Z80_A13]     = Z80_A13_PIN;
    pinMap[Z80_A14]     = Z80_A14_PIN;
    pinMap[Z80_A15]     = Z80_A15_PIN;
    pinMap[Z80_A16]     = Z80_A16_PIN;
    pinMap[Z80_A17]     = Z80_A17_PIN;
    pinMap[Z80_A18]     = Z80_A18_PIN;

    pinMap[Z80_D0]      = Z80_D0_PIN;
    pinMap[Z80_D1]      = Z80_D1_PIN;
    pinMap[Z80_D2]      = Z80_D2_PIN;
    pinMap[Z80_D3]      = Z80_D3_PIN;
    pinMap[Z80_D4]      = Z80_D4_PIN;
    pinMap[Z80_D5]      = Z80_D5_PIN;
    pinMap[Z80_D6]      = Z80_D6_PIN;
    pinMap[Z80_D7]      = Z80_D7_PIN;

    pinMap[Z80_MEM0]    = Z80_MEM0_PIN;
    pinMap[Z80_MEM1]    = Z80_MEM1_PIN;
    pinMap[Z80_MEM2]    = Z80_MEM2_PIN;
    pinMap[Z80_MEM3]    = Z80_MEM3_PIN;
    pinMap[Z80_MEM4]    = Z80_MEM4_PIN;

    pinMap[Z80_IORQ]    = Z80_IORQ_PIN;
    pinMap[Z80_MREQ]    = Z80_MREQ_PIN;
    pinMap[Z80_RD]      = Z80_RD_PIN;
    pinMap[Z80_WR]      = Z80_WR_PIN;
    pinMap[Z80_WAIT]    = Z80_WAIT_PIN;
    pinMap[Z80_BUSACK]  = Z80_BUSACK_PIN;

    pinMap[Z80_NMI]     = Z80_NMI_PIN;
    pinMap[Z80_INT]     = Z80_INT_PIN;

    pinMap[CTL_BUSACK]  = CTL_BUSACK_PIN;
    pinMap[CTL_BUSRQ]   = CTL_BUSRQ_PIN;
    pinMap[CTL_RFSH]    = CTL_RFSH_PIN;
    pinMap[CTL_HALT]    = CTL_HALT_PIN;
    pinMap[CTL_M1]      = CTL_M1_PIN;
    pinMap[CTL_CLK]     = CTL_CLK_PIN;
    pinMap[CTL_CLKSLCT] = CTL_CLKSLCT_PIN;

    // Now build the config array for all the ports. This aids in more rapid function switching as opposed to using
    // the pinMode and digitalRead/Write functions provided in the Teensy lib.
    //
    for(uint8_t idx=0; idx < MAX_TRANZPUTER_PINS; idx++)
    {
        ioPin[pinMap[idx]] = portConfigRegister(pinMap[idx]);

        // Set to input, will change as functionality dictates.
        pinInput(idx);
    }

    // Initialise control structure.
    z80Control.refreshAddr  = 0x00;
    z80Control.runCtrlLatch = readCtrlLatch(); 
    z80Control.ctrlMode     = Z80_RUN;
    z80Control.busDir       = TRISTATE;

    return;
}


// Method to request the Z80 Bus. This halts the Z80 and sets all main signals to tri-state.
// Return: 0 - Z80 Bus obtained.
//         1 - Failed to obtain the Z80 bus.
uint8_t reqZ80Bus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t  result = 0;
    uint32_t startTime = *ms;

    // Set the control pins in order to request the bus.
    //
    pinOutputSet(CTL_BUSRQ, HIGH);
    pinInput(Z80_BUSACK);

    // Set BUSRQ low which sets the Z80 BUSRQ low.
    pinLow(CTL_BUSRQ);

    // Wait for the Z80 to acknowledge the request.
    while((*ms - startTime) < timeout && pinGet(Z80_BUSACK));

    // If we timed out, reset the pins and return error.
    //
    if((*ms - startTime) >= timeout)
    {
        pinInput(CTL_BUSRQ);
        result = 1;
    }
    
    // Store the control latch state before we start modifying anything enabling a return to the pre bus request state.
    z80Control.runCtrlLatch = readCtrlLatch(); 

    return(result);
}

// Simple method to release the Z80 Bus.
//
void relinquishZ80Bus(void)
{
    pinInput(CTL_BUSACK);
    pinInput(CTL_BUSRQ);
    return;
}

// Method to request access to the main motherboard circuitry. This involves requesting the Z80 bus and then
// setting the Z80_RD and Z80_WR signals to low, this maps to a ENABLE_BUS signal which ensures that one half of
// the Mainboard BUSACK request signal is disabled (BUSACK to the motherboard has the opposite meaning, when active
// the mainboard is tri-stated and signals do not pass from th e Z80 and local memory to the mainboard). The other
// half of the mainboard request signal is disabled by setting CTL_BUSACK high.
//
uint8_t reqMainboardBus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t            result = 0;

    // Set the CTL BUSACK signal high so we dont assert the mainboard BUSACK signal.
    pinOutputSet(CTL_BUSACK, HIGH);

    // Requst the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
        // Now request the mainboard by setting BUSACK high and Z80_RD/Z80_WR low.
        pinOutput(Z80_RD);
        pinOutput(Z80_WR);

        // A special mode in the FlashRAM decoder detects both RD and WR being low at the same time, this is not feasible under normal Z80 operating conditions, but what it signals
        // here is to raise ENABLE_BUS which ensures that BUSACK on the mainboard is deasserted. This is for the case where the Z80 may be running in tranZPUter memory with the
        // mainboard disabled.
        pinLow(Z80_RD);
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 1; pulseWidth++);

        // Immediately return the RD/WR to HIGH to complete the ENABLE_BUS latch action.
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);

        // Store the mode.
        z80Control.ctrlMode = MAINBOARD_ACCESS;

        // For mainboard access, MEM4:0 should be 0 so no activity is made to the tranZPUter circuitry except the control latch.
        z80Control.curCtrlLatch = 0b00000000;
    } else
    {
        printf("Failed to request Mainboard Bus\n");
    }

    return(result);
}

// Method to request the local tranZPUter bus. This involves making a Z80 bus request and when relinquished, setting
// the CTL_BUSACK signal to low which disables (tri-states) the mainboard circuitry.
//
uint8_t reqTranZPUterBus(uint32_t timeout)
{
    // Locals.
    //
    uint8_t  result = 0;

    // Set the CTL BUSACK signal high so we dont assert the mainboard BUSACK signal.
    pinOutputSet(CTL_BUSACK, HIGH);

    // Requst the Z80 Bus to tri-state the Z80.
    if((result=reqZ80Bus(timeout)) == 0)
    {
        // Now disable the mainboard by setting BUSACK low.
        pinLow(CTL_BUSACK);
       
        // Store the mode.
        z80Control.ctrlMode = TRANZPUTER_ACCESS;
      
        // For tranZPUter access, MEM4:0 should be 1 so no activity is made to the mainboard circuitry.
        z80Control.curCtrlLatch = 0b00011111;
    }

    return(result);
}

// Method to set all the pins to be able to perform a transaction on the Z80 bus.
//
void setupSignalsForZ80Access(enum BUS_DIRECTION dir)
{
    // Address lines (apart from the upper bank address lines A18:16) need to be outputs.
    for(uint8_t idx=Z80_A0; idx <= Z80_A15; idx++)
    {
        pinOutput(idx);
    }
    pinInput(Z80_A16);       // The upper address bits can only be changed via the 273 latch IC which requires a Z80 IO Write transaction.
    pinInput(Z80_A17);
    pinInput(Z80_A18);

    // Control signals need to be output and deasserted.
    //
    pinOutputSet(Z80_IORQ, HIGH);
    pinOutputSet(Z80_MREQ, HIGH);
    pinOutputSet(Z80_RD,   HIGH);
    pinOutputSet(Z80_WR,   HIGH);

    // Additional control lines need to be outputs and deasserted. These lines are for the main motherboard and not used on the tranZPUter.
    //
    pinOutputSet(CTL_HALT, HIGH);
    pinOutputSet(CTL_RFSH, HIGH);
    pinOutputSet(CTL_M1,   HIGH);

    // Setup bus direction.
    //
    setZ80Direction(dir);
    return;
}

// Method to release the Z80, set all signals to input and disable BUSRQ.
//
void releaseZ80(void)
{
    // All address lines to inputs, upper A18:16 are always defined as inputs as they can only be read by the K64F, they are driven by a
    // latch on the tranZPUter board.
    //
    for(uint8_t idx=Z80_A0; idx <= Z80_A15; idx++)
    {
        pinInput(idx);
    }
    // Same for data lines, revert to being inputs.
    for(uint8_t idx=Z80_D0; idx <= Z80_D7; idx++)
    {
        pinInput(idx);
    }

    // All control signals to inputs.
    pinInput(CTL_HALT);
    pinInput(CTL_RFSH);
    pinInput(CTL_M1);
    pinInput(Z80_IORQ);
    pinInput(Z80_MREQ);
    pinInput(Z80_RD);
    pinInput(Z80_WR);

    // Finally release the Z80 by deasserting the CTL_BUSRQ signal.
    relinquishZ80Bus();
    
    // Store the mode.
    z80Control.ctrlMode = Z80_RUN;
       
    // Indicate bus direction.
    z80Control.busDir = TRISTATE;
    return;
}


// Method to write a memory mapped byte onto the Z80 bus.
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
//
uint8_t writeZ80Memory(uint16_t addr, uint8_t data)
{
    // Locals.
    uint32_t startTime = *ms;

    // Set the data and address on the bus.
    //
    setZ80Addr(addr);
    setZ80Data(data);
    pinLow(Z80_MREQ);

    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // If WAIT has been asserted, loop. Set a timeout to prevent a total lockup. Wait shouldnt exceed 100mS, it it does there is a
        // hardware fault and components such as DRAM will lose data due to no refresh.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);

        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
        while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);
    }

    // Complete the write cycle.
    //
    pinHigh(Z80_WR);
    pinHigh(Z80_MREQ);

    return(0);
}

// Method to read a memory mapped byte from the Z80 bus.
// Keep to the Z80 timing diagram, but for better accuracy of matching the timing diagram this needs to be coded in assembler.
//
uint8_t readZ80Memory(uint16_t addr)
{
    // Locals.
    uint32_t startTime = *ms;
    uint8_t  data;

    // Set the address on the bus and assert MREQ and RD.
    //
    setZ80Addr(addr);
    pinLow(Z80_MREQ);
    pinLow(Z80_RD);
  
    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
        
        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        for(volatile uint32_t pulseWidth=0; pulseWidth < 1; pulseWidth++);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readDataBus();

    // Complete the read cycle.
    //
    pinHigh(Z80_RD);
    pinHigh(Z80_MREQ);

    // Finally pass back the byte read to the caller.
    return(data);
}


// Method to write a byte onto the Z80 I/O bus. This method is almost identical to the memory mapped method but are kept seperate for different
// timings as needed, the more code will create greater delays in the pulse width and timing.
//
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
//
uint8_t writeZ80IO(uint16_t addr, uint8_t data)
{
    // Locals.
    uint32_t startTime = *ms;

    // Set the data and address on the bus.
    //
    setZ80Addr(addr);
    setZ80Data(data);
    pinLow(Z80_IORQ);

    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // If WAIT has been asserted, loop. Set a timeout to prevent a total lockup. Wait shouldnt exceed 100mS, it it does there is a
        // hardware fault and components such as DRAM will lose data due to no refresh.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);

        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);

        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // Another wait loop check as the Z80 can assert wait at the time of Write or anytime before it is deasserted.
        while((*ms - startTime) < 200 && pinGet(Z80_WAIT) == 0);
    } else
    {
        // Start the write cycle, MREQ and WR go low.
        pinLow(Z80_WR);
    }

    // Complete the write cycle.
    //
    pinHigh(Z80_WR);
    pinHigh(Z80_IORQ);

    return(0);
}

// Method to read a byte from the Z80 I/O bus. This method is almost identical to the memory mapped method but are kept seperate for different
// timings as needed, the more code will create greater delays in the pulse width and timing.
//
// As the underlying motherboard is 2MHz we keep to its timing best we can in C, for faster motherboards this method may need to 
// be coded in assembler.
uint8_t readZ80IO(uint16_t addr)
{
    // Locals.
    uint32_t startTime = *ms;
    uint8_t  data;

    // Set the address on the bus and assert MREQ and RD.
    //
    setZ80Addr(addr);
    pinLow(Z80_IORQ);
    pinLow(Z80_RD);
  
    // Different logic according to what is being accessed. The mainboard needs to uphold timing and WAIT signals whereas the Tranzputer logic has no wait
    // signals and faster memory.
    //
    if(z80Control.ctrlMode == MAINBOARD_ACCESS)
    {
        // On a Teensy3.5 K64F running at 120MHz this delay gives a pulsewidth of 760nS. This gives time for the addressed device to present the data
        // on the data bus.
        //for(volatile uint32_t pulseWidth=0; pulseWidth < 2; pulseWidth++);

        // A wait loop check as the Z80 can assert wait during the Read operation to request more time. Set a timeout in case of hardware lockup.
        while((*ms - startTime) < 100 && pinGet(Z80_WAIT) == 0);
    }

    // Fetch the data before deasserting the signals.
    //
    data = readDataBus();

    // Complete the read cycle.
    //
    pinHigh(Z80_RD);
    pinHigh(Z80_IORQ);

    // Finally pass back the byte read to the caller.
    return(data);
}

// Method to perform a refresh cycle on the z80 mainboard bus to ensure dynamic RAM contents are maintained during extended bus control periods.
//
// Under normal z80 processing a refresh cycle is issued every instruction during the T3/T4 cycles. As we arent reading instructions but just reading/writing
// then the refresh cycles are made after a cnofigurable set number of bus transactions (ie. 128 read/writes). This has to occur when either of the busses
// are under K64F control so a call to this method should be made frequently.
// 
void refreshZ80(void)
{
    // Locals.
    volatile uint8_t idx = 0;

    // Set 7 bits on the address bus.
    setZ80RefreshAddr(z80Control.refreshAddr);

    // If we are controlling the tranZPUter bus then a switch to the mainboard needs to be made first.
    //
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Quick way to gain mainboard access, force an enable of the Z80_BUSACK on the mainboard by setting RD/WR low, this fires an enable pulse at the 279 RS Flip Flop
        // and setting CTL_BUSACK high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_BUSACK);
    }

    // Assert Refresh.
    pinLow(CTL_RFSH);
    pinLow(Z80_MREQ);
    idx++;              // Increase the pulse width of the MREQ signal.
    pinHigh(Z80_MREQ);
    pinHigh(CTL_RFSH);

    // Restore access to tranZPUter bus if this was the original mode.
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Restore tranZPUter bus control.
        pinLow(CTL_BUSACK);
    }
   
    // Increment refresh address to complete.
    z80Control.refreshAddr++;
    z80Control.refreshAddr &= 0x7f;
    return;
}

// Method to perform a full row refresh on the dynamic DRAM. This method writes out a full 7bit address so that all rows are refreshed.
//
void refreshZ80AllRows(void)
{
    // Locals.
    volatile uint8_t idx;

    // If we are controlling the tranZPUter bus then a switch to the mainboard needs to be made first.
    //
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Quick way to gain mainboard access, force an enable of the Z80_BUSACK on the mainboard by setting RD/WR low, this fires an enable pulse at the 279 RS Flip Flop
        // and setting CTL_BUSACK high enables the second component forming the Z80_BUSACK signal.
        pinLow(Z80_RD);
        pinLow(Z80_WR);
        pinHigh(Z80_RD);
        pinHigh(Z80_WR);
        pinHigh(CTL_BUSACK);
    }

    // Loop through all 7 bits of refresh rows.
    idx = 0;
    while(idx < 0x80)
    {
        // Set 7 bits on the address bus.
        setZ80RefreshAddr(idx);

        // Assert Refresh.
        pinLow(CTL_RFSH);
        pinLow(Z80_MREQ);
        idx++;
        pinHigh(Z80_MREQ);
        pinHigh(CTL_RFSH);
    }

    // Restore access to tranZPUter bus if this was the original mode.
    if(z80Control.ctrlMode == TRANZPUTER_ACCESS)
    {
        // Restore tranZPUter bus control.
        pinLow(CTL_BUSACK);
    }
    return;
}

// Method to fill memory under the Z80 control, either the mainboard or tranZPUter memory.
//
void fillZ80Memory(uint32_t addr, uint32_t size, uint8_t data, uint8_t mainBoard)
{
    // Locals.

    if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // Fill the memory but every FILL_RFSH_BYTE_CNT perform a DRAM refresh.
        //
        for(uint32_t idx=addr; idx < (addr+size); idx++)
        {
            // If the address changes the upper address bits, update the latch to reflect the change.
            if((uint8_t)(idx >> 16) != readUpperAddr())
            {
                // Write the upper address bits to the 273 control latch. The lower 5 bits remain as set by the bus select methods.
                writeCtrlLatch( (uint8_t)((idx >> 11) | (z80Control.curCtrlLatch & 0b00011111)) );
            }

            if(idx % FILL_RFSH_BYTE_CNT == 0)
            {
                // Perform a full row refresh to maintain the DRAM.
                refreshZ80AllRows();
            }
            writeZ80Memory((uint16_t)idx, data);
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// A method to read the full video frame buffer from the Sharp MZ80A and store it in local memory (control structure). 
// No refresh cycles are needed as we grab the frame but between frames a full refresh is performed.
//
void captureVideoFrame(enum VIDEO_FRAMES frame, uint8_t noAttributeFrame)
{
    // Locals.

    if(reqMainboardBus(100) == 0)
    {
        // Setup the pins to perform a read operation (after setting the latch to starting value).
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);
        setZ80Direction(READ);

        // No need for refresh as we take less than 2ms time, just grab the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            z80Control.videoRAM[frame][idx] = readZ80Memory((uint16_t)idx+MZ_VID_RAM_ADDR);
        }

        // Perform a full row refresh to maintain the DRAM.
        refreshZ80AllRows();

        // If flag not set capture the attribute RAM. This is normally not present on a standard Sharp MZ80A only present on models
        // with the MZ80A Colour Board upgrade.
        //
        if(noAttributeFrame == 0)
        {
            // Same for the attribute frame, no need for refresh as we take 2ms or less, just grab it.
            for(uint16_t idx=0; idx < MZ_ATTR_RAM_SIZE; idx++)
            {
                z80Control.attributeRAM[frame][idx] = readZ80Memory((uint16_t)idx+MZ_ATTR_RAM_ADDR);
            }

            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// Method to refresh the video frame buffer on the Sharp MZ80A with the data held in local memory.
//
void refreshVideoFrame(enum VIDEO_FRAMES frame, uint8_t scrolHome, uint8_t noAttributeFrame)
{
    // Locals.

    if(reqMainboardBus(100) == 0)
    {
        // Setup the pins to perform a write operation.
        //
        setupSignalsForZ80Access(WRITE);
        writeCtrlLatch(z80Control.curCtrlLatch);

        // No need for refresh as we take less than 2ms time, just write the video frame.
        for(uint16_t idx=0; idx < MZ_VID_RAM_SIZE; idx++)
        {
            writeZ80Memory((uint16_t)idx+MZ_VID_RAM_ADDR, z80Control.videoRAM[frame][idx]);
        }

        // Perform a full row refresh to maintain the DRAM.
        refreshZ80AllRows();
       
        // If flag not set write out to the attribute RAM. This is normally not present on a standard Sharp MZ80A only present on models
        // with the MZ80A Colour Board upgrade.
        //
        if(noAttributeFrame == 0)
        {
            // No need for refresh as we take less than 2ms time, just write the video frame.
            for(uint16_t idx=0; idx < MZ_ATTR_RAM_SIZE; idx++)
            {
                writeZ80Memory((uint16_t)idx+MZ_ATTR_RAM_ADDR, z80Control.attributeRAM[frame][idx]);
            }
       
            // Perform a full row refresh to maintain the DRAM.
            refreshZ80AllRows();
        }

        // If the Scroll Home flag is set, this means execute a read against the hardware scoll register to restore the position 0,0 to location MZ_VID_RAM_ADDR.
        if(scrolHome)
        {
            setZ80Direction(READ);
            readZ80Memory((uint16_t)MZ_SCROL_BASE);
        }

        // Restore the control latch to its original configuration.
        //
        setZ80Direction(WRITE);
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }
    return;
}

// Method to load up the local video frame buffer from a file.
//
FRESULT loadVideoFrameBuffer(char *src, enum VIDEO_FRAMES frame)
{
    // Locals.
    //
    FIL           File;
    unsigned int  readSize;
    FRESULT       fr0;

    // Sanity check on filenames.
    if(src == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and open the source file.
    fr0 = f_open(&File, src, FA_OPEN_EXISTING | FA_READ);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        memset(z80Control.videoRAM[frame], MZ_VID_DFLT_BYTE, MZ_VID_RAM_SIZE);
        fr0 = f_read(&File, z80Control.videoRAM[frame], MZ_VID_RAM_SIZE, &readSize);
        if (!fr0)
        {
            memset(z80Control.attributeRAM[frame], MZ_ATTR_DFLT_BYTE, MZ_ATTR_RAM_SIZE);
            fr0 = f_read(&File, z80Control.attributeRAM[frame], MZ_ATTR_RAM_SIZE, &readSize);
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("File not found:%s\n", src);
    }

    return(fr0 ? fr0 : FR_OK);    
}

// Method to save the local video frame buffer into a file.
//
FRESULT saveVideoFrameBuffer(char *dst, enum VIDEO_FRAMES frame)
{
    // Locals.
    //
    FIL           File;
    unsigned int  writeSize;        
    FRESULT       fr0;

    // Sanity check on filenames.
    if(dst == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and create the destination file.
    fr0 = f_open(&File, dst, FA_CREATE_ALWAYS | FA_WRITE);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        // Write the entire framebuffer to the SD file, video then attribute.
        //
        fr0 = f_write(&File, z80Control.videoRAM[frame], MZ_VID_RAM_SIZE, &writeSize);
        if (!fr0 && writeSize == MZ_VID_RAM_SIZE) 
        {
            fr0 = f_write(&File, z80Control.attributeRAM[frame], MZ_ATTR_RAM_SIZE, &writeSize);
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("Cannot create file:%s\n", dst);
    }

    return(fr0 ? fr0 : FR_OK);    
}


// Method to load a file from the SD card directly into the tranZPUter static RAM or mainboard RAM.
//
FRESULT loadZ80Memory(char *src, uint32_t addr, uint8_t mainBoard, uint8_t releaseBus)
{
    // Locals.
    //
    FIL           File;
    uint32_t      loadSize       = 0L;
    uint32_t      memPtr         = addr;
    unsigned int  readSize;
    unsigned char buf[SECTOR_SIZE];
    FRESULT       fr0;

    // Sanity check on filenames.
    if(src == NULL)
        return(FR_INVALID_PARAMETER);
    
    // Try and open the source file.
    fr0 = f_open(&File, src, FA_OPEN_EXISTING | FA_READ);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        // If the Z80 is in RUN mode, request the bus.
        // This mechanism allows for the load command to leave the BUS under the tranZPUter control for upload of multiple files. 
        // Care needs to be taken though that no more than 2-3ms passes without a call to refreshZ80AllRows() otherwise memory loss 
        // may occur.
        //
        if(z80Control.ctrlMode == Z80_RUN)
        {
            // Request the board according to the mainboard flag, mainboard = 1 then the mainboard is controlled otherwise the tranZPUter board.
            if(mainBoard == 0)
            {
                reqTranZPUterBus(100);
            } else
            {
                reqMainboardBus(100);
            }

            // If successful, setup the control pins for upload mode.
            //
            if(z80Control.ctrlMode != Z80_RUN)
            {
                // Setup the pins to perform a write operation.
                //
                setupSignalsForZ80Access(WRITE);
               
                // Setup the control latch to the required starting configuration.
                writeCtrlLatch(z80Control.curCtrlLatch);
            }
        } else
        {
            // See if the bus needs changing.
            //
            enum CTRL_MODE newMode = (mainBoard == 0) ? TRANZPUTER_ACCESS : MAINBOARD_ACCESS;
            reqZ80BusChange(newMode);
        }

        // If we have taken control of the bus, commence upload.
        //
        if(z80Control.ctrlMode != Z80_RUN)
        {
            // Loop, reading a sector at a time from SD file and writing it directly into the Z80 tranZPUter RAM or mainboard RAM.
            //
            loadSize = 0;
            memPtr = addr;
            for (;;) {
                // Wrap a disk read with two full refresh periods to counter for the amount of time taken to read disk.
                refreshZ80AllRows();
                fr0 = f_read(&File, buf, SECTOR_SIZE, &readSize);
                refreshZ80AllRows();
                if (fr0 || readSize == 0) break;   /* error or eof */

                // Go through each byte in sector and send to Z80 bus.
                for(unsigned int idx=0; idx < readSize; idx++)
                {
                    // If the address changes the upper address bits, update the latch to reflect the change.
                    if((uint8_t)(memPtr >> 16) != readUpperAddr())
                    {
                        // Write the upper address bits to the 273 control latch. The lower 5 bits remain as set by the bus select methods.
                        writeCtrlLatch( (uint8_t)((memPtr >> 11) | (z80Control.curCtrlLatch & 0b00011111)) );
                    }

                    // At the halfway mark perform a full refresh.
                    if(idx == (SECTOR_SIZE/2)) 
                    {
                        refreshZ80AllRows();
                    }
                   
                    // And now write the byte and to the next address!
                    writeZ80Memory((uint16_t)memPtr, buf[idx]);
                    memPtr++;
                }
                loadSize += readSize;
            }
        } else
        {
            printf("Failed to request Z80 access.\n");
            fr0 = FR_INT_ERR;
        }
       
        // Close to sync files.
        f_close(&File);
    } else
    {
        printf("File not found:%s\n", src);
    }

    // If requested or an error occurs, then release the Z80 bus as no more uploads will be taking place in this batch.
    //
    if(releaseBus == 1 || fr0)
    {
        // Restore the control latch to its original configuration.
        //
        writeCtrlLatch(z80Control.runCtrlLatch);
        releaseZ80();
    }

    return(fr0 ? fr0 : FR_OK);    
}

// Method to read a section of the tranZPUter/mainboard memory and store it in an SD file.
//
FRESULT saveZ80Memory(char *dst, uint32_t addr, uint32_t size, uint8_t mainBoard)
{
    // Locals.
    //
    FIL           File;
    uint32_t      saveSize       = 0L;
    uint32_t      sizeToWrite;
    uint32_t      memPtr         = addr;
    uint32_t      endAddr        = addr + size;
    unsigned int  writeSize;        
    unsigned char buf[SECTOR_SIZE];
    FRESULT       fr0;

    // Sanity check on filenames.
    if(dst == NULL || size == 0)
        return(FR_INVALID_PARAMETER);
    
    // Try and create the destination file.
    fr0 = f_open(&File, dst, FA_CREATE_ALWAYS | FA_WRITE);
   
    // If no errors in opening the file, proceed with reading and loading into memory.
    if(!fr0)
    {
        if( (mainBoard == 0 && reqTranZPUterBus(100) == 0) || (mainBoard != 0 && reqMainboardBus(100) == 0) )
        {
            // Setup the pins to perform a read operation (after setting the latch to starting value).
            //
            setupSignalsForZ80Access(WRITE);
            writeCtrlLatch(z80Control.curCtrlLatch);
            setZ80Direction(READ);

            // Loop, reading a sector worth of data (or upto limit remaining) from the Z80 tranZPUter RAM or mainboard RAM and writing it into the open SD card file.
            //
            saveSize = 0;
            for (;;) {

                // Work out how many bytes to write in the sector then fetch from the Z80.
                sizeToWrite = (endAddr-saveSize) > SECTOR_SIZE ? SECTOR_SIZE : endAddr - saveSize;
                for(unsigned int idx=0; idx < sizeToWrite; idx++)
                {
                    // If the address changes the upper address bits, update the latch to reflect the change.
                    if((uint8_t)(memPtr >> 16) != readUpperAddr())
                    {
                        // Write the upper address bits to the 273 control latch. The lower 5 bits remain as set by the bus select methods.
                        setZ80Direction(WRITE);
                        writeCtrlLatch( (uint8_t)((memPtr >> 11) | (z80Control.curCtrlLatch & 0b00011111)) );
                        setZ80Direction(READ);
                    }

                    // At the halfway mark perform a full refresh.
                    if(idx == (SECTOR_SIZE/2))
                    {
                        refreshZ80AllRows();
                    }

                    // And now read the byte and to the next address!
                    buf[idx] = readZ80Memory((uint16_t)memPtr);
                    memPtr++;
                }

                // Wrap disk write with two full refresh periods to counter for the amount of time taken to write to disk.
                refreshZ80AllRows();
                fr0 = f_write(&File, buf, sizeToWrite, &writeSize);
                refreshZ80AllRows();
                saveSize += writeSize;
                if (fr0 || writeSize < sizeToWrite || saveSize >= size) break;       // error, disk full or range written.
            }

            // Restore the control latch to its original configuration.
            //
            setZ80Direction(WRITE);
            writeCtrlLatch(z80Control.runCtrlLatch);
            releaseZ80();
            printf("Saved %ld bytes, final address:%lx\n", saveSize, memPtr);

        } else
        {
            printf("Failed to request Z80 access.\n");
        }
       
        // Close to sync files.
        f_close(&File);

    } else
    {
        printf("Cannot create file:%s\n", dst);
    }

    return(fr0 ? fr0 : FR_OK);    
}

#if defined DEBUG
// Simple method to output the Z80 signals to the console - feel good factor. To be of real use the signals need to be captured against the system
// clock and the replayed, perhaps a todo!
//
void displaySignals(void)
{
    uint32_t ADDR = 0;
    uint8_t  DATA = 0;
    uint8_t  RD = 0;
    uint8_t  WR = 0;
    uint8_t  IORQ = 0;
    uint8_t  MREQ = 0;
    uint8_t  NMI = 0;
    uint8_t  INT = 0;
    uint8_t  M1 = 0;
    uint8_t  RFSH = 0;
    uint8_t  WAIT = 0;
    uint8_t  BUSRQ = 0;
    uint8_t  BUSACK = 0;
    uint8_t  ZBUSACK = 0;
    uint8_t  HALT = 0;
    uint8_t  CLKSLCT = 0;

    setupPins(NULL);

    printf("Z80 Bus Signals:\r\n");
    while(1)
    {
        ADDR  = (pinGet(Z80_A18) & 0x1) << 18;
        ADDR |= (pinGet(Z80_A17) & 0x1) << 17;
        ADDR |= (pinGet(Z80_A16) & 0x1) << 16;
        ADDR |= (pinGet(Z80_A15) & 0x1) << 15;
        ADDR |= (pinGet(Z80_A14) & 0x1) << 14;
        ADDR |= (pinGet(Z80_A13) & 0x1) << 13;
        ADDR |= (pinGet(Z80_A12) & 0x1) << 12;
        ADDR |= (pinGet(Z80_A11) & 0x1) << 11;
        ADDR |= (pinGet(Z80_A10) & 0x1) << 10;
        ADDR |= (pinGet(Z80_A9)  & 0x1) << 9;
        ADDR |= (pinGet(Z80_A8)  & 0x1) << 8;
        ADDR |= (pinGet(Z80_A7)  & 0x1) << 7;
        ADDR |= (pinGet(Z80_A6)  & 0x1) << 6;
        ADDR |= (pinGet(Z80_A5)  & 0x1) << 5;
        ADDR |= (pinGet(Z80_A4)  & 0x1) << 4;
        ADDR |= (pinGet(Z80_A3)  & 0x1) << 3;
        ADDR |= (pinGet(Z80_A2)  & 0x1) << 2;
        ADDR |= (pinGet(Z80_A1)  & 0x1) << 1;
        ADDR |= (pinGet(Z80_A0)  & 0x1);
        DATA  = (pinGet(Z80_D7)  & 0x1) << 7;
        DATA |= (pinGet(Z80_D6)  & 0x1) << 6;
        DATA |= (pinGet(Z80_D5)  & 0x1) << 5;
        DATA |= (pinGet(Z80_D4)  & 0x1) << 4;
        DATA |= (pinGet(Z80_D3)  & 0x1) << 3;
        DATA |= (pinGet(Z80_D2)  & 0x1) << 2;
        DATA |= (pinGet(Z80_A1)  & 0x1) << 1;
        DATA |= (pinGet(Z80_D0)  & 0x1);
        RD=pinGet(Z80_RD);
        WR=pinGet(Z80_WR);
        MREQ=pinGet(Z80_MREQ);
        IORQ=pinGet(Z80_IORQ);
        NMI=pinGet(Z80_NMI);
        INT=pinGet(Z80_INT);
        M1=pinGet(CTL_M1);
        RFSH=pinGet(CTL_RFSH);
        WAIT=pinGet(Z80_WAIT);
        BUSRQ=pinGet(CTL_BUSRQ);
        BUSACK=pinGet(CTL_BUSACK);
        ZBUSACK=pinGet(Z80_BUSACK);
        HALT=pinGet(CTL_HALT);
        CLKSLCT=pinGet(CTL_CLKSLCT);
    
        printf("\rADDR=%06lx %08x %3s %3s %3s %3s %3s %3s %2s %4s %4s %2s %2s %3s %4s %4s", ADDR, DATA,
                                  (RD == 0 && MREQ == 0 && WR == 1 && IORQ == 1) ? "MRD" : "   ",
                                  (RD == 0 && IORQ == 0 && WR == 1 && MREQ == 1) ? "IRD" : "   ",
                                  (WR == 0 && MREQ == 0 && RD == 1 && IORQ == 1) ? "MWR" : "   ",
                                  (WR == 0 && IORQ == 0 && RD == 1 && MREQ == 1) ? "IWR" : "   ",
                                  (NMI == 0)     ? "NMI"   : "   ",
                                  (INT == 0)     ? "INT"   : "   ",
                                  (M1 == 0)      ? "M1"    : "  ",
                                  (RFSH == 0)    ? "RFSH"  : "    ",
                                  (WAIT == 0)    ? "WAIT"  : "    ",
                                  (BUSRQ == 0)   ? "BR"    : "  ",
                                  (BUSACK == 0)  ? "BA"    : "  ",
                                  (ZBUSACK == 0) ? "ZBA"   : "   ",
                                  (HALT == 0)    ? "HALT"  : "    ",
                                  (CLKSLCT == 0) ? "CLKS"  : "    "
              );
    }

    return;
}
#endif


#ifdef __cplusplus
}
#endif
