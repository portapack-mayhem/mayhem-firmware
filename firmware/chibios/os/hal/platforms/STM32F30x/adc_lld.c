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
 * @file    STM32F30x/adc_lld.c
 * @brief   STM32F30x ADC subsystem low level driver source.
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

#if STM32_ADC_DUAL_MODE
#if STM32_ADC_COMPACT_SAMPLES
/* Compact type dual mode.*/
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_PSIZE_HWORD)
#define ADC_DMA_MDMA    ADC_CCR_MDMA_HWORD

#else /* !STM32_ADC_COMPACT_SAMPLES */
/* Large type dual mode.*/
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_PSIZE_WORD)
#define ADC_DMA_MDMA    ADC_CCR_MDMA_WORD
#endif /* !STM32_ADC_COMPACT_SAMPLES */

#else /* !STM32_ADC_DUAL_MODE */
#if STM32_ADC_COMPACT_SAMPLES
/* Compact type single mode.*/
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE)
#define ADC_DMA_MDMA    ADC_CCR_MDMA_DISABLED

#else /* !STM32_ADC_COMPACT_SAMPLES */
/* Large type single mode.*/
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_PSIZE_HWORD)
#define ADC_DMA_MDMA    ADC_CCR_MDMA_DISABLED
#endif /* !STM32_ADC_COMPACT_SAMPLES */
#endif /* !STM32_ADC_DUAL_MODE */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief ADC1 driver identifier.*/
#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
ADCDriver ADCD1;
#endif

/** @brief ADC1 driver identifier.*/
#if STM32_ADC_USE_ADC3 || defined(__DOXYGEN__)
ADCDriver ADCD3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Enables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_vreg_on(ADCDriver *adcp) {

  adcp->adcm->CR = 0;   /* RM 12.4.3.*/
  adcp->adcm->CR = ADC_CR_ADVREGEN_0;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR = ADC_CR_ADVREGEN_0;
#endif
  halPolledDelay(US2RTT(10));
}

/**
 * @brief   Disables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_vreg_off(ADCDriver *adcp) {

  adcp->adcm->CR = 0;   /* RM 12.4.3.*/
  adcp->adcm->CR = ADC_CR_ADVREGEN_1;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR = ADC_CR_ADVREGEN_1;
#endif
}

/**
 * @brief   Enables the ADC analog circuit.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_analog_on(ADCDriver *adcp) {

  adcp->adcm->CR |= ADC_CR_ADEN;
  while ((adcp->adcm->ISR & ADC_ISR_ADRDY) == 0)
    ;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR |= ADC_CR_ADEN;
  while ((adcp->adcs->ISR & ADC_ISR_ADRDY) == 0)
    ;
#endif
}

/**
 * @brief   Disables the ADC  analog circuit.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_analog_off(ADCDriver *adcp) {

  adcp->adcm->CR |= ADC_CR_ADDIS;
  while ((adcp->adcm->CR & ADC_CR_ADDIS) != 0)
    ;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR |= ADC_CR_ADDIS;
  while ((adcp->adcs->CR & ADC_CR_ADDIS) != 0)
    ;
#endif
}

/**
 * @brief   Calibrates and ADC unit.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_calibrate(ADCDriver *adcp) {

  chDbgAssert(adcp->adcm->CR == ADC_CR_ADVREGEN_0, "adc_lld_calibrate(), #1",
                                                   "invalid register state");
  adcp->adcm->CR |= ADC_CR_ADCAL;
  while ((adcp->adcm->CR & ADC_CR_ADCAL) != 0)
    ;
#if STM32_ADC_DUAL_MODE
  chDbgAssert(adcp->adcs->CR == ADC_CR_ADVREGEN_0, "adc_lld_calibrate(), #2",
                                                   "invalid register state");
  adcp->adcs->CR |= ADC_CR_ADCAL;
  while ((adcp->adcs->CR & ADC_CR_ADCAL) != 0)
    ;
#endif
}

/**
 * @brief   Stops an ongoing conversion, if any.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 */
