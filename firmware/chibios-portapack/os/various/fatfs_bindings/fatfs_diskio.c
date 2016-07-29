/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ch.h"
#include "hal.h"

#include <string.h>

#include "diskio.h"

#if HAL_USE_MMC_SPI && HAL_USE_SDC
#error "cannot specify both MMC_SPI and SDC drivers"
#endif

#if HAL_USE_MMC_SPI
extern MMCDriver MMCD1;
#elif HAL_USE_SDC
extern SDCDriver SDCD1;
#else
#error "MMC_SPI or SDC driver must be specified"
#endif

#if HAL_USE_RTC
#include "chrtclib.h"
extern RTCDriver RTCD1;
#endif

/* Definitions of physical drive number for each drive */
#define MMC     0
#define SDC     0


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv   /* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat;

  switch (pdrv) {
#if HAL_USE_MMC_SPI
  case MMC:
    stat = 0;
    /* It is initialized externally, just reads the status.*/
    if (blkGetDriverState(&MMCD1) != BLK_READY)
      stat |= STA_NOINIT;
    if (mmcIsWriteProtected(&MMCD1))
      stat |= STA_PROTECT;
    return stat;
#else
  case SDC:
    stat = 0;
    /* It is initialized externally, just reads the status.*/
    if (blkGetDriverState(&SDCD1) != BLK_READY)
      stat |= STA_NOINIT;
    if (sdcIsWriteProtected(&SDCD1))
      stat |= STA_PROTECT;
    return stat;
#endif
  }
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
  BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat;

  switch (pdrv) {
#if HAL_USE_MMC_SPI
  case MMC:
    stat = 0;
    /* It is initialized externally, just reads the status.*/
    if (blkGetDriverState(&MMCD1) != BLK_READY)
      stat |= STA_NOINIT;
    if (mmcIsWriteProtected(&MMCD1))
      stat |= STA_PROTECT;
    return stat;
#else
  case SDC:
    stat = 0;
    /* It is initialized externally, just reads the status.*/
    if (blkGetDriverState(&SDCD1) != BLK_READY)
      stat |= STA_NOINIT;
    if (sdcIsWriteProtected(&SDCD1))
      stat |= STA_PROTECT;
    return stat;
#endif
  }
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
  BYTE pdrv,    /* Physical drive nmuber to identify the drive */
  BYTE *buff,   /* Data buffer to store read data */
  DWORD sector, /* Sector address in LBA */
  UINT count    /* Number of sectors to read */
)
{
  switch (pdrv) {
#if HAL_USE_MMC_SPI
  case MMC:
    if (blkGetDriverState(&MMCD1) != BLK_READY)
      return RES_NOTRDY;
    if (mmcStartSequentialRead(&MMCD1, sector))
      return RES_ERROR;
    while (count > 0) {
      if (mmcSequentialRead(&MMCD1, buff))
        return RES_ERROR;
      buff += MMCSD_BLOCK_SIZE;
      count--;
    }
    if (mmcStopSequentialRead(&MMCD1))
        return RES_ERROR;
    return RES_OK;
#else
  case SDC:
    if (blkGetDriverState(&SDCD1) != BLK_READY)
      return RES_NOTRDY;
    if (sdcRead(&SDCD1, sector, buff, count))
      return RES_ERROR;
    return RES_OK;
#endif
  }
  return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
  BYTE pdrv,      /* Physical drive nmuber to identify the drive */
  const BYTE *buff, /* Data to be written */
  DWORD sector,   /* Sector address in LBA */
  UINT count      /* Number of sectors to write */
)
{
  switch (pdrv) {
#if HAL_USE_MMC_SPI
  case MMC:
    if (blkGetDriverState(&MMCD1) != BLK_READY)
        return RES_NOTRDY;
    if (mmcIsWriteProtected(&MMCD1))
        return RES_WRPRT;
    if (mmcStartSequentialWrite(&MMCD1, sector))
        return RES_ERROR;
    while (count > 0) {
        if (mmcSequentialWrite(&MMCD1, buff))
            return RES_ERROR;
        buff += MMCSD_BLOCK_SIZE;
        count--;
    }
    if (mmcStopSequentialWrite(&MMCD1))
        return RES_ERROR;
    return RES_OK;
#else
  case SDC:
    if (blkGetDriverState(&SDCD1) != BLK_READY)
      return RES_NOTRDY;
    if (sdcWrite(&SDCD1, sector, buff, count))
      return RES_ERROR;
    return RES_OK;
#endif
  }
  return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
  BYTE pdrv,    /* Physical drive nmuber (0..) */
  BYTE cmd,     /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
  switch (pdrv) {
#if HAL_USE_MMC_SPI
  case MMC:
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_SIZE:
        *((WORD *)buff) = MMCSD_BLOCK_SIZE;
        return RES_OK;
    case MMC_GET_TYPE:
        *((BYTE *)buff) = SDCD1.cardmode;
        return RES_OK;
    case MMC_GET_CSD:
        memcpy(buff, &SDCD1.csd, sizeof(SDCD1.csd));
        return RES_OK;
#if _USE_ERASE
    case CTRL_ERASE_SECTOR:
        mmcErase(&MMCD1, *((DWORD *)buff), *((DWORD *)buff + 1));
        return RES_OK;
#endif
    default:
        return RES_PARERR;
    }
#else
  case SDC:
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *((DWORD *)buff) = mmcsdGetCardCapacity(&SDCD1);
        return RES_OK;
    case GET_SECTOR_SIZE:
        *((WORD *)buff) = MMCSD_BLOCK_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = 1; /* Unknown, TODO: implement? */
        return RES_OK;
    case MMC_GET_TYPE:
        *((BYTE *)buff) = SDCD1.cardmode;
        return RES_OK;
    case MMC_GET_CSD:
        memcpy(buff, &SDCD1.csd, sizeof(SDCD1.csd));
        return RES_OK;
#if _USE_ERASE
    case CTRL_ERASE_SECTOR:
        sdcErase(&SDCD1, *((DWORD *)buff), *((DWORD *)buff + 1));
        return RES_OK;
#endif
    default:
        return RES_PARERR;
    }
#endif
  }
  return RES_PARERR;
}

DWORD get_fattime(void) {
#if HAL_USE_RTC
    return rtcGetTimeFat(&RTCD1);
#else
    return ((uint32_t)0 | (1 << 16)) | (1 << 21); /* wrong but valid time */
#endif
}



