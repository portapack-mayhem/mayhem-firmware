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
 * @file    AT91SAM7/serial_lld.c
 * @brief   AT91SAM7 low level serial driver code.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

#if (SAM7_PLATFORM == SAM7S64) || (SAM7_PLATFORM == SAM7S128) ||            \
    (SAM7_PLATFORM == SAM7S256) || (SAM7_PLATFORM == SAM7S512)

#define SAM7_USART0_RX    AT91C_PA5_RXD0
#define SAM7_USART0_TX    AT91C_PA6_TXD0
#define SAM7_USART1_RX    AT91C_PA21_RXD1
#define SAM7_USART1_TX    AT91C_PA22_TXD1
#define SAM7_DBGU_RX      AT91C_PA9_DRXD
#define SAM7_DBGU_TX      AT91C_PA10_DTXD

#elif (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) ||         \
      (SAM7_PLATFORM == SAM7X512)

#define SAM7_USART0_RX    AT91C_PA0_RXD0
#define SAM7_USART0_TX    AT91C_PA1_TXD0
#define SAM7_USART1_RX    AT91C_PA5_RXD1
#define SAM7_USART1_TX    AT91C_PA6_TXD1
#define SAM7_DBGU_RX      AT91C_PA27_DRXD
#define SAM7_DBGU_TX      AT91C_PA28_DTXD

#elif (SAM7_PLATFORM == SAM7A3)
#define SAM7_USART0_RX    AT91C_PA2_RXD0
#define SAM7_USART0_TX    AT91C_PA3_TXD0
#define SAM7_USART1_RX    AT91C_PA7_RXD1
#define SAM7_USART1_TX    AT91C_PA8_TXD1
#define SAM7_USART2_RX    AT91C_PA9_RXD2
#define SAM7_USART2_TX    AT91C_PA10_TXD2
#define SAM7_DBGU_RX      AT91C_PA30_DRXD
#define SAM7_DBGU_TX      AT91C_PA31_DTXD

#else
#error "serial lines not defined for this SAM7 version"
#endif /* HAL_USE_SERIAL */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if USE_SAM7_USART0 || defined(__DOXYGEN__)
/** @brief USART0 serial driver identifier.*/
SerialDriver SD1;
#endif

#if USE_SAM7_USART1 || defined(__DOXYGEN__)
/** @brief USART1 serial driver identifier.*/
SerialDriver SD2;
#endif

#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2 || defined(__DOXYGEN__)
/** @brief USART2 serial driver identifier.*/
SerialDriver SD3;
#endif
#endif