static void adc_lld_stop_adc(ADCDriver *adcp) {

  if (adcp->adcm->CR & ADC_CR_ADSTART) {
    adcp->adcm->CR |= ADC_CR_ADSTP;
    while (adcp->adcm->CR & ADC_CR_ADSTP)
      ;
  }
}

/**
 * @brief   ADC DMA ISR service routine.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void adc_lld_serve_dma_interrupt(ADCDriver *adcp, uint32_t flags) {

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    /* DMA, this could help only if the DMA tries to access an unmapped
       address space or violates alignment rules.*/
    _adc_isr_error_code(adcp, ADC_ERR_DMAFAILURE);
  }
  else {
    /* It is possible that the conversion group has already be reset by the
       ADC error handler, in this case this interrupt is spurious.*/
    if (adcp->grpp != NULL) {
      if ((flags & STM32_DMA_ISR_TCIF) != 0) {
        /* Transfer complete processing.*/
        _adc_isr_full_code(adcp);
      }
      else if ((flags & STM32_DMA_ISR_HTIF) != 0) {
        /* Half transfer processing.*/
        _adc_isr_half_code(adcp);
      }
    }
  }
}

/**
 * @brief   ADC ISR service routine.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 * @param[in] isr       content of the ISR register
 */
static void adc_lld_serve_interrupt(ADCDriver *adcp, uint32_t isr) {

  /* It could be a spurious interrupt caused by overflows after DMA disabling,
     just ignore it in this case.*/
  if (adcp->grpp != NULL) {
    /* Note, an overflow may occur after the conversion ended before the driver
       is able to stop the ADC, this is why the DMA channel is checked too.*/
    if ((isr & ADC_ISR_OVR) &&
        (dmaStreamGetTransactionSize(adcp->dmastp) > 0)) {
      /* ADC overflow condition, this could happen only if the DMA is unable
         to read data fast enough.*/
      _adc_isr_error_code(adcp, ADC_ERR_OVERFLOW);
    }
    if (isr & ADC_ISR_AWD1) {
      /* Analog watchdog error.*/
      _adc_isr_error_code(adcp, ADC_ERR_AWD1);
    }
    if (isr & ADC_ISR_AWD2) {
      /* Analog watchdog error.*/
      _adc_isr_error_code(adcp, ADC_ERR_AWD2);
    }
    if (isr & ADC_ISR_AWD3) {
      /* Analog watchdog error.*/
      _adc_isr_error_code(adcp, ADC_ERR_AWD3);
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
/**
 * @brief   ADC1/ADC2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector88) {
  uint32_t isr;

  CH_IRQ_PROLOGUE();

#if STM32_ADC_DUAL_MODE
  isr  = ADC1->ISR;
  isr |= ADC2->ISR;
  ADC1->ISR = isr;
  ADC2->ISR = isr;
#else /* !STM32_ADC_DUAL_MODE */
  isr  = ADC1->ISR;
  ADC1->ISR = isr;
#endif /* !STM32_ADC_DUAL_MODE */

  adc_lld_serve_interrupt(&ADCD1, isr);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ADC_USE_ADC1 */

#if STM32_ADC_USE_ADC3 || defined(__DOXYGEN__)
/**
 * @brief   ADC3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorFC) {
  uint32_t isr;

  CH_IRQ_PROLOGUE();

  isr  = ADC3->ISR;
  ADC3->ISR = isr;

  adc_lld_serve_interrupt(&ADCD3, isr);

  CH_IRQ_EPILOGUE();
}

#if STM32_ADC_DUAL_MODE
/**
 * @brief   ADC4 interrupt handler (as ADC3 slave).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector134) {
  uint32_t isr;

  CH_IRQ_PROLOGUE();

  isr  = ADC4->ISR;
  ADC4->ISR = isr;

  adc_lld_serve_interrupt(&ADCD3, isr);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ADC_DUAL_MODE */
#endif /* STM32_ADC_USE_ADC3 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if STM32_ADC_USE_ADC1
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  ADCD1.adcc = ADC1_2;
  ADCD1.adcm = ADC1;
#if STM32_ADC_DUAL_MODE
  ADCD1.adcs = ADC2;
#endif
  ADCD1.dmastp  = STM32_DMA1_STREAM1;
  ADCD1.dmamode = ADC_DMA_SIZE |
                  STM32_DMA_CR_PL(STM32_ADC_ADC12_DMA_PRIORITY) |
                  STM32_DMA_CR_DIR_P2M |
                  STM32_DMA_CR_MINC        | STM32_DMA_CR_TCIE        |
                  STM32_DMA_CR_DMEIE       | STM32_DMA_CR_TEIE;
  nvicEnableVector(ADC1_2_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_ADC_ADC12_IRQ_PRIORITY));
#endif /* STM32_ADC_USE_ADC1 */

#if STM32_ADC_USE_ADC3
  /* Driver initialization.*/
  adcObjectInit(&ADCD3);
  ADCD3.adcc = ADC3_4;
  ADCD3.adcm = ADC3;
#if STM32_ADC_DUAL_MODE
  ADCD3.adcs = ADC4;
#endif
  ADCD3.dmastp  = STM32_DMA2_STREAM5;
  ADCD3.dmamode = ADC_DMA_SIZE |
                  STM32_DMA_CR_PL(STM32_ADC_ADC12_DMA_PRIORITY) |
                  STM32_DMA_CR_DIR_P2M |
                  STM32_DMA_CR_MINC        | STM32_DMA_CR_TCIE        |
                  STM32_DMA_CR_DMEIE       | STM32_DMA_CR_TEIE;
  nvicEnableVector(ADC3_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_ADC_ADC34_IRQ_PRIORITY));
#if STM32_ADC_DUAL_MODE
  nvicEnableVector(ADC4_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_ADC_ADC34_IRQ_PRIORITY));
