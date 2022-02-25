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
 * @file    LPC13xx/gpt_lld.c
 * @brief   LPC13xx GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_GPT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPT1 driver identifier.
 * @note    The driver GPT1 allocates the complex timer CT16B0 when enabled.
 */
#if LPC13xx_GPT_USE_CT16B0 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif

/**
 * @brief   GPT2 driver identifier.
 * @note    The driver GPT2 allocates the timer CT16B1 when enabled.
 */
#if LPC13xx_GPT_USE_CT16B1 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif

/**
 * @brief   GPT3 driver identifier.
 * @note    The driver GPT3 allocates the timer CT32B0 when enabled.
 */
#if LPC13xx_GPT_USE_CT32B0 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif

/**
 * @brief   GPT4 driver identifier.
 * @note    The driver GPT4 allocates the timer CT32B1 when enabled.
 */
#if LPC13xx_GPT_USE_CT32B1 || defined(__DOXYGEN__)
GPTDriver GPTD4;
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

  gptp->tmr->IR = 1;                        /* Clear interrupt on match MR0.*/
  if (gptp->state == GPT_ONESHOT) {
    gptp->state = GPT_READY;                /* Back in GPT_READY state.     */
    gpt_lld_stop_timer(gptp);               /* Timer automatically stopped. */
  }
  gptp->config->callback(gptp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if LPC13xx_GPT_USE_CT16B0
/**
 * @brief   CT16B0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorE4) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD1);

  CH_IRQ_EPILOGUE();
}
#endif /* LPC13xx_GPT_USE_CT16B0 */

#if LPC13xx_GPT_USE_CT16B1
/**
 * @brief   CT16B1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorE8) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD2);

  CH_IRQ_EPILOGUE();
}
#endif /* LPC13xx_GPT_USE_CT16B0 */

#if LPC13xx_GPT_USE_CT32B0
/**
 * @brief   CT32B0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorEC) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD3);

  CH_IRQ_EPILOGUE();
}
#endif /* LPC13xx_GPT_USE_CT32B0 */

#if LPC13xx_GPT_USE_CT32B1
/**
 * @brief   CT32B1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorF0) {

  CH_IRQ_PROLOGUE();

  gpt_lld_serve_interrupt(&GPTD4);

  CH_IRQ_EPILOGUE();
}
#endif /* LPC13xx_GPT_USE_CT32B1 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

#if LPC13xx_GPT_USE_CT16B0
  /* Driver initialization.*/
  GPTD1.tmr = LPC_TMR16B0;
  gptObjectInit(&GPTD1);
#endif

#if LPC13xx_GPT_USE_CT16B1
  /* Driver initialization.*/
  GPTD2.tmr = LPC_TMR16B1;
  gptObjectInit(&GPTD2);
#endif

#if LPC13xx_GPT_USE_CT32B0
  /* Driver initialization.*/
  GPTD3.tmr = LPC_TMR32B0;
  gptObjectInit(&GPTD3);
#endif

#if LPC13xx_GPT_USE_CT32B1
  /* Driver initialization.*/
  GPTD4.tmr = LPC_TMR32B1;
  gptObjectInit(&GPTD4);
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
  uint32_t pr;

  if (gptp->state == GPT_STOP) {
    /* Clock activation.*/
#if LPC13xx_GPT_USE_CT16B0
    if (&GPTD1 == gptp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 7);
      nvicEnableVector(TIMER_16_0_IRQn, CORTEX_PRIORITY_MASK(2));
    }
#endif
#if LPC13xx_GPT_USE_CT16B1
    if (&GPTD2 == gptp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 8);
      nvicEnableVector(TIMER_16_1_IRQn, CORTEX_PRIORITY_MASK(3));
    }
#endif
#if LPC13xx_GPT_USE_CT32B0
    if (&GPTD3 == gptp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);
      nvicEnableVector(TIMER_32_0_IRQn, CORTEX_PRIORITY_MASK(2));
    }
#endif
#if LPC13xx_GPT_USE_CT32B1
    if (&GPTD4 == gptp) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 10);
      nvicEnableVector(TIMER_32_1_IRQn, CORTEX_PRIORITY_MASK(2));
    }
#endif
  }

  /* Prescaler value calculation.*/
  pr = (uint16_t)((LPC13xx_SYSCLK / gptp->config->frequency) - 1);
  chDbgAssert(((uint32_t)(pr + 1) * gptp->config->frequency) == LPC13xx_SYSCLK,
              "gpt_lld_start(), #1", "invalid frequency");

  /* Timer configuration.*/
  gptp->tmr->PR  = pr;
  gptp->tmr->IR  = 1;
  gptp->tmr->MCR = 0;
  gptp->tmr->TCR = 0;
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
    gptp->tmr->MCR = 0;
    gptp->tmr->TCR = 0;

#if LPC13xx_GPT_USE_CT16B0
    if (&GPTD1 == gptp) {
      nvicDisableVector(TIMER_16_0_IRQn);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 7);
    }
#endif
#if LPC13xx_GPT_USE_CT16B1
    if (&GPTD2 == gptp) {
      nvicDisableVector(TIMER_16_1_IRQn);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 8);
    }
#endif
#if LPC13xx_GPT_USE_CT32B0
    if (&GPTD3 == gptp) {
      nvicDisableVector(TIMER_32_0_IRQn);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 9);
    }
#endif
#if LPC13xx_GPT_USE_CT32B1
    if (&GPTD4 == gptp) {
      nvicDisableVector(TIMER_32_1_IRQn);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 10);
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

  gptp->tmr->MR0 = interval - 1;
  gptp->tmr->IR  = 1;
  gptp->tmr->MCR = 3;                       /* IRQ and clr TC on match MR0. */
  gptp->tmr->TCR = 2;                       /* Reset counter and prescaler. */
  gptp->tmr->TCR = 1;                       /* Timer enabled.               */
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {

  gptp->tmr->IR  = 1;
  gptp->tmr->MCR = 0;
  gptp->tmr->TCR = 0;
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

  gptp->tmr->MR0 = interval - 1;
  gptp->tmr->IR  = 1;
  gptp->tmr->MCR = 4;                       /* Stop TC on match MR0.        */
  gptp->tmr->TCR = 2;                       /* Reset counter and prescaler. */
  gptp->tmr->TCR = 1;                       /* Timer enabled.               */
  while (gptp->tmr->TCR & 1)
    ;
}

#endif /* HAL_USE_GPT */

/** @} */
