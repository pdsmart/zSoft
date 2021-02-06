/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            sdmmc.c
// Created:         June 2019
// Author(s):       ChaN (framework), Philip Smart (zpu SoC customisation)
// Description:     Functionality to enable connectivity between the FatFS ((C) ChaN) and the ZPU SoC
//                  for SD drives. The majority of SD logic exists in hardware, this module provides
//                  the public interfaces to interact with the hardware.
//
// Credits:         
// Copyright:       (C) 2013, ChaN, all rights reserved - framework.
// Copyright:       (C) 2019 Philip Smart <philip.smart@net2net.org>
//
// History:         January 2019   - Initial script written for the STORM processor then changed to the ZPU.
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

#include "ff.h"        /* Obtains integer types for FatFs */
#include "diskio.h"    /* Common include file for FatFs and disk I/O layer */

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/


#if defined __K64F__
  #include <stdlib.h>
  #include <string.h>
  #define uint32_t __uint32_t
  #define uint16_t __uint16_t
  #define uint8_t  __uint8_t
  #define int32_t  __int32_t
  #define int16_t  __int16_t
  #define int8_t   __int8_t
#else
  #include <stdint.h>
  #include <stdio.h>
  #if defined __SHARPMZ__
    #include "sharpmz.h"
  #endif
#endif

#include "zpu_soc.h"
#include "uart.h"
#include "utils.h"

/*--------------------------------------------------------------------------
   Module Private Functions
---------------------------------------------------------------------------*/

/* MMC/SD command (SPI mode) */
#define CMD0         (0)         /* GO_IDLE_STATE */
#define CMD1         (1)         /* SEND_OP_COND */
#define ACMD41       (0x80+41)   /* SEND_OP_COND (SDC) */
#define CMD8         (8)         /* SEND_IF_COND */
#define CMD9         (9)         /* SEND_CSD */
#define CMD10        (10)        /* SEND_CID */
#define CMD12        (12)        /* STOP_TRANSMISSION */
#define CMD13        (13)        /* SEND_STATUS */
#define ACMD13       (0x80+13)   /* SD_STATUS (SDC) */
#define CMD16        (16)        /* SET_BLOCKLEN */
#define CMD17        (17)        /* READ_SINGLE_BLOCK */
#define CMD18        (18)        /* READ_MULTIPLE_BLOCK */
#define CMD23        (23)        /* SET_BLOCK_COUNT */
#define ACMD23       (0x80+23)   /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24        (24)        /* WRITE_BLOCK */
#define CMD25        (25)        /* WRITE_MULTIPLE_BLOCK */
#define CMD32        (32)        /* ERASE_ER_BLK_START */
#define CMD33        (33)        /* ERASE_ER_BLK_END */
#define CMD38        (38)        /* ERASE */
#define CMD55        (55)        /* APP_CMD */
#define CMD58        (58)        /* READ_OCR */
#define SECTOR_SIZE  512         /* Default size of an SD Sector */

static
DSTATUS Stat[SD_DEVICE_CNT] = { STA_NOINIT };    /* Disk status */

/*--------------------------------------------------------------------------
   Public Functions
---------------------------------------------------------------------------*/

#if defined __SHARPMZ__
// Volume to partiion map. This map specifies which Volume resides on which disk/partition.
// NB. When using the ZPU as a host on the Sharp MZ computers, the K64F hosts the SD card so the first volume will be the second on the actual physical SD card.
//
PARTITION VolToPart[FF_VOLUMES] = {
    {0, 2},     /* "0:" ==> 1st partition on physical drive 0 */
    {0, 3},     /* "1:" ==> 2nd partition on physical drive 0 */
    {0, 4},     /* "2:" ==> 3rd partition on physical drive 0 */
    {1, 1},     /* "4:" ==> Physical drive 1, 1st partition */    
#if FF_VOLUMES > 4
    {1, 2},     /* "5:" ==> Physical drive 1, 2nd partition */    
    {1, 3},     /* "6:" ==> Physical drive 1, 3rd partition */    
    {1, 4},     /* "7:" ==> Physical drive 1, 4th partition */    
    {2, 1},     /* "8:" ==> Physical drive 2, 1st partition */    
#endif
};
#else
PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1},     /* "0:" ==> 1st partition on physical drive 0 */
    {0, 2},     /* "1:" ==> 2nd partition on physical drive 0 */
    {0, 3},     /* "2:" ==> 3rd partition on physical drive 0 */
    {0, 4},     /* "3:" ==> 3rd partition on physical drive 0 */
