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
 * @file    STM32F0xx/hal_lld.h
 * @brief   STM32F0xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSEDRV.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F030 for Value Line devices.
 *          - STM32F0XX_LD for Low Density devices.
 *          - STM32F0XX_MD for Medium Density devices.
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "stm32.h"
#include "stm32_registry.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS FALSE

/**
 * @name    Platform identification macros
 * @{
 */
#if defined(STM32F0XX_MD) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32F051xx/F061xx Entry Level Medium Density devices"
#define STM32F0XX

#elif defined(STM32F0XX_LD)
#define PLATFORM_NAME           "STM32F050xx/F060xx Entry Level Low Density devices"
#define STM32F0XX

#elif defined(STM32F030)
#define PLATFORM_NAME           "STM32F030xx Entry Level Value Line devices"
#define STM32F0XX

#else
#error "STM32F0xx device not specified"
#endif
/** @} */

/**
 * @name    Absolute Maximum Ratings
 * @{
 */
/**
 * @brief   Maximum system clock frequency.
 */
#define STM32_SYSCLK_MAX        48000000

/**
 * @brief   Maximum HSE clock frequency.
 */
#define STM32_HSECLK_MAX        32000000

/**
 * @brief   Minimum HSE clock frequency.
 */
#define STM32_HSECLK_MIN        1000000

/**
 * @brief   Maximum LSE clock frequency.
 */
#define STM32_LSECLK_MAX        1000000

/**
 * @brief   Minimum LSE clock frequency.
 */
#define STM32_LSECLK_MIN        32768

/**
 * @brief   Maximum PLLs input clock frequency.
 */
#define STM32_PLLIN_MAX         25000000

/**
 * @brief   Minimum PLLs input clock frequency.
 */
#define STM32_PLLIN_MIN         1000000

/**
 * @brief   Maximum PLL output clock frequency.
 */
#define STM32_PLLOUT_MAX        48000000

/**
 * @brief   Minimum PLL output clock frequency.
 */
#define STM32_PLLOUT_MIN        16000000

/**
 * @brief   Maximum APB clock frequency.
 */
#define STM32_PCLK_MAX          48000000

/**
 * @brief   Maximum ADC clock frequency.
 */
#define STM32_ADCCLK_MAX        14000000
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define STM32_HSICLK            8000000     /**< High speed internal clock. */
#define STM32_HSI14CLK          14000000    /**< 14MHz speed internal clock.*/
#define STM32_LSICLK            40000       /**< Low speed internal clock.  */
/** @} */

/**
 * @name    PWR_CR register bits definitions
 * @{
 */
#define STM32_PLS_MASK          (7 << 5)    /**< PLS bits mask.             */
#define STM32_PLS_LEV0          (0 << 5)    /**< PVD level 0.               */
#define STM32_PLS_LEV1          (1 << 5)    /**< PVD level 1.               */
#define STM32_PLS_LEV2          (2 << 5)    /**< PVD level 2.               */
#define STM32_PLS_LEV3          (3 << 5)    /**< PVD level 3.               */
#define STM32_PLS_LEV4          (4 << 5)    /**< PVD level 4.               */
#define STM32_PLS_LEV5          (5 << 5)    /**< PVD level 5.               */
#define STM32_PLS_LEV6          (6 << 5)    /**< PVD level 6.               */
#define STM32_PLS_LEV7          (7 << 5)    /**< PVD level 7.               */
/** @} */

/**
 * @name    RCC_CFGR register bits definitions
 * @{
 */
#define STM32_SW_HSI            (0 << 0)    /**< SYSCLK source is HSI.      */
#define STM32_SW_HSE            (1 << 0)    /**< SYSCLK source is HSE.      */
#define STM32_SW_PLL            (2 << 0)    /**< SYSCLK source is PLL.      */

