/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    LPC122x/serial_lld.c
 * @brief   LPC122x low level serial driver code.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if LPC122x_SERIAL_USE_UART0 || defined(__DOXYGEN__)
/** @brief UART0 serial driver identifier.*/
SerialDriver SD1;
#endif

#if LPC122x_SERIAL_USE_UART1 || defined(__DOXYGEN__)
/** @brief UART0 serial driver identifier.*/
SerialDriver SD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig default_config = {
  SERIAL_DEFAULT_BITRATE,
  LCR_WL8 | LCR_STOP1 | LCR_NOPARITY,
  FCR_TRIGGER0
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   UART initialization.
 *
 * @param[in] sdp       communication channel associated to the UART
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void uart_init(SerialDriver *sdp, const SerialConfig *config) {
  LPC_UART0_Type *u = sdp->uart;
  uint32_t upclk;
#if LPC122x_SERIAL_USE_UART0
    if (&SD1 == sdp) {
      upclk = LPC122x_SERIAL_UART0_PCLK;
    }
#endif
#if LPC122x_SERIAL_USE_UART1
    if (&SD2 == sdp) {
      upclk = LPC122x_SERIAL_UART1_PCLK;
    }
#endif
  uint32_t div = upclk / (config->sc_speed << 4);
  u->LCR = config->sc_lcr | LCR_DLAB;
  u->DLL = div;
  u->DLM = div >> 8;
  u->LCR = config->sc_lcr;
  u->FCR = FCR_ENABLE | FCR_RXRESET | FCR_TXRESET | config->sc_fcr;
  u->ACR = 0;
  u->FDR = 0x10;
  u->TER = TER_ENABLE;
  u->IER = IER_RBR | IER_STATUS;
}

/**
 * @brief   UART de-initialization.
 *
 * @param[in] u         pointer to an UART I/O block
 */
static void uart_deinit(LPC_UART0_Type *u) {

  u->LCR = LCR_DLAB;
  u->DLL = 1;
  u->DLM = 0;
  u->LCR = 0;
  u->FDR = 0x10;
  u->IER = 0;
  u->FCR = FCR_RXRESET | FCR_TXRESET;
  u->ACR = 0;
  u->TER = TER_ENABLE;
}

/**
 * @brief   Error handling routine.
 *
 * @param[in] sdp       communication channel associated to the UART
 * @param[in] err       UART LSR register value
 */
static void set_error(SerialDriver *sdp, IOREG32 err) {
  flagsmask_t sts = 0;

  if (err & LSR_OVERRUN)
    sts |= SD_OVERRUN_ERROR;
  if (err & LSR_PARITY)
    sts |= SD_PARITY_ERROR;
  if (err & LSR_FRAMING)
    sts |= SD_FRAMING_ERROR;
  if (err & LSR_BREAK)
    sts |= SD_BREAK_DETECTED;
  chSysLockFromIsr();
  chnAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

/**
 * @brief   Common IRQ handler.
 * @note    Tries hard to clear all the pending interrupt sources, we don't
 *          want to go through the whole ISR and have another interrupt soon
 *          after.
 *
 * @param[in] u         pointer to an UART I/O block
 * @param[in] sdp       communication channel associated to the UART
 */
static void serve_interrupt(SerialDriver *sdp) {
  LPC_UART0_Type *u = sdp->uart;

  while (TRUE) {
    switch (u->IIR & IIR_SRC_MASK) {
    case IIR_SRC_NONE:
      return;
    case IIR_SRC_ERROR:
      set_error(sdp, u->LSR);
      break;
    case IIR_SRC_TIMEOUT:
    case IIR_SRC_RX:
      chSysLockFromIsr();
      if (chIQIsEmptyI(&sdp->iqueue))
        chnAddFlagsI(sdp, CHN_INPUT_AVAILABLE);
      chSysUnlockFromIsr();
      while (u->LSR & LSR_RBR_FULL) {
        chSysLockFromIsr();
        if (chIQPutI(&sdp->iqueue, u->RBR) < Q_OK)
          chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
        chSysUnlockFromIsr();
      }
      break;
    case IIR_SRC_TX:
      {
        int i = LPC122x_SERIAL_FIFO_PRELOAD;
        do {
          msg_t b;

          chSysLockFromIsr();
          b = chOQGetI(&sdp->oqueue);
          chSysUnlockFromIsr();
          if (b < Q_OK) {
            u->IER &= ~IER_THRE;
            chSysLockFromIsr();
            chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
            chSysUnlockFromIsr();
            break;
          }
          u->THR = b;
        } while (--i);
      }
      break;
    default:
      (void) u->THR;
      (void) u->RBR;
    }
  }
}

/**
 * @brief   Attempts a TX FIFO preload.
 */
static void preload(SerialDriver *sdp) {
  LPC_UART0_Type *u = sdp->uart;

  if (u->LSR & LSR_THRE) {
    int i = LPC122x_SERIAL_FIFO_PRELOAD;
    do {
      msg_t b = chOQGetI(&sdp->oqueue);
      if (b < Q_OK) {
        chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
        return;
      }
      u->THR = b;
    } while (--i);
  }
  u->IER |= IER_THRE;
}

/**
 * @brief   Driver SD1 output notification.
 */
#if LPC122x_SERIAL_USE_UART0 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  preload(&SD1);
}
#endif

/**
 * @brief   Driver SD2 output notification.
 */
#if LPC122x_SERIAL_USE_UART1 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  preload(&SD2);
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   UART0 IRQ handler.
 *
 * @isr
 */
#if LPC122x_SERIAL_USE_UART0 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(Vector88) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART0 IRQ handler.
 *
 * @isr
 */
#if LPC122x_SERIAL_USE_UART1 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(Vector8C) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {

#if LPC122x_SERIAL_USE_UART0
  sdObjectInit(&SD1, NULL, notify1);
  SD1.uart = LPC_UART0;
#if LLPC122x_SERIAL_RXD0_SELECTOR == RXD0_IS_PIO0_1
  LPC_IOCON->PIO0_1 = 0x82;                 /* RDX0 without resistors.       */
#else /* LPC122x_SERIAL_RXD0_SELECTOR == RXD1_IS_PIO2_1 */
  LPC_IOCON->PIO2_1  = 0x84;                /* RXD0 without resistors.      */
#endif
#if LLPC122x_SERIAL_TXD0_SELECTOR == TXD0_IS_PIO0_8
  LPC_IOCON->PIO0_2 = 0x82;                 /* TDX0 without resistors.       */
#else /* LPC122x_SERIAL_TXD0_SELECTOR == TXD0_IS_PIO2_2 */
  LPC_IOCON->PIO2_2  = 0x84;                /* TXD0 without resistors.      */
#endif
#endif

#if LPC122x_SERIAL_USE_UART1
  sdObjectInit(&SD2, NULL, notify1);
  SD2.uart = (LPC_UART0_Type *) LPC_UART1;
#if LLPC122x_SERIAL_RXD1_SELECTOR == RXD1_IS_PIO0_8
  LPC_IOCON->PIO0_8 = 0x82;          /* RXD1 without resistors.      */
#elif LPC122x_SERIAL_RXD1_SELECTOR == RXD1_IS_PIO2_11
  LPC_IOCON->PIO2_11 = 0x85;                /* RXD1 without resistors.      */
#else /* LPC122x_SERIAL_RXD1_SELECTOR == RXD1_IS_PIO2_12 */
  LPC_IOCON->PIO2_12  = 0x83;                /* RXD1 without resistors.      */
#endif
#if LLPC122x_SERIAL_TXD1_SELECTOR == TXD1_IS_PIO0_8
  LPC_IOCON->PIO0_9 = 0x82;          /* TXD1 without resistors.      */
#elif LPC122x_SERIAL_TXD1_SELECTOR == TXD1_IS_PIO2_11
  LPC_IOCON->PIO2_10 = 0x85;                /* TXD1 without resistors.      */
#else /* LPC122x_SERIAL_TXD1_SELECTOR == TXD1_IS_PIO2_12 */
  LPC_IOCON->PIO2_13  = 0x83;                /* TXD1 without resistors.      */
#endif
#endif
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @notapi
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {

  if (config == NULL)
    config = &default_config;

  if (sdp->state == SD_STOP) {
#if LPC122x_SERIAL_USE_UART0
    if (&SD1 == sdp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);
      LPC_SYSCON->UART0CLKDIV = LPC122x_SERIAL_UART0CLKDIV;
      nvicEnableVector(UART0_IRQn,
                       CORTEX_PRIORITY_MASK(LPC122x_SERIAL_UART0_IRQ_PRIORITY));
    }
#endif
#if LPC122x_SERIAL_USE_UART1
    if (&SD2 == sdp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 13);
      LPC_SYSCON->UART1CLKDIV = LPC122x_SERIAL_UART1CLKDIV;
      nvicEnableVector(UART1_IRQn,
                       CORTEX_PRIORITY_MASK(LPC122x_SERIAL_UART1_IRQ_PRIORITY));
    }
#endif
  }
  uart_init(sdp, config);
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the UART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 *
 * @notapi
 */
void sd_lld_stop(SerialDriver *sdp) {

  if (sdp->state == SD_READY) {
    uart_deinit(sdp->uart);
#if LPC122x_SERIAL_USE_UART0
    if (&SD1 == sdp) {
      LPC_SYSCON->UART0CLKDIV = 0;
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 12);
      nvicDisableVector(UART0_IRQn);
      return;
    }
#endif
#if LPC122x_SERIAL_USE_UART1
    if (&SD2 == sdp) {
      LPC_SYSCON->UART1CLKDIV = 0;
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 13);
      nvicDisableVector(UART1_IRQn);
      return;
    }
#endif
  }
}

#endif /* HAL_USE_SERIAL */

/** @} */
