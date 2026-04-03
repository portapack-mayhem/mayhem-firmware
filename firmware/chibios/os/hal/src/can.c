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
 * @file    can.c
 * @brief   CAN Driver code.
 *
 * @addtogroup CAN
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   CAN Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void canInit(void) {

  can_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p CANDriver structure.
 *
 * @param[out] canp     pointer to the @p CANDriver object
 *
 * @init
 */
void canObjectInit(CANDriver *canp) {

  canp->state    = CAN_STOP;
  canp->config   = NULL;
  chSemInit(&canp->txsem, 0);
  chSemInit(&canp->rxsem, 0);
  chEvtInit(&canp->rxfull_event);
  chEvtInit(&canp->txempty_event);
  chEvtInit(&canp->error_event);
#if CAN_USE_SLEEP_MODE
  chEvtInit(&canp->sleep_event);
  chEvtInit(&canp->wakeup_event);
#endif /* CAN_USE_SLEEP_MODE */
}

/**
 * @brief   Configures and activates the CAN peripheral.
 * @note    Activating the CAN bus can be a slow operation this this function
 *          is not atomic, it waits internally for the initialization to
 *          complete.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] config    pointer to the @p CANConfig object. Depending on
 *                      the implementation the value can be @p NULL.
 *
 * @api
 */
void canStart(CANDriver *canp, const CANConfig *config) {

  chDbgCheck(canp != NULL, "canStart");

  chSysLock();
  chDbgAssert((canp->state == CAN_STOP) ||
              (canp->state == CAN_STARTING) ||
              (canp->state == CAN_READY),
              "canStart(), #1", "invalid state");
  while (canp->state == CAN_STARTING)
    chThdSleepS(1);
  if (canp->state == CAN_STOP) {
    canp->config = config;
    can_lld_start(canp);
    canp->state = CAN_READY;
  }
  chSysUnlock();
}

/**
 * @brief   Deactivates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @api
 */
void canStop(CANDriver *canp) {

  chDbgCheck(canp != NULL, "canStop");

  chSysLock();
  chDbgAssert((canp->state == CAN_STOP) || (canp->state == CAN_READY),
              "canStop(), #1", "invalid state");
  can_lld_stop(canp);
  canp->state  = CAN_STOP;
  chSemResetI(&canp->rxsem, 0);
  chSemResetI(&canp->txsem, 0);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Can frame transmission.
 * @details The specified frame is queued for transmission, if the hardware
 *          queue is full then the invoking thread is queued.
 * @note    Trying to transmit while in sleep mode simply enqueues the thread.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[in] ctfp      pointer to the CAN frame to be transmitted
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation result.
 * @retval RDY_OK       the frame has been queued for transmission.
 * @retval RDY_TIMEOUT  The operation has timed out.
 * @retval RDY_RESET    The driver has been stopped while waiting.
 *
 * @api
 */
msg_t canTransmit(CANDriver *canp,
                  canmbx_t mailbox,
                  const CANTxFrame *ctfp,
                  systime_t timeout) {

  chDbgCheck((canp != NULL) && (ctfp != NULL) && (mailbox <= CAN_TX_MAILBOXES),
             "canTransmit");

  chSysLock();
  chDbgAssert((canp->state == CAN_READY) || (canp->state == CAN_SLEEP),
              "canTransmit(), #1", "invalid state");
  while ((canp->state == CAN_SLEEP) || !can_lld_is_tx_empty(canp, mailbox)) {
    msg_t msg = chSemWaitTimeoutS(&canp->txsem, timeout);
    if (msg != RDY_OK) {
      chSysUnlock();
      return msg;
    }
  }
  can_lld_transmit(canp, mailbox, ctfp);
  chSysUnlock();
  return RDY_OK;
}

/**
 * @brief   Can frame receive.
 * @details The function waits until a frame is received.
 * @note    Trying to receive while in sleep mode simply enqueues the thread.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[out] crfp     pointer to the buffer where the CAN frame is copied
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout (useful in an
 *                        event driven scenario where a thread never blocks
 *                        for I/O).
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation result.
 * @retval RDY_OK       a frame has been received and placed in the buffer.
 * @retval RDY_TIMEOUT  The operation has timed out.
 * @retval RDY_RESET    The driver has been stopped while waiting.
 *
 * @api
 */
msg_t canReceive(CANDriver *canp,
                 canmbx_t mailbox,
                 CANRxFrame *crfp,
                 systime_t timeout) {

  chDbgCheck((canp != NULL) && (crfp != NULL) && (mailbox < CAN_RX_MAILBOXES),
             "canReceive");

  chSysLock();
  chDbgAssert((canp->state == CAN_READY) || (canp->state == CAN_SLEEP),
              "canReceive(), #1", "invalid state");
  while ((canp->state == CAN_SLEEP) || !can_lld_is_rx_nonempty(canp, mailbox)) {
    msg_t msg = chSemWaitTimeoutS(&canp->rxsem, timeout);
    if (msg != RDY_OK) {
      chSysUnlock();
      return msg;
    }
  }
  can_lld_receive(canp, mailbox, crfp);
  chSysUnlock();
  return RDY_OK;
}

#if CAN_USE_SLEEP_MODE || defined(__DOXYGEN__)
/**
 * @brief   Enters the sleep mode.
 * @details This function puts the CAN driver in sleep mode and broadcasts
 *          the @p sleep_event event source.
 * @pre     In order to use this function the option @p CAN_USE_SLEEP_MODE must
 *          be enabled and the @p CAN_SUPPORTS_SLEEP mode must be supported
 *          by the low level driver.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @api
 */
void canSleep(CANDriver *canp) {

  chDbgCheck(canp != NULL, "canSleep");

  chSysLock();
  chDbgAssert((canp->state == CAN_READY) || (canp->state == CAN_SLEEP),
              "canSleep(), #1", "invalid state");
  if (canp->state == CAN_READY) {
    can_lld_sleep(canp);
    canp->state = CAN_SLEEP;
    chEvtBroadcastI(&canp->sleep_event);
    chSchRescheduleS();
  }
  chSysUnlock();
}

/**
 * @brief   Enforces leaving the sleep mode.
 * @note    The sleep mode is supposed to be usually exited automatically by
 *          an hardware event.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 */
void canWakeup(CANDriver *canp) {

  chDbgCheck(canp != NULL, "canWakeup");

  chSysLock();
  chDbgAssert((canp->state == CAN_READY) || (canp->state == CAN_SLEEP),
              "canWakeup(), #1", "invalid state");
  if (canp->state == CAN_SLEEP) {
    can_lld_wakeup(canp);
    canp->state = CAN_READY;
    chEvtBroadcastI(&canp->wakeup_event);
    chSchRescheduleS();
  }
  chSysUnlock();
}
#endif /* CAN_USE_SLEEP_MODE */

#endif /* HAL_USE_CAN */

/** @} */