#endif
#endif /* STM32_ADC_USE_ADC3 */
}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start(ADCDriver *adcp) {

  /* If in stopped state then enables the ADC and DMA clocks.*/
  if (adcp->state == ADC_STOP) {
#if STM32_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      bool_t b;
      b = dmaStreamAllocate(adcp->dmastp,
                            STM32_ADC_ADC12_DMA_IRQ_PRIORITY,
                            (stm32_dmaisr_t)adc_lld_serve_dma_interrupt,
                            (void *)adcp);
      chDbgAssert(!b, "adc_lld_start(), #1", "stream already allocated");
      rccEnableADC12(FALSE);
    }
#endif /* STM32_ADC_USE_ADC1 */

#if STM32_ADC_USE_ADC3
    if (&ADCD3 == adcp) {
      bool_t b;
      b = dmaStreamAllocate(adcp->dmastp,
                            STM32_ADC_ADC34_DMA_IRQ_PRIORITY,
                            (stm32_dmaisr_t)adc_lld_serve_dma_interrupt,
                            (void *)adcp);
      chDbgAssert(!b, "adc_lld_start(), #2", "stream already allocated");
      rccEnableADC34(FALSE);
    }
#endif /* STM32_ADC_USE_ADC2 */

    /* Setting DMA peripheral-side pointer.*/
#if STM32_ADC_DUAL_MODE
      dmaStreamSetPeripheral(adcp->dmastp, &adcp->adcc->CDR);
#else
      dmaStreamSetPeripheral(adcp->dmastp, &adcp->adcm->DR);
#endif

    /* Clock source setting.*/
    adcp->adcc->CCR = STM32_ADC_ADC12_CLOCK_MODE | ADC_DMA_MDMA;

    /* Master ADC calibration.*/
    adc_lld_vreg_on(adcp);
    adc_lld_calibrate(adcp);

    /* Master ADC enabled here in order to reduce conversions latencies.*/
    adc_lld_analog_on(adcp);
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

  /* If in ready state then disables the ADC clock and analog part.*/
  if (adcp->state == ADC_READY) {

    /* Releasing the associated DMA channel.*/
    dmaStreamRelease(adcp->dmastp);

    /* Stopping the ongoing conversion, if any.*/
    adc_lld_stop_adc(adcp);

    /* Disabling ADC analog circuit and regulator.*/
    adc_lld_analog_off(adcp);
    adc_lld_vreg_off(adcp);

#if STM32_ADC_USE_ADC1
    if (&ADCD1 == adcp)
      rccDisableADC12(FALSE);
#endif

#if STM32_ADC_USE_ADC3
    if (&ADCD3 == adcp)
      rccDisableADC34(FALSE);
#endif
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
  uint32_t dmamode, ccr, cfgr;
  const ADCConversionGroup *grpp = adcp->grpp;

  chDbgAssert(!STM32_ADC_DUAL_MODE || ((grpp->num_channels & 1) == 0),
              "adc_lld_start_conversion(), #1",
              "odd number of channels in dual mode");

  /* Calculating control registers values.*/
  dmamode = adcp->dmamode;
  ccr     = grpp->ccr | (adcp->adcc->CCR & (ADC_CCR_CKMODE_MASK |
                                            ADC_CCR_MDMA_MASK));
  cfgr    = grpp->cfgr | ADC_CFGR_DMAEN;
  if (grpp->circular) {
    dmamode |= STM32_DMA_CR_CIRC;
#if STM32_ADC_DUAL_MODE
    ccr  |= ADC_CCR_DMACFG_CIRCULAR;
#else
    cfgr |= ADC_CFGR_DMACFG_CIRCULAR;
#endif
    if (adcp->depth > 1) {
      /* If circular buffer depth > 1, then the half transfer interrupt
         is enabled in order to allow streaming processing.*/
      dmamode |= STM32_DMA_CR_HTIE;
    }
  }

  /* DMA setup.*/
  dmaStreamSetMemory0(adcp->dmastp, adcp->samples);
#if STM32_ADC_DUAL_MODE
  dmaStreamSetTransactionSize(adcp->dmastp, ((uint32_t)grpp->num_channels/2) *
                                            (uint32_t)adcp->depth);
#else
    dmaStreamSetTransactionSize(adcp->dmastp, (uint32_t)grpp->num_channels *
                                              (uint32_t)adcp->depth);
#endif
  dmaStreamSetMode(adcp->dmastp, dmamode);
  dmaStreamEnable(adcp->dmastp);

  /* Configuring the CCR register with the static settings ORed with
     the user-specified settings in the conversion group configuration
     structure.*/
  adcp->adcc->CCR   = ccr;

  /* ADC setup, if it is defined a callback for the analog watch dog then it
     is enabled.*/
  adcp->adcm->ISR   = adcp->adcm->ISR;
  adcp->adcm->IER   = ADC_IER_OVR | ADC_IER_AWD1;
  adcp->adcm->TR1   = grpp->tr1;
#if STM32_ADC_DUAL_MODE
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1  = grpp->sqr[0] | ADC_SQR1_NUM_CH(grpp->num_channels / 2);
  adcp->adcm->SQR2  = grpp->sqr[1];
  adcp->adcm->SQR3  = grpp->sqr[2];
  adcp->adcm->SQR4  = grpp->sqr[3];
  adcp->adcs->SMPR1 = grpp->ssmpr[0];
  adcp->adcs->SMPR2 = grpp->ssmpr[1];
  adcp->adcs->SQR1  = grpp->ssqr[0] | ADC_SQR1_NUM_CH(grpp->num_channels / 2);
  adcp->adcs->SQR2  = grpp->ssqr[1];
  adcp->adcs->SQR3  = grpp->ssqr[2];
  adcp->adcs->SQR4  = grpp->ssqr[3];

#else /* !STM32_ADC_DUAL_MODE */
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1  = grpp->sqr[0] | ADC_SQR1_NUM_CH(grpp->num_channels);
  adcp->adcm->SQR2  = grpp->sqr[1];
  adcp->adcm->SQR3  = grpp->sqr[2];
  adcp->adcm->SQR4  = grpp->sqr[3];
#endif /* !STM32_ADC_DUAL_MODE */

  /* ADC configuration.*/
  adcp->adcm->CFGR  = cfgr;

  /* Starting conversion.*/
  adcp->adcm->CR   |= ADC_CR_ADSTART;
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop_conversion(ADCDriver *adcp) {

  dmaStreamDisable(adcp->dmastp);
  adc_lld_stop_adc(adcp);
}

#endif /* HAL_USE_ADC */

/** @} */
