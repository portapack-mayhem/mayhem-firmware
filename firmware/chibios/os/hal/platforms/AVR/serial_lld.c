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
 * @file    AVR/serial_lld.c
 * @brief   AVR low level serial driver code.
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

/**
 * @brief   USART0 serial driver identifier.
 * @note    The name does not follow the convention used in the other ports
 *          (COMn) because a name conflict with the AVR headers.
 */
#if USE_AVR_USART0 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/**
 * @brief   USART1 serial driver identifier.
 * @note    The name does not follow the convention used in the other ports
 *          (COMn) because a name conflict with the AVR headers.
 */
#if USE_AVR_USART1 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SerialConfig default_config = {
  UBRR(SERIAL_DEFAULT_BITRATE),
  USART_CHAR_SIZE_8
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void set_error(uint8_t sra, SerialDriver *sdp) {
  flagsmask_t sts = 0;
  uint8_t dor = 0;
  uint8_t upe = 0;
  uint8_t fe = 0;

#if USE_AVR_USART0
  if (&SD1 == sdp) {
    dor = (1 << DOR0);
    upe = (1 << UPE0);
    fe = (1 << FE0);
  }
#endif

#if USE_AVR_USART1
  if (&SD2 == sdp) {
    dor = (1 << DOR1);
    upe = (1 << UPE1);
    fe = (1 << FE1);
  }
#endif

  if (sra & dor)
    sts |= SD_OVERRUN_ERROR;
  if (sra & upe)
    sts |= SD_PARITY_ERROR;
  if (sra & fe)
    sts |= SD_FRAMING_ERROR;
  chSysLockFromIsr();
  chnAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

#if USE_AVR_USART0 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  UCSR0B |= (1 << UDRIE0);
}

/**
 * @brief   USART0 initialization.
 *
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart0_init(const SerialConfig *config) {

  UBRR0L = config->sc_brr;
  UBRR0H = config->sc_brr >> 8;
  UCSR0A = 0;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  switch (config->sc_bits_per_char) {
  case USART_CHAR_SIZE_5:
    UCSR0C = 0;
    break;
  case USART_CHAR_SIZE_6:
    UCSR0C = (1 << UCSZ00);
    break;
  case USART_CHAR_SIZE_7:
    UCSR0C = (1 << UCSZ01);
    break;
  case USART_CHAR_SIZE_9:
    UCSR0B |= (1 << UCSZ02);
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
    break;
  case USART_CHAR_SIZE_8:
  default:
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
  }
}

/**
 * @brief   USART0 de-initialization.
 */
static void usart0_deinit(void) {

  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = 0;
}
#endif

#if USE_AVR_USART1 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  UCSR1B |= (1 << UDRIE1);
}

/**
 * @brief   USART1 initialization.
 *
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart1_init(const SerialConfig *config) {

  UBRR1L = config->sc_brr;
  UBRR1H = config->sc_brr >> 8;
  UCSR1A = 0;
  UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
  switch (config->sc_bits_per_char) {
  case USART_CHAR_SIZE_5:
    UCSR1C = 0;
    break;
  case USART_CHAR_SIZE_6:
    UCSR1C = (1 << UCSZ10);
    break;
  case USART_CHAR_SIZE_7:
    UCSR1C = (1 << UCSZ11);
    break;
  case USART_CHAR_SIZE_9:
    UCSR1B |= (1 << UCSZ12);
    UCSR1C = (1 << UCSZ10) | (1 << UCSZ11);
    break;
  case USART_CHAR_SIZE_8:
  default:
    UCSR1C = (1 << UCSZ10) | (1 << UCSZ11);
  }
}

/**
 * @brief   USART1 de-initialization.
 */
static void usart1_deinit(void) {

  UCSR1A = 0;
  UCSR1B = 0;
  UCSR1C = 0;
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if USE_AVR_USART0 || defined(__DOXYGEN__)
/**
 * @brief   USART0 RX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART0_RX_vect) {
  uint8_t sra;

  CH_IRQ_PROLOGUE();

  sra = UCSR0A;
  if (sra & ((1 << DOR0) | (1 << UPE0) | (1 << FE0)))
    set_error(sra, &SD1);
  chSysLockFromIsr();
  sdIncomingDataI(&SD1, UDR0);
  chSysUnlockFromIsr();

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   USART0 TX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART0_UDRE_vect) {
  msg_t b;

  CH_IRQ_PROLOGUE();

  chSysLockFromIsr();
  b = sdRequestDataI(&SD1);
  chSysUnlockFromIsr();
  if (b < Q_OK)
    UCSR0B &= ~(1 << UDRIE0);
  else
    UDR0 = b;

  CH_IRQ_EPILOGUE();
}
#endif /* USE_AVR_USART0 */

#if USE_AVR_USART1 || defined(__DOXYGEN__)
/**
 * @brief   USART1 RX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART1_RX_vect) {
  uint8_t sra;

  CH_IRQ_PROLOGUE();

  sra = UCSR1A;
  if (sra & ((1 << DOR1) | (1 << UPE1) | (1 << FE1)))
    set_error(sra, &SD2);
  chSysLockFromIsr();
  sdIncomingDataI(&SD2, UDR1);
  chSysUnlockFromIsr();

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   USART1 TX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(USART1_UDRE_vect) {
  msg_t b;

  CH_IRQ_PROLOGUE();

  chSysLockFromIsr();
  b = sdRequestDataI(&SD2);
  chSysUnlockFromIsr();
  if (b < Q_OK)
    UCSR1B &= ~(1 << UDRIE1);
  else
    UDR1 = b;

  CH_IRQ_EPILOGUE();
}
#endif /* USE_AVR_USART1 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {

#if USE_AVR_USART0
  sdObjectInit(&SD1, NULL, notify1);
#endif
#if USE_AVR_USART1
  sdObjectInit(&SD2, NULL, notify2);
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

#if USE_AVR_USART0
  if (&SD1 == sdp) {
    usart0_init(config);
    return;
  }
#endif
#if USE_AVR_USART1
  if (&SD2 == sdp) {
    usart1_init(config);
    return;
  }
#endif
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

#if USE_AVR_USART0
  if (&SD1 == sdp)
    usart0_deinit();
#endif
#if USE_AVR_USART1
  if (&SD2 == sdp)
    usart1_deinit();
#endif
}

#endif /* HAL_USE_SERIAL */

/** @} */