#define STM32_HPRE_DIV1         (0 << 4)    /**< SYSCLK divided by 1.       */
#define STM32_HPRE_DIV2         (8 << 4)    /**< SYSCLK divided by 2.       */
#define STM32_HPRE_DIV4         (9 << 4)    /**< SYSCLK divided by 4.       */
#define STM32_HPRE_DIV8         (10 << 4)   /**< SYSCLK divided by 8.       */
#define STM32_HPRE_DIV16        (11 << 4)   /**< SYSCLK divided by 16.      */
#define STM32_HPRE_DIV64        (12 << 4)   /**< SYSCLK divided by 64.      */
#define STM32_HPRE_DIV128       (13 << 4)   /**< SYSCLK divided by 128.     */
#define STM32_HPRE_DIV256       (14 << 4)   /**< SYSCLK divided by 256.     */
#define STM32_HPRE_DIV512       (15 << 4)   /**< SYSCLK divided by 512.     */

#define STM32_PPRE_DIV1         (0 << 8)    /**< HCLK divided by 1.         */
#define STM32_PPRE_DIV2         (4 << 8)    /**< HCLK divided by 2.         */
#define STM32_PPRE_DIV4         (5 << 8)    /**< HCLK divided by 4.         */
#define STM32_PPRE_DIV8         (6 << 8)    /**< HCLK divided by 8.         */
#define STM32_PPRE_DIV16        (7 << 8)    /**< HCLK divided by 16.        */

#define STM32_ADCPRE_DIV2       (0 << 14)   /**< PCLK divided by 2.         */
#define STM32_ADCPRE_DIV4       (1 << 14)   /**< PCLK divided by 4.         */

#define STM32_PLLSRC_HSI        (0 << 16)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 16)   /**< PLL clock source is HSE.   */

#define STM32_MCOSEL_NOCLOCK    (0 << 24)   /**< No clock on MCO pin.       */
#define STM32_MCOSEL_HSI14      (3 << 24)   /**< HSI14 clock on MCO pin.    */
#define STM32_MCOSEL_SYSCLK     (4 << 24)   /**< SYSCLK on MCO pin.         */
#define STM32_MCOSEL_HSI        (5 << 24)   /**< HSI clock on MCO pin.      */
#define STM32_MCOSEL_HSE        (6 << 24)   /**< HSE clock on MCO pin.      */
#define STM32_MCOSEL_PLLDIV2    (7 << 24)   /**< PLL/2 clock on MCO pin.    */
/** @} */

/**
 * @name    RCC_BDCR register bits definitions
 * @{
 */
#define STM32_RTCSEL_MASK       (3 << 8)    /**< RTC clock source mask.     */
#define STM32_RTCSEL_NOCLOCK    (0 << 8)    /**< No clock.                  */
#define STM32_RTCSEL_LSE        (1 << 8)    /**< LSE used as RTC clock.     */
#define STM32_RTCSEL_LSI        (2 << 8)    /**< LSI used as RTC clock.     */
#define STM32_RTCSEL_HSEDIV     (3 << 8)    /**< HSE divided by 32 used as
                                                 RTC clock.                 */
/** @} */

/**
 * @name    RCC_CFGR3 register bits definitions
 * @{
 */
#define STM32_USART1SW_MASK     (3 << 0)    /**< USART1 clock source mask.  */
#define STM32_USART1SW_PCLK     (0 << 0)    /**< USART1 clock is PCLK.      */
#define STM32_USART1SW_SYSCLK   (1 << 0)    /**< USART1 clock is SYSCLK.    */
#define STM32_USART1SW_LSE      (2 << 0)    /**< USART1 clock is LSE.       */
#define STM32_USART1SW_HSI      (3 << 0)    /**< USART1 clock is HSI.       */
#define STM32_I2C1SW_MASK       (1 << 4)    /**< I2C clock source mask.     */
#define STM32_I2C1SW_HSI        (0 << 4)    /**< I2C clock is HSI.          */
#define STM32_I2C1SW_SYSCLK     (1 << 4)    /**< I2C clock is SYSCLK.       */
#define STM32_CECSW_MASK        (1 << 6)    /**< CEC clock source mask.     */
#define STM32_CECSW_HSI         (0 << 6)    /**< CEC clock is HSI/244.      */
#define STM32_CECSW_LSE         (1 << 6)    /**< CEC clock is LSE.          */
#define STM32_ADCSW_MASK        (1 << 8)    /**< ADC clock source mask.     */
#define STM32_ADCSW_HSI14       (0 << 8)    /**< ADC clock is HSI14.        */
#define STM32_ADCSW_PCLK        (1 << 8)    /**< ADC clock is PCLK/2|4.     */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Disables the PWR/RCC initialization in the HAL.
 */
