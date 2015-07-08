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
 * @file    STM32L1xx/hal_lld.c
 * @brief   STM32L1xx HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

/* TODO: LSEBYP like in F3.*/

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

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Initializes the backup domain.
 */
static void hal_lld_backup_domain_init(void) {

  /* Backup domain access enabled and left open.*/
  PWR->CR |= PWR_CR_DBP;

  /* Reset BKP domain if different clock source selected.*/
  if ((RCC->CSR & STM32_RTCSEL_MASK) != STM32_RTCSEL){
    /* Backup domain reset.*/
    RCC->CSR |= RCC_CSR_RTCRST;
    RCC->CSR &= ~RCC_CSR_RTCRST;
  }

  /* If enabled then the LSE is started.*/
#if STM32_LSE_ENABLED
  RCC->CSR |= RCC_CSR_LSEON;
  while ((RCC->CSR & RCC_CSR_LSERDY) == 0)
    ;                                     /* Waits until LSE is stable.   */
#endif

#if STM32_RTCSEL != STM32_RTCSEL_NOCLOCK
  /* If the backup domain hasn't been initialized yet then proceed with
     initialization.*/
  if ((RCC->CSR & RCC_CSR_RTCEN) == 0) {
    /* Selects clock source.*/
    RCC->CSR |= STM32_RTCSEL;

    /* RTC clock enabled.*/
    RCC->CSR |= RCC_CSR_RTCEN;
  }
#endif /* STM32_RTCSEL != STM32_RTCSEL_NOCLOCK */
}

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

  /* Reset of all peripherals.*/
  rccResetAHB(~RCC_AHBRSTR_FLITFRST);
  rccResetAPB1(~RCC_APB1RSTR_PWRRST);
  rccResetAPB2(~0);

  /* SysTick initialization using the system clock.*/
  SysTick->LOAD = STM32_HCLK / CH_FREQUENCY - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;

  /* DWT cycle counter enable.*/
  SCS_DEMCR |= SCS_DEMCR_TRCENA;
  DWT_CTRL  |= DWT_CTRL_CYCCNTENA;

  /* PWR clock enabled.*/
  rccEnablePWRInterface(FALSE);

  /* Initializes the backup domain.*/
  hal_lld_backup_domain_init();

#if defined(STM32_DMA_REQUIRED)
  dmaInit();
#endif

  /* Programmable voltage detector enable.*/
#if STM32_PVD_ENABLE
  PWR->CR |= PWR_CR_PVDE | (STM32_PLS & STM32_PLS_MASK);
#endif /* STM32_PVD_ENABLE */
}

/**
 * @brief   STM32L1xx voltage, clocks and PLL initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function should be invoked just after the system reset.
 *
 * @special
 */
/**
 * @brief   Clocks and internal voltage initialization.
 */
void stm32_clock_init(void) {

#if !STM32_NO_INIT
  /* PWR clock enable.*/
  RCC->APB1ENR = RCC_APB1ENR_PWREN;

  /* Core voltage setup.*/
  while ((PWR->CSR & PWR_CSR_VOSF) != 0)
    ;                           /* Waits until regulator is stable.         */
  PWR->CR = STM32_VOS;
  while ((PWR->CSR & PWR_CSR_VOSF) != 0)
    ;                           /* Waits until regulator is stable.         */

  /* Initial clocks setup and wait for MSI stabilization, the MSI clock is
     always enabled because it is the fallback clock when PLL the fails.
     Trim fields are not altered from reset values.*/
  RCC->CFGR  = 0;
  RCC->ICSCR = (RCC->ICSCR & ~STM32_MSIRANGE_MASK) | STM32_MSIRANGE;
  RCC->CR    = RCC_CR_MSION;
  while ((RCC->CR & RCC_CR_MSIRDY) == 0)
    ;                           /* Waits until MSI is stable.               */

#if STM32_HSI_ENABLED
  /* HSI activation.*/
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0)
    ;                           /* Waits until HSI is stable.               */
#endif

#if STM32_HSE_ENABLED
#if defined(STM32_HSE_BYPASS)
  /* HSE Bypass.*/
  RCC->CR |= RCC_CR_HSEBYP;
#endif
  /* HSE activation.*/
  RCC->CR |= RCC_CR_HSEON;
  while ((RCC->CR & RCC_CR_HSERDY) == 0)
    ;                           /* Waits until HSE is stable.               */
#endif

#if STM32_LSI_ENABLED
  /* LSI activation.*/
  RCC->CSR |= RCC_CSR_LSION;
  while ((RCC->CSR & RCC_CSR_LSIRDY) == 0)
    ;                           /* Waits until LSI is stable.               */
#endif

#if STM32_LSE_ENABLED
  /* LSE activation, have to unlock the register.*/
  if ((RCC->CSR & RCC_CSR_LSEON) == 0) {
    PWR->CR |= PWR_CR_DBP;
    RCC->CSR |= RCC_CSR_LSEON;
    PWR->CR &= ~PWR_CR_DBP;
  }
  while ((RCC->CSR & RCC_CSR_LSERDY) == 0)
    ;                           /* Waits until LSE is stable.               */
#endif

#if STM32_ACTIVATE_PLL
  /* PLL activation.*/
  RCC->CFGR |= STM32_PLLDIV | STM32_PLLMUL | STM32_PLLSRC;
  RCC->CR   |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY))
    ;                           /* Waits until PLL is stable.               */
#endif

  /* Other clock-related settings (dividers, MCO etc).*/
  RCC->CR   |= STM32_RTCPRE;
  RCC->CFGR |= STM32_MCOPRE | STM32_MCOSEL |
               STM32_PPRE2 | STM32_PPRE1 | STM32_HPRE;
  RCC->CSR  |= STM32_RTCSEL;

  /* Flash setup and final clock selection.*/
#if defined(STM32_FLASHBITS1)
  FLASH->ACR = STM32_FLASHBITS1;
#endif
#if defined(STM32_FLASHBITS2)
  FLASH->ACR = STM32_FLASHBITS2;
#endif

  /* Switching to the configured clock source if it is different from MSI.*/
#if (STM32_SW != STM32_SW_MSI)
  RCC->CFGR |= STM32_SW;        /* Switches on the selected clock source.   */
  while ((RCC->CFGR & RCC_CFGR_SWS) != (STM32_SW << 2))
    ;
#endif
#endif /* STM32_NO_INIT */

  /* SYSCFG clock enabled here because it is a multi-functional unit shared
     among multiple drivers.*/
  rccEnableAPB2(RCC_APB2ENR_SYSCFGEN, TRUE);
}

/** @} */
