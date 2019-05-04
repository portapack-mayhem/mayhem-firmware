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

#if LPC43XX_ADC_USE_ADC0
static const adc_resources_t adc0_resources = {
  .base = { .clk = &LPC_CGU->BASE_APB3_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1U << 0) },
  .branch = { .cfg = &LPC_CCU1->CLK_APB3_ADC0_CFG, .stat = &LPC_CCU1->CLK_APB3_ADC0_STAT },
  .reset = { .output_index = 40 },
  .interrupt = { .irq = ADC0_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_ADC0_IRQ_PRIORITY) },
};
#endif /* LPC43XX_ADC_USE_ADC0 */

#if LPC43XX_ADC_USE_ADC1
static const adc_resources_t adc1_resources = {
  .base = { .clk = &LPC_CGU->BASE_APB3_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1U << 0) },
  .branch = { .cfg = &LPC_CCU1->CLK_APB3_ADC1_CFG, .stat = &LPC_CCU1->CLK_APB3_ADC1_STAT },
  .reset = { .output_index = 41 },
  .interrupt = { .irq = ADC1_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_ADC1_IRQ_PRIORITY) },
};
#endif /* LPC43XX_ADC_USE_ADC1 */

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
  ADCD0.resources = &adc0_resources;
#endif /* LPC43XX_ADC_USE_ADC0 */

#if LPC43XX_ADC_USE_ADC1
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  /* TODO: Implement */
  ADCD1.adc = LPC_ADC1;
  ADCD1.resources = &adc1_resources;
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
    base_clock_enable(&adcp->resources->base);
    branch_clock_enable(&adcp->resources->branch);
    peripheral_reset(&adcp->resources->reset);
    interrupt_enable(&adcp->resources->interrupt);

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
    interrupt_disable(&adcp->resources->interrupt);
    peripheral_reset(&adcp->resources->reset);
    branch_clock_disable(&adcp->resources->branch);
    base_clock_disable(&adcp->resources->base);
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
