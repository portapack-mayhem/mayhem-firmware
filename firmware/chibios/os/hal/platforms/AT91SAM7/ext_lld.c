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
 * @file    AT91SAM7/ext_lld.c
 * @brief   AT91SAM7 EXT subsystem low level driver source.
 *
 * @addtogroup EXT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   EXTDA driver identifier.
 */
EXTDriver EXTDA;

#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
/**
 * @brief   EXTDB driver identifier.
 */
EXTDriver EXTDB;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Handles external interrupts.
 *
 * @param[in] extp      pointer to the driver that received the interrupt
 */
static void ext_lld_serveInterrupt(EXTDriver *extp) {
  uint32_t irqFlags;
  uint32_t ch;

  chSysLockFromIsr();

  /* Read flags of pending PIO interrupts.*/
  irqFlags = extp->pio->PIO_ISR;

  /* Call callback function for any pending interrupt.*/
  for(ch = 0; ch < EXT_MAX_CHANNELS; ch++) {

    /* Check if the channel is activated and if its IRQ flag is set.*/
    if((extp->config->channels[ch].mode &
        EXT_CH_MODE_ENABLED & EXT_CH_MODE_EDGES_MASK)
        && ((1 << ch) & irqFlags)) {
      (extp->config->channels[ch].cb)(extp, ch);
    }
  }

  chSysUnlockFromIsr();

  AT91C_BASE_AIC->AIC_EOICR = 0;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   EXTI[0] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(EXTIA_IRQHandler) {

  CH_IRQ_PROLOGUE();

  ext_lld_serveInterrupt(&EXTDA);

  CH_IRQ_EPILOGUE();
}

#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
/**
 * @brief   EXTI[1] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(EXTIB_IRQHandler) {
  CH_IRQ_PROLOGUE();

  ext_lld_serveInterrupt(&EXTDB);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level EXT driver initialization.
 *
 * @notapi
 */
void ext_lld_init(void) {

  /* Driver initialization.*/
  extObjectInit(&EXTDA);

  /* Set PIO base addresses.*/
  EXTDA.pio = AT91C_BASE_PIOA;

  /* Set peripheral IDs.*/
  EXTDA.pid = AT91C_ID_PIOA;

#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
  /* Same for PIOB.*/
  extObjectInit(&EXTDB);
  EXTDB.pio = AT91C_BASE_PIOB;
  EXTDB.pid = AT91C_ID_PIOB;
#endif
}

/**
 * @brief   Configures and activates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_start(EXTDriver *extp) {
  uint16_t ch;
  uint32_t ier = 0;
  const EXTConfig *config = extp->config;

  switch(extp->pid) {
  case AT91C_ID_PIOA:
    AIC_ConfigureIT(AT91C_ID_PIOA, SAM7_computeSMR(config->mode,
                                                   config->priority),
                                                   EXTIA_IRQHandler);
    break;
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
  case AT91C_ID_PIOB:
    AIC_ConfigureIT(AT91C_ID_PIOB, SAM7_computeSMR(config->mode,
                                                   config->priority),
                                                   EXTIB_IRQHandler);
    break;
#endif
  }

  /* Enable and Disable channels with respect to config.*/
  for(ch = 0; ch < EXT_MAX_CHANNELS; ch++) {
    ier |= (config->channels[ch].mode & EXT_CH_MODE_EDGES_MASK & EXT_CH_MODE_ENABLED ? 1 : 0) << ch;
  }
  extp->pio->PIO_IER = ier;
  extp->pio->PIO_IDR = ~ier;

  /* Enable interrupt on corresponding PIO port in AIC.*/
  AIC_EnableIT(extp->pid);
}

/**
 * @brief   Deactivates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_stop(EXTDriver *extp) {

  /* Disable interrupt on corresponding PIO port in AIC.*/
  AIC_DisableIT(extp->pid);
}

/**
 * @brief   Enables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be enabled
 *
 * @notapi
 */
void ext_lld_channel_enable(EXTDriver *extp, expchannel_t channel) {

  chDbgCheck((extp->config->channels[channel].cb != NULL),
      "Call back pointer can not be NULL");

  extp->pio->PIO_IER = (1 << channel);
}

/**
 * @brief   Disables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be disabled
 *
 * @notapi
 */
void ext_lld_channel_disable(EXTDriver *extp, expchannel_t channel) {

  extp->pio->PIO_IDR = (1 << channel);
}

#endif /* HAL_USE_EXT */

/** @} */