#if !defined(STM32_NO_INIT) || defined(__DOXYGEN__)
#define STM32_NO_INIT               FALSE
#endif

/**
 * @brief   Enables or disables the programmable voltage detector.
 */
#if !defined(STM32_PVD_ENABLE) || defined(__DOXYGEN__)
#define STM32_PVD_ENABLE            FALSE
#endif

/**
 * @brief   Sets voltage level for programmable voltage detector.
 */
#if !defined(STM32_PLS) || defined(__DOXYGEN__)
#define STM32_PLS                   STM32_PLS_LEV0
#endif

/**
 * @brief   Enables or disables the HSI clock source.
 */
#if !defined(STM32_HSI_ENABLED) || defined(__DOXYGEN__)
#define STM32_HSI_ENABLED           TRUE
#endif

/**
 * @brief   Enables or disables the HSI14 clock source.
 */
#if !defined(STM32_HSI14_ENABLED) || defined(__DOXYGEN__)
#define STM32_HSI14_ENABLED         TRUE
#endif

/**
 * @brief   Enables or disables the LSI clock source.
 */
#if !defined(STM32_LSI_ENABLED) || defined(__DOXYGEN__)
#define STM32_LSI_ENABLED           FALSE
#endif

/**
 * @brief   Enables or disables the HSE clock source.
 */
#if !defined(STM32_HSE_ENABLED) || defined(__DOXYGEN__)
#define STM32_HSE_ENABLED           TRUE
#endif

/**
 * @brief   Enables or disables the LSE clock source.
 */
#if !defined(STM32_LSE_ENABLED) || defined(__DOXYGEN__)
#define STM32_LSE_ENABLED           FALSE
#endif

/**
 * @brief   Main clock source selection.
 * @note    If the selected clock source is not the PLL then the PLL is not
 *          initialized and started.
 * @note    The default value is calculated for a 48MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_SW) || defined(__DOXYGEN__)
#define STM32_SW                    STM32_SW_PLL
#endif

/**
 * @brief   Clock source for the PLL.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 48MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_PLLSRC) || defined(__DOXYGEN__)
#define STM32_PLLSRC                STM32_PLLSRC_HSE
#endif

/**
 * @brief   Crystal PLL pre-divider.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 72MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_PREDIV_VALUE) || defined(__DOXYGEN__)
#define STM32_PREDIV_VALUE                  1
#endif

/**
 * @brief   PLL multiplier value.
 * @note    The allowed range is 2...16.
 * @note    The default value is calculated for a 48MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_PLLMUL_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLMUL_VALUE          6
#endif

/**
 * @brief   AHB prescaler value.
 * @note    The default value is calculated for a 48MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_HPRE) || defined(__DOXYGEN__)
#define STM32_HPRE                  STM32_HPRE_DIV1
#endif

/**
 * @brief   APB1 prescaler value.
 */
#if !defined(STM32_PPRE) || defined(__DOXYGEN__)
#define STM32_PPRE                  STM32_PPRE_DIV1
#endif

/**
 * @brief   MCO pin setting.
 */
