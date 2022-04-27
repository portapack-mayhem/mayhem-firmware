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
 * @file    LPC43xx_M4/hal_lld.c
 * @brief   LPC43xx M4 HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/* TODO: Somehow share this value between the M4 and M0 cores. The M0 always
 * runs at the same speed as the M4 core.
 */
static halclock_t hal_clock_f = LPC43XX_M4_CLK;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

halclock_t halLPCGetSystemClock(void) {
  return hal_clock_f;
}

void halLPCSetSystemClock(const halclock_t new_frequency) {
  hal_clock_f = new_frequency;
}

void systick_stop() {
  SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
}

void systick_adjust_period(const uint32_t counts_per_tick) {
  SysTick->LOAD = counts_per_tick;
}

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {
#if defined(LPC43XX_M4_CLK_SRC)
  LPC_CGU->BASE_M4_CLK.AUTOBLOCK = 1;
  LPC_CGU->BASE_M4_CLK.CLK_SEL = LPC43XX_M4_CLK_SRC;
#endif

  /* SysTick initialization using the system clock.*/
  systick_adjust_period(halLPCGetSystemClock() / CH_FREQUENCY - 1);
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;

  /* DWT cycle counter enable.*/
  SCS_DEMCR |= SCS_DEMCR_TRCENA;
  DWT_CTRL  |= DWT_CTRL_CYCCNTENA;
}

/** @} */
