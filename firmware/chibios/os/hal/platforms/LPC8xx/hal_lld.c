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
 * @file    LPC8xx/hal_lld.c
 * @brief   LPC8xx HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {

  /* SysTick initialization using the system clock.*/
  nvicSetSystemHandlerPriority(HANDLER_SYSTICK, CORTEX_PRIORITY_SYSTICK);
  SysTick->LOAD = LPC8xx_SYSCLK / CH_FREQUENCY - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;
}

/**
 * @brief   LPC8xx clocks and PLL initialization.
 * @note    This function must be invoked only after the system reset.
 *
 * @special
 */
void lpc8xx_clock_init(void) {
  int i;

  /* Enable clocks to IOCON & SWM. */
  LPC_SYSCON->SYSAHBCLKCTRL |= ((1<<18)|(1<<7));

  /* System oscillator initialization.*/
#if (LPC8xx_PLLCLK_SOURCE == SYSPLLCLKSEL_SYSOSC)
  // switch off pull-ups
  LPC_IOCON->PIO0_8   &= ~(3<<3);
  LPC_IOCON->PIO0_9   &= ~(3<<3);

  // enable xtalin/xtalout
  LPC_SWM->PINENABLE0 &= ~(3<<4);

  LPC_SYSCON->SYSOSCCTRL = LPC8xx_SYSOSCCTRL;
  LPC_SYSCON->PDRUNCFG &= ~(1<<5);              /* System oscillator ON.    */
  for (i = 0; i<200; i++)
    __NOP();                                    /* Stabilization delay.     */
#endif

  /* CLKIN initialization.*/
#if (LPC8xx_PLLCLK_SOURCE == SYSPLLCLKSEL_CLKIN)
  // switch off pull-up
  LPC_IOCON->PIO0_1   &= ~(3<<3);

  // enable clkin
  LPC_SWM->PINENABLE0 &= ~(1<<7);
#endif

  /* Always set PLL clock source - 
     PLL IN can be a main clock source */
  LPC_SYSCON->SYSPLLCLKSEL = LPC8xx_PLLCLK_SOURCE;
  LPC_SYSCON->SYSPLLCLKUEN = 1;
  while (!(LPC_SYSCON->SYSPLLCLKUEN & 0x01));     /* Wait Until Updated       */

  /* PLL initialization.*/
#if LPC8xx_MAINCLK_SOURCE == SYSMAINCLKSEL_PLLOUT
  LPC_SYSCON->SYSPLLCTRL = LPC8xx_SYSPLLCTRL_MSEL | LPC8xx_SYSPLLCTRL_PSEL;
  LPC_SYSCON->PDRUNCFG &= ~(1<<7);            /* System PLL ON.           */
  while (!(LPC_SYSCON->SYSPLLSTAT & 1))      /* Wait PLL lock.           */
    ;
#endif

#if LPC8xx_MAINCLK_SOURCE == SYSMAINCLKSEL_WDGOSC
#error "WatchDog Oscillator not configured! Dont use as main clock source"
  LPC_SYSCON->WDTOSCCTRL = ??;
  LPC_SYSCON->PDRUNCFG  &= ~(1<<6);        /* WDT OSC On       */
  for (i = 0; i<200; i++)
    __NOP();                                    /* Stabilization delay.     */
#endif

  /* Flash wait states setting, the code takes care to not touch TBD bits.*/
  LPC_FLASHCTRL->FLASHCFG = (LPC_FLASHCTRL->FLASHCFG & ~1) |
                              LPC8xx_FLASHCFG_FLASHTIM;

  /* ABH divider initialization. Set this **before** switching Main clock
     source to ensure AHB clock stays in spec */
  LPC_SYSCON->SYSAHBCLKDIV = LPC8xx_SYSABHCLK_DIV;

  /* Main clock source selection.*/
  LPC_SYSCON->MAINCLKSEL = LPC8xx_MAINCLK_SOURCE;
  LPC_SYSCON->MAINCLKUEN = 1;
  while (!(LPC_SYSCON->MAINCLKUEN & 1));     /* Wait switch completion.  */

  /* Disable clocks to IOCON, SWM, FLASHREG & ROM. */
  LPC_SYSCON->SYSAHBCLKCTRL &= ~((1<<18)|(1<<7)|(1<<3)|(1<<1));

}

/** @} */