#if !defined(STM32_MCOSEL) || defined(__DOXYGEN__)
#define STM32_MCOSEL                STM32_MCOSEL_NOCLOCK
#endif

/**
 * @brief   ADC prescaler value.
 */
#if !defined(STM32_ADCPRE) || defined(__DOXYGEN__)
#define STM32_ADCPRE                STM32_ADCPRE_DIV4
#endif

/**
 * @brief   ADC clock source.
 */
#if !defined(STM32_ADCSW) || defined(__DOXYGEN__)
#define STM32_ADCSW                 STM32_ADCSW_HSI14
#endif

/**
 * @brief   CEC clock source.
 */
#if !defined(STM32_CECSW) || defined(__DOXYGEN__)
#define STM32_CECSW                 STM32_CECSW_HSI
#endif

/**
 * @brief   I2C1 clock source.
 */
#if !defined(STM32_I2C1SW) || defined(__DOXYGEN__)
#define STM32_I2C1SW                STM32_I2C1SW_HSI
#endif

/**
 * @brief   USART1 clock source.
 */
#if !defined(STM32_USART1SW) || defined(__DOXYGEN__)
#define STM32_USART1SW              STM32_USART1SW_PCLK
#endif

/**
 * @brief   RTC clock source.
 */
#if !defined(STM32_RTCSEL) || defined(__DOXYGEN__)
#define STM32_RTCSEL                STM32_RTCSEL_LSI
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Configuration-related checks.
 */
#if !defined(STM32F0xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32F0xx_MCUCONF not defined"
#endif

/*
 * HSI related checks.
 */
#if STM32_HSI_ENABLED
#else /* !STM32_HSI_ENABLED */

#if STM32_SW == STM32_SW_HSI
#error "HSI not enabled, required by STM32_SW"
#endif

#if STM32_CECSW == STM32_CECSW_HSI
#error "HSI not enabled, required by STM32_CECSW"
#endif

#if STM32_I2C1SW == STM32_I2C1SW_HSI
#error "HSI not enabled, required by STM32_I2C1SW"
#endif

#if STM32_USART1SW == STM32_USART1SW_HSI
#error "HSI not enabled, required by STM32_USART1SW"
#endif

#if (STM32_SW == STM32_SW_PLL) && (STM32_PLLSRC == STM32_PLLSRC_HSI)
#error "HSI not enabled, required by STM32_SW and STM32_PLLSRC"
#endif

#if (STM32_MCOSEL == STM32_MCOSEL_HSI) ||                                   \
    ((STM32_MCOSEL == STM32_MCOSEL_PLLDIV2) &&                              \
     (STM32_PLLSRC == STM32_PLLSRC_HSI))
#error "HSI not enabled, required by STM32_MCOSEL"
#endif

#endif /* !STM32_HSI_ENABLED */

/*
 * HSI14 related checks.
 */
#if STM32_HSI14_ENABLED
#else /* !STM32_HSI14_ENABLED */

#if STM32_MCOSEL == STM32_MCOSEL_HSI14
#error "HSI14 not enabled, required by STM32_MCOSEL"
#endif

#if STM32_ADCSW == STM32_ADCSW_HSI14
#error "HSI14 not enabled, required by STM32_ADCSW"
#endif

#endif /* !STM32_HSI14_ENABLED */

/*
 * HSE related checks.
 */
#if STM32_HSE_ENABLED

#if STM32_HSECLK == 0
#error "HSE frequency not defined"
#elif (STM32_HSECLK < STM32_HSECLK_MIN) || (STM32_HSECLK > STM32_HSECLK_MAX)
#error "STM32_HSECLK outside acceptable range (STM32_HSECLK_MIN...STM32_HSECLK_MAX)"
#endif

#else /* !STM32_HSE_ENABLED */

#if STM32_SW == STM32_SW_HSE
#error "HSE not enabled, required by STM32_SW"
#endif

