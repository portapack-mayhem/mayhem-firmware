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
 * @file    mac.c
 * @brief   MAC Driver code.
 *
 * @addtogroup MAC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_MAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if MAC_USE_ZERO_COPY && !MAC_SUPPORTS_ZERO_COPY
#error "MAC_USE_ZERO_COPY not supported by this implementation"
#endif

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
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   MAC Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void macInit(void) {

  mac_lld_init();
}

/**
 * @brief   Initialize the standard part of a @p MACDriver structure.
 *
 * @param[out] macp     pointer to the @p MACDriver object
 *
 * @init
 */
void macObjectInit(MACDriver *macp) {

  macp->state  = MAC_STOP;
  macp->config = NULL;
  chSemInit(&macp->tdsem, 0);
  chSemInit(&macp->rdsem, 0);
#if MAC_USE_EVENTS
  chEvtInit(&macp->rdevent);
#endif
}

/**
 * @brief   Configures and activates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] config    pointer to the @p MACConfig object
 *
 * @api
 */
void macStart(MACDriver *macp, const MACConfig *config) {

  chDbgCheck((macp != NULL) && (config != NULL), "macStart");

  chSysLock();
  chDbgAssert(macp->state == MAC_STOP,
              "macStart(), #1", "invalid state");
  macp->config = config;
  mac_lld_start(macp);
  macp->state = MAC_ACTIVE;
  chSysUnlock();
}

/**
 * @brief   Deactivates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @api
 */
void macStop(MACDriver *macp) {

  chDbgCheck(macp != NULL, "macStop");

  chSysLock();
  chDbgAssert((macp->state == MAC_STOP) || (macp->state == MAC_ACTIVE),
              "macStop(), #1", "invalid state");
  mac_lld_stop(macp);
  macp->state = MAC_STOP;
  chSysUnlock();
}

/**
 * @brief   Allocates a transmission descriptor.
 * @details One of the available transmission descriptors is locked and
 *          returned. If a descriptor is not currently available then the
 *          invoking thread is queued until one is freed.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] tdp      pointer to a @p MACTransmitDescriptor structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       the descriptor was obtained.
 * @retval RDY_TIMEOUT  the operation timed out, descriptor not initialized.
 *
 * @api
 */
msg_t macWaitTransmitDescriptor(MACDriver *macp,
                                MACTransmitDescriptor *tdp,
                                systime_t time) {
  msg_t msg;
  systime_t now;

  chDbgCheck((macp != NULL) && (tdp != NULL), "macWaitTransmitDescriptor");
  chDbgAssert(macp->state == MAC_ACTIVE, "macWaitTransmitDescriptor(), #1",
              "not active");

  while (((msg = mac_lld_get_transmit_descriptor(macp, tdp)) != RDY_OK) &&
         (time > 0)) {
    chSysLock();
    now = chTimeNow();
    if ((msg = chSemWaitTimeoutS(&macp->tdsem, time)) == RDY_TIMEOUT) {
      chSysUnlock();
      break;
    }
    if (time != TIME_INFINITE)
      time -= (chTimeNow() - now);
    chSysUnlock();
  }
  return msg;
}

/**
 * @brief   Releases a transmit descriptor and starts the transmission of the
 *          enqueued data as a single frame.
 *
 * @param[in] tdp       the pointer to the @p MACTransmitDescriptor structure
 *
 * @api
 */
void macReleaseTransmitDescriptor(MACTransmitDescriptor *tdp) {

  chDbgCheck((tdp != NULL), "macReleaseTransmitDescriptor");

  mac_lld_release_transmit_descriptor(tdp);
}

/**
 * @brief   Waits for a received frame.
 * @details Stops until a frame is received and buffered. If a frame is
 *          not immediately available then the invoking thread is queued
 *          until one is received.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] rdp      pointer to a @p MACReceiveDescriptor structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       the descriptor was obtained.
 * @retval RDY_TIMEOUT  the operation timed out, descriptor not initialized.
 *
 * @api
 */
msg_t macWaitReceiveDescriptor(MACDriver *macp,
                               MACReceiveDescriptor *rdp,
                               systime_t time) {
  msg_t msg;
  systime_t now;

  chDbgCheck((macp != NULL) && (rdp != NULL), "macWaitReceiveDescriptor");
  chDbgAssert(macp->state == MAC_ACTIVE, "macWaitReceiveDescriptor(), #1",
              "not active");

  while (((msg = mac_lld_get_receive_descriptor(macp, rdp)) != RDY_OK) &&
         (time > 0)) {
    chSysLock();
    now = chTimeNow();
    if ((msg = chSemWaitTimeoutS(&macp->rdsem, time)) == RDY_TIMEOUT) {
      chSysUnlock();
      break;
    }
    if (time != TIME_INFINITE)
      time -= (chTimeNow() - now);
    chSysUnlock();
  }
  return msg;
}

/**
 * @brief   Releases a receive descriptor.
 * @details The descriptor and its buffer are made available for more incoming
 *          frames.
 *
 * @param[in] rdp       the pointer to the @p MACReceiveDescriptor structure
 *
 * @api
 */
void macReleaseReceiveDescriptor(MACReceiveDescriptor *rdp) {

  chDbgCheck((rdp != NULL), "macReleaseReceiveDescriptor");

  mac_lld_release_receive_descriptor(rdp);
}

/**
 * @brief   Updates and returns the link status.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @return              The link status.
 * @retval TRUE         if the link is active.
 * @retval FALSE        if the link is down.
 *
 * @api
 */
bool_t macPollLinkStatus(MACDriver *macp) {

  chDbgCheck((macp != NULL), "macPollLinkStatus");
  chDbgAssert(macp->state == MAC_ACTIVE, "macPollLinkStatus(), #1",
              "not active");

  return mac_lld_poll_link_status(macp);
}

#endif /* HAL_USE_MAC */

/** @} */