#if USE_SAM7_DBGU_UART || defined(__DOXYGEN__)
/** @brief DBGU_UART serial driver identifier.*/
SerialDriver SDDBG;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig default_config = {
  SERIAL_DEFAULT_BITRATE,
  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   USART initialization.
 *
 * @param[in] sdp       communication channel associated to the USART
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart_init(SerialDriver *sdp, const SerialConfig *config) {
  AT91PS_USART u = sdp->usart;

  /* Disables IRQ sources and stop operations.*/
  u->US_IDR = 0xFFFFFFFF;
  u->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA;

  /* New parameters setup.*/
  if (config->sc_mr & AT91C_US_OVER)
    u->US_BRGR = MCK / (config->sc_speed * 8);
  else
    u->US_BRGR = MCK / (config->sc_speed * 16);
  u->US_MR = config->sc_mr;
  u->US_RTOR = 0;
  u->US_TTGR = 0;

  /* Enables operations and IRQ sources.*/
  u->US_CR = AT91C_US_RXEN | AT91C_US_TXEN | AT91C_US_DTREN | AT91C_US_RTSEN;
  u->US_IER = AT91C_US_RXRDY | AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE |
              AT91C_US_RXBRK;
}

/**
 * @brief   USART de-initialization.
 *
 * @param[in] u         pointer to an USART I/O block
 */
static void usart_deinit(AT91PS_USART u) {

  /* Disables IRQ sources and stop operations.*/
  u->US_IDR = 0xFFFFFFFF;
  u->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA;
  u->US_MR = 0;
  u->US_RTOR = 0;
  u->US_TTGR = 0;
}

/**
 * @brief   Error handling routine.
 *
 * @param[in] err       USART CSR register value
 * @param[in] sdp       communication channel associated to the USART
 */
static void set_error(SerialDriver *sdp, AT91_REG csr) {
  flagsmask_t sts = 0;

  if (csr & AT91C_US_OVRE)
    sts |= SD_OVERRUN_ERROR;
  if (csr & AT91C_US_PARE)
    sts |= SD_PARITY_ERROR;
  if (csr & AT91C_US_FRAME)
    sts |= SD_FRAMING_ERROR;
  if (csr & AT91C_US_RXBRK)
    sts |= SD_BREAK_DETECTED;
  chSysLockFromIsr();
  chnAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
#if !USE_SAM7_DBGU_UART
static
#endif
/**
 * @brief   Common IRQ handler.
 *
 * @param[in] sdp       communication channel associated to the USART
 */
void sd_lld_serve_interrupt(SerialDriver *sdp) {
  uint32_t csr;
  AT91PS_USART u = sdp->usart;

  csr = u->US_CSR;
  if (csr & AT91C_US_RXRDY) {
    chSysLockFromIsr();
    sdIncomingDataI(sdp, u->US_RHR);
    chSysUnlockFromIsr();
  }
  if ((u->US_IMR & AT91C_US_TXRDY) && (csr & AT91C_US_TXRDY)) {
    msg_t b;

    chSysLockFromIsr();
    b = chOQGetI(&sdp->oqueue);
    if (b < Q_OK) {
      chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
      u->US_IDR = AT91C_US_TXRDY;
    }
    else
      u->US_THR = b;
    chSysUnlockFromIsr();
  }
  csr &= (AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE | AT91C_US_RXBRK);
  if (csr != 0) {
    set_error(sdp, csr);
    u->US_CR = AT91C_US_RSTSTA;
  }
  AT91C_BASE_AIC->AIC_EOICR = 0;
}

#if USE_SAM7_USART0 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  AT91C_BASE_US0->US_IER = AT91C_US_TXRDY;
}
#endif

#if USE_SAM7_USART1 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  AT91C_BASE_US1->US_IER = AT91C_US_TXRDY;
}
#endif

#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2 || defined(__DOXYGEN__)
static void notify3(GenericQueue *qp) {

  (void)qp;
  AT91C_BASE_US2->US_IER = AT91C_US_TXRDY;
}
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */

