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
 * @file    STM32/gpt_lld.c
 * @brief   STM32 GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_GPT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPTD1 driver identifier.
 * @note    The driver GPTD1 allocates the complex timer TIM1 when enabled.
 */
#if STM32_GPT_USE_TIM1 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif

/**
 * @brief   GPTD2 driver identifier.
 * @note    The driver GPTD2 allocates the timer TIM2 when enabled.
 */
#if STM32_GPT_USE_TIM2 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif

/**
 * @brief   GPTD3 driver identifier.
 * @note    The driver GPTD3 allocates the timer TIM3 when enabled.
 */
#if STM32_GPT_USE_TIM3 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif

/**
 * @brief   GPTD4 driver identifier.
 * @note    The driver GPTD4 allocates the timer TIM4 when enabled.
 */
#if STM32_GPT_USE_TIM4 || defined(__DOXYGEN__)
GPTDriver GPTD4;
#endif

/**
 * @brief   GPTD5 driver identifier.
 * @note    The driver GPTD5 allocates the timer TIM5 when enabled.
 */
#if STM32_GPT_USE_TIM5 || defined(__DOXYGEN__)
GPTDriver GPTD5;
#endif

/**
 * @brief   GPTD6 driver identifier.
 * @note    The driver GPTD6 allocates the timer TIM6 when enabled.
 */
#if STM32_GPT_USE_TIM6 || defined(__DOXYGEN__)
GPTDriver GPTD6;
#endif

/**
 * @brief   GPTD7 driver identifier.
 * @note    The driver GPTD7 allocates the timer TIM7 when enabled.
 */
#if STM32_GPT_USE_TIM7 || defined(__DOXYGEN__)
GPTDriver GPTD7;
#endif

/**
 * @brief   GPTD8 driver identifier.
 * @note    The driver GPTD8 allocates the timer TIM8 when enabled.
 */
#if STM32_GPT_USE_TIM8 || defined(__DOXYGEN__)
GPTDriver GPTD8;
#endif

/**
 * @brief   GPTD9 driver identifier.
 * @note    The driver GPTD9 allocates the timer TIM9 when enabled.
 */
#if STM32_GPT_USE_TIM9 || defined(__DOXYGEN__)
GPTDriver GPTD9;
#endif

/**
 * @brief   GPTD11 driver identifier.
 * @note    The driver GPTD11 allocates the timer TIM11 when enabled.
 */
#if STM32_GPT_USE_TIM11 || defined(__DOXYGEN__)
GPTDriver GPTD11;
#endif

/**
 * @brief   GPTD12 driver identifier.
 * @note    The driver GPTD12 allocates the timer TIM12 when enabled.
 */
#if STM32_GPT_USE_TIM12 || defined(__DOXYGEN__)
GPTDriver GPTD12;
#endif

/**
 * @brief   GPTD14 driver identifier.
 * @note    The driver GPTD14 allocates the timer TIM14 when enabled.
 */
#if STM32_GPT_USE_TIM14 || defined(__DOXYGEN__)
GPTDriver GPTD14;
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
 * @param[in] gptp      pointer to a @p GPTDriver object
 */
