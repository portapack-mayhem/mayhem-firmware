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
 * @file    uart.c
 * @brief   UART Driver code.
 *
 * @addtogroup UART
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_UART || defined(__DOXYGEN__)

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
 * @brief   UART Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void uartInit(void) {

  uart_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p UARTDriver structure.
 *
 * @param[out] uartp    pointer to the @p UARTDriver object
 *
 * @init
 */
void uartObjectInit(UARTDriver *uartp) {

  uartp->state   = UART_STOP;
  uartp->txstate = UART_TX_IDLE;
  uartp->rxstate = UART_RX_IDLE;
  uartp->config  = NULL;
  /* Optional, user-defined initializer.*/
#if defined(UART_DRIVER_EXT_INIT_HOOK)
  UART_DRIVER_EXT_INIT_HOOK(uartp);
#endif
}

/**
 * @brief   Configures and activates the UART peripheral.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] config    pointer to the @p UARTConfig object
 *
 * @api
 */
void uartStart(UARTDriver *uartp, const UARTConfig *config) {

  chDbgCheck((uartp != NULL) && (config != NULL), "uartStart");

  chSysLock();
  chDbgAssert((uartp->state == UART_STOP) || (uartp->state == UART_READY),
              "uartStart(), #1", "invalid state");

  uartp->config = config;
  uart_lld_start(uartp);
  uartp->state = UART_READY;
  chSysUnlock();
}

/**
 * @brief   Deactivates the UART peripheral.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @api
 */
void uartStop(UARTDriver *uartp) {

  chDbgCheck(uartp != NULL, "uartStop");

  chSysLock();
  chDbgAssert((uartp->state == UART_STOP) || (uartp->state == UART_READY),
              "uartStop(), #1", "invalid state");

  uart_lld_stop(uartp);
  uartp->state = UART_STOP;
  uartp->txstate = UART_TX_IDLE;
  uartp->rxstate = UART_RX_IDLE;
  chSysUnlock();
}

/**
 * @brief   Starts a transmission on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @api
 */
void uartStartSend(UARTDriver *uartp, size_t n, const void *txbuf) {

  chDbgCheck((uartp != NULL) && (n > 0) && (txbuf != NULL),
             "uartStartSend");
             
  chSysLock();
  chDbgAssert(uartp->state == UART_READY,
              "uartStartSend(), #1", "is active");
  chDbgAssert(uartp->txstate != UART_TX_ACTIVE,
              "uartStartSend(), #2", "tx active");

  uart_lld_start_send(uartp, n, txbuf);
  uartp->txstate = UART_TX_ACTIVE;
  chSysUnlock();
}

/**
 * @brief   Starts a transmission on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 * @note    This function has to be invoked from a lock zone.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @iclass
 */
void uartStartSendI(UARTDriver *uartp, size_t n, const void *txbuf) {

  chDbgCheckClassI();
  chDbgCheck((uartp != NULL) && (n > 0) && (txbuf != NULL),
             "uartStartSendI");
  chDbgAssert(uartp->state == UART_READY,
              "uartStartSendI(), #1", "is active");
  chDbgAssert(uartp->txstate != UART_TX_ACTIVE,
              "uartStartSendI(), #2", "tx active");

  uart_lld_start_send(uartp, n, txbuf);
  uartp->txstate = UART_TX_ACTIVE;
}

/**
 * @brief   Stops any ongoing transmission.
 * @note    Stopping a transmission also suppresses the transmission callbacks.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not transmitted by the
 *                      stopped transmit operation.
 * @retval 0            There was no transmit operation in progress.
 *
 * @api
 */
size_t uartStopSend(UARTDriver *uartp) {
  size_t n;

  chDbgCheck(uartp != NULL, "uartStopSend");

  chSysLock();
  chDbgAssert(uartp->state == UART_READY, "uartStopSend(), #1", "not active");

  if (uartp->txstate == UART_TX_ACTIVE) {
    n = uart_lld_stop_send(uartp);
    uartp->txstate = UART_TX_IDLE;
  }
  else
    n = 0;
  chSysUnlock();
  return n;
}

/**
 * @brief   Stops any ongoing transmission.
 * @note    Stopping a transmission also suppresses the transmission callbacks.
 * @note    This function has to be invoked from a lock zone.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not transmitted by the
 *                      stopped transmit operation.
 * @retval 0            There was no transmit operation in progress.
 *
 * @iclass
 */
size_t uartStopSendI(UARTDriver *uartp) {

  chDbgCheckClassI();
  chDbgCheck(uartp != NULL, "uartStopSendI");
  chDbgAssert(uartp->state == UART_READY, "uartStopSendI(), #1", "not active");

  if (uartp->txstate == UART_TX_ACTIVE) {
    size_t n = uart_lld_stop_send(uartp);
    uartp->txstate = UART_TX_IDLE;
    return n;
  }
  return 0;
}

/**
 * @brief   Starts a receive operation on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[in] rxbuf     the pointer to the receive buffer
 *
 * @api
 */
void uartStartReceive(UARTDriver *uartp, size_t n, void *rxbuf) {

  chDbgCheck((uartp != NULL) && (n > 0) && (rxbuf != NULL),
             "uartStartReceive");

  chSysLock();
  chDbgAssert(uartp->state == UART_READY,
              "uartStartReceive(), #1", "is active");
  chDbgAssert(uartp->rxstate != UART_RX_ACTIVE,
              "uartStartReceive(), #2", "rx active");

  uart_lld_start_receive(uartp, n, rxbuf);
  uartp->rxstate = UART_RX_ACTIVE;
  chSysUnlock();
}

/**
 * @brief   Starts a receive operation on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 * @note    This function has to be invoked from a lock zone.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @iclass
 */
void uartStartReceiveI(UARTDriver *uartp, size_t n, void *rxbuf) {

  chDbgCheckClassI();
  chDbgCheck((uartp != NULL) && (n > 0) && (rxbuf != NULL),
             "uartStartReceiveI");
  chDbgAssert(uartp->state == UART_READY,
              "uartStartReceiveI(), #1", "is active");
  chDbgAssert(uartp->rxstate != UART_RX_ACTIVE,
              "uartStartReceiveI(), #2", "rx active");

  uart_lld_start_receive(uartp, n, rxbuf);
  uartp->rxstate = UART_RX_ACTIVE;
}

/**
 * @brief   Stops any ongoing receive operation.
 * @note    Stopping a receive operation also suppresses the receive callbacks.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not received by the
 *                      stopped receive operation.
 * @retval 0            There was no receive operation in progress.
 *
 * @api
 */
size_t uartStopReceive(UARTDriver *uartp) {
  size_t n;

  chDbgCheck(uartp != NULL, "uartStopReceive");

  chSysLock();
  chDbgAssert(uartp->state == UART_READY,
              "uartStopReceive(), #1", "not active");

  if (uartp->rxstate == UART_RX_ACTIVE) {
    n = uart_lld_stop_receive(uartp);
    uartp->rxstate = UART_RX_IDLE;
  }
  else
    n = 0;
  chSysUnlock();
  return n;
}

/**
 * @brief   Stops any ongoing receive operation.
 * @note    Stopping a receive operation also suppresses the receive callbacks.
 * @note    This function has to be invoked from a lock zone.
 *
 * @param[in] uartp      pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not received by the
 *                      stopped receive operation.
 * @retval 0            There was no receive operation in progress.
 *
 * @iclass
 */
size_t uartStopReceiveI(UARTDriver *uartp) {

  chDbgCheckClassI();
  chDbgCheck(uartp != NULL, "uartStopReceiveI");
  chDbgAssert(uartp->state == UART_READY,
              "uartStopReceiveI(), #1", "not active");

  if (uartp->rxstate == UART_RX_ACTIVE) {
    size_t n = uart_lld_stop_receive(uartp);
    uartp->rxstate = UART_RX_IDLE;
    return n;
  }
  return 0;
}

#endif /* HAL_USE_UART */

/** @} */