#if USE_SAM7_DBGU_UART || defined(__DOXYGEN__)
static void notify_dbg(GenericQueue *qp) {

  (void)qp;
  AT91C_BASE_DBGU->DBGU_IER = AT91C_US_TXRDY;
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if USE_SAM7_USART0 || defined(__DOXYGEN__)
/**
 * @brief   USART0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART0IrqHandler) {

  CH_IRQ_PROLOGUE();
  sd_lld_serve_interrupt(&SD1);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif

#if USE_SAM7_USART1 || defined(__DOXYGEN__)
/**
 * @brief   USART1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART1IrqHandler) {

  CH_IRQ_PROLOGUE();
  sd_lld_serve_interrupt(&SD2);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif

#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2 || defined(__DOXYGEN__)
/**
 * @brief   USART2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART2IrqHandler) {

  CH_IRQ_PROLOGUE();
  sd_lld_serve_interrupt(&SD3);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */

/* note - DBGU_UART IRQ is the SysIrq in board.c
   since it's not vectored separately by the AIC.*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {

#if USE_SAM7_USART0
  sdObjectInit(&SD1, NULL, notify1);
  SD1.usart = AT91C_BASE_US0;
  AT91C_BASE_PIOA->PIO_PDR   = SAM7_USART0_RX | SAM7_USART0_TX;
  AT91C_BASE_PIOA->PIO_ASR   = SAM7_USART0_RX | SAM7_USART0_TX;
  AT91C_BASE_PIOA->PIO_PPUDR = SAM7_USART0_RX | SAM7_USART0_TX;
  AIC_ConfigureIT(AT91C_ID_US0,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | SAM7_USART0_PRIORITY,
                  USART0IrqHandler);
#endif

#if USE_SAM7_USART1
  sdObjectInit(&SD2, NULL, notify2);
  SD2.usart = AT91C_BASE_US1;
  AT91C_BASE_PIOA->PIO_PDR   = SAM7_USART1_RX | SAM7_USART1_TX;
  AT91C_BASE_PIOA->PIO_ASR   = SAM7_USART1_RX | SAM7_USART1_TX;
  AT91C_BASE_PIOA->PIO_PPUDR = SAM7_USART1_RX | SAM7_USART1_TX;
  AIC_ConfigureIT(AT91C_ID_US1,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | SAM7_USART1_PRIORITY,
                  USART1IrqHandler);
#endif

#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2
  sdObjectInit(&SD3, NULL, notify3);
  SD3.usart = AT91C_BASE_US2;
  AT91C_BASE_PIOA->PIO_PDR   = SAM7_USART2_RX | SAM7_USART2_TX;
  AT91C_BASE_PIOA->PIO_ASR   = SAM7_USART2_RX | SAM7_USART2_TX;
  AT91C_BASE_PIOA->PIO_PPUDR = SAM7_USART2_RX | SAM7_USART2_TX;
  AIC_ConfigureIT(AT91C_ID_US2,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | SAM7_USART2_PRIORITY,
                  USART2IrqHandler);
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */

#if USE_SAM7_DBGU_UART
  sdObjectInit(&SDDBG, NULL, notify_dbg);
  /* this is a little cheap, but OK for now since there's enough overlap
     between dbgu and usart register maps.  it means we can reuse all the
     same usart interrupt handling and config that already exists.*/
  SDDBG.usart = (AT91PS_USART)AT91C_BASE_DBGU;
  AT91C_BASE_PIOA->PIO_PDR   = SAM7_DBGU_RX | SAM7_DBGU_TX;
  AT91C_BASE_PIOA->PIO_ASR   = SAM7_DBGU_RX | SAM7_DBGU_TX;
  AT91C_BASE_PIOA->PIO_PPUDR = SAM7_DBGU_RX | SAM7_DBGU_TX;
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
#if USE_SAM7_USART0
    if (&SD1 == sdp) {
      /* Starts the clock and clears possible sources of immediate interrupts.*/
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US0);
      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_US0);
    }
#endif
#if USE_SAM7_USART1
    if (&SD2 == sdp) {
      /* Starts the clock and clears possible sources of immediate interrupts.*/
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US1);
      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_US1);
    }
#endif
#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2
    if (&SD3 == sdp) {
      /* Starts the clock and clears possible sources of immediate interrupts.*/
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US2);
      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_US2);
    }
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */
  /* Note - no explicit start for SD3 (DBGU_UART) since it's not included
     in the AIC or PMC.*/
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
#if USE_SAM7_USART0
    if (&SD1 == sdp) {
      AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_US0);
      AIC_DisableIT(AT91C_ID_US0);
      return;
    }
#endif
#if USE_SAM7_USART1
    if (&SD2 == sdp) {
      AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_US1);
      AIC_DisableIT(AT91C_ID_US1);
      return;
    }
#endif
#if USE_SAM7_DBGU_UART
    if (&SDDBG == sdp) {
      AT91C_BASE_DBGU->DBGU_IDR = 0xFFFFFFFF;
      return;
    }
#endif
  }
}

#endif /* HAL_USE_SERIAL */

/** @} */
