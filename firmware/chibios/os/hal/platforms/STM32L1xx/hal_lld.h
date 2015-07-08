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
 * @file    STM32L1xx/hal_lld.h
 * @brief   STM32L1xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32L1XX_MD for Ultra Low Power Medium-density devices.
 *          - STM32L1XX_MDP for Ultra Low Power Medium-density Plus devices.
 *          - STM32L1XX_HD for Ultra Low Power High-density devices.
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
#define HAL_IMPLEMENTS_COUNTERS TRUE

/**
 * @name    Platform identification
 * @{
 */
#if defined(STM32L1XX_MD) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power Medium Density"
#define STM32L1XX

#elif defined(STM32L1XX_MDP)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power Medium Density Plus"
#define STM32L1XX

#elif defined(STM32L1XX_HD)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power High Density"
#define STM32L1XX

#else
#error "STM32L1xx device not specified"
#endif
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define STM32_HSICLK            16000000    /**< High speed internal clock. */
#define STM32_LSICLK            38000       /**< Low speed internal clock.  */
/** @} */

/**
 * @name    PWR_CR register bits definitions
 * @{
 */
#define STM32_VOS_MASK          (3 << 11)   /**< Core voltage mask.         */
#define STM32_VOS_1P8           (1 << 11)   /**< Core voltage 1.8 Volts.    */
#define STM32_VOS_1P5           (2 << 11)   /**< Core voltage 1.5 Volts.    */
#define STM32_VOS_1P2           (3 << 11)   /**< Core voltage 1.2 Volts.    */

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
 * @name    RCC_CR register bits definitions
 * @{
 */
#define STM32_RTCPRE_MASK       (3 << 29)   /**< RTCPRE mask.               */
#define STM32_RTCPRE_DIV2       (0 << 29)   /**< HSE divided by 2.          */
#define STM32_RTCPRE_DIV4       (1 << 29)   /**< HSE divided by 4.          */
#define STM32_RTCPRE_DIV8       (2 << 29)   /**< HSE divided by 2.          */
#define STM32_RTCPRE_DIV16      (3 << 29)   /**< HSE divided by 16.         */
/** @} */

/**
 * @name    RCC_CFGR register bits definitions
 * @{
 */
#define STM32_SW_MSI            (0 << 0)    /**< SYSCLK source is MSI.      */
#define STM32_SW_HSI            (1 << 0)    /**< SYSCLK source is HSI.      */
#define STM32_SW_HSE            (2 << 0)    /**< SYSCLK source is HSE.      */
#define STM32_SW_PLL            (3 << 0)    /**< SYSCLK source is PLL.      */

#define STM32_HPRE_DIV1         (0 << 4)    /**< SYSCLK divided by 1.       */
#define STM32_HPRE_DIV2         (8 << 4)    /**< SYSCLK divided by 2.       */
#define STM32_HPRE_DIV4         (9 << 4)    /**< SYSCLK divided by 4.       */
#define STM32_HPRE_DIV8         (10 << 4)   /**< SYSCLK divided by 8.       */
#define STM32_HPRE_DIV16        (11 << 4)   /**< SYSCLK divided by 16.      */
#define STM32_HPRE_DIV64        (12 << 4)   /**< SYSCLK divided by 64.      */
#define STM32_HPRE_DIV128       (13 << 4)   /**< SYSCLK divided by 128.     */
#define STM32_HPRE_DIV256       (14 << 4)   /**< SYSCLK divided by 256.     */
#define STM32_HPRE_DIV512       (15 << 4)   /**< SYSCLK divided by 512.     */

#define STM32_PPRE1_DIV1        (0 << 8)    /**< HCLK divided by 1.         */
#define STM32_PPRE1_DIV2        (4 << 8)    /**< HCLK divided by 2.         */
#define STM32_PPRE1_DIV4        (5 << 8)    /**< HCLK divided by 4.         */
#define STM32_PPRE1_DIV8        (6 << 8)    /**< HCLK divided by 8.         */
#define STM32_PPRE1_DIV16       (7 << 8)    /**< HCLK divided by 16.        */

#define STM32_PPRE2_DIV1        (0 << 11)   /**< HCLK divided by 1.         */
#define STM32_PPRE2_DIV2        (4 << 11)   /**< HCLK divided by 2.         */
#define STM32_PPRE2_DIV4        (5 << 11)   /**< HCLK divided by 4.         */
#define STM32_PPRE2_DIV8        (6 << 11)   /**< HCLK divided by 8.         */
#define STM32_PPRE2_DIV16       (7 << 11)   /**< HCLK divided by 16.        */

#define STM32_PLLSRC_HSI        (0 << 16)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 16)   /**< PLL clock source is HSE.   */

#define STM32_MCOSEL_NOCLOCK    (0 << 24)   /**< No clock on MCO pin.       */
#define STM32_MCOSEL_SYSCLK     (1 << 24)   /**< SYSCLK on MCO pin.         */
#define STM32_MCOSEL_HSI        (2 << 24)   /**< HSI clock on MCO pin.      */
#define STM32_MCOSEL_MSI        (3 << 24)   /**< MSI clock on MCO pin.      */
#define STM32_MCOSEL_HSE        (4 << 24)   /**< HSE clock on MCO pin.      */
#define STM32_MCOSEL_PLL        (5 << 24)   /**< PLL clock on MCO pin.      */
#define STM32_MCOSEL_LSI        (6 << 24)   /**< LSI clock on MCO pin.      */
#define STM32_MCOSEL_LSE        (7 << 24)   /**< LSE clock on MCO pin.      */

#define STM32_MCOPRE_DIV1       (0 << 28)   /**< MCO divided by 1.          */
#define STM32_MCOPRE_DIV2       (1 << 28)   /**< MCO divided by 1.          */
#define STM32_MCOPRE_DIV4       (2 << 28)   /**< MCO divided by 1.          */
#define STM32_MCOPRE_DIV8       (3 << 28)   /**< MCO divided by 1.          */
#define STM32_MCOPRE_DIV16      (4 << 28)   /**< MCO divided by 1.          */
/** @} */

/**
 * @name    RCC_ICSCR register bits definitions
 * @{
 */
#define STM32_MSIRANGE_MASK     (7 << 13)   /**< MSIRANGE field mask.       */
#define STM32_MSIRANGE_64K      (0 << 13)   /**< 64kHz nominal.             */
#define STM32_MSIRANGE_128K     (1 << 13)   /**< 128kHz nominal.            */
#define STM32_MSIRANGE_256K     (2 << 13)   /**< 256kHz nominal.            */
#define STM32_MSIRANGE_512K     (3 << 13)   /**< 512kHz nominal.            */
#define STM32_MSIRANGE_1M       (4 << 13)   /**< 1MHz nominal.              */
#define STM32_MSIRANGE_2M       (5 << 13)   /**< 2MHz nominal.              */
#define STM32_MSIRANGE_4M       (6 << 13)   /**< 4MHz nominal               */
/** @} */

/**
 * @name    RCC_CSR register bits definitions
 * @{
 */
#define STM32_RTCSEL_MASK       (3 << 16)   /**< RTC source mask.           */
#define STM32_RTCSEL_NOCLOCK    (0 << 16)   /**< No RTC source.             */
#define STM32_RTCSEL_LSE        (1 << 16)   /**< RTC source is LSE.         */
#define STM32_RTCSEL_LSI        (2 << 16)   /**< RTC source is LSI.         */
#define STM32_RTCSEL_HSEDIV     (3 << 16)   /**< RTC source is HSE divided. */
/** @} */

/*===========================================================================*/
/* Platform specific friendly IRQ names.                                     */
/*===========================================================================*/

/**
 * @name  IRQ VECTOR names
 * @{
 */
#define WWDG_IRQHandler         Vector40    /**< Window Watchdog.           */
#define PVD_IRQHandler          Vector44    /**< PVD through EXTI Line
                                                 detect.                    */
#define TAMPER_STAMP_IRQHandler Vector48    /**< Tamper and Time Stamp
                                                 through EXTI.              */
#define RTC_WKUP_IRQHandler     Vector4C    /**< RTC Wakeup Timer through
                                                 EXTI.                      */
#define FLASH_IRQHandler        Vector50    /**< Flash.                     */
#define RCC_IRQHandler          Vector54    /**< RCC.                       */
#define EXTI0_IRQHandler        Vector58    /**< EXTI Line 0.               */
#define EXTI1_IRQHandler        Vector5C    /**< EXTI Line 1.               */
#define EXTI2_IRQHandler        Vector60    /**< EXTI Line 2.               */
#define EXTI3_IRQHandler        Vector64    /**< EXTI Line 3.               */
#define EXTI4_IRQHandler        Vector68    /**< EXTI Line 4.               */
#define DMA1_Ch1_IRQHandler     Vector6C    /**< DMA1 Channel 1.            */
#define DMA1_Ch2_IRQHandler     Vector70    /**< DMA1 Channel 2.            */
#define DMA1_Ch3_IRQHandler     Vector74    /**< DMA1 Channel 3.            */
#define DMA1_Ch4_IRQHandler     Vector78    /**< DMA1 Channel 4.            */
#define DMA1_Ch5_IRQHandler     Vector7C    /**< DMA1 Channel 5.            */
#define DMA1_Ch6_IRQHandler     Vector80    /**< DMA1 Channel 6.            */
#define DMA1_Ch7_IRQHandler     Vector84    /**< DMA1 Channel 7.            */
#define ADC1_IRQHandler         Vector88    /**< ADC1.                      */
#define USB_HP_IRQHandler       Vector8C    /**< USB High Priority.         */
#define USB_LP_IRQHandler       Vector90    /**< USB Low Priority.          */
#define DAC_IRQHandler          Vector94    /**< DAC.                       */
#define COMP_IRQHandler         Vector98    /**< Comparator through EXTI.   */
#define EXTI9_5_IRQHandler      Vector9C    /**< EXTI Line 9..5.            */
#define LCD_IRQHandler          VectorA0    /**< LCD.                       */
#define TIM9_IRQHandler         VectorA4    /**< TIM9.                      */
#define TIM10_IRQHandler        VectorA8    /**< TIM10.                     */
#define TIM11_IRQHandler        VectorAC    /**< TIM11.                     */
#define TIM2_IRQHandler         VectorB0    /**< TIM2.                      */
#define TIM3_IRQHandler         VectorB4    /**< TIM3.                      */
#define TIM4_IRQHandler         VectorB8    /**< TIM4.                      */
#define I2C1_EV_IRQHandler      VectorBC    /**< I2C1 Event.                */
#define I2C1_ER_IRQHandler      VectorC0    /**< I2C1 Error.                */
#define I2C2_EV_IRQHandler      VectorC4    /**< I2C2 Event.                */
#define I2C2_ER_IRQHandler      VectorC8    /**< I2C2 Error.                */
#define SPI1_IRQHandler         VectorCC    /**< SPI1.                      */
#define SPI2_IRQHandler         VectorD0    /**< SPI2.                      */
#define USART1_IRQHandler       VectorD4    /**< USART1.                    */
#define USART2_IRQHandler       VectorD8    /**< USART2.                    */
#define USART3_IRQHandler       VectorDC    /**< USART3.                    */
#define EXTI15_10_IRQHandler    VectorE0    /**< EXTI Line 15..10.          */
#define RTC_Alarm_IRQHandler    VectorE4    /**< RTC Alarm through EXTI.    */
#define USB_FS_WKUP_IRQHandler  VectorE8    /**< USB Wakeup from suspend.   */
#define TIM6_IRQHandler         VectorEC    /**< TIM6.                      */
#define TIM7_IRQHandler         VectorF0    /**< TIM7.                      */
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
 * @brief   Core voltage selection.
 * @note    This setting affects all the performance and clock related
 *          settings, the maximum performance is only obtainable selecting
 *          the maximum voltage.
 */
#if !defined(STM32_VOS) || defined(__DOXYGEN__)
#define STM32_VOS                   STM32_VOS_1P8
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
 * @brief   Enables or disables the LSI clock source.
 */
#if !defined(STM32_LSI_ENABLED) || defined(__DOXYGEN__)
#define STM32_LSI_ENABLED           TRUE
#endif

/**
 * @brief   Enables or disables the HSE clock source.
 */
#if !defined(STM32_HSE_ENABLED) || defined(__DOXYGEN__)
#define STM32_HSE_ENABLED           FALSE
#endif

/**
 * @brief   Enables or disables the LSE clock source.
 */
#if !defined(STM32_HSE_ENABLED) || defined(__DOXYGEN__)
#define STM32_LSE_ENABLED           FALSE
#endif

/**
 * @brief   ADC clock setting.
 */
#if !defined(STM32_ADC_CLOCK_ENABLED) || defined(__DOXYGEN__)
#define STM32_ADC_CLOCK_ENABLED     TRUE
#endif

/**
 * @brief   USB clock setting.
 */
#if !defined(STM32_USB_CLOCK_ENABLED) || defined(__DOXYGEN__)
#define STM32_USB_CLOCK_ENABLED     TRUE
#endif

/**
 * @brief   MSI frequency setting.
 */
#if !defined(STM32_MSIRANGE) || defined(__DOXYGEN__)
#define STM32_MSIRANGE              STM32_MSIRANGE_2M
#endif

/**
 * @brief   Main clock source selection.
 * @note    If the selected clock source is not the PLL then the PLL is not
 *          initialized and started.
 * @note    The default value is calculated for a 32MHz system clock from
 *          the internal 16MHz HSI clock.
 */
#if !defined(STM32_SW) || defined(__DOXYGEN__)
#define STM32_SW                    STM32_SW_PLL
#endif

/**
 * @brief   Clock source for the PLL.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 32MHz system clock from
 *          the internal 16MHz HSI clock.
 */
#if !defined(STM32_PLLSRC) || defined(__DOXYGEN__)
#define STM32_PLLSRC                STM32_PLLSRC_HSI
#endif

/**
 * @brief   PLL multiplier value.
 * @note    The allowed values are 3, 4, 6, 8, 12, 16, 32, 48.
 * @note    The default value is calculated for a 32MHz system clock from
 *          the internal 16MHz HSI clock.
 */
#if !defined(STM32_PLLMUL_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLMUL_VALUE          6
#endif

/**
 * @brief   PLL divider value.
 * @note    The allowed values are 2, 3, 4.
 * @note    The default value is calculated for a 32MHz system clock from
 *          the internal 16MHz HSI clock.
 */
#if !defined(STM32_PLLDIV_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLDIV_VALUE          3
#endif

/**
 * @brief   AHB prescaler value.
 * @note    The default value is calculated for a 32MHz system clock from
 *          the internal 16MHz HSI clock.
 */
#if !defined(STM32_HPRE) || defined(__DOXYGEN__)
#define STM32_HPRE                  STM32_HPRE_DIV1
#endif

/**
 * @brief   APB1 prescaler value.
 */
#if !defined(STM32_PPRE1) || defined(__DOXYGEN__)
#define STM32_PPRE1                 STM32_PPRE1_DIV1
#endif

/**
 * @brief   APB2 prescaler value.
 */
#if !defined(STM32_PPRE2) || defined(__DOXYGEN__)
#define STM32_PPRE2                 STM32_PPRE2_DIV1
#endif

/**
 * @brief   MCO clock source.
 */
#if !defined(STM32_MCOSEL) || defined(__DOXYGEN__)
#define STM32_MCOSEL                STM32_MCOSEL_NOCLOCK
#endif

/**
 * @brief   MCO divider setting.
 */
#if !defined(STM32_MCOPRE) || defined(__DOXYGEN__)
#define STM32_MCOPRE                STM32_MCOPRE_DIV1
#endif

/**
 * @brief   RTC/LCD clock source.
 */
#if !defined(STM32_RTCSEL) || defined(__DOXYGEN__)
#define STM32_RTCSEL                STM32_RTCSEL_LSE
#endif

/**
 * @brief   HSE divider toward RTC setting.
 */
#if !defined(STM32_RTCPRE) || defined(__DOXYGEN__)
#define STM32_RTCPRE                STM32_RTCPRE_DIV2
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Configuration-related checks.
 */
#if !defined(STM32L1xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32L1xx_MCUCONF not defined"
#endif

/* Voltage related limits.*/
#if (STM32_VOS == STM32_VOS_1P8) || defined(__DOXYGEN__)
/**
 * @brief   Maximum HSE clock frequency at current voltage setting.
 */
#define STM32_HSECLK_MAX            32000000

/**
 * @brief   Maximum SYSCLK clock frequency at current voltage setting.
 */
#define STM32_SYSCLK_MAX            32000000

/**
 * @brief   Maximum VCO clock frequency at current voltage setting.
 */
#define STM32_PLLVCO_MAX            96000000

/**
 * @brief   Minimum VCO clock frequency at current voltage setting.
 */
#define STM32_PLLVCO_MIN            6000000

/**
 * @brief   Maximum APB1 clock frequency.
 */
#define STM32_PCLK1_MAX             32000000

/**
 * @brief   Maximum APB2 clock frequency.
 */
#define STM32_PCLK2_MAX             32000000

/**
 * @brief   Maximum frequency not requiring a wait state for flash accesses.
 */
#define STM32_0WS_THRESHOLD         16000000

/**
 * @brief   HSI availability at current voltage settings.
 */
#define STM32_HSI_AVAILABLE         TRUE

#elif STM32_VOS == STM32_VOS_1P5
#define STM32_HSECLK_MAX            16000000
#define STM32_SYSCLK_MAX            16000000
#define STM32_PLLVCO_MAX            48000000
#define STM32_PLLVCO_MIN            6000000
#define STM32_PCLK1_MAX             16000000
#define STM32_PCLK2_MAX             16000000
#define STM32_0WS_THRESHOLD         8000000
#define STM32_HSI_AVAILABLE         TRUE
#elif STM32_VOS == STM32_VOS_1P2
#define STM32_HSECLK_MAX            4000000
#define STM32_SYSCLK_MAX            4000000
#define STM32_PLLVCO_MAX            24000000
#define STM32_PLLVCO_MIN            6000000
#define STM32_PCLK1_MAX             4000000
#define STM32_PCLK2_MAX             4000000
#define STM32_0WS_THRESHOLD         2000000
#define STM32_HSI_AVAILABLE         FALSE
#else
#error "invalid STM32_VOS value specified"
#endif

/* HSI related checks.*/
#if STM32_HSI_ENABLED
#if !STM32_HSI_AVAILABLE
  #error "impossible to activate HSI under the current voltage settings"
#endif
#else /* !STM32_HSI_ENABLED */
#if STM32_ADC_CLOCK_ENABLED ||                                              \
    (STM32_SW == STM32_SW_HSI) ||                                           \
    ((STM32_SW == STM32_SW_PLL) &&                                          \
     (STM32_PLLSRC == STM32_PLLSRC_HSI)) ||                                 \
    (STM32_MCOSEL == STM32_MCOSEL_HSI) ||                                   \
    ((STM32_MCOSEL == STM32_MCOSEL_PLL) &&                                  \
     (STM32_PLLSRC == STM32_PLLSRC_HSI))
#error "required HSI clock is not enabled"
#endif
#endif /* !STM32_HSI_ENABLED */

/* HSE related checks.*/
#if STM32_HSE_ENABLED
#if STM32_HSECLK == 0
#error "impossible to activate HSE"
#endif
#if (STM32_HSECLK < 1000000) || (STM32_HSECLK > STM32_HSECLK_MAX)
#error "STM32_HSECLK outside acceptable range (1MHz...STM32_HSECLK_MAX)"
#endif
#else /* !STM32_HSE_ENABLED */
#if (STM32_SW == STM32_SW_HSE) ||                                           \
    ((STM32_SW == STM32_SW_PLL) &&                                          \
     (STM32_PLLSRC == STM32_PLLSRC_HSE)) ||                                 \
    (STM32_MCOSEL == STM32_MCOSEL_HSE) ||                                   \
    ((STM32_MCOSEL == STM32_MCOSEL_PLL) &&                                  \
     (STM32_PLLSRC == STM32_PLLSRC_HSE)) ||                                 \
    (STM32_RTCSEL == STM32_RTCSEL_HSEDIV)
#error "required HSE clock is not enabled"
#endif
#endif /* !STM32_HSE_ENABLED */

/* LSI related checks.*/
#if STM32_LSI_ENABLED
#else /* !STM32_LSI_ENABLED */

#if STM32_MCOSEL == STM32_MCOSEL_LSI
#error "LSI not enabled, required by STM32_MCOSEL"
#endif

#if STM32_RTCSEL == STM32_RTCSEL_LSI
#error "LSI not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSI_ENABLED */

/* LSE related checks.*/
#if STM32_LSE_ENABLED
#if (STM32_LSECLK == 0)
#error "impossible to activate LSE"
#endif
#if (STM32_LSECLK < 1000) || (STM32_LSECLK > 1000000)
#error "STM32_LSECLK outside acceptable range (1...1000kHz)"
#endif

#else /* !STM32_LSE_ENABLED */

#if STM32_MCOSEL == STM32_MCOSEL_LSE
#error "LSE not enabled, required by STM32_MCOSEL"
#endif

#if STM32_RTCSEL == STM32_RTCSEL_LSE
#error "LSE not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSE_ENABLED */

/* PLL related checks.*/
#if STM32_USB_CLOCK_ENABLED ||                                              \
    (STM32_SW == STM32_SW_PLL) ||                                           \
    (STM32_MCOSEL == STM32_MCOSEL_PLL) ||                                   \
    defined(__DOXYGEN__)
/**
 * @brief   PLL activation flag.
 */
#define STM32_ACTIVATE_PLL          TRUE
#else
#define STM32_ACTIVATE_PLL          FALSE
#endif

/**
 * @brief   PLLMUL field.
 */
#if (STM32_PLLMUL_VALUE == 3) || defined(__DOXYGEN__)
#define STM32_PLLMUL                (0 << 18)
#elif STM32_PLLMUL_VALUE == 4
#define STM32_PLLMUL                (1 << 18)
#elif STM32_PLLMUL_VALUE == 6
#define STM32_PLLMUL                (2 << 18)
#elif STM32_PLLMUL_VALUE == 8
#define STM32_PLLMUL                (3 << 18)
#elif STM32_PLLMUL_VALUE == 12
#define STM32_PLLMUL                (4 << 18)
#elif STM32_PLLMUL_VALUE == 16
#define STM32_PLLMUL                (5 << 18)
#elif STM32_PLLMUL_VALUE == 24
#define STM32_PLLMUL                (6 << 18)
#elif STM32_PLLMUL_VALUE == 32
#define STM32_PLLMUL                (7 << 18)
#elif STM32_PLLMUL_VALUE == 48
#define STM32_PLLMUL                (8 << 18)
#else
#error "invalid STM32_PLLMUL_VALUE value specified"
#endif

/**
 * @brief   PLLDIV field.
 */
#if (STM32_PLLDIV_VALUE == 2) || defined(__DOXYGEN__)
#define STM32_PLLDIV                (1 << 22)
#elif STM32_PLLDIV_VALUE == 3
#define STM32_PLLDIV                (2 << 22)
#elif STM32_PLLDIV_VALUE == 4
#define STM32_PLLDIV                (3 << 22)
#else
#error "invalid STM32_PLLDIV_VALUE value specified"
#endif

/**
 * @brief   PLL input clock frequency.
 */
#if (STM32_PLLSRC == STM32_PLLSRC_HSE) || defined(__DOXYGEN__)
#define STM32_PLLCLKIN              STM32_HSECLK
#elif STM32_PLLSRC == STM32_PLLSRC_HSI
#define STM32_PLLCLKIN              STM32_HSICLK
#else
#error "invalid STM32_PLLSRC value specified"
#endif

/* PLL input frequency range check.*/
#if (STM32_PLLCLKIN < 2000000) || (STM32_PLLCLKIN > 24000000)
#error "STM32_PLLCLKIN outside acceptable range (2...24MHz)"
#endif

/**
 * @brief   PLL VCO frequency.
 */
#define STM32_PLLVCO                (STM32_PLLCLKIN * STM32_PLLMUL_VALUE)

/* PLL output frequency range check.*/
#if (STM32_PLLVCO < STM32_PLLVCO_MIN) || (STM32_PLLVCO > STM32_PLLVCO_MAX)
#error "STM32_PLLVCO outside acceptable range (STM32_PLLVCO_MIN...STM32_PLLVCO_MAX)"
#endif

/**
 * @brief   PLL output clock frequency.
 */
#define STM32_PLLCLKOUT             (STM32_PLLVCO / STM32_PLLDIV_VALUE)

/* PLL output frequency range check.*/
#if (STM32_PLLCLKOUT < 2000000) || (STM32_PLLCLKOUT > 32000000)
#error "STM32_PLLCLKOUT outside acceptable range (2...32MHz)"
#endif

/**
 * @brief   MSI frequency.
 * @note    Values are taken from the STM8Lxx datasheet.
 */
#if STM32_MSIRANGE == STM32_MSIRANGE_64K
#define STM32_MSICLK                65500
#elif STM32_MSIRANGE == STM32_MSIRANGE_128K
#define STM32_MSICLK                131000
#elif STM32_MSIRANGE == STM32_MSIRANGE_256K
#define STM32_MSICLK                262000
#elif STM32_MSIRANGE == STM32_MSIRANGE_512K
#define STM32_MSICLK                524000
#elif STM32_MSIRANGE == STM32_MSIRANGE_1M
#define STM32_MSICLK                1050000
#elif STM32_MSIRANGE == STM32_MSIRANGE_2M
#define STM32_MSICLK                2100000
#elif STM32_MSIRANGE == STM32_MSIRANGE_4M
#define STM32_MSICLK                4200000
#else
#error "invalid STM32_MSIRANGE value specified"
#endif

/**
 * @brief   System clock source.
 */
#if STM32_NO_INIT || defined(__DOXYGEN__)
#define STM32_SYSCLK                2100000
#elif (STM32_SW == STM32_SW_MSI)
#define STM32_SYSCLK                STM32_MSICLK
#elif (STM32_SW == STM32_SW_HSI)
#define STM32_SYSCLK                STM32_HSICLK
#elif (STM32_SW == STM32_SW_HSE)
#define STM32_SYSCLK                STM32_HSECLK
#elif (STM32_SW == STM32_SW_PLL)
#define STM32_SYSCLK                STM32_PLLCLKOUT
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
 * @brief   APB1 frequency.
 */
#if (STM32_PPRE1 == STM32_PPRE1_DIV1) || defined(__DOXYGEN__)
#define STM32_PCLK1                 (STM32_HCLK / 1)
#elif STM32_PPRE1 == STM32_PPRE1_DIV2
#define STM32_PCLK1                 (STM32_HCLK / 2)
#elif STM32_PPRE1 == STM32_PPRE1_DIV4
#define STM32_PCLK1                 (STM32_HCLK / 4)
#elif STM32_PPRE1 == STM32_PPRE1_DIV8
#define STM32_PCLK1                 (STM32_HCLK / 8)
#elif STM32_PPRE1 == STM32_PPRE1_DIV16
#define STM32_PCLK1                 (STM32_HCLK / 16)
#else
#error "invalid STM32_PPRE1 value specified"
#endif

/* APB1 frequency check.*/
#if STM32_PCLK1 > STM32_PCLK1_MAX
#error "STM32_PCLK1 exceeding maximum frequency (STM32_PCLK1_MAX)"
#endif

/**
 * @brief   APB2 frequency.
 */
#if (STM32_PPRE2 == STM32_PPRE2_DIV1) || defined(__DOXYGEN__)
#define STM32_PCLK2                 (STM32_HCLK / 1)
#elif STM32_PPRE2 == STM32_PPRE2_DIV2
#define STM32_PCLK2                 (STM32_HCLK / 2)
#elif STM32_PPRE2 == STM32_PPRE2_DIV4
#define STM32_PCLK2                 (STM32_HCLK / 4)
#elif STM32_PPRE2 == STM32_PPRE2_DIV8
#define STM32_PCLK2                 (STM32_HCLK / 8)
#elif STM32_PPRE2 == STM32_PPRE2_DIV16
#define STM32_PCLK2                 (STM32_HCLK / 16)
#else
#error "invalid STM32_PPRE2 value specified"
#endif

/* APB2 frequency check.*/
#if STM32_PCLK2 > STM32_PCLK2_MAX
#error "STM32_PCLK2 exceeding maximum frequency (STM32_PCLK2_MAX)"
#endif

/**
 * @brief   MCO divider clock.
 */
#if (STM32_MCOSEL == STM32_MCOSEL_NOCLOCK) || defined(__DOXYGEN__)
#define STM32_MCODIVCLK             0
#elif STM32_MCOSEL == STM32_MCOSEL_HSI
#define STM32_MCODIVCLK             STM32_HSICLK
#elif STM32_MCOSEL == STM32_MCOSEL_MSI
#define STM32_MCODIVCLK             STM32_MSICLK
#elif STM32_MCOSEL == STM32_MCOSEL_HSE
#define STM32_MCODIVCLK             STM32_HSECLK
#elif STM32_MCOSEL == STM32_MCOSEL_PLL
#define STM32_MCODIVCLK             STM32_PLLCLKOUT
#elif STM32_MCOSEL == STM32_MCOSEL_LSI
#define STM32_MCODIVCLK             STM32_LSICLK
#elif STM32_MCOSEL == STM32_MCOSEL_LSE
#define STM32_MCODIVCLK             STM32_LSECLK
#else
#error "invalid STM32_MCOSEL value specified"
#endif

/**
 * @brief   MCO output pin clock.
 */
#if (STM32_MCOPRE == STM32_MCOPRE_DIV1) || defined(__DOXYGEN__)
#define STM32_MCOCLK                STM32_MCODIVCLK
#elif STM32_MCOPRE == STM32_MCOPRE_DIV2
#define STM32_MCOCLK                (STM32_MCODIVCLK / 2)
#elif STM32_MCOPRE == STM32_MCOPRE_DIV4
#define STM32_MCOCLK                (STM32_MCODIVCLK / 4)
#elif STM32_MCOPRE == STM32_MCOPRE_DIV8
#define STM32_MCOCLK                (STM32_MCODIVCLK / 8)
#elif STM32_MCOPRE == STM32_MCOPRE_DIV16
#define STM32_MCOCLK                (STM32_MCODIVCLK / 16)
#else
#error "invalid STM32_MCOPRE value specified"
#endif

/**
 * @brief   HSE divider toward RTC clock.
 */
#if (STM32_RTCPRE == STM32_RTCPRE_DIV2) || defined(__DOXYGEN__)
#define STM32_HSEDIVCLK             (STM32_HSECLK / 2)
#elif (STM32_RTCPRE == STM32_RTCPRE_DIV4) || defined(__DOXYGEN__)
#define STM32_HSEDIVCLK             (STM32_HSECLK / 4)
#elif (STM32_RTCPRE == STM32_RTCPRE_DIV8) || defined(__DOXYGEN__)
#define STM32_HSEDIVCLK             (STM32_HSECLK / 8)
#elif (STM32_RTCPRE == STM32_RTCPRE_DIV16) || defined(__DOXYGEN__)
#define STM32_HSEDIVCLK             (STM32_HSECLK / 16)
#else
#error "invalid STM32_RTCPRE value specified"
#endif

/**
 * @brief   RTC/LCD clock.
 */
#if (STM32_RTCSEL == STM32_RTCSEL_NOCLOCK) || defined(__DOXYGEN__)
#define STM32_RTCCLK                  0
#elif STM32_RTCSEL == STM32_RTCSEL_LSE
#define STM32_RTCCLK                  STM32_LSECLK
#elif STM32_RTCSEL == STM32_RTCSEL_LSI
#define STM32_RTCCLK                  STM32_LSICLK
#elif STM32_RTCSEL == STM32_RTCSEL_HSEDIV
#define STM32_RTCCLK                  STM32_HSEDIVCLK
#else
#error "invalid STM32_RTCSEL value specified"
#endif

/**
 * @brief   ADC frequency.
 */
#define STM32_ADCCLK                STM32_HSICLK

/**
 * @brief   USB frequency.
 */
#define STM32_USBCLK                (STM32_PLLVCO / 2)

/**
 * @brief   Timers 2, 3, 4, 6, 7 clock.
 */
#if (STM32_PPRE1 == STM32_PPRE1_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK1               (STM32_PCLK1 * 1)
#else
#define STM32_TIMCLK1               (STM32_PCLK1 * 2)
#endif

/**
 * @brief   Timers 9, 10, 11 clock.
 */
#if (STM32_PPRE2 == STM32_PPRE2_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK2               (STM32_PCLK2 * 1)
#else
#define STM32_TIMCLK2               (STM32_PCLK2 * 2)
#endif

/**
 * @brief   Flash settings.
 */
#if (STM32_HCLK <= STM32_0WS_THRESHOLD) || defined(__DOXYGEN__)
#define STM32_FLASHBITS1            0x00000000
#else
#define STM32_FLASHBITS1            0x00000004
#define STM32_FLASHBITS2            0x00000007
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type representing a system clock frequency.
 */
typedef uint32_t halclock_t;

/**
 * @brief   Type of the realtime free counter value.
 */
typedef uint32_t halrtcnt_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the current value of the system free running counter.
 * @note    This service is implemented by returning the content of the
 *          DWT_CYCCNT register.
 *
 * @return              The value of the system free running counter of
 *                      type halrtcnt_t.
 *
 * @notapi
 */
#define hal_lld_get_counter_value()         DWT_CYCCNT

/**
 * @brief   Realtime counter frequency.
 * @note    The DWT_CYCCNT register is incremented directly by the system
 *          clock so this function returns STM32_HCLK.
 *
 * @return              The realtime counter frequency of type halclock_t.
 *
 * @notapi
 */
#define hal_lld_get_counter_frequency()     STM32_HCLK

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
