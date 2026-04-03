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
 * @file    serial_usb.c
 * @brief   Serial over USB Driver code.
 *
 * @addtogroup SERIAL_USB
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*
 * Current Line Coding.
 */
static cdc_linecoding_t linecoding = {
  {0x00, 0x96, 0x00, 0x00},             /* 38400.                           */
  LC_STOP_1, LC_PARITY_NONE, 8
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*
 * Interface implementation.
 */

static size_t write(void *ip, const uint8_t *bp, size_t n) {

  return chOQWriteTimeout(&((SerialUSBDriver *)ip)->oqueue, bp,
                          n, TIME_INFINITE);
}

static size_t read(void *ip, uint8_t *bp, size_t n) {

  return chIQReadTimeout(&((SerialUSBDriver *)ip)->iqueue, bp,
                         n, TIME_INFINITE);
}

static msg_t put(void *ip, uint8_t b) {

  return chOQPutTimeout(&((SerialUSBDriver *)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t get(void *ip) {

  return chIQGetTimeout(&((SerialUSBDriver *)ip)->iqueue, TIME_INFINITE);
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout) {

  return chOQPutTimeout(&((SerialUSBDriver *)ip)->oqueue, b, timeout);
}

static msg_t gett(void *ip, systime_t timeout) {

  return chIQGetTimeout(&((SerialUSBDriver *)ip)->iqueue, timeout);
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t time) {

  return chOQWriteTimeout(&((SerialUSBDriver *)ip)->oqueue, bp, n, time);
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time) {

  return chIQReadTimeout(&((SerialUSBDriver *)ip)->iqueue, bp, n, time);
}

static const struct SerialUSBDriverVMT vmt = {
  write, read, put, get,
  putt, gett, writet, readt
};

/**
 * @brief   Notification of data removed from the input queue.
 */
static void inotify(GenericQueue *qp) {
  size_t n, maxsize;
  SerialUSBDriver *sdup = chQGetLink(qp);

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(sdup->config->usbp) != USB_ACTIVE) ||
      (sdup->state != SDU_READY))
    return;

  /* If there is in the queue enough space to hold at least one packet and
     a transaction is not yet started then a new transaction is started for
     the available space.*/
  maxsize = sdup->config->usbp->epc[sdup->config->bulk_out]->out_maxsize;
  if (!usbGetReceiveStatusI(sdup->config->usbp, sdup->config->bulk_out) &&
      ((n = chIQGetEmptyI(&sdup->iqueue)) >= maxsize)) {
    chSysUnlock();

    n = (n / maxsize) * maxsize;
    usbPrepareQueuedReceive(sdup->config->usbp,
                            sdup->config->bulk_out,
                            &sdup->iqueue, n);

    chSysLock();
    usbStartReceiveI(sdup->config->usbp, sdup->config->bulk_out);
  }
}

/**
 * @brief   Notification of data inserted into the output queue.
 */
static void onotify(GenericQueue *qp) {
  size_t n;
  SerialUSBDriver *sdup = chQGetLink(qp);

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(sdup->config->usbp) != USB_ACTIVE) ||
      (sdup->state != SDU_READY))
    return;

  /* If there is not an ongoing transaction and the output queue contains
     data then a new transaction is started.*/
  if (!usbGetTransmitStatusI(sdup->config->usbp, sdup->config->bulk_in) &&
      ((n = chOQGetFullI(&sdup->oqueue)) > 0)) {
    chSysUnlock();

    usbPrepareQueuedTransmit(sdup->config->usbp,
                             sdup->config->bulk_in,
                             &sdup->oqueue, n);

    chSysLock();
    usbStartTransmitI(sdup->config->usbp, sdup->config->bulk_in);
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Serial Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void sduInit(void) {
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sdup     pointer to a @p SerialUSBDriver structure
 *
 * @init
 */
void sduObjectInit(SerialUSBDriver *sdup) {

  sdup->vmt = &vmt;
  chEvtInit(&sdup->event);
  sdup->state = SDU_STOP;
  chIQInit(&sdup->iqueue, sdup->ib, SERIAL_USB_BUFFERS_SIZE, inotify, sdup);
  chOQInit(&sdup->oqueue, sdup->ob, SERIAL_USB_BUFFERS_SIZE, onotify, sdup);
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 * @param[in] config    the serial over USB driver configuration
 *
 * @api
 */
void sduStart(SerialUSBDriver *sdup, const SerialUSBConfig *config) {
  USBDriver *usbp = config->usbp;

  chDbgCheck(sdup != NULL, "sduStart");

  chSysLock();
  chDbgAssert((sdup->state == SDU_STOP) || (sdup->state == SDU_READY),
              "sduStart(), #1",
              "invalid state");
  usbp->in_params[config->bulk_in - 1]   = sdup;
  usbp->out_params[config->bulk_out - 1] = sdup;
  usbp->in_params[config->int_in - 1]    = sdup;
  sdup->config = config;
  sdup->state = SDU_READY;
  chSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @api
 */
void sduStop(SerialUSBDriver *sdup) {
  USBDriver *usbp = sdup->config->usbp;

  chDbgCheck(sdup != NULL, "sdStop");

  chSysLock();

  chDbgAssert((sdup->state == SDU_STOP) || (sdup->state == SDU_READY),
              "sduStop(), #1",
              "invalid state");

  /* Driver in stopped state.*/
  usbp->in_params[sdup->config->bulk_in - 1]   = NULL;
  usbp->out_params[sdup->config->bulk_out - 1] = NULL;
  usbp->in_params[sdup->config->int_in - 1]    = NULL;
  sdup->state = SDU_STOP;

  /* Queues reset in order to signal the driver stop to the application.*/
  chnAddFlagsI(sdup, CHN_DISCONNECTED);
  chIQResetI(&sdup->iqueue);
  chOQResetI(&sdup->oqueue);
  chSchRescheduleS();

  chSysUnlock();
}

/**
 * @brief   USB device configured handler.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @iclass
 */
void sduConfigureHookI(SerialUSBDriver *sdup) {
  USBDriver *usbp = sdup->config->usbp;

  chIQResetI(&sdup->iqueue);
  chOQResetI(&sdup->oqueue);
  chnAddFlagsI(sdup, CHN_CONNECTED);

  /* Starts the first OUT transaction immediately.*/
  usbPrepareQueuedReceive(usbp, sdup->config->bulk_out, &sdup->iqueue,
                          usbp->epc[sdup->config->bulk_out]->out_maxsize);
  usbStartReceiveI(usbp, sdup->config->bulk_out);
}

/**
 * @brief   Default requests hook.
 * @details Applications wanting to use the Serial over USB driver can use
 *          this function as requests hook in the USB configuration.
 *          The following requests are emulated:
 *          - CDC_GET_LINE_CODING.
 *          - CDC_SET_LINE_CODING.
 *          - CDC_SET_CONTROL_LINE_STATE.
 *          .
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The hook status.
 * @retval TRUE         Message handled internally.
 * @retval FALSE        Message not handled.
 */
bool_t sduRequestsHook(USBDriver *usbp) {

  if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
    switch (usbp->setup[1]) {
    case CDC_GET_LINE_CODING:
      usbSetupTransfer(usbp, (uint8_t *)&linecoding, sizeof(linecoding), NULL);
      return TRUE;
    case CDC_SET_LINE_CODING:
      usbSetupTransfer(usbp, (uint8_t *)&linecoding, sizeof(linecoding), NULL);
      return TRUE;
    case CDC_SET_CONTROL_LINE_STATE:
      /* Nothing to do, there are no control lines.*/
      usbSetupTransfer(usbp, NULL, 0, NULL);
      return TRUE;
    default:
      return FALSE;
    }
  }
  return FALSE;
}

/**
 * @brief   Default data transmitted callback.
 * @details The application must use this function as callback for the IN
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void sduDataTransmitted(USBDriver *usbp, usbep_t ep) {
  size_t n;
  SerialUSBDriver *sdup = usbp->in_params[ep - 1];

  if (sdup == NULL)
    return;

  chSysLockFromIsr();
  chnAddFlagsI(sdup, CHN_OUTPUT_EMPTY);

  if ((n = chOQGetFullI(&sdup->oqueue)) > 0) {
    /* The endpoint cannot be busy, we are in the context of the callback,
       so it is safe to transmit without a check.*/
    chSysUnlockFromIsr();

    usbPrepareQueuedTransmit(usbp, ep, &sdup->oqueue, n);

    chSysLockFromIsr();
    usbStartTransmitI(usbp, ep);
  }
  else if ((usbp->epc[ep]->in_state->txsize > 0) &&
           !(usbp->epc[ep]->in_state->txsize &
             (usbp->epc[ep]->in_maxsize - 1))) {
    /* Transmit zero sized packet in case the last one has maximum allowed
       size. Otherwise the recipient may expect more data coming soon and
       not return buffered data to app. See section 5.8.3 Bulk Transfer
       Packet Size Constraints of the USB Specification document.*/
    chSysUnlockFromIsr();

    usbPrepareQueuedTransmit(usbp, ep, &sdup->oqueue, 0);

    chSysLockFromIsr();
    usbStartTransmitI(usbp, ep);
  }

  chSysUnlockFromIsr();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the OUT
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void sduDataReceived(USBDriver *usbp, usbep_t ep) {
  size_t n, maxsize;
  SerialUSBDriver *sdup = usbp->out_params[ep - 1];

  if (sdup == NULL)
    return;

  chSysLockFromIsr();
  chnAddFlagsI(sdup, CHN_INPUT_AVAILABLE);

  /* Writes to the input queue can only happen when there is enough space
     to hold at least one packet.*/
  maxsize = usbp->epc[ep]->out_maxsize;
  if ((n = chIQGetEmptyI(&sdup->iqueue)) >= maxsize) {
    /* The endpoint cannot be busy, we are in the context of the callback,
       so a packet is in the buffer for sure.*/
    chSysUnlockFromIsr();

    n = (n / maxsize) * maxsize;
    usbPrepareQueuedReceive(usbp, ep, &sdup->iqueue, n);

    chSysLockFromIsr();
    usbStartReceiveI(usbp, ep);
  }

  chSysUnlockFromIsr();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the IN
 *          interrupt endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void sduInterruptTransmitted(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

#endif /* HAL_USE_SERIAL */

/** @} */