#if (STM32_SW == STM32_SW_PLL) && (STM32_PLLSRC == STM32_PLLSRC_HSE)
#error "HSE not enabled, required by STM32_SW and STM32_PLLSRC"
#endif

#if (STM32_MCOSEL == STM32_MCOSEL_HSE) ||                                   \
    ((STM32_MCOSEL == STM32_MCOSEL_PLLDIV2) &&                              \
     (STM32_PLLSRC == STM32_PLLSRC_HSE))
#error "HSE not enabled, required by STM32_MCOSEL"
#endif

#if STM32_RTCSEL == STM32_RTCSEL_HSEDIV
#error "HSE not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_HSE_ENABLED */

/*
 * LSI related checks.
 */
#if STM32_LSI_ENABLED
#else /* !STM32_LSI_ENABLED */

#if STM32_RTCSEL == STM32_RTCSEL_LSI
#error "LSI not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSI_ENABLED */

/*
 * LSE related checks.
 */
#if STM32_LSE_ENABLED

#if (STM32_LSECLK == 0)
#error "LSE frequency not defined"
#endif

#if (STM32_LSECLK < STM32_LSECLK_MIN) || (STM32_LSECLK > STM32_LSECLK_MAX)
#error "STM32_LSECLK outside acceptable range (STM32_LSECLK_MIN...STM32_LSECLK_MAX)"
#endif

#if !defined(STM32_LSEDRV)
#error "STM32_LSEDRV not defined"
#endif

#if (STM32_LSEDRV >> 3) > 3
#error "STM32_LSEDRV outside acceptable range ((0<<3)...(3<<3))"
#endif

#if STM32_CECSW == STM32_CECSW_LSE
#error "LSE not enabled, required by STM32_CECSW"
#endif

#if STM32_USART1SW == STM32_USART1SW_LSE
#error "LSE not enabled, required by STM32_USART1SW"
#endif

#else /* !STM32_LSE_ENABLED */

#if STM32_RTCSEL == STM32_RTCSEL_LSE
#error "LSE not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSE_ENABLED */

/* PLL activation conditions.*/
#if (STM32_SW == STM32_SW_PLL) ||                                           \
    (STM32_MCOSEL == STM32_MCOSEL_PLLDIV2) ||                               \
    defined(__DOXYGEN__)
/**
 * @brief   PLL activation flag.
 */
#define STM32_ACTIVATE_PLL          TRUE
#else
#define STM32_ACTIVATE_PLL          FALSE
#endif

/* HSE prescaler setting check.*/
#if ((STM32_PREDIV_VALUE >= 1) || (STM32_PREDIV_VALUE <= 16))
#define STM32_PREDIV                ((STM32_PREDIV_VALUE - 1) << 0)
#else
#error "invalid STM32_PREDIV value specified"
#endif

/**
 * @brief   PLLMUL field.
 */
#if ((STM32_PLLMUL_VALUE >= 2) && (STM32_PLLMUL_VALUE <= 16)) ||            \
    defined(__DOXYGEN__)
#define STM32_PLLMUL                ((STM32_PLLMUL_VALUE - 2) << 18)
#else
#error "invalid STM32_PLLMUL_VALUE value specified"
#endif

/**
 * @brief   PLL input clock frequency.
 */
#if (STM32_PLLSRC == STM32_PLLSRC_HSE) || defined(__DOXYGEN__)
#define STM32_PLLCLKIN              (STM32_HSECLK / STM32_PREDIV_VALUE)
#elif STM32_PLLSRC == STM32_PLLSRC_HSI
#define STM32_PLLCLKIN              (STM32_HSICLK / 2)
#else
#error "invalid STM32_PLLSRC value specified"
#endif

/* PLL input frequency range check.*/
#if (STM32_PLLCLKIN < STM32_PLLIN_MIN) || (STM32_PLLCLKIN > STM32_PLLIN_MAX)
#error "STM32_PLLCLKIN outside acceptable range (STM32_PLLIN_MIN...STM32_PLLIN_MAX)"
#endif

