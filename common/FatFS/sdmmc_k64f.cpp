/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            sdmmc_teensy.c
// Created:         June 2019 - April 2020
// Author(s):       ChaN (framework), Philip Smart (K64F SoC customisation)
// Description:     Functionality to enable connectivity between the FatFS ((C) ChaN) and the Teensy K64F
//                  for SD drives. The majority of SD logic exists in the class provided by NXP and
//                  updated by PJRC, this module provides the public interfaces to interface to the NXP
//                  module.
//
// Credits:         
// Copyright:       (C) 2013    ChaN, all rights reserved - framework.
// Copyright:       (C) 2019-20 Philip Smart <philip.smart@net2net.org>
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

#include <SPI.h>
#include <ioreg.h>
#include <Sd2PinMap.h>
#include <SdInfo.h>
#include "NXP_SDHC.h"
#define BUILTIN_SDCARD 254

#ifdef __cplusplus
    extern "C" {
#endif

#include "ff.h"        /* Obtains integer types for FatFs */
#include "diskio.h"    /* Common include file for FatFs and disk I/O layer */

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/


#include <stdlib.h>
#include <string.h>
#define uint32_t __uint32_t
#define uint16_t __uint16_t
#define uint8_t  __uint8_t
#define int32_t  __int32_t
#define int16_t  __int16_t
#define int8_t   __int8_t

#include "k64f_soc.h"
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
uint8_t card_type;

/*--------------------------------------------------------------------------
   Public Functions
---------------------------------------------------------------------------*/


// Volume to partiion map. This map specifies which Volume resides on which disk/partition.
// NB. When using the ZPU as a host on the Sharp MZ computers, the K64F hosts the SD card so the first volume will be the second on the actual physical SD card.
//
PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1},     /* "0:" ==> 1st partition on physical drive 0 */
    {0, 2},     /* "1:" ==> 2nd partition on physical drive 0 */
    {0, 3},     /* "2:" ==> 3rd partition on physical drive 0 */
    {1, 0}      /* "3:" ==> Physical drive 1 */    
};

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status ( BYTE drv )          /* Drive number */
{
    // Ignore any drives which havent been implemented.
    if (drv > SD_DEVICE_CNT) return STA_NOINIT;
   
    // Check parameters, only 1 drive allowed.
    if(drv > 0) return RES_NOTRDY;

    // Return the status of the drive.
    return Stat[drv];
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize ( BYTE drv,             /* Physical drive nmuber */
                          BYTE cardType )       /* 0 = SD, 1 = SDHC */
{
    uint8_t ret;

    // Ignore any drives which havent been implemented.
    if (drv > SD_DEVICE_CNT) return RES_NOTRDY;
   
    // Check parameters, only 1 drive allowed.
    if(drv > 0) return RES_NOTRDY;

    // Call the methods in the NXP_SDHC class to initialise and obtain the
    // card type. Chip Select is embedded within this module so we dont
    // need to seperately enable/disable it.
    //
    ret = SDHC_CardInit();
    card_type = SDHC_CardGetType();

    // Activate the drive within the Fat module if an OK result was obtained.
    if(ret == 0)
        Stat[drv] = 0;

    // Return the status of the drive.
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
    int   status;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;

    // Check parameters, only 1 drive catered for and count must be >= 1
    if(drv > 0) return RES_NOTRDY;
    if(count == 0) return RES_NOTRDY;

    // call the NXP method to read in a sector, loop for required number of sectors.
    do {
        status = SDHC_CardReadBlock(buff, sector);
        buff += 512;
        sector++;
    } while(status == 0 && --count);

    // Any status other than OK results in a FAT ERROR.
    return status == 0 ? RES_OK : RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write ( BYTE drv,            /* Physical drive nmuber (0) */
                     const BYTE *buff,    /* Pointer to the data to be written */
                     DWORD sector,        /* Start sector number (LBA) */
                     UINT count       )   /* Sector count (1..128) */
{
    int   status;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
 
    // Check parameters, only 1 drive catered for and count must be >= 1
    if(drv > 0) return RES_NOTRDY;
    if(count == 0) return RES_NOTRDY;
   
    // call the NXP method to write a sector, loop for required number of sectors.
    do {
        status = SDHC_CardWriteBlock(buff, sector);
        buff += 512;
        sector++;
    } while(status == 0 && --count);

    // Any status other than OK results in a FAT ERROR.
    return status == 0 ? RES_OK : RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl ( BYTE drv,         /* Physical drive nmuber (0) */
                     BYTE ctrl,        /* Control code */
                     void *buff )      /* Buffer to send/receive control data */
{
    DRESULT  res = RES_ERROR;

    // Check the drive, if it hasnt been initialised then exit.
    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
   
    // Check parameters, only 1 drive catered allowed.
    if(drv > 0) return RES_NOTRDY;
 
    res = RES_ERROR;
    switch (ctrl)
    {
        case CTRL_SYNC :              // Make sure that no pending write process
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT :       // Get number of sectors on the disk (DWORD)

            // Temporary.
            *(DWORD*)buff = 16777216; // 16Gb.
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE :         // Get erase block size in unit of sector (DWORD)
            *(DWORD*)buff = 128;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}

#ifdef __cplusplus
    }
#endif