#if FF_VOLUMES > 4
    {1, 1},     /* "4:" ==> Physical drive 1, 1st partition */    
    {1, 2},     /* "5:" ==> Physical drive 1, 2nd partition */    
    {1, 3},     /* "6:" ==> Physical drive 1, 3rd partition */    
    {1, 4},     /* "7:" ==> Physical drive 1, 4th partition */    
#endif
};
#endif

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status ( BYTE drv )          /* Drive number */
{
    // Ignore any drives which havent been implemented.
    if (drv > SD_DEVICE_CNT) return STA_NOINIT;

    return Stat[drv];
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize ( BYTE drv,             /* Physical drive nmuber */
                          BYTE cardType )       /* 0 = SD, 1 = SDHC */
{
    //uint32_t status;
    // Ignore any drives which havent been implemented.
    if (drv > SD_DEVICE_CNT) return RES_NOTRDY;

  #if defined __SHARPMZ__
    if(mzSDInit(drv) == 0)
        Stat[drv] = 0;
  #else
    // Set the card type.
    SD_CMD(drv) = (cardType == 0 ? SD_CMD_CARDTYPE_SD : SD_CMD_CARDTYPE_SDHC );

    // Issue the reset command to initialise the drive.
    SD_CMD(drv) = SD_CMD_RESET;

    // Setup a 5 second delay count, if this timer expires then initialisation failed.
    TIMER_SECONDS_DOWN = 5;

    // Wait until the drive becomes ready.
    while(IS_SD_BUSY(drv) && TIMER_SECONDS_DOWN > 0);

    // If there is an error code, then the drive didnt initialise.
    if(!(SD_STATUS(drv) & SD_STATUS_ERROR) && TIMER_SECONDS_DOWN > 0)
        Stat[drv] = 0;
  #endif

    return Stat[drv];
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read ( BYTE drv,            /* Physical drive nmuber (0) */
                    BYTE *buff,          /* Pointer to the data buffer to store read data */
                    DWORD sector,        /* Start sector number (LBA) */
                    UINT count    )      /* Sector count (1..128) */
{
    uint8_t  idx;
    uint32_t status;
    uint32_t retry = 3;
    uint32_t rxCount;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;

    // For multiple blocks we have to issue single block reads so loop for count blocks.
    idx = 0;
    rxCount = 0;
    do {
        // Setup a 5 second delay count, if this timer expires then reset and retry.
        TIMER_SECONDS_DOWN = 5;

  #if defined __SHARPMZ__
        // When running on the Sharp MZ host, we send an SD request to the I/O processor and await the results!
        do {
            status = mzSDRead(drv, sector, buff);
        } while(status != 0 && TIMER_SECONDS_DOWN > 0);

        if(status == 0)
        {
            sector = sector + SECTOR_SIZE;
            idx++;
        }
  #else
        // Set the sector to retrieve.
        SD_ADDR(drv) = sector;
        SD_CMD(drv) = SD_CMD_READ;
       
        // Receive all bytes until Busy goes inactive or timer timesout.
        do {
            status = SD_STATUS(drv);
            if(status & SD_STATUS_DATA_VALID)
            {
                *(BYTE *)(buff) = (uint8_t)SD_DATA(drv);
                buff++;
                rxCount++;
            }
        } while((status & (SD_STATUS_BUSY|SD_STATUS_DATA_VALID)) != 0 && rxCount < SECTOR_SIZE && TIMER_SECONDS_DOWN > 0);

        // If we exitted due to a timeout, retry. If no more retries available, exit with last error.
        if(TIMER_SECONDS_DOWN == 0 || rxCount != SECTOR_SIZE)
        {
            // Issue the reset command to initialise the drive.
            SD_CMD(drv) = SD_CMD_RESET;

            // Wait until the drive becomes ready.
            while(IS_SD_BUSY(drv));

            retry--;
        } else
        {
            sector = sector + SECTOR_SIZE;
            idx++;
        }

        if(retry == 0) break;
        if(status & SD_STATUS_ERROR) break;
  #endif
    } while( idx < count );

    // Return error if the last read failed.
    return status & SD_STATUS_ERROR ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write ( BYTE drv,            /* Physical drive nmuber (0) */
                     const BYTE *buff,    /* Pointer to the data to be written */
                     DWORD sector,        /* Start sector number (LBA) */
                     UINT count       )   /* Sector count (1..128) */
{
    uint8_t  idx;
    uint32_t status;
    uint32_t retry = 3;
    uint32_t txCount = 0;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
 
    // For multiple blocks we have to issue single block writes so loop for count blocks.
    idx = 0;
    do {
        // Setup a 5 second delay count, if this timer expires then reset and retry.
        TIMER_SECONDS_DOWN = 5;

  #if defined __SHARPMZ__
        // When running on the Sharp MZ host, we send an SD request to the I/O processor and await the results!
        do {
            status = mzSDWrite(drv, sector, buff);
        } while(status != 0 && TIMER_SECONDS_DOWN > 0);

        if(status == 0)
        {
            sector = sector + SECTOR_SIZE;
            idx++;
        }
  #else
        // Set the sector to retrieve.
        SD_ADDR(drv) = sector;
        SD_CMD(drv) = SD_CMD_WRITE;

        // Send bytes upto sector limit or until busy goes inactive or timer times out.
        txCount = 0;
        do {
            status = SD_STATUS(drv);

            if(status & SD_STATUS_DATA_REQ)
            {
                SD_DATA(drv) = *buff;
                buff++;
                txCount++;
            }
        } while((status & SD_STATUS_BUSY) && TIMER_SECONDS_DOWN > 0);

        // If we exitted due to a timeout, retry. If no more retries available, exit with last error.
        if(TIMER_SECONDS_DOWN == 0 || txCount != SECTOR_SIZE)
        {
            // Issue the reset command to initialise the drive.
            SD_CMD(drv) = SD_CMD_RESET;

            // Wait until the drive becomes ready.
            while(IS_SD_BUSY(drv));
            retry--;
        } else
        {
            idx++;
            sector = sector + SECTOR_SIZE;
        }

        if(retry == 0) break;
        if(status & SD_STATUS_ERROR) break;
  #endif
    } while (idx < count);

    // Return error if the last write failed.
    return status & SD_STATUS_ERROR ? RES_ERROR : RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl ( BYTE drv,         /* Physical drive nmuber (0) */
                     BYTE ctrl,        /* Control code */
                     void *buff )      /* Buffer to send/receive control data */
{
    DRESULT  res;
    uint32_t status;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
 
    // Setup a 5 second delay count, if this timer expires then we have an error.
    TIMER_SECONDS_DOWN = 5;

    res = RES_ERROR;
    switch (ctrl)
    {
        case CTRL_SYNC :        /* Make sure that no pending write process */
            // Wait until the drive becomes available or we timeout. 
            do {
                status = SD_STATUS(drv);
            } while((status & SD_STATUS_BUSY) && TIMER_SECONDS_DOWN > 0);

            // If we timed out then an error has occurred, so reset the drive and return error code.
            if(TIMER_SECONDS_DOWN == 0)
            {
                // Issue the reset command to initialise the drive.
                SD_CMD(drv) = SD_CMD_RESET;

                // Wait until the drive becomes ready.
                while(IS_SD_BUSY(drv));
            } else
                res = RES_OK;
            break;

        case GET_SECTOR_COUNT :    /* Get number of sectors on the disk (DWORD) */

            // Temporary.
            *(DWORD*)buff = 2097152; // 1Gb.
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE :    /* Get erase block size in unit of sector (DWORD) */
            *(DWORD*)buff = 128;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}
