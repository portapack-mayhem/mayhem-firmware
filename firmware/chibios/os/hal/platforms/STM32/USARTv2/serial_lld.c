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
 * @file    STM32/USARTv2/serial_lld.c
 * @brief   STM32 low level serial driver code.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief USART1 serial driver identifier.*/
#if STM32_SERIAL_USE_USART1 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/** @brief USART2 serial driver identifier.*/
#if STM32_SERIAL_USE_USART2 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/** @brief USART3 serial driver identifier.*/
#if STM32_SERIAL_USE_USART3 || defined(__DOXYGEN__)
SerialDriver SD3;
#endif

/** @brief UART4 serial driver identifier.*/
#if STM32_SERIAL_USE_UART4 || defined(__DOXYGEN__)
SerialDriver SD4;
#endif

/** @brief UART5 serial driver identifier.*/
#if STM32_SERIAL_USE_UART5 || defined(__DOXYGEN__)
SerialDriver SD5;
#endif

/** @brief USART6 serial driver identifier.*/
#if STM32_SERIAL_USE_USART6 || defined(__DOXYGEN__)
SerialDriver SD6;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig default_config =
{
  SERIAL_DEFAULT_BITRATE,
  0,
  USART_CR2_STOP1_BITS | USART_CR2_LINEN,
  0
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   USART initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart_init(SerialDriver *sdp, const SerialConfig *config) {
  USART_TypeDef *u = sdp->usart;

  /* Baud rate setting.*/
  u->BRR = (uint16_t)(sdp->clock / config->speed);

  /* Note that some bits are enforced.*/
  u->CR2 = config->cr2 | USART_CR2_LBDIE;
  u->CR3 = config->cr3 | USART_CR3_EIE;
  u->CR1 = config->cr1 | USART_CR1_UE | USART_CR1_PEIE |
                         USART_CR1_RXNEIE | USART_CR1_TE |
                         USART_CR1_RE;
  u->ICR = 0xFFFFFFFF;
}

/**
 * @brief   USART de-initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] u         pointer to an USART I/O block
 */
static void usart_deinit(USART_TypeDef *u) {

  u->CR1 = 0;
  u->CR2 = 0;
  u->CR3 = 0;
}

/**
 * @brief   Error handling routine.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] isr       USART ISR register value
 */
static void set_error(SerialDriver *sdp, uint32_t isr) {
  flagsmask_t sts = 0;

  if (isr & USART_ISR_ORE)
    sts |= SD_OVERRUN_ERROR;
  if (isr & USART_ISR_PE)
    sts |= SD_PARITY_ERROR;
  if (isr & USART_ISR_FE)
    sts |= SD_FRAMING_ERROR;
  if (isr & USART_ISR_NE)
    sts |= SD_NOISE_ERROR;
  chSysLockFromIsr();
  chnAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] sdp       communication channel associated to the USART
 */
static void serve_interrupt(SerialDriver *sdp) {
  USART_TypeDef *u = sdp->usart;
  uint32_t cr1 = u->CR1;
  uint32_t isr;

  /* Reading and clearing status.*/
  isr = u->ISR;
  u->ICR = isr;

  /* Error condition detection.*/
  if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE  | USART_ISR_PE))
    set_error(sdp, isr);

  /* Special case, LIN break detection.*/
  if (isr & USART_ISR_LBD) {
    chSysLockFromIsr();
    chnAddFlagsI(sdp, SD_BREAK_DETECTED);
    chSysUnlockFromIsr();
  }

  /* Data available.*/
  if (isr & USART_ISR_RXNE) {
    chSysLockFromIsr();
    sdIncomingDataI(sdp, (uint8_t)u->RDR);
    chSysUnlockFromIsr();
  }

  /* Transmission buffer empty.*/
  if ((cr1 & USART_CR1_TXEIE) && (isr & USART_ISR_TXE)) {
    msg_t b;
    chSysLockFromIsr();
    b = chOQGetI(&sdp->oqueue);
    if (b < Q_OK) {
      chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
      u->CR1 = (cr1 & ~USART_CR1_TXEIE) | USART_CR1_TCIE;
    }
    else
      u->TDR = b;
    chSysUnlockFromIsr();
  }

  /* Physical transmission end.*/
  if (isr & USART_ISR_TC) {
    chSysLockFromIsr();
    if (chOQIsEmptyI(&sdp->oqueue))
      chnAddFlagsI(sdp, CHN_TRANSMISSION_END);
    u->CR1 = cr1 & ~USART_CR1_TCIE;
    chSysUnlockFromIsr();
  }
}