/**
 * @brief   PLL output clock frequency.
 */
#define STM32_PLLCLKOUT             (STM32_PLLCLKIN * STM32_PLLMUL_VALUE)

/* PLL output frequency range check.*/
#if (STM32_PLLCLKOUT < STM32_PLLOUT_MIN) || (STM32_PLLCLKOUT > STM32_PLLOUT_MAX)
#error "STM32_PLLCLKOUT outside acceptable range (STM32_PLLOUT_MIN...STM32_PLLOUT_MAX)"
#endif

/**
 * @brief   System clock source.
 */
#if (STM32_SW == STM32_SW_PLL) || defined(__DOXYGEN__)
#define STM32_SYSCLK                STM32_PLLCLKOUT
#elif (STM32_SW == STM32_SW_HSI)
#define STM32_SYSCLK                STM32_HSICLK
#elif (STM32_SW == STM32_SW_HSE)
#define STM32_SYSCLK                STM32_HSECLK
#else
#error "invalid STM32_SW value specified"
#endif

/* Check on the system clock.*/
#if STM32_SYSCLK > STM32_SYSCLK_MAX
#error "STM32_SYSCLK above maximum rated frequency (STM32_SYSCLK_MAX)"
#endif

/**
 * @brief   AHB frequency.
 */
#if (STM32_HPRE == STM32_HPRE_DIV1) || defined(__DOXYGEN__)
#define STM32_HCLK                  (STM32_SYSCLK / 1)
#elif STM32_HPRE == STM32_HPRE_DIV2
#define STM32_HCLK                  (STM32_SYSCLK / 2)
#elif STM32_HPRE == STM32_HPRE_DIV4
#define STM32_HCLK                  (STM32_SYSCLK / 4)
#elif STM32_HPRE == STM32_HPRE_DIV8
#define STM32_HCLK                  (STM32_SYSCLK / 8)
#elif STM32_HPRE == STM32_HPRE_DIV16
#define STM32_HCLK                  (STM32_SYSCLK / 16)
#elif STM32_HPRE == STM32_HPRE_DIV64
#define STM32_HCLK                  (STM32_SYSCLK / 64)
#elif STM32_HPRE == STM32_HPRE_DIV128
#define STM32_HCLK                  (STM32_SYSCLK / 128)
#elif STM32_HPRE == STM32_HPRE_DIV256
#define STM32_HCLK                  (STM32_SYSCLK / 256)
#elif STM32_HPRE == STM32_HPRE_DIV512
#define STM32_HCLK                  (STM32_SYSCLK / 512)
#else
#error "invalid STM32_HPRE value specified"
#endif

/* AHB frequency check.*/
#if STM32_HCLK > STM32_SYSCLK_MAX
#error "STM32_HCLK exceeding maximum frequency (STM32_SYSCLK_MAX)"
#endif

/**
 * @brief   APB frequency.
 */
#if (STM32_PPRE == STM32_PPRE_DIV1) || defined(__DOXYGEN__)
#define STM32_PCLK                  (STM32_HCLK / 1)
#elif STM32_PPRE == STM32_PPRE_DIV2
#define STM32_PCLK                  (STM32_HCLK / 2)
#elif STM32_PPRE == STM32_PPRE_DIV4
#define STM32_PCLK                  (STM32_HCLK / 4)
#elif STM32_PPRE == STM32_PPRE_DIV8
#define STM32_PCLK                  (STM32_HCLK / 8)
#elif STM32_PPRE == STM32_PPRE_DIV16
#define STM32_PCLK                  (STM32_HCLK / 16)
#else
#error "invalid STM32_PPRE value specified"
#endif

/* APB frequency check.*/
#if STM32_PCLK > STM32_PCLK_MAX
#error "STM32_PCLK exceeding maximum frequency (STM32_PCLK_MAX)"
#endif

