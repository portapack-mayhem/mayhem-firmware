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
 * @file    LPC43xx/gpt_lld.c
 * @brief   LPC43xx GPT subsystem low level driver source.
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

#if LPC43XX_GPT_USE_TIMER0
static const timer_resources_t timer0_resources = {
  .branch = { .cfg = &LPC_CCU1->CLK_M4_TIMER0_CFG, .stat = &LPC_CCU1->CLK_M4_TIMER0_STAT },
  .reset = { .output_index = 32 },
  .interrupt = { .irq = TIMER0_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_GPT_TIMER0_IRQ_PRIORITY) },
};
#endif /* LPC43XX_GPT_USE_TIMER0 */

#if LPC43XX_GPT_USE_TIMER1
static const timer_resources_t timer1_resources = {
  .branch = { .cfg = &LPC_CCU1->CLK_M4_TIMER1_CFG, .stat = &LPC_CCU1->CLK_M4_TIMER1_STAT },
  .reset = { .output_index = 33 },
  .interrupt = { .irq = TIMER1_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_GPT_TIMER1_IRQ_PRIORITY) },
};
#endif /* LPC43XX_GPT_USE_TIMER1 */

#if LPC43XX_GPT_USE_TIMER2
static const timer_resources_t timer2_resources = {
  .branch = { .cfg = &LPC_CCU1->CLK_M4_TIMER2_CFG, .stat = &LPC_CCU1->CLK_M4_TIMER2_STAT },
  .reset = { .output_index = 34 },
  .interrupt = { .irq = TIMER2_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_GPT_TIMER2_IRQ_PRIORITY) },
};
#endif /* LPC43XX_GPT_USE_TIMER3 */

#if LPC43XX_GPT_USE_TIMER3
static const timer_resources_t timer3_resources = {
  .branch = { .cfg = &LPC_CCU1->CLK_M4_TIMER3_CFG, .stat = &LPC_CCU1->CLK_M4_TIMER3_STAT },
  .reset = { .output_index = 35 },
  .interrupt = { .irq = TIMER3_IRQn, .priority_mask = CORTEX_PRIORITY_MASK(LPC43XX_GPT_TIMER3_IRQ_PRIORITY) },
};
#endif /* LPC43XX_GPT_USE_TIMER3 */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPTD1 driver identifier.
 */
#if LPC43XX_GPT_USE_TIMER0 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif

/**
 * @brief   GPTD2 driver identifier.
 */
#if LPC43XX_GPT_USE_TIMER1 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif

/**
 * @brief   GPTD3 driver identifier.
 */
#if LPC43XX_GPT_USE_TIMER2 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif

/**
 * @brief   GPTD4 driver identifier.
 */
#if LPC43XX_GPT_USE_TIMER3 || defined(__DOXYGEN__)
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

  gptp->tmr->IR = (1U << 0);                /* Clear interrupt on match MR0.*/
  if (gptp->state == GPT_ONESHOT) {
    gptp->state = GPT_READY;                /* Back in GPT_READY state.     */
    gpt_lld_stop_timer(gptp);               /* Timer automatically stopped. */
  }
  gptp->config->callback(gptp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if LPC43XX_GPT_USE_TIMER0
/**
 * @brief   TIMER0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Timer0_IRQHandler) {
  CH_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD1);
  CH_IRQ_EPILOGUE();
}
#endif /* LPC43XX_GPT_USE_TIMER0 */

#if LPC43XX_GPT_USE_TIMER1
/**
 * @brief   TIMER1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Timer1_IRQHandler) {
  CH_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD2);
  CH_IRQ_EPILOGUE();
}
#endif /* LPC43XX_GPT_USE_TIMER1 */

#if LPC43XX_GPT_USE_TIMER2
/**
 * @brief   TIMER2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Timer2_IRQHandler) {
  CH_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD3);
  CH_IRQ_EPILOGUE();
}
#endif /* LPC43XX_GPT_USE_TIMER2 */

#if LPC43XX_GPT_USE_TIMER3
/**
 * @brief   TIMER3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Timer3_IRQHandler) {
  CH_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD4);
  CH_IRQ_EPILOGUE();
}
#endif /* LPC43XX_GPT_USE_TIMER3 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

#if LPC43XX_GPT_USE_TIMER0
  /* Driver initialization.*/
  gptObjectInit(&GPTD1);
  GPTD1.tmr = LPC_TIMER0;
  GPTD1.resources = &timer0_resources;
#endif /* LPC43XX_GPT_USE_TIMER0 */

#if LPC43XX_GPT_USE_TIMER1
  /* Driver initialization.*/
  gptObjectInit(&GPTD2);
  GPTD2.tmr = LPC_TIMER1;
  GPTD2.resources = &timer1_resources;
#endif /* LPC43XX_GPT_USE_TIMER1 */

#if LPC43XX_GPT_USE_TIMER2
  /* Driver initialization.*/
  gptObjectInit(&GPTD3);
  GPTD3.tmr = LPC_TIMER2;
  GPTD3.resources = &timer2_resources;
#endif /* LPC43XX_GPT_USE_TIMER2 */

#if LPC43XX_GPT_USE_TIMER3
  /* Driver initialization.*/
  gptObjectInit(&GPTD4);
  GPTD4.tmr = LPC_TIMER3;
  GPTD4.resources = &timer3_resources;
#endif /* LPC43XX_GPT_USE_TIMER3 */
}

/**
 * @brief   Configures and activates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {

  if (gptp->state == GPT_STOP) {
    /* Enables the peripheral.*/
    branch_clock_enable(&gptp->resources->branch);
    peripheral_reset(&gptp->resources->reset);
    interrupt_enable(&gptp->resources->interrupt);
  }

  /* Timer configuration.*/
  gptp->tmr->PR  = gptp->config->pr;
  gptp->tmr->IR  = (1U << 0);
  gptp->tmr->MCR = 0; /* Disable all interrupt sources */
  gptp->tmr->TCR = 0; /* Disable counter */
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
    /* Resets the peripheral.*/
    gptp->tmr->MCR = 0;
    gptp->tmr->TCR = 0;

    interrupt_disable(&gptp->resources->interrupt);
    peripheral_reset(&gptp->resources->reset);
    branch_clock_disable(&gptp->resources->branch);
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

  gptp->tmr->MR[0] = interval - 1;
  gptp->tmr->IR    = 1;
  gptp->tmr->MCR   = 3;                       /* IRQ and clr TC on match MR0. */
  gptp->tmr->TCR   = 2;                       /* Reset counter and prescaler. */
  gptp->tmr->TCR   = 1;                       /* Timer enabled.               */
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

  gptp->tmr->MR[0] = interval - 1;
  gptp->tmr->IR    = 1;
  gptp->tmr->MCR   = 4;                       /* Stop TC on match MR0.        */
  gptp->tmr->TCR   = 2;                       /* Reset counter and prescaler. */
  gptp->tmr->TCR   = 1;                       /* Timer enabled.               */
  while (gptp->tmr->TCR & 1)
    ;
}

#endif /* HAL_USE_GPT */

/** @} */
