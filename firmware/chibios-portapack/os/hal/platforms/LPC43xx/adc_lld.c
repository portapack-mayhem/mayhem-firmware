/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    LPC43xx/adc_lld.c
 * @brief   LPC43xx ADC subsystem low level driver source.
 *
 * @addtogroup ADC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief ADC0 driver identifier.
 */
#if LPC43XX_ADC_USE_ADC0 || defined(__DOXYGEN__)
ADCDriver ADCD0;
#endif

/**
 * @brief ADC1 driver identifier.
 */
#if LPX43XX_ADC_USE_ADC1 || defined(__DOXYGEN__)
ADCDriver ADCD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   ADC ISR service routine.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void adc_lld_serve_interrupt(ADCDriver *adcp) {
  /* TODO: Implement */
  uint32_t dr[8];
  uint32_t stat = adcp->adc->STAT;
  for(size_t i=0; i<8; i++) {
    if( stat & 1 ) {
      dr[i] = adcp->adc->DR[i];
    }
    stat >>= 1;
  }

  if( adcp->grpp ) {

  } else {
    /* Conversion is stopped. Ignore */
    /* TODO: Read conversion registers to clear interrupt? */
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if LPC43XX_ADC_USE_ADC0 || defined(__DOXYGEN__)
/**
 * @brief ADC0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(ADC0_IRQHandler) {
  CH_IRQ_PROLOGUE();
  adc_lld_serve_interrupt(&ADCD0);
  CH_IRQ_EPILOGUE();
}
#endif

#if LPC43XX_ADC_USE_ADC1 || defined(__DOXYGEN__)
/**
 * @brief ADC1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(ADC1_IRQHandler) {
  CH_IRQ_PROLOGUE();
  adc_lld_serve_interrupt(&ADCD1);
  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if LPC43XX_ADC_USE_ADC0
  /* Driver initialization.*/
  adcObjectInit(&ADCD0);
  /* TODO: Implement */
  ADCD0.adc = LPC_ADC0;
#endif /* LPC43XX_ADC_USE_ADC0 */

#if LPC43XX_ADC_USE_ADC1
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  /* TODO: Implement */
  ADCD1.adc = LPC_ADC1;
#endif /* LPC43XX_ADC_USE_ADC1 */
}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start(ADCDriver *adcp) {

  if (adcp->state == ADC_STOP) {
    /* Enables the peripheral.*/
#if LPC43XX_ADC_USE_ADC0
    if (&ADCD0 == adcp) {
      LPC_CCU1->CLK_APB3_ADC0_CFG.RUN = 1;
      LPC_CGU->BASE_APB3_CLK.PD = 0;
      nvicEnableVector(ADC0_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_ADC0_IRQ_PRIORITY));
    }
#endif /* LPC43XX_ADC_USE_ADC0 */
#if LPC43XX_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      LPC_CCU1->CLK_APB3_ADC1_CFG.RUN = 1;
      LPC_CGU->BASE_APB3_CLK.PD = 0;
      nvicEnableVector(ADC1_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_ADC1_IRQ_PRIORITY));
    }
#endif /* LPC43XX_ADC_USE_ADC1 */

    /* Configures the peripheral.*/
    adcp->adc->CR =
        (0x00 << 0)
      | (adcp->config->cr_clkdiv << 8)
      | (0 << 16)
      | (adcp->config->cr_clks << 17)
      | (1 << 21)
      | (0 << 24)
      | (0 << 27)
      ;
  }
}

/**
 * @brief   Deactivates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop(ADCDriver *adcp) {

  if (adcp->state == ADC_READY) {
    /* Resets the peripheral.*/
    /* Reset START to 0x0, power down. */
    adcp->adc->CR =
        (0x00 << 0)
      | (adcp->config->cr_clkdiv << 8)
      | (0 << 16)
      | (adcp->config->cr_clks << 17)
      | (0 << 21)
      | (0 << 24)
      | (0 << 27)
      ;

    /* Disables the peripheral.*/
#if LPC43XX_ADC_USE_ADC0
    if (&ADCD0 == adcp) {
      nvicDisableVector(ADC0_IRQn);
      LPC_CCU1->CLK_APB3_ADC0_CFG.AUTO = 1;
      LPC_CCU1->CLK_APB3_ADC0_CFG.RUN = 0;
    }
#endif /* LPC43XX_ADC_USE_ADC0 */

#if LPC43XX_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      nvicDisableVector(ADC1_IRQn);
      LPC_CCU1->CLK_APB3_ADC1_CFG.AUTO = 1;
      LPC_CCU1->CLK_APB3_ADC1_CFG.RUN = 0;
    }
#endif /* LPC43XX_ADC_USE_ADC1 */
  }
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start_conversion(ADCDriver *adcp) {
  const ADCConversionGroup *grpp = adcp->grpp;

  adcp->adc->CR =
      (grpp->cr_sel << 0)
    | (adcp->config->cr_clkdiv << 8)
    | (0 << 16)
    | (adcp->config->cr_clks << 17)
    | (1 << 21)
    | (1 << 24)
    | (0 << 27)
    ;
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop_conversion(ADCDriver *adcp) {
    adcp->adc->CR =
        (0x00 << 0)
      | (adcp->config->cr_clkdiv << 8)
      | (0 << 16)
      | (adcp->config->cr_clks << 17)
      | (1 << 21)
      | (0 << 24)
      | (0 << 27)
      ;
}

#endif /* HAL_USE_ADC */

/** @} */