static void gpt_lld_serve_interrupt(GPTDriver *gptp) {

  gptp->tim->SR = 0;
  if (gptp->state == GPT_ONESHOT) {
    gptp->state = GPT_READY;                /* Back in GPT_READY state.     */
    gpt_lld_stop_timer(gptp);               /* Timer automatically stopped. */
  }
  gptp->config->callback(gptp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_GPT_USE_TIM1
#if !defined(STM32_TIM1_UP_HANDLER)
#error "STM32_TIM1_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM1_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD1);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM1 */

#if STM32_GPT_USE_TIM2
#if !defined(STM32_TIM2_HANDLER)
#error "STM32_TIM2_HANDLER not defined"
#endif
/**
 * @brief   TIM2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM2_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD2);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM2 */

#if STM32_GPT_USE_TIM3
#if !defined(STM32_TIM3_HANDLER)
#error "STM32_TIM3_HANDLER not defined"
#endif
/**
 * @brief   TIM3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM3_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD3);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM3 */

#if STM32_GPT_USE_TIM4
#if !defined(STM32_TIM4_HANDLER)
#error "STM32_TIM4_HANDLER not defined"
#endif
/**
 * @brief   TIM4 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM4_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD4);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM4 */

#if STM32_GPT_USE_TIM5
#if !defined(STM32_TIM5_HANDLER)
#error "STM32_TIM5_HANDLER not defined"
#endif
/**
 * @brief   TIM5 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM5_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD5);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM5 */

#if STM32_GPT_USE_TIM6
#if !defined(STM32_TIM6_HANDLER)
#error "STM32_TIM6_HANDLER not defined"
#endif
/**
 * @brief   TIM6 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM6_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD6);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM6 */

#if STM32_GPT_USE_TIM7
#if !defined(STM32_TIM7_HANDLER)
#error "STM32_TIM7_HANDLER not defined"
#endif
/**
 * @brief   TIM7 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM7_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD7);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM7 */

#if STM32_GPT_USE_TIM8
#if !defined(STM32_TIM8_UP_HANDLER)
#error "STM32_TIM8_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM8 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM8_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD8);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM8 */

#if STM32_GPT_USE_TIM9
#if !defined(STM32_TIM9_HANDLER)
#error "STM32_TIM9_HANDLER not defined"
#endif
/**
 * @brief   TIM9 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM9_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD9);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM9 */

#if STM32_GPT_USE_TIM11
#if !defined(STM32_TIM11_HANDLER)
#error "STM32_TIM11_HANDLER not defined"
#endif
/**
 * @brief   TIM11 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM11_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD11);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM11 */

#if STM32_GPT_USE_TIM12
#if !defined(STM32_TIM12_HANDLER)
#error "STM32_TIM12_HANDLER not defined"
#endif
/**
 * @brief   TIM12 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM12_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD12);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM12 */

#if STM32_GPT_USE_TIM14
#if !defined(STM32_TIM14_HANDLER)
#error "STM32_TIM14_HANDLER not defined"
#endif
/**
 * @brief   TIM14 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM14_HANDLER) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD14);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_GPT_USE_TIM14 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

#if STM32_GPT_USE_TIM1
  /* Driver initialization.*/
  GPTD1.tim = STM32_TIM1;
  gptObjectInit(&GPTD1);
#endif

#if STM32_GPT_USE_TIM2
  /* Driver initialization.*/
  GPTD2.tim = STM32_TIM2;
  gptObjectInit(&GPTD2);
#endif

#if STM32_GPT_USE_TIM3
  /* Driver initialization.*/
  GPTD3.tim = STM32_TIM3;
  gptObjectInit(&GPTD3);
#endif

#if STM32_GPT_USE_TIM4
  /* Driver initialization.*/
  GPTD4.tim = STM32_TIM4;
  gptObjectInit(&GPTD4);
#endif

#if STM32_GPT_USE_TIM5
  /* Driver initialization.*/
  GPTD5.tim = STM32_TIM5;
  gptObjectInit(&GPTD5);
#endif

#if STM32_GPT_USE_TIM6
  /* Driver initialization.*/
  GPTD6.tim = STM32_TIM6;
  gptObjectInit(&GPTD6);
#endif

#if STM32_GPT_USE_TIM7
  /* Driver initialization.*/
  GPTD7.tim = STM32_TIM7;
  gptObjectInit(&GPTD7);
#endif

#if STM32_GPT_USE_TIM8
  /* Driver initialization.*/
  GPTD8.tim = STM32_TIM8;
  gptObjectInit(&GPTD8);
#endif

#if STM32_GPT_USE_TIM9
  /* Driver initialization.*/
  GPTD9.tim = STM32_TIM9;
  gptObjectInit(&GPTD9);
#endif

#if STM32_GPT_USE_TIM11
  /* Driver initialization.*/
  GPTD11.tim = STM32_TIM11;
  gptObjectInit(&GPTD11);
#endif

#if STM32_GPT_USE_TIM12
  /* Driver initialization.*/
  GPTD12.tim = STM32_TIM12;
  gptObjectInit(&GPTD12);
#endif

#if STM32_GPT_USE_TIM14
  /* Driver initialization.*/
  GPTD14.tim = STM32_TIM14;
  gptObjectInit(&GPTD14);
#endif
}

/**
 * @brief   Configures and activates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {
  uint16_t psc;

  if (gptp->state == GPT_STOP) {
    /* Clock activation.*/
#if STM32_GPT_USE_TIM1
    if (&GPTD1 == gptp) {
      rccEnableTIM1(FALSE);
      rccResetTIM1();
      nvicEnableVector(STM32_TIM1_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM1_IRQ_PRIORITY));
#if defined(STM32_TIM1CLK)
      gptp->clock = STM32_TIM1CLK;
#else
      gptp->clock = STM32_TIMCLK2;
#endif
    }
