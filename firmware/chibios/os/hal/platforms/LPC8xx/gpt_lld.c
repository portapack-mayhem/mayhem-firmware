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
 * @file    LPC8xx/gpt_lld.c
 * @brief   LPC8xx GPT subsystem low level driver source.
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
 * @note    The driver GPT1 allocates MRT channel0 when enabled.
 */
#if LPC8xx_GPT_USE_MRT0 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif

/**
 * @brief   GPT2 driver identifier.
 * @note    The driver GPT1 allocates MRT channel1 when enabled.
 */
#if LPC8xx_GPT_USE_MRT1 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif

/**
 * @brief   GPT3 driver identifier.
 * @note    The driver GPT1 allocates MRT channel2 when enabled.
 */
#if LPC8xx_GPT_USE_MRT2 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif

/**
 * @brief   GPT4 driver identifier.
 * @note    The driver GPT1 allocates MRT channel3 when enabled.
 */
#if LPC8xx_GPT_USE_MRT3 || defined(__DOXYGEN__)
GPTDriver GPTD4;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/
static uint32_t clk_enabled;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
/**
 * @brief   Shared IRQ handler.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @param[in] irq_flag  irq flag bit
 */

static void gpt_lld_serve_interrupt( GPTDriver *gptp ) {

  if (gptp->tmr->STAT & 0x01) {
    gptp->tmr->STAT |= 0x01;

    if (gptp->state == GPT_ONESHOT) {
      gptp->state = GPT_READY;                /* Back in GPT_READY state.     */
    }
    gptp->config->callback(gptp);
  }
  return;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**

 * @brief   MRT IRQ handler.
 *
 */
CH_IRQ_HANDLER(Vector68) {
  CH_IRQ_PROLOGUE();
  
#if LPC8xx_GPT_USE_MRT0
  gpt_lld_serve_interrupt( &GPTD1 );
#endif
  
#if LPC8xx_GPT_USE_MRT1
  gpt_lld_serve_interrupt( &GPTD2 );
#endif

#if LPC8xx_GPT_USE_MRT2
  gpt_lld_serve_interrupt( &GPTD3 );
#endif

#if LPC8xx_GPT_USE_MRT3
  gpt_lld_serve_interrupt( &GPTD4 );
#endif

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

#if LPC8xx_GPT_USE_MRT0
  GPTD1.tmr = &(LPC_MRT->Channel[0]);
  gptObjectInit(&GPTD1);
  GPTD1.mask = (1<<0);
#endif

#if LPC8xx_GPT_USE_MRT1
  GPTD2.tmr = &(LPC_MRT->Channel[1]);
  gptObjectInit(&GPTD2);
  GPTD1.mask = (1<<1);
#endif

#if LPC8xx_GPT_USE_MRT2
  GPTD3.tmr = &(LPC_MRT->Channel[2]);
  gptObjectInit(&GPTD3);
  GPTD1.mask = (1<<2);
#endif

#if LPC8xx_GPT_USE_MRT3
  GPTD4.tmr = &(LPC_MRT->Channel[3]);
  gptObjectInit(&GPTD4);
  GPTD1.mask = (1<<3);
#endif

  clk_enabled = FALSE;
  return;
}

/**
 * @brief   Configures and activates a GPT channel.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {

  if( !clk_enabled ) {

    /* Enable clock & reset MRT */
    LPC_SYSCON->SYSAHBCLKCTRL |=  (1<<10);
    LPC_SYSCON->PRESETCTRL    &= ~(1<<7);
    LPC_SYSCON->PRESETCTRL    |=  (1<<7);

    nvicEnableVector(MRT_IRQn,
                     CORTEX_PRIORITY_MASK(LPC8xx_GPT_MRT_IRQ_PRIORITY));

    clk_enabled |= gptp->mask;
  }

  /* Prescaler value calculation.*/
  gptp->pr = (LPC8xx_SYSCLK / gptp->config->frequency);
  chDbgAssert((gptp->pr * gptp->config->frequency) == LPC8xx_SYSCLK,
              "gpt_lld_start(), #1", "invalid frequency");

  /* MRT Channel configuration.*/
  gptp->tmr->CTRL  = 0;
  gptp->tmr->STAT |= 1;

}

/**
 * @brief   Deactivates a GPT channel.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop(GPTDriver *gptp) {

  /* Shared peripheral - 
     mark this channel as disabled */
  clk_enabled &= ~gptp->mask;

  /* All channels disabled? */
  if( !clk_enabled )
  {
    /* Disable periheral */
    nvicDisableVector(MRT_IRQn);
    LPC_SYSCON->SYSAHBCLKCTRL &=  ~(1<<10);
  }
    
  return;
}

/**
 * @brief   Starts the timer in continuous/One shot mode.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  period in ticks
 *
 * @notapi
 */
void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval) {

  gptp->tmr->INTVAL = (1<<31)|((interval*gptp->pr) - 1);

  if (gptp->state == GPT_ONESHOT)
    gptp->tmr->CTRL = (1<<1)|1;
  else
    gptp->tmr->CTRL = 1;
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {

  gptp->tmr->INTVAL = (1<<31);
  gptp->tmr->CTRL   = 0;
  gptp->tmr->STAT  |= 1;
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

  gptp->tmr->INTVAL = (1<<31)|((interval*gptp->pr) - 1);
  gptp->tmr->CTRL = (1<<1);

  while (gptp->tmr->STAT & (1<<1))
    ;

  gptp->tmr->CTRL   = 0;
  gptp->tmr->STAT  |= 1;
}

#endif /* HAL_USE_GPT */

/** @} */
