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
 * @file    can.h
 * @brief   CAN Driver macros and structures.
 *
 * @addtogroup CAN
 * @{
 */

#ifndef _CAN_H_
#define _CAN_H_

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    CAN status flags
 * @{
 */
/**
 * @brief   Errors rate warning.
 */
#define CAN_LIMIT_WARNING           1
/**
 * @brief   Errors rate error.
 */
#define CAN_LIMIT_ERROR             2
/**
 * @brief   Bus off condition reached.
 */
#define CAN_BUS_OFF_ERROR           4
/**
 * @brief   Framing error of some kind on the CAN bus.
 */
#define CAN_FRAMING_ERROR           8
/**
 * @brief   Overflow in receive queue.
 */
#define CAN_OVERFLOW_ERROR          16
/** @} */

/**
 * @brief   Special mailbox identifier.
 */
#define CAN_ANY_MAILBOX             0

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    CAN configuration options
 * @{
 */
/**
 * @brief   Sleep mode related APIs inclusion switch.
 * @details This option can only be enabled if the CAN implementation supports
 *          the sleep mode, see the macro @p CAN_SUPPORTS_SLEEP exported by
 *          the underlying implementation.
 */
#if !defined(CAN_USE_SLEEP_MODE) || defined(__DOXYGEN__)
#define CAN_USE_SLEEP_MODE          TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !CH_USE_SEMAPHORES || !CH_USE_EVENTS
#error "CAN driver requires CH_USE_SEMAPHORES and CH_USE_EVENTS"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  CAN_UNINIT = 0,                           /**< Not initialized.           */
  CAN_STOP = 1,                             /**< Stopped.                   */
  CAN_STARTING = 2,                         /**< Starting.                  */
  CAN_READY = 3,                            /**< Ready.                     */
  CAN_SLEEP = 4                             /**< Sleep state.               */
} canstate_t;

#include "can_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Converts a mailbox index to a bit mask.
 */
#define CAN_MAILBOX_TO_MASK(mbx) (1 << ((mbx) - 1))
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void canInit(void);
  void canObjectInit(CANDriver *canp);
  void canStart(CANDriver *canp, const CANConfig *config);
  void canStop(CANDriver *canp);
  msg_t canTransmit(CANDriver *canp,
                    canmbx_t mailbox,
                    const CANTxFrame *ctfp,
                    systime_t timeout);
  msg_t canReceive(CANDriver *canp,
                   canmbx_t mailbox,
                   CANRxFrame *crfp,
                   systime_t timeout);
#if CAN_USE_SLEEP_MODE
  void canSleep(CANDriver *canp);
  void canWakeup(CANDriver *canp);
#endif /* CAN_USE_SLEEP_MODE */
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CAN */

#endif /* _CAN_H_ */

/** @} */
