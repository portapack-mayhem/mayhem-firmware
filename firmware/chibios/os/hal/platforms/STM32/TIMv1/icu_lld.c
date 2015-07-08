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
/*
   Concepts and parts of this file have been contributed by Fabio Utzig and
   Xo Wang.
 */

/**
 * @file    STM32/icu_lld.c
 * @brief   STM32 ICU subsystem low level driver header.
 *
 * @addtogroup ICU
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_ICU || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   ICUD1 driver identifier.
 * @note    The driver ICUD1 allocates the complex timer TIM1 when enabled.
 */
#if STM32_ICU_USE_TIM1 || defined(__DOXYGEN__)
ICUDriver ICUD1;
#endif

/**
 * @brief   ICUD2 driver identifier.
 * @note    The driver ICUD1 allocates the timer TIM2 when enabled.
 */
#if STM32_ICU_USE_TIM2 || defined(__DOXYGEN__)
ICUDriver ICUD2;
#endif

/**
 * @brief   ICUD3 driver identifier.
 * @note    The driver ICUD1 allocates the timer TIM3 when enabled.
 */
#if STM32_ICU_USE_TIM3 || defined(__DOXYGEN__)
ICUDriver ICUD3;
#endif

/**
 * @brief   ICUD4 driver identifier.
 * @note    The driver ICUD4 allocates the timer TIM4 when enabled.
 */
#if STM32_ICU_USE_TIM4 || defined(__DOXYGEN__)
ICUDriver ICUD4;
#endif

/**
 * @brief   ICUD5 driver identifier.
 * @note    The driver ICUD5 allocates the timer TIM5 when enabled.
 */
#if STM32_ICU_USE_TIM5 || defined(__DOXYGEN__)
ICUDriver ICUD5;
#endif

/**
 * @brief   ICUD8 driver identifier.
 * @note    The driver ICUD8 allocates the timer TIM8 when enabled.
 */
#if STM32_ICU_USE_TIM8 || defined(__DOXYGEN__)
ICUDriver ICUD8;
#endif

/**
 * @brief   ICUD9 driver identifier.
 * @note    The driver ICUD9 allocates the timer TIM9 when enabled.
 */
#if STM32_ICU_USE_TIM9 || defined(__DOXYGEN__)
ICUDriver ICUD9;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared IRQ handler.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 */
static void icu_lld_serve_interrupt(ICUDriver *icup) {
  uint16_t sr;

  sr  = icup->tim->SR;
  sr &= icup->tim->DIER & STM32_TIM_DIER_IRQ_MASK;
  icup->tim->SR = ~sr;
  if (icup->config->channel == ICU_CHANNEL_1) {
    if ((sr & STM32_TIM_SR_CC1IF) != 0)
      _icu_isr_invoke_period_cb(icup);
    if ((sr & STM32_TIM_SR_CC2IF) != 0)
      _icu_isr_invoke_width_cb(icup);
  } else {
    if ((sr & STM32_TIM_SR_CC1IF) != 0)
      _icu_isr_invoke_width_cb(icup);
    if ((sr & STM32_TIM_SR_CC2IF) != 0)
      _icu_isr_invoke_period_cb(icup);
  }
  if ((sr & STM32_TIM_SR_UIF) != 0)
    _icu_isr_invoke_overflow_cb(icup);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_ICU_USE_TIM1
#if !defined(STM32_TIM1_UP_HANDLER)
#error "STM32_TIM1_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM1 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM1_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD1);

  CH_IRQ_EPILOGUE();
}

#if !defined(STM32_TIM1_CC_HANDLER)
#error "STM32_TIM1_CC_HANDLER not defined"
#endif
/**
 * @brief   TIM1 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM1_CC_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD1);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM1 */

#if STM32_ICU_USE_TIM2
#if !defined(STM32_TIM2_HANDLER)
#error "STM32_TIM2_HANDLER not defined"
#endif
/**
 * @brief   TIM2 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM2_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD2);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM2 */

#if STM32_ICU_USE_TIM3
#if !defined(STM32_TIM3_HANDLER)
#error "STM32_TIM3_HANDLER not defined"
#endif
/**
 * @brief   TIM3 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM3_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD3);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM3 */

