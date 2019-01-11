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
 * @file    LPC43xx_M0/hal_lld.c
 * @brief   LPC43xx M0 HAL subsystem low level driver source.
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
static halclock_t hal_clock_f = LPC43XX_M0_CLK_PLL1_AT_BOOT;

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

/* TODO: Expose RIT code, move elsewhere. */
static void ritimer_stop(void) {
  LPC_RITIMER->CTRL =
      (0 << 0)  /* RITINT */
    | (1 << 1)  /* RITENCLR */
    | (1 << 2)  /* RITENBR */
    | (0 << 3)  /* RITEN */
    ;
}

static void ritimer_start(void) {
  LPC_RITIMER->CTRL =
      (0 << 0)  /* RITINT */
    | (1 << 1)  /* RITENCLR */
    | (1 << 2)  /* RITENBR */
    | (1 << 3)  /* RITEN */
    ;
}

void systick_adjust_period(const uint32_t counts_per_tick) {
  ritimer_stop();
  LPC_RITIMER->COMPVAL = counts_per_tick;
  LPC_RITIMER->COUNTER = 0;
  ritimer_start();
}

void systick_stop() {
  ritimer_stop();
}

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {
  /* Initialize timer 3 to serve as a cycle (PCLK) counter. */
  LPC_CCU1->CLK_M4_TIMER3_CFG.AUTO = 1;
  LPC_CCU1->CLK_M4_TIMER3_CFG.RUN = 1;
  while(!LPC_CCU1->CLK_M4_TIMER3_STAT.RUN);
  LPC_TIMER3->TCR = (1 << 1); /* CRST=1 */
  LPC_TIMER3->TCR = 0;        /* CRST=0 */
  LPC_TIMER3->TC = 0;
  LPC_TIMER3->PR = 0;
  LPC_TIMER3->TCR = (1 << 0); /* CEN=1 */

  /* Initialize repetitive interrupt timer (RIT) to act like SysTick for
   * operating system process timing.
   */
  LPC_CCU1->CLK_M4_RITIMER_CFG.AUTO = 1;
  LPC_CCU1->CLK_M4_RITIMER_CFG.RUN = 1;
  while(!LPC_CCU1->CLK_M4_RITIMER_STAT.RUN);
  LPC_RITIMER->CTRL =
      (1 << 0)  /* RITINT */
    | (1 << 1)  /* RITENCLR */
    | (1 << 2)  /* RITENBR */
    | (0 << 3)  /* RITEN */
    ;
  LPC_RITIMER->MASK = 0;
  systick_adjust_period(LPC43XX_M0_CLK_PLL1_AT_BOOT / CH_FREQUENCY - 1);
  
  nvicEnableVector(RITIMER_OR_WWDT_IRQn, CORTEX_PRIORITY_MASK(CORTEX_PRIORITY_SYSTICK));
}

/* Work-around to use RITimer in place of SysTick, which isn't available on
 * the LPC43xx M0 core.
 */
CH_IRQ_HANDLER(RITimer_Or_WWDT_IRQHandler) {
  /* Same code as in SysTickVector */
  CH_IRQ_PROLOGUE();

  chSysLockFromIsr();
  chSysTimerHandlerI();
  chSysUnlockFromIsr();

  LPC_RITIMER->CTRL =
      (1 << 0)  /* RITINT */
    | (1 << 1)  /* RITENCLR */
    | (1 << 2)  /* RITENBR */
    | (1 << 3)  /* RITEN */
    ;

  CH_IRQ_EPILOGUE();
}

/** @} */