#endif
#if STM32_GPT_USE_TIM2
    if (&GPTD2 == gptp) {
      rccEnableTIM2(FALSE);
      rccResetTIM2();
      nvicEnableVector(STM32_TIM2_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM2_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_GPT_USE_TIM3
    if (&GPTD3 == gptp) {
      rccEnableTIM3(FALSE);
      rccResetTIM3();
      nvicEnableVector(STM32_TIM3_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM3_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_GPT_USE_TIM4
    if (&GPTD4 == gptp) {
      rccEnableTIM4(FALSE);
      rccResetTIM4();
      nvicEnableVector(STM32_TIM4_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM4_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_GPT_USE_TIM5
    if (&GPTD5 == gptp) {
      rccEnableTIM5(FALSE);
      rccResetTIM5();
      nvicEnableVector(STM32_TIM5_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM5_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_GPT_USE_TIM6
    if (&GPTD6 == gptp) {
      rccEnableTIM6(FALSE);
      rccResetTIM6();
      nvicEnableVector(STM32_TIM6_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM6_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_GPT_USE_TIM7
    if (&GPTD7 == gptp) {
      rccEnableTIM7(FALSE);
      rccResetTIM7();
      nvicEnableVector(STM32_TIM7_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM7_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_GPT_USE_TIM8
    if (&GPTD8 == gptp) {
      rccEnableTIM8(FALSE);
      rccResetTIM8();
      nvicEnableVector(STM32_TIM8_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM8_IRQ_PRIORITY));
#if defined(STM32_TIM8CLK)
      gptp->clock = STM32_TIM8CLK;
#else
      gptp->clock = STM32_TIMCLK2;
#endif
    }
#endif

#if STM32_GPT_USE_TIM9
    if (&GPTD9 == gptp) {
      rccEnableTIM9(FALSE);
      rccResetTIM9();
      nvicEnableVector(STM32_TIM9_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM9_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK2;
    }
#endif

#if STM32_GPT_USE_TIM11
    if (&GPTD11 == gptp) {
      rccEnableTIM11(FALSE);
      rccResetTIM11();
      nvicEnableVector(STM32_TIM11_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM11_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK2;
    }
#endif

#if STM32_GPT_USE_TIM12
    if (&GPTD12 == gptp) {
      rccEnableTIM12(FALSE);
      rccResetTIM12();
      nvicEnableVector(STM32_TIM12_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM12_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_GPT_USE_TIM14
    if (&GPTD14 == gptp) {
      rccEnableTIM14(FALSE);
      rccResetTIM14();
      nvicEnableVector(STM32_TIM14_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_GPT_TIM14_IRQ_PRIORITY));
      gptp->clock = STM32_TIMCLK1;
    }
#endif
  }

  /* Prescaler value calculation.*/
  psc = (uint16_t)((gptp->clock / gptp->config->frequency) - 1);
  chDbgAssert(((uint32_t)(psc + 1) * gptp->config->frequency) == gptp->clock,
              "gpt_lld_start(), #1", "invalid frequency");

  /* Timer configuration.*/
  gptp->tim->CR1  = 0;                          /* Initially stopped.       */
  gptp->tim->CR2  = STM32_TIM_CR2_CCDS;         /* DMA on UE (if any).      */
  gptp->tim->PSC  = psc;                        /* Prescaler value.         */
  gptp->tim->DIER = gptp->config->dier &        /* DMA-related DIER bits.   */
                    STM32_TIM_DIER_IRQ_MASK;
  gptp->tim->SR   = 0;                          /* Clear pending IRQs.      */
}

/**
 * @brief   Deactivates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop(GPTDriver *gptp) {

  if (gptp->state == GPT_READY) {
    gptp->tim->CR1  = 0;                        /* Timer disabled.          */
    gptp->tim->DIER = 0;                        /* All IRQs disabled.       */
    gptp->tim->SR   = 0;                        /* Clear pending IRQs.      */

#if STM32_GPT_USE_TIM1
    if (&GPTD1 == gptp) {
      nvicDisableVector(STM32_TIM1_UP_NUMBER);
      rccDisableTIM1(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM2
    if (&GPTD2 == gptp) {
      nvicDisableVector(STM32_TIM2_NUMBER);
      rccDisableTIM2(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM3
    if (&GPTD3 == gptp) {
      nvicDisableVector(STM32_TIM3_NUMBER);
      rccDisableTIM3(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM4
    if (&GPTD4 == gptp) {
      nvicDisableVector(STM32_TIM4_NUMBER);
      rccDisableTIM4(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM5
    if (&GPTD5 == gptp) {
      nvicDisableVector(STM32_TIM5_NUMBER);
      rccDisableTIM5(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM6
    if (&GPTD6 == gptp) {
      nvicDisableVector(STM32_TIM6_NUMBER);
      rccDisableTIM6(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM7
    if (&GPTD7 == gptp) {
      nvicDisableVector(STM32_TIM7_NUMBER);
      rccDisableTIM7(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM8
    if (&GPTD8 == gptp) {
      nvicDisableVector(STM32_TIM8_UP_NUMBER);
      rccDisableTIM8(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM9
    if (&GPTD9 == gptp) {
      nvicDisableVector(STM32_TIM9_NUMBER);
      rccDisableTIM9(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM11
    if (&GPTD11 == gptp) {
      nvicDisableVector(STM32_TIM11_NUMBER);
      rccDisableTIM11(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM12
    if (&GPTD12 == gptp) {
      nvicDisableVector(STM32_TIM12_NUMBER);
      rccDisableTIM12(FALSE);
    }
#endif
#if STM32_GPT_USE_TIM14
    if (&GPTD14 == gptp) {
      nvicDisableVector(STM32_TIM14_NUMBER);
      rccDisableTIM14(FALSE);
    }
#endif
  }
}

/**
 * @brief   Starts the timer in continuous mode.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  period in ticks
 *
 * @notapi
 */
void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval) {

  gptp->tim->ARR   = (uint32_t)(interval - 1);  /* Time constant.           */
  gptp->tim->EGR   = STM32_TIM_EGR_UG;          /* Update event.            */
  gptp->tim->CNT   = 0;                         /* Reset counter.           */

  /* NOTE: After generating the UG event it takes several clock cycles before
     SR bit 0 goes to 1. This is because the clearing of CNT has been inserted
     before the clearing of SR, to give it some time.*/
  gptp->tim->SR    = 0;                         /* Clear pending IRQs.      */
  gptp->tim->DIER |= STM32_TIM_DIER_UIE;        /* Update Event IRQ enabled.*/
  gptp->tim->CR1   = STM32_TIM_CR1_URS | STM32_TIM_CR1_CEN;
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {

  gptp->tim->CR1   = 0;                         /* Initially stopped.       */
  gptp->tim->SR    = 0;                         /* Clear pending IRQs.      */

  /* All interrupts disabled.*/
  gptp->tim->DIER &= ~STM32_TIM_DIER_IRQ_MASK;
}

/**
 * @brief   Starts the timer in one shot mode and waits for completion.
 * @details This function specifically polls the timer waiting for completion
 *          in order to not have extra delays caused by interrupt servicing,
 *          this function is only recommended for short delays.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  time interval in ticks
 *
 * @notapi
 */
void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {

  gptp->tim->ARR  = (uint32_t)(interval - 1);   /* Time constant.           */
  gptp->tim->EGR  = STM32_TIM_EGR_UG;           /* Update event.            */
  gptp->tim->SR   = 0;                          /* Clear pending IRQs.      */
  gptp->tim->CR1  = STM32_TIM_CR1_OPM | STM32_TIM_CR1_URS | STM32_TIM_CR1_CEN;
  while (!(gptp->tim->SR & STM32_TIM_SR_UIF))
    ;
}

#endif /* HAL_USE_GPT */

/** @} */