#if STM32_ICU_USE_TIM4
#if !defined(STM32_TIM4_HANDLER)
#error "STM32_TIM4_HANDLER not defined"
#endif
/**
 * @brief   TIM4 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM4_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD4);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM4 */

#if STM32_ICU_USE_TIM5
#if !defined(STM32_TIM5_HANDLER)
#error "STM32_TIM5_HANDLER not defined"
#endif
/**
 * @brief   TIM5 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM5_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD5);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM5 */

#if STM32_ICU_USE_TIM8
#if !defined(STM32_TIM8_UP_HANDLER)
#error "STM32_TIM8_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM8 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM8_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD8);

  CH_IRQ_EPILOGUE();
}

#if !defined(STM32_TIM8_CC_HANDLER)
#error "STM32_TIM8_CC_HANDLER not defined"
#endif
/**
 * @brief   TIM8 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM8_CC_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD8);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM8 */

#if STM32_ICU_USE_TIM9
#if !defined(STM32_TIM9_HANDLER)
#error "STM32_TIM9_HANDLER not defined"
#endif
/**
 * @brief   TIM9 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM9_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD9);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_ICU_USE_TIM9 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ICU driver initialization.
 *
 * @notapi
 */
void icu_lld_init(void) {

#if STM32_ICU_USE_TIM1
  /* Driver initialization.*/
  icuObjectInit(&ICUD1);
  ICUD1.tim = STM32_TIM1;
#endif

#if STM32_ICU_USE_TIM2
  /* Driver initialization.*/
  icuObjectInit(&ICUD2);
  ICUD2.tim = STM32_TIM2;
#endif

#if STM32_ICU_USE_TIM3
  /* Driver initialization.*/
  icuObjectInit(&ICUD3);
  ICUD3.tim = STM32_TIM3;
#endif

#if STM32_ICU_USE_TIM4
  /* Driver initialization.*/
  icuObjectInit(&ICUD4);
  ICUD4.tim = STM32_TIM4;
#endif

#if STM32_ICU_USE_TIM5
  /* Driver initialization.*/
  icuObjectInit(&ICUD5);
  ICUD5.tim = STM32_TIM5;
#endif

#if STM32_ICU_USE_TIM8
  /* Driver initialization.*/
  icuObjectInit(&ICUD8);
  ICUD8.tim = STM32_TIM8;
#endif

#if STM32_ICU_USE_TIM9
  /* Driver initialization.*/
  icuObjectInit(&ICUD9);
  ICUD9.tim = STM32_TIM9;
#endif
}

/**
 * @brief   Configures and activates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_start(ICUDriver *icup) {
  uint32_t psc;

  chDbgAssert((icup->config->channel == ICU_CHANNEL_1) ||
              (icup->config->channel == ICU_CHANNEL_2),
              "icu_lld_start(), #1", "invalid input");

  if (icup->state == ICU_STOP) {
    /* Clock activation and timer reset.*/
#if STM32_ICU_USE_TIM1
    if (&ICUD1 == icup) {
      rccEnableTIM1(FALSE);
      rccResetTIM1();
      nvicEnableVector(STM32_TIM1_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM1_IRQ_PRIORITY));
      nvicEnableVector(STM32_TIM1_CC_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM1_IRQ_PRIORITY));
#if defined(STM32_TIM1CLK)
      icup->clock = STM32_TIM1CLK;
#else
      icup->clock = STM32_TIMCLK2;
#endif
    }