/**
 * @brief   RTC clock.
 */
#if (STM32_RTCSEL == STM32_RTCSEL_LSE) || defined(__DOXYGEN__)
#define STM32_RTCCLK                STM32_LSECLK
#elif STM32_RTCSEL == STM32_RTCSEL_LSI
#define STM32_RTCCLK                STM32_LSICLK
#elif STM32_RTCSEL == STM32_RTCSEL_HSEDIV
#define STM32_RTCCLK                (STM32_HSECLK / 32)
#elif STM32_RTCSEL == STM32_RTCSEL_NOCLOCK
#define STM32_RTCCLK                0
#else
#error "invalid source selected for RTC clock"
#endif

/**
 * @brief   ADC frequency.
 */
#if STM32_ADCSW == STM32_ADCSW_HSI14
#define STM32_ADCCLK                STM32_HSI14CLK
#elif STM32_ADCSW == STM32_ADCSW_PCLK
#if (STM32_ADCPRE == STM32_ADCPRE_DIV2) || defined(__DOXYGEN__)
#define STM32_ADCCLK                (STM32_PCLK / 2)
#elif STM32_ADCPRE == STM32_ADCPRE_DIV4
#define STM32_ADCCLK                (STM32_PCLK / 4)
#else
#error "invalid STM32_ADCPRE value specified"
#endif
#else
#error "invalid source selected for ADC clock"
#endif

/* ADC frequency check.*/
#if STM32_ADCCLK > STM32_ADCCLK_MAX
#error "STM32_ADCCLK exceeding maximum frequency (STM32_ADCCLK_MAX)"
#endif

/**
 * @brief   CEC frequency.
 */
#if STM32_CECSW == STM32_CECSW_HSI
#define STM32_CECCLK                STM32_HSICLK
#elif STM32_CECSW == STM32_CECSW_LSE
#define STM32_CECCLK                STM32_LSECLK
#else
#error "invalid source selected for CEC clock"
#endif

/**
 * @brief   I2C1 frequency.
 */
#if STM32_I2CSW == STM32_I2C1SW_HSI
#define STM32_I2C1CLK               STM32_HSICLK
#elif STM32_I2CSW == STM32_I2C1SW_SYSCLK
#define STM32_I2C1CLK               STM32_SYSCLK
#else
#error "invalid source selected for I2C1 clock"
#endif

/**
 * @brief   USART1 frequency.
 */
#if STM32_USART1SW == STM32_USART1SW_PCLK
#define STM32_USART1CLK             STM32_PCLK
#elif STM32_USART1SW == STM32_USART1SW_SYSCLK
#define STM32_USART1CLK             STM32_SYSCLK
#elif STM32_USART1SW == STM32_USART1SW_LSECLK
#define STM32_USART1CLK             STM32_LSECLK
#elif STM32_USART1SW == STM32_USART1SW_HSICLK
#define STM32_USART1CLK             STM32_HSICLK
#else
#error "invalid source selected for USART1 clock"
#endif

/**
 * @brief   USART2 frequency.
 */
#define STM32_USART2CLK             STM32_PCLK

/**
 * @brief   Timers clock.
 */
#if (STM32_PPRE == STM32_PPRE_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK1               (STM32_PCLK * 1)
#define STM32_TIMCLK2               (STM32_PCLK * 1)
#else
#define STM32_TIMCLK1               (STM32_PCLK * 2)
#define STM32_TIMCLK2               (STM32_PCLK * 2)
#endif

/**
 * @brief   Flash settings.
 */
#if (STM32_HCLK <= 24000000) || defined(__DOXYGEN__)
#define STM32_FLASHBITS             0x00000010
#else
#define STM32_FLASHBITS             0x00000011
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* STM32 ISR, DMA and RCC helpers.*/
#include "stm32_isr.h"
#include "stm32_dma.h"
#include "stm32_rcc.h"

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void stm32_clock_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