#if STM32_SERIAL_USE_USART1 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  USART1->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_USE_USART2 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  USART2->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_USE_USART3 || defined(__DOXYGEN__)
static void notify3(GenericQueue *qp) {

  (void)qp;
  USART3->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_USE_UART4 || defined(__DOXYGEN__)
static void notify4(GenericQueue *qp) {

  (void)qp;
  UART4->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_USE_UART5 || defined(__DOXYGEN__)
static void notify5(GenericQueue *qp) {

  (void)qp;
  UART5->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_USE_USART6 || defined(__DOXYGEN__)
static void notify6(GenericQueue *qp) {

  (void)qp;
  USART6->CR1 |= USART_CR1_TXEIE;
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_SERIAL_USE_USART1 || defined(__DOXYGEN__)
#if !defined(STM32_USART1_HANDLER)
#error "STM32_USART1_HANDLER not defined"
#endif
/**
 * @brief   USART1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART1_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_USE_USART2 || defined(__DOXYGEN__)
#if !defined(STM32_USART2_HANDLER)
#error "STM32_USART2_HANDLER not defined"
#endif
/**
 * @brief   USART2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART2_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_USE_USART3 || defined(__DOXYGEN__)
#if !defined(STM32_USART3_HANDLER)
#error "STM32_USART3_HANDLER not defined"
#endif
/**
 * @brief   USART3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART3_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD3);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_USE_UART4 || defined(__DOXYGEN__)
#if !defined(STM32_UART4_HANDLER)
#error "STM32_UART4_HANDLER not defined"
#endif
/**
 * @brief   UART4 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_UART4_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD4);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_USE_UART5 || defined(__DOXYGEN__)
#if !defined(STM32_UART5_HANDLER)
#error "STM32_UART5_HANDLER not defined"
#endif
/**
 * @brief   UART5 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_UART5_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD5);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_USE_USART6 || defined(__DOXYGEN__)
#if !defined(STM32_USART6_HANDLER)
#error "STM32_USART6_HANDLER not defined"
#endif
/**
 * @brief   USART1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART6_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&SD6);

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

#if STM32_SERIAL_USE_USART1
  sdObjectInit(&SD1, NULL, notify1);
  SD1.usart = USART1;
  SD1.clock = STM32_USART1CLK;
#endif

#if STM32_SERIAL_USE_USART2
  sdObjectInit(&SD2, NULL, notify2);
  SD2.usart = USART2;
  SD2.clock = STM32_USART2CLK;
#endif

#if STM32_SERIAL_USE_USART3
  sdObjectInit(&SD3, NULL, notify3);
  SD3.usart = USART3;
  SD3.clock = STM32_USART3CLK;
#endif

#if STM32_SERIAL_USE_UART4
  sdObjectInit(&SD4, NULL, notify4);
  SD4.usart = UART4;
  SD4.clock = STM32_UART4CLK;
#endif

#if STM32_SERIAL_USE_UART5
  sdObjectInit(&SD5, NULL, notify5);
  SD5.usart = UART5;
  SD5.clock = STM32_UART5CLK;
#endif

#if STM32_SERIAL_USE_USART6
  sdObjectInit(&SD6, NULL, notify6);
  SD6.usart = USART6;
  SD6.clock = STM32_USART6CLK;
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
#if STM32_SERIAL_USE_USART1
    if (&SD1 == sdp) {
      rccEnableUSART1(FALSE);
      nvicEnableVector(STM32_USART1_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_USART1_PRIORITY));
    }
#endif
#if STM32_SERIAL_USE_USART2
    if (&SD2 == sdp) {
      rccEnableUSART2(FALSE);
      nvicEnableVector(STM32_USART2_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_USART2_PRIORITY));
    }
#endif
#if STM32_SERIAL_USE_USART3
    if (&SD3 == sdp) {
      rccEnableUSART3(FALSE);
      nvicEnableVector(STM32_USART3_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_USART3_PRIORITY));
    }
#endif
#if STM32_SERIAL_USE_UART4
    if (&SD4 == sdp) {
      rccEnableUART4(FALSE);
      nvicEnableVector(STM32_UART4_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_UART4_PRIORITY));
    }
#endif
#if STM32_SERIAL_USE_UART5
    if (&SD5 == sdp) {
      rccEnableUART5(FALSE);
      nvicEnableVector(STM32_UART5_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_UART5_PRIORITY));
    }
#endif
#if STM32_SERIAL_USE_USART6
    if (&SD6 == sdp) {
      rccEnableUSART6(FALSE);
      nvicEnableVector(STM32_USART6_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_SERIAL_USART6_PRIORITY));
    }
#endif
  }
  usart_init(sdp, config);
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 *
 * @notapi
 */
void sd_lld_stop(SerialDriver *sdp) {

  if (sdp->state == SD_READY) {
    usart_deinit(sdp->usart);
#if STM32_SERIAL_USE_USART1
    if (&SD1 == sdp) {
      rccDisableUSART1(FALSE);
      nvicDisableVector(STM32_USART1_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_USE_USART2
    if (&SD2 == sdp) {
      rccDisableUSART2(FALSE);
      nvicDisableVector(STM32_USART2_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_USE_USART3
    if (&SD3 == sdp) {
      rccDisableUSART3(FALSE);
      nvicDisableVector(STM32_USART3_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_USE_UART4
    if (&SD4 == sdp) {
      rccDisableUART4(FALSE);
      nvicDisableVector(STM32_UART4_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_USE_UART5
    if (&SD5 == sdp) {
      rccDisableUART5(FALSE);
      nvicDisableVector(STM32_UART5_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_USE_USART6
    if (&SD6 == sdp) {
      rccDisableUSART6(FALSE);
      nvicDisableVector(STM32_USART6_NUMBER);
      return;
    }
#endif
  }
}

#endif /* HAL_USE_SERIAL */

/** @} */