#endif
#if STM32_ICU_USE_TIM2
    if (&ICUD2 == icup) {
      rccEnableTIM2(FALSE);
      rccResetTIM2();
      nvicEnableVector(STM32_TIM2_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM2_IRQ_PRIORITY));
      icup->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_ICU_USE_TIM3
    if (&ICUD3 == icup) {
      rccEnableTIM3(FALSE);
      rccResetTIM3();
      nvicEnableVector(STM32_TIM3_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM3_IRQ_PRIORITY));
      icup->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_ICU_USE_TIM4
    if (&ICUD4 == icup) {
      rccEnableTIM4(FALSE);
      rccResetTIM4();
      nvicEnableVector(STM32_TIM4_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM4_IRQ_PRIORITY));
      icup->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_ICU_USE_TIM5
    if (&ICUD5 == icup) {
      rccEnableTIM5(FALSE);
      rccResetTIM5();
      nvicEnableVector(STM32_TIM5_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM5_IRQ_PRIORITY));
      icup->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_ICU_USE_TIM8
    if (&ICUD8 == icup) {
      rccEnableTIM8(FALSE);
      rccResetTIM8();
      nvicEnableVector(STM32_TIM8_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM8_IRQ_PRIORITY));
      nvicEnableVector(STM32_TIM8_CC_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM8_IRQ_PRIORITY));
#if defined(STM32_TIM8CLK)
      icup->clock = STM32_TIM8CLK;
#else
      icup->clock = STM32_TIMCLK2;
#endif
    }
#endif
#if STM32_ICU_USE_TIM9
    if (&ICUD9 == icup) {
      rccEnableTIM9(FALSE);
      rccResetTIM9();
      nvicEnableVector(STM32_TIM9_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_ICU_TIM9_IRQ_PRIORITY));
      icup->clock = STM32_TIMCLK2;
    }
#endif
  }
  else {
    /* Driver re-configuration scenario, it must be stopped first.*/
    icup->tim->CR1    = 0;                  /* Timer disabled.              */
    icup->tim->CCR[0] = 0;                  /* Comparator 1 disabled.       */
    icup->tim->CCR[1] = 0;                  /* Comparator 2 disabled.       */
    icup->tim->CNT    = 0;                  /* Counter reset to zero.       */
  }

  /* Timer configuration.*/
  icup->tim->SR   = 0;                     /* Clear eventual pending IRQs. */
  icup->tim->DIER = icup->config->dier &   /* DMA-related DIER settings.   */
                      ~STM32_TIM_DIER_IRQ_MASK;
  psc = (icup->clock / icup->config->frequency) - 1;
  chDbgAssert((psc <= 0xFFFF) &&
              ((psc + 1) * icup->config->frequency) == icup->clock,
              "icu_lld_start(), #1", "invalid frequency");
  icup->tim->PSC  = (uint16_t)psc;
  icup->tim->ARR  = 0xFFFF;

  if (icup->config->channel == ICU_CHANNEL_1) {
    /* Selected input 1.
       CCMR1_CC1S = 01 = CH1 Input on TI1.
       CCMR1_CC2S = 10 = CH2 Input on TI1.*/
    icup->tim->CCMR1 = STM32_TIM_CCMR1_CC1S(1) | STM32_TIM_CCMR1_CC2S(2);

    /* SMCR_TS  = 101, input is TI1FP1.
       SMCR_SMS = 100, reset on rising edge.*/
    icup->tim->SMCR  = STM32_TIM_SMCR_TS(5) | STM32_TIM_SMCR_SMS(4);

    /* The CCER settings depend on the selected trigger mode.
       ICU_INPUT_ACTIVE_HIGH: Active on rising edge, idle on falling edge.
       ICU_INPUT_ACTIVE_LOW:  Active on falling edge, idle on rising edge.*/
    if (icup->config->mode == ICU_INPUT_ACTIVE_HIGH)
      icup->tim->CCER = STM32_TIM_CCER_CC1E |
                        STM32_TIM_CCER_CC2E | STM32_TIM_CCER_CC2P;
    else
      icup->tim->CCER = STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC1P |
                        STM32_TIM_CCER_CC2E;

    /* Direct pointers to the capture registers in order to make reading
       data faster from within callbacks.*/
    icup->wccrp = &icup->tim->CCR[1];
    icup->pccrp = &icup->tim->CCR[0];
  } else {
    /* Selected input 2.
       CCMR1_CC1S = 10 = CH1 Input on TI2.
       CCMR1_CC2S = 01 = CH2 Input on TI2.*/
    icup->tim->CCMR1 = STM32_TIM_CCMR1_CC1S(2) | STM32_TIM_CCMR1_CC2S(1);

    /* SMCR_TS  = 110, input is TI2FP2.
       SMCR_SMS = 100, reset on rising edge.*/
    icup->tim->SMCR  = STM32_TIM_SMCR_TS(6) | STM32_TIM_SMCR_SMS(4);

    /* The CCER settings depend on the selected trigger mode.
       ICU_INPUT_ACTIVE_HIGH: Active on rising edge, idle on falling edge.
       ICU_INPUT_ACTIVE_LOW:  Active on falling edge, idle on rising edge.*/
    if (icup->config->mode == ICU_INPUT_ACTIVE_HIGH)
      icup->tim->CCER = STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC1P |
                        STM32_TIM_CCER_CC2E;
    else
      icup->tim->CCER = STM32_TIM_CCER_CC1E |
                        STM32_TIM_CCER_CC2E | STM32_TIM_CCER_CC2P;

    /* Direct pointers to the capture registers in order to make reading
       data faster from within callbacks.*/
    icup->wccrp = &icup->tim->CCR[0];
    icup->pccrp = &icup->tim->CCR[1];
  }
}

