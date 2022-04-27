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
 * @file    AT91SAM7/hal_lld.c
 * @brief   AT91SAM7 HAL subsystem low level driver source.
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

static CH_IRQ_HANDLER(spurious_handler) {

  CH_IRQ_PROLOGUE();

  AT91SAM7_SPURIOUS_HANDLER_HOOK();

  AT91C_BASE_AIC->AIC_EOICR = 0;

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {
  unsigned i;

  /* FIQ Handler weak symbol defined in vectors.s.*/
  void FiqHandler(void);

  /* Default AIC setup, the device drivers will modify it as needed.*/
  AT91C_BASE_AIC->AIC_ICCR = 0xFFFFFFFF;
  AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF;
  AT91C_BASE_AIC->AIC_SVR[0] = (AT91_REG)FiqHandler;
  for (i = 1; i < 31; i++) {
    AT91C_BASE_AIC->AIC_SVR[i] = (AT91_REG)NULL;
    AT91C_BASE_AIC->AIC_EOICR = (AT91_REG)i;
  }
  AT91C_BASE_AIC->AIC_SPU  = (AT91_REG)spurious_handler;

}

/**
 * @brief   AT91SAM7 clocks and PLL initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function must be invoked only after the system reset.
 *
 * @special
 */
void at91sam7_clock_init(void) {

  /* wait for reset */
  while((AT91C_BASE_RSTC->RSTC_RSR & (AT91C_RSTC_SRCMP | AT91C_RSTC_NRSTL)) != AT91C_RSTC_NRSTL)
    ;
  /* enable reset */
  AT91C_BASE_RSTC->RSTC_RMR = ((0xA5 << 24) | AT91C_RSTC_URSTEN);

  /* Flash Memory: 1 wait state, about 50 cycles in a microsecond.*/
#if SAM7_PLATFORM == SAM7X512
  AT91C_BASE_MC->MC0_FMR = (AT91C_MC_FMCN & (50 << 16)) | AT91C_MC_FWS_1FWS;
  AT91C_BASE_MC->MC1_FMR = (AT91C_MC_FMCN & (50 << 16)) | AT91C_MC_FWS_1FWS;
#else
  AT91C_BASE_MC->MC_FMR = (AT91C_MC_FMCN & (50 << 16)) | AT91C_MC_FWS_1FWS;
#endif

  /* Enables the main oscillator and waits 56 slow cycles as startup time.*/
  AT91C_BASE_PMC->PMC_MOR = (AT91C_CKGR_OSCOUNT & (7 << 8)) | AT91C_CKGR_MOSCEN;
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCS))
    ;

  /* PLL setup: DIV = 14, MUL = 72, PLLCOUNT = 10
     PLLfreq = 96109714 Hz (rounded).*/
  AT91C_BASE_PMC->PMC_PLLR = (AT91C_CKGR_DIV & 14) |
                             (AT91C_CKGR_PLLCOUNT & (10 << 8)) |
                             (AT91SAM7_USBDIV) |
                             (AT91C_CKGR_MUL & (72 << 16));
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCK))
    ;

  /* Master clock = PLLfreq / 2 = 48054858 Hz (rounded).*/
  AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_PRES_CLK_2;
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY))
    ;

  AT91C_BASE_PMC->PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK;
  while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY))
    ;
}

/** @} */
