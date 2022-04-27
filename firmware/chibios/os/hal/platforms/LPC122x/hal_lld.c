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
 * @file    LPC122x/hal_lld.c
 * @brief   LPC122x HAL subsystem low level driver source.
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
/* Driver local variables and types.                                         */
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
  SysTick->LOAD = LPC122x_SYSCLK / CH_FREQUENCY - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;
}

/**
 * @brief   LPC122x clocks and PLL initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function must be invoked only after the system reset.
 *
 * @special
 */
void lpc122x_clock_init(void) {
  unsigned i;

  LPC_WWDT->MOD = 0;                          /* Disable Watchdog */
  LPC_WWDT->FEED = 0xAA;
  LPC_WWDT->FEED = 0x55;

#if LPC122x_FLASHCFG_FLASHTIM == 0
  LPC_SYSCON->PRESETCTRL |= (1 << 15);         /* Flash 1-cycle read mode */
#else
  LPC_FLASHCTRL->FLASHCFG = 0;                 /* 2 system clock cycles flash access time */
  LPC_SYSCON->PRESETCTRL &= ~(1 << 15);        /* Flash multi-cycle read mode */
#endif

  /* System oscillator initialization if required.*/
#if LPC122x_MAINCLK_SOURCE == SYSMAINCLKSEL_PLLOUT
#if LPC122x_PLLCLK_SOURCE == SYSPLLCLKSEL_SYSOSC
  LPC_SYSCON->SYSOSCCTRL = LPC122x_SYSOSCCTRL;
  LPC_SYSCON->PDRUNCFG &= ~(1 << 5);            /* System oscillator ON.    */
  for (i = 0; i < 200; i++)
    __NOP();                                    /* Stabilization delay.     */
#endif /* LPC122x_PLLCLK_SOURCE == SYSPLLCLKSEL_SYSOSC */

  /* PLL initialization if required.*/
  LPC_SYSCON->SYSPLLCLKSEL = LPC122x_PLLCLK_SOURCE;
  LPC_SYSCON->SYSPLLCLKUEN = 1;                 /* Really required?         */
  LPC_SYSCON->SYSPLLCLKUEN = 0;
  LPC_SYSCON->SYSPLLCLKUEN = 1;
  LPC_SYSCON->SYSPLLCTRL = LPC122x_SYSPLLCTRL_MSEL | LPC122x_SYSPLLCTRL_PSEL;
  LPC_SYSCON->PDRUNCFG &= ~(1 << 7);            /* System PLL ON.           */
  while ((LPC_SYSCON->SYSPLLSTAT & 1) == 0)     /* Wait PLL lock.           */
    ;
#endif /* LPC122x_MAINCLK_SOURCE == SYSMAINCLKSEL_PLLOUT */

  /* Main clock source selection.*/
  LPC_SYSCON->MAINCLKSEL = LPC122x_MAINCLK_SOURCE;
  LPC_SYSCON->MAINCLKUEN = 1;                   /* Really required?         */
  LPC_SYSCON->MAINCLKUEN = 0;
  LPC_SYSCON->MAINCLKUEN = 1;
  while ((LPC_SYSCON->MAINCLKUEN & 1) == 0)     /* Wait switch completion.  */
    ;

  /* ABH divider initialization, peripheral clocks are initially disabled,
     the various device drivers will handle their own setup except GPIO and
     IOCON that are left enabled.*/
  LPC_SYSCON->SYSAHBCLKDIV = LPC122x_SYSABHCLK_DIV;
  LPC_SYSCON->SYSAHBCLKCTRL = 0xE009001F;

  /* Memory remapping, vectors always in ROM.*/
  LPC_SYSCON->SYSMEMREMAP = 2;
}

/** @} */