/**
 * @brief   Deactivates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_stop(ICUDriver *icup) {

  if (icup->state == ICU_READY) {
    /* Clock deactivation.*/
    icup->tim->CR1  = 0;                    /* Timer disabled.              */
    icup->tim->DIER = 0;                    /* All IRQs disabled.           */
    icup->tim->SR   = 0;                    /* Clear eventual pending IRQs. */

#if STM32_ICU_USE_TIM1
    if (&ICUD1 == icup) {
      nvicDisableVector(STM32_TIM1_UP_NUMBER);
      nvicDisableVector(STM32_TIM1_CC_NUMBER);
      rccDisableTIM1(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM2
    if (&ICUD2 == icup) {
      nvicDisableVector(STM32_TIM2_NUMBER);
      rccDisableTIM2(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM3
    if (&ICUD3 == icup) {
      nvicDisableVector(STM32_TIM3_NUMBER);
      rccDisableTIM3(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM4
    if (&ICUD4 == icup) {
      nvicDisableVector(STM32_TIM4_NUMBER);
      rccDisableTIM4(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM5
    if (&ICUD5 == icup) {
      nvicDisableVector(STM32_TIM5_NUMBER);
      rccDisableTIM5(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM8
    if (&ICUD8 == icup) {
      nvicDisableVector(STM32_TIM8_UP_NUMBER);
      nvicDisableVector(STM32_TIM8_CC_NUMBER);
      rccDisableTIM8(FALSE);
    }
#endif
#if STM32_ICU_USE_TIM9
    if (&ICUD9 == icup) {
      nvicDisableVector(STM32_TIM9_NUMBER);
      rccDisableTIM9(FALSE);
    }
#endif
  }
}

/**
 * @brief   Enables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_enable(ICUDriver *icup) {

  icup->tim->EGR |= STM32_TIM_EGR_UG;
  icup->tim->SR = 0;                        /* Clear pending IRQs (if any). */
  if (icup->config->channel == ICU_CHANNEL_1) {
    if (icup->config->period_cb != NULL)
      icup->tim->DIER |= STM32_TIM_DIER_CC1IE;
    if (icup->config->width_cb != NULL)
      icup->tim->DIER |= STM32_TIM_DIER_CC2IE;
  } else {
    if (icup->config->width_cb != NULL)
      icup->tim->DIER |= STM32_TIM_DIER_CC1IE;
    if (icup->config->period_cb != NULL)
      icup->tim->DIER |= STM32_TIM_DIER_CC2IE;
  }
  if (icup->config->overflow_cb != NULL)
    icup->tim->DIER |= STM32_TIM_DIER_UIE;
  icup->tim->CR1 = STM32_TIM_CR1_URS | STM32_TIM_CR1_CEN;
}

/**
 * @brief   Disables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_disable(ICUDriver *icup) {

  icup->tim->CR1   = 0;                     /* Initially stopped.           */
  icup->tim->SR    = 0;                     /* Clear pending IRQs (if any). */

  /* All interrupts disabled.*/
  icup->tim->DIER &= ~STM32_TIM_DIER_IRQ_MASK;
}

#endif /* HAL_USE_ICU */

/** @} */
