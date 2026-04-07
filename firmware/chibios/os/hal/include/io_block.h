/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    io_block.h
 * @brief   I/O block devices access.
 * @details This header defines an abstract interface useful to access generic
 *          I/O block devices in a standardized way.
 *
 * @addtogroup IO_BLOCK
 * @details This module defines an abstract interface for accessing generic
 *          block devices.<br>
 *          Note that no code is present, just abstract interfaces-like
 *          structures, you should look at the system as to a set of
 *          abstract C++ classes (even if written in C). This system
 *          has then advantage to make the access to block devices
 *          independent from the implementation logic.
 * @{
 */

#ifndef _IO_BLOCK_H_
#define _IO_BLOCK_H_

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  BLK_UNINIT = 0,                   /**< Not initialized.                   */
  BLK_STOP = 1,                     /**< Stopped.                           */
  BLK_ACTIVE = 2,                   /**< Interface active.                  */
  BLK_CONNECTING = 3,               /**< Connection in progress.            */
  BLK_DISCONNECTING = 4,            /**< Disconnection in progress.         */
  BLK_READY = 5,                    /**< Device ready.                      */
  BLK_READING = 6,                  /**< Read operation in progress.        */
  BLK_WRITING = 7,                  /**< Write operation in progress.       */
  BLK_SYNCING = 8                   /**< Sync. operation in progress.       */
} blkstate_t;

/**
 * @brief   Block device info.
 */
typedef struct {
  uint32_t      blk_size;           /**< @brief Block size in bytes.        */
  uint32_t      blk_num;            /**< @brief Total number of blocks.     */
} BlockDeviceInfo;

/**
 * @brief   @p BaseBlockDevice specific methods.
 */
#define _base_block_device_methods                                          \
  /* Removable media detection.*/                                           \
  bool_t (*is_inserted)(void *instance);                                    \
  /* Removable write protection detection.*/                                \
  bool_t (*is_protected)(void *instance);                                   \
  /* Connection to the block device.*/                                      \
  bool_t (*connect)(void *instance);                                        \
  /* Disconnection from the block device.*/                                 \
  bool_t (*disconnect)(void *instance);                                     \
  /* Reads one or more blocks.*/                                            \
  bool_t (*read)(void *instance, uint32_t startblk,                         \
                 uint8_t *buffer, uint32_t n);                              \
  /* Writes one or more blocks.*/                                           \
  bool_t (*write)(void *instance, uint32_t startblk,                        \
                  const uint8_t *buffer, uint32_t n);                       \
  /* Write operations synchronization.*/                                    \
  bool_t (*sync)(void *instance);                                           \
  /* Obtains info about the media.*/                                        \
  bool_t (*get_info)(void *instance, BlockDeviceInfo *bdip);

/**
 * @brief   @p BaseBlockDevice specific data.
 */
#define _base_block_device_data                                             \
  /* Driver state.*/                                                        \
  blkstate_t            state;

/**
 * @brief   @p BaseBlockDevice virtual methods table.
 */
struct BaseBlockDeviceVMT {
  _base_block_device_methods
};

/**
 * @brief   Base block device class.
 * @details This class represents a generic, block-accessible, device.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseBlockDeviceVMT *vmt;
  _base_block_device_data
} BaseBlockDevice;

/**
 * @name    Macro Functions (BaseBlockDevice)
 * @{
 */
/**
 * @brief   Returns the driver state.
 * @note    Can be called in ISR context.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The driver state.
 *
 * @special
 */
#define blkGetDriverState(ip) ((ip)->state)

/**
 * @brief   Determines if the device is transferring data.
 * @note    Can be called in ISR context.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The driver state.
 * @retval FALSE        the device is not transferring data.
 * @retval TRUE         the device not transferring data.
 *
 * @special
 */
#define blkIsTransferring(ip) ((((ip)->state) == BLK_CONNECTING) ||         \
                               (((ip)->state) == BLK_DISCONNECTING) ||      \
                               (((ip)->state) == BLK_READING) ||            \
                               (((ip)->state) == BLK_WRITING))

/**
 * @brief   Returns the media insertion status.
 * @note    On some implementations this function can only be called if the
 *          device is not transferring data.
 *          The function @p blkIsTransferring() should be used before calling
 *          this function.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The media state.
 * @retval FALSE        media not inserted.
 * @retval TRUE         media inserted.
 *
 * @api
 */
#define blkIsInserted(ip) ((ip)->vmt->is_inserted(ip))

/**
 * @brief   Returns the media write protection status.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The media state.
 * @retval FALSE        writable media.
 * @retval TRUE         non writable media.
 *
 * @api
 */
#define blkIsWriteProtected(ip) ((ip)->vmt->is_protected(ip))

/**
 * @brief   Performs the initialization procedure on the block device.
 * @details This function should be performed before I/O operations can be
 *          attempted on the block device and after insertion has been
 *          confirmed using @p blkIsInserted().
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkConnect(ip) ((ip)->vmt->connect(ip))

/**
 * @brief   Terminates operations on the block device.
 * @details This operation safely terminates operations on the block device.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkDisconnect(ip) ((ip)->vmt->disconnect(ip))

/**
 * @brief   Reads one or more blocks.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 * @param[in] startblk  first block to read
 * @param[out] buf      pointer to the read buffer
 * @param[in] n         number of blocks to read
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkRead(ip, startblk, buf, n)                                       \
  ((ip)->vmt->read(ip, startblk, buf, n))

/**
 * @brief   Writes one or more blocks.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 * @param[in] startblk  first block to write
 * @param[out] buf      pointer to the write buffer
 * @param[in] n         number of blocks to write
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkWrite(ip, startblk, buf, n)                                      \
  ((ip)->vmt->write(ip, startblk, buf, n))

/**
 * @brief   Ensures write synchronization.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkSync(ip) ((ip)->vmt->sync(ip))

/**
 * @brief   Returns a media information structure.
 *
 * @param[in] ip        pointer to a @p BaseBlockDevice or derived class
 * @param[out] bdip     pointer to a @p BlockDeviceInfo structure
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define blkGetInfo(ip, bdip) ((ip)->vmt->get_info(ip, bdip))

/** @} */

#endif /* _IO_BLOCK_H_ */

/** @} */
