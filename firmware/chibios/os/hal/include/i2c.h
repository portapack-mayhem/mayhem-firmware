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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    i2c.h
 * @brief   I2C Driver macros and structures.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef _I2C_H_
#define _I2C_H_

#if HAL_USE_I2C || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    I2C bus error conditions
 * @{
 */
#define I2CD_NO_ERROR               0x00   /**< @brief No error.            */
#define I2CD_BUS_ERROR              0x01   /**< @brief Bus Error.           */
#define I2CD_ARBITRATION_LOST       0x02   /**< @brief Arbitration Lost.    */
#define I2CD_ACK_FAILURE            0x04   /**< @brief Acknowledge Failure. */
#define I2CD_OVERRUN                0x08   /**< @brief Overrun/Underrun.    */
#define I2CD_PEC_ERROR              0x10   /**< @brief PEC Error in
                                                reception.                  */
#define I2CD_TIMEOUT                0x20   /**< @brief Hardware timeout.    */
#define I2CD_SMB_ALERT              0x40   /**< @brief SMBus Alert.         */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Enables the mutual exclusion APIs on the I2C bus.
 */
#if !defined(I2C_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define I2C_USE_MUTUAL_EXCLUSION    TRUE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if I2C_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "I2C_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES and/or CH_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  I2C_UNINIT = 0,                           /**< Not initialized.           */
  I2C_STOP = 1,                             /**< Stopped.                   */
  I2C_READY = 2,                            /**< Ready.                     */
  I2C_ACTIVE_TX = 3,                        /**< Transmitting.              */
  I2C_ACTIVE_RX = 4,                        /**< Receiving.                 */
  I2C_LOCKED = 5                            /**> Bus or driver locked.      */
} i2cstate_t;

#include "i2c_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Wrap i2cMasterTransmitTimeout function with TIME_INFINITE timeout.
 * @api
 */
#define i2cMasterTransmit(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes)       \
  (i2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes,     \
                            TIME_INFINITE))

/**
 * @brief   Wrap i2cMasterReceiveTimeout function with TIME_INFINITE timeout.
 * @api
 */
#define i2cMasterReceive(i2cp, addr, rxbuf, rxbytes)                        \
  (i2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxbytes, TIME_INFINITE))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void i2cInit(void);
  void i2cObjectInit(I2CDriver *i2cp);
  void i2cStart(I2CDriver *i2cp, const I2CConfig *config);
  void i2cStop(I2CDriver *i2cp);
  i2cflags_t i2cGetErrors(I2CDriver *i2cp);
  msg_t i2cMasterTransmitTimeout(I2CDriver *i2cp,
                                 i2caddr_t addr,
                                 const uint8_t *txbuf, size_t txbytes,
                                 uint8_t *rxbuf, size_t rxbytes,
                                 systime_t timeout);
  msg_t i2cMasterReceiveTimeout(I2CDriver *i2cp,
                                i2caddr_t addr,
                                uint8_t *rxbuf, size_t rxbytes,
                                systime_t timeout);
#if I2C_USE_MUTUAL_EXCLUSION
  void i2cAcquireBus(I2CDriver *i2cp);
  void i2cReleaseBus(I2CDriver *i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C */

#endif /* _I2C_H_ */

/** @} */
