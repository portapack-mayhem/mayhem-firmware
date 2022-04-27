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
 * @file    uart.h
 * @brief   UART Driver macros and structures.
 *
 * @addtogroup UART
 * @{
 */

#ifndef _UART_H_
#define _UART_H_

#if HAL_USE_UART || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    UART status flags
 * @{
 */
#define UART_NO_ERROR           0   /**< @brief No pending conditions.      */
#define UART_PARITY_ERROR       4   /**< @brief Parity error happened.      */
#define UART_FRAMING_ERROR      8   /**< @brief Framing error happened.     */
#define UART_OVERRUN_ERROR      16  /**< @brief Overflow happened.          */
#define UART_NOISE_ERROR        32  /**< @brief Noise on the line.          */
#define UART_BREAK_DETECTED     64  /**< @brief Break detected.             */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  UART_UNINIT = 0,                  /**< Not initialized.                   */
  UART_STOP = 1,                    /**< Stopped.                           */
  UART_READY = 2                    /**< Ready.                             */
} uartstate_t;

/**
 * @brief   Transmitter state machine states.
 */
typedef enum {
  UART_TX_IDLE = 0,                 /**< Not transmitting.                  */
  UART_TX_ACTIVE = 1,               /**< Transmitting.                      */
  UART_TX_COMPLETE = 2              /**< Buffer complete.                   */
} uarttxstate_t;

/**
 * @brief   Receiver state machine states.
 */
typedef enum {
  UART_RX_IDLE = 0,                 /**< Not receiving.                     */
  UART_RX_ACTIVE = 1,               /**< Receiving.                         */
  UART_RX_COMPLETE = 2              /**< Buffer complete.                   */
} uartrxstate_t;

#include "uart_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void uartInit(void);
  void uartObjectInit(UARTDriver *uartp);
  void uartStart(UARTDriver *uartp, const UARTConfig *config);
  void uartStop(UARTDriver *uartp);
  void uartStartSend(UARTDriver *uartp, size_t n, const void *txbuf);
  void uartStartSendI(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uartStopSend(UARTDriver *uartp);
  size_t uartStopSendI(UARTDriver *uartp);
  void uartStartReceive(UARTDriver *uartp, size_t n, void *rxbuf);
  void uartStartReceiveI(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uartStopReceive(UARTDriver *uartp);
  size_t uartStopReceiveI(UARTDriver *uartp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_UART */

#endif /* _UART_H_ */

/** @} */
