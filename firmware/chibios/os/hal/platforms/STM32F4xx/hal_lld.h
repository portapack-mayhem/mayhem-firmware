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
 * @file    STM32F4xx/hal_lld.h
 * @brief   STM32F4xx/STM32F2xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          - STM32_VDD (as hundredths of Volt).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F2XX for High-performance STM32 F-2 devices.
 *          - STM32F401xx for High-performance STM32 F-4 devices.
 *          - STM32F40_41xxx for High-performance STM32 F-4 devices.
 *          - STM32F427_437xx for High-performance STM32 F-4 devices.
 *          - STM32F429_439xx for High-performance STM32 F-4 devices.
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "stm32.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS TRUE

/**
 * @name    Platform identification macros
 * @{
 */
#if defined(STM32F429_439xx) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32F429/F439 High Performance with DSP and FPU"
#define STM32F4XX

#elif defined(STM32F427_437xx)
#define PLATFORM_NAME           "STM32F427/F437 High Performance with DSP and FPU"
#define STM32F4XX

#elif defined(STM32F40_41xxx)
#define PLATFORM_NAME           "STM32F407/F417 High Performance with DSP and FPU"
#define STM32F4XX

#elif defined(STM32F401xx)
#define PLATFORM_NAME           "STM32F401 High Performance with DSP and FPU"
#define STM32F4XX

#elif defined(STM32F2XX)
#define PLATFORM_NAME           "STM32F2xx High Performance"

#else
#error "STM32F2xx/F4xx device not specified"
#endif
/** @} */

/**
 * @name    Absolute Maximum Ratings
 * @{
 */
/**
 * @name    Absolute Maximum Ratings
 * @{
 */
#if defined(STM32F427_437xx) || defined(STM32F429_439xx) ||                 \
    defined(__DOXYGEN__)
/**
 * @brief   Absolute maximum system clock.
 */
#define STM32_SYSCLK_MAX        180000000

/**
 * @brief   Maximum HSE clock frequency.
 */
#define STM32_HSECLK_MAX        26000000

/**
 * @brief   Maximum HSE clock frequency using an external source.
 */
#define STM32_HSECLK_BYP_MAX    50000000

/**
 * @brief   Minimum HSE clock frequency.
 */
#define STM32_HSECLK_MIN        4000000

/**
 * @brief   Minimum HSE clock frequency.
 */
#define STM32_HSECLK_BYP_MIN    1000000

/**
 * @brief   Maximum LSE clock frequency.
 */
#define STM32_LSECLK_MAX        32768

/**
 * @brief   Maximum LSE clock frequency.
 */
#define STM32_LSECLK_BYP_MAX    1000000

/**
 * @brief   Minimum LSE clock frequency.
 */
#define STM32_LSECLK_MIN        32768

/**
 * @brief   Maximum PLLs input clock frequency.
 */
#define STM32_PLLIN_MAX         2100000

/**
 * @brief   Minimum PLLs input clock frequency.
 */
#define STM32_PLLIN_MIN         950000

/**
 * @brief   Maximum PLLs VCO clock frequency.
 */
#define STM32_PLLVCO_MAX        432000000

/**
 * @brief   Maximum PLLs VCO clock frequency.
 */
#define STM32_PLLVCO_MIN        192000000

/**
 * @brief   Maximum PLL output clock frequency.
 */
#define STM32_PLLOUT_MAX        180000000

/**
 * @brief   Minimum PLL output clock frequency.
 */
#define STM32_PLLOUT_MIN        24000000

/**
 * @brief   Maximum APB1 clock frequency.
 */
#define STM32_PCLK1_MAX         (STM32_PLLOUT_MAX /4)

/**
 * @brief   Maximum APB2 clock frequency.
 */
#define STM32_PCLK2_MAX         (STM32_PLLOUT_MAX / 2)

/**
 * @brief   Maximum SPI/I2S clock frequency.
 */
#define STM32_SPII2S_MAX        45000000
#endif /* STM32F40_41xxx */

#if defined(STM32F40_41xxx) || defined(__DOXYGEN__)
#define STM32_SYSCLK_MAX        168000000
#define STM32_HSECLK_MAX        26000000
#define STM32_HSECLK_BYP_MAX    50000000
#define STM32_HSECLK_MIN        4000000
#define STM32_HSECLK_BYP_MIN    1000000
#define STM32_LSECLK_MAX        32768
#define STM32_LSECLK_BYP_MAX    1000000
#define STM32_LSECLK_MIN        32768
#define STM32_PLLIN_MAX         2100000
#define STM32_PLLIN_MIN         950000
#define STM32_PLLVCO_MAX        432000000
#define STM32_PLLVCO_MIN        192000000
#define STM32_PLLOUT_MAX        168000000
#define STM32_PLLOUT_MIN        24000000
#define STM32_PCLK1_MAX         42000000
#define STM32_PCLK2_MAX         84000000
#define STM32_SPII2S_MAX        42000000
#endif /* STM32F40_41xxx */

#if defined(STM32F401xx) || defined(__DOXYGEN__)
#define STM32_SYSCLK_MAX        84000000
#define STM32_HSECLK_MAX        26000000
#define STM32_HSECLK_BYP_MAX    50000000
#define STM32_HSECLK_MIN        4000000
#define STM32_HSECLK_BYP_MIN    1000000
#define STM32_LSECLK_MAX        32768
#define STM32_LSECLK_BYP_MAX    1000000
#define STM32_LSECLK_MIN        32768
#define STM32_PLLIN_MAX         2100000
#define STM32_PLLIN_MIN         950000
#define STM32_PLLVCO_MAX        432000000
#define STM32_PLLVCO_MIN        192000000
#define STM32_PLLOUT_MAX        84000000
#define STM32_PLLOUT_MIN        24000000
#define STM32_PCLK1_MAX         42000000
#define STM32_PCLK2_MAX         84000000
#define STM32_SPII2S_MAX        42000000
#endif /* STM32F40_41xxx */

#if defined(STM32F2XX)
#define STM32_SYSCLK_MAX        120000000
#define STM32_HSECLK_MAX        26000000
#define STM32_HSECLK_BYP_MAX    26000000
#define STM32_HSECLK_MIN        1000000
#define STM32_HSECLK_BYP_MIN    1000000
#define STM32_LSECLK_MAX        32768
#define STM32_LSECLK_BYP_MAX    1000000
#define STM32_LSECLK_MIN        32768
#define STM32_PLLIN_MAX         2000000
#define STM32_PLLIN_MIN         950000
#define STM32_PLLVCO_MAX        432000000
#define STM32_PLLVCO_MIN        192000000
#define STM32_PLLOUT_MAX        120000000
#define STM32_PLLOUT_MIN        24000000
#define STM32_PCLK1_MAX         30000000
#define STM32_PCLK2_MAX         60000000
#define STM32_SPII2S_MAX        30000000
#endif /* defined(STM32F2XX) */
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define STM32_HSICLK            16000000    /**< High speed internal clock. */
#define STM32_LSICLK            32000       /**< Low speed internal clock.  */
/** @} */

/**
 * @name    PWR_CR register bits definitions
 * @{
 */
#define STM32_VOS_SCALE3        (PWR_CR_VOS_0)
#define STM32_VOS_SCALE2        (PWR_CR_VOS_1)
#define STM32_VOS_SCALE1        (PWR_CR_VOS_1 | PWR_CR_VOS_0)
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
 * @name    RCC_PLLCFGR register bits definitions
 * @{
 */
#define STM32_PLLP_MASK			(3 << 16)	/**< PLLP mask.                 */
#define STM32_PLLP_DIV2			(0 << 16)	/**< PLL clock divided by 2.    */
#define STM32_PLLP_DIV4			(1 << 16)	/**< PLL clock divided by 4.    */
#define STM32_PLLP_DIV6			(2 << 16)   /**< PLL clock divided by 6.    */
#define STM32_PLLP_DIV8			(3 << 16)   /**< PLL clock divided by 8.    */

#define STM32_PLLSRC_HSI        (0 << 22)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 22)   /**< PLL clock source is HSE.   */
/** @} */

/**
 * @name    RCC_CFGR register bits definitions
 * @{
 */
#define STM32_SW_MASK           (3 << 0)    /**< SW mask.                   */
#define STM32_SW_HSI            (0 << 0)    /**< SYSCLK source is HSI.      */
#define STM32_SW_HSE            (1 << 0)    /**< SYSCLK source is HSE.      */
#define STM32_SW_PLL            (2 << 0)    /**< SYSCLK source is PLL.      */

#define STM32_HPRE_MASK         (15 << 4)   /**< HPRE mask.                 */
#define STM32_HPRE_DIV1         (0 << 4)    /**< SYSCLK divided by 1.       */
#define STM32_HPRE_DIV2         (8 << 4)    /**< SYSCLK divided by 2.       */
#define STM32_HPRE_DIV4         (9 << 4)    /**< SYSCLK divided by 4.       */
#define STM32_HPRE_DIV8         (10 << 4)   /**< SYSCLK divided by 8.       */
#define STM32_HPRE_DIV16        (11 << 4)   /**< SYSCLK divided by 16.      */
#define STM32_HPRE_DIV64        (12 << 4)   /**< SYSCLK divided by 64.      */
#define STM32_HPRE_DIV128       (13 << 4)   /**< SYSCLK divided by 128.     */
#define STM32_HPRE_DIV256       (14 << 4)   /**< SYSCLK divided by 256.     */
#define STM32_HPRE_DIV512       (15 << 4)   /**< SYSCLK divided by 512.     */

#define STM32_PPRE1_MASK        (7 << 10)	/**< PPRE1 mask.                */
#define STM32_PPRE1_DIV1        (0 << 10)   /**< HCLK divided by 1.         */
#define STM32_PPRE1_DIV2        (4 << 10)   /**< HCLK divided by 2.         */
#define STM32_PPRE1_DIV4        (5 << 10)   /**< HCLK divided by 4.         */
#define STM32_PPRE1_DIV8        (6 << 10)   /**< HCLK divided by 8.         */
#define STM32_PPRE1_DIV16       (7 << 10)   /**< HCLK divided by 16.        */

#define STM32_PPRE2_MASK        (7 << 13)	/**< PPRE2 mask.                */
#define STM32_PPRE2_DIV1        (0 << 13)   /**< HCLK divided by 1.         */
#define STM32_PPRE2_DIV2        (4 << 13)   /**< HCLK divided by 2.         */
#define STM32_PPRE2_DIV4        (5 << 13)   /**< HCLK divided by 4.         */
#define STM32_PPRE2_DIV8        (6 << 13)   /**< HCLK divided by 8.         */
#define STM32_PPRE2_DIV16       (7 << 13)   /**< HCLK divided by 16.        */

#define STM32_RTCPRE_MASK       (31 << 16)  /**< RTCPRE mask.               */

#define STM32_MCO1SEL_MASK      (3 << 21)   /**< MCO1 mask.                 */
#define STM32_MCO1SEL_HSI       (0 << 21)   /**< HSI clock on MCO1 pin.     */
#define STM32_MCO1SEL_LSE       (1 << 21)   /**< LSE clock on MCO1 pin.     */
#define STM32_MCO1SEL_HSE       (2 << 21)   /**< HSE clock on MCO1 pin.     */
#define STM32_MCO1SEL_PLL       (3 << 21)   /**< PLL clock on MCO1 pin.     */

#define STM32_I2SSRC_MASK       (1 << 23)   /**< I2CSRC mask.               */
#define STM32_I2SSRC_PLLI2S     (0 << 23)   /**< I2SSRC is PLLI2S.          */
#define STM32_I2SSRC_CKIN       (1 << 23)   /**< I2S_CKIN is PLLI2S.        */

#define STM32_MCO1PRE_MASK      (7 << 24)   /**< MCO1PRE mask.              */
#define STM32_MCO1PRE_DIV1      (0 << 24)   /**< MCO1 divided by 1.         */
#define STM32_MCO1PRE_DIV2      (4 << 24)   /**< MCO1 divided by 2.         */
#define STM32_MCO1PRE_DIV3      (5 << 24)   /**< MCO1 divided by 3.         */
#define STM32_MCO1PRE_DIV4      (6 << 24)   /**< MCO1 divided by 4.         */
#define STM32_MCO1PRE_DIV5      (7 << 24)   /**< MCO1 divided by 5.         */

#define STM32_MCO2PRE_MASK      (7 << 27)   /**< MCO2PRE mask.              */
#define STM32_MCO2PRE_DIV1      (0 << 27)   /**< MCO2 divided by 1.         */
#define STM32_MCO2PRE_DIV2      (4 << 27)   /**< MCO2 divided by 2.         */
#define STM32_MCO2PRE_DIV3      (5 << 27)   /**< MCO2 divided by 3.         */
#define STM32_MCO2PRE_DIV4      (6 << 27)   /**< MCO2 divided by 4.         */
#define STM32_MCO2PRE_DIV5      (7 << 27)   /**< MCO2 divided by 5.         */

#define STM32_MCO2SEL_MASK      (3U << 30)  /**< MCO2 mask.                 */
#define STM32_MCO2SEL_SYSCLK    (0U << 30)  /**< SYSCLK clock on MCO2 pin.  */
#define STM32_MCO2SEL_PLLI2S    (1U << 30)  /**< PLLI2S clock on MCO2 pin.  */
#define STM32_MCO2SEL_HSE       (2U << 30)  /**< HSE clock on MCO2 pin.     */
#define STM32_MCO2SEL_PLL       (3U << 30)  /**< PLL clock on MCO2 pin.     */

#define STM32_RTC_NOCLOCK       (0 << 8)    /**< No clock.                  */
#define STM32_RTC_LSE           (1 << 8)    /**< LSE used as RTC clock.     */
#define STM32_RTC_LSI           (2 << 8)    /**< LSI used as RTC clock.     */
#define STM32_RTC_HSE           (3 << 8)    /**< HSE divided by programmable
                                                 prescaler used as RTC clock*/

/**
 * @name    RCC_PLLI2SCFGR register bits definitions
 * @{
 */
#define STM32_PLLI2SN_MASK      (511 << 6)  /**< PLLI2SN mask.              */
#define STM32_PLLI2SR_MASK      (7 << 28)   /**< PLLI2SR mask.              */
/** @} */

/**
 * @name    RCC_BDCR register bits definitions
 * @{
 */
#define STM32_RTCSEL_MASK       (3 << 8)    /**< RTC source mask.           */
#define STM32_RTCSEL_NOCLOCK    (0 << 8)    /**< No RTC source.             */
#define STM32_RTCSEL_LSE        (1 << 8)    /**< RTC source is LSE.         */
#define STM32_RTCSEL_LSI        (2 << 8)    /**< RTC source is LSI.         */
#define STM32_RTCSEL_HSEDIV     (3 << 8)    /**< RTC source is HSE divided. */
/** @} */

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    STM32F4xx capabilities
 * @{
 */
/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_ADC1_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(2, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 4))
#define STM32_ADC1_DMA_CHN      0x00000000

#define STM32_HAS_ADC2          TRUE
#define STM32_ADC2_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(2, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 3))
#define STM32_ADC2_DMA_CHN      0x00001100

#define STM32_HAS_ADC3          TRUE
#define STM32_ADC3_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(2, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 1))
#define STM32_ADC3_DMA_CHN      0x00000022

#define STM32_HAS_ADC4          FALSE
#define STM32_ADC4_DMA_MSK      0x00000000
#define STM32_ADC4_DMA_CHN      0x00000000

/* CAN attributes.*/
#define STM32_HAS_CAN1          TRUE
#define STM32_HAS_CAN2          TRUE
#define STM32_CAN_MAX_FILTERS   28

/* DAC attributes.*/
#define STM32_HAS_DAC           FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      TRUE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          TRUE

/* ETH attributes.*/
#if !defined(STM32F401xx)
#define STM32_HAS_ETH           TRUE
#else /* defined(STM32F401xx) */
#define STM32_HAS_ETH           FALSE
#endif /* defined(STM32F401xx) */

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 23

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         TRUE
#define STM32_HAS_GPIOH         TRUE
#if !defined(STM32F401xx)
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         TRUE
#define STM32_HAS_GPIOI         TRUE
#else /* defined(STM32F401xx) */
#define STM32_HAS_GPIOF         FALSE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOI         FALSE
#endif /* defined(STM32F401xx) */

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C1_RX_DMA_CHN   0x00100001
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_I2C1_TX_DMA_CHN   0x11000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_I2C2_RX_DMA_CHN   0x00007700
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_I2C2_TX_DMA_CHN   0x70000000

#define STM32_HAS_I2C3          TRUE
#define STM32_I2C3_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_I2C3_RX_DMA_CHN   0x00000300
#define STM32_I2C3_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C3_TX_DMA_CHN   0x00030000

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#if defined(STM32F4XX) || defined(__DOXYGEN__)
#define STM32_RTC_HAS_SUBSECONDS TRUE
#else
#define STM32_RTC_HAS_SUBSECONDS FALSE
#endif
#define STM32_RTC_IS_CALENDAR   TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO          TRUE
#define STM32_SDC_SDIO_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(2, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 6))
#define STM32_SDC_SDIO_DMA_CHN  0x04004000

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 2))
#define STM32_SPI1_RX_DMA_CHN   0x00000303
#define STM32_SPI1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_SPI1_TX_DMA_CHN   0x00303000

#define STM32_HAS_SPI2          TRUE
#define STM32_SPI2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_SPI2_RX_DMA_CHN   0x00000000
#define STM32_SPI2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_SPI2_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI3          TRUE
#define STM32_SPI3_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_SPI3_TX_DMA_CHN   0x00000000

#if defined(STM32F427_437xx) || defined(STM32F429_439xx) ||                 \
    defined(STM32F401xx)
#define STM32_HAS_SPI4          TRUE
#define STM32_SPI4_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 0) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 3))
#define STM32_SPI4_RX_DMA_CHN   0x00005004
#define STM32_SPI4_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 1) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 4))
#define STM32_SPI4_TX_DMA_CHN   0x00050040
#else
#define STM32_HAS_SPI4          FALSE
#endif

#if defined(STM32F427_437xx) || defined(STM32F429_439xx)
#define STM32_HAS_SPI5          TRUE
#define STM32_SPI5_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_SPI5_RX_DMA_CHN   0x00702000
#define STM32_SPI5_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 4) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 6))
#define STM32_SPI5_TX_DMA_CHN   0x07020000

#define STM32_HAS_SPI6          TRUE
#define STM32_SPI6_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 6))
#define STM32_SPI6_RX_DMA_CHN   0x01000000
#define STM32_SPI6_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_SPI6_TX_DMA_CHN   0x00100000

#else /* !(defined(STM32F427_437xx) || defined(STM32F429_439xx)) */
#define STM32_HAS_SPI5          FALSE
#define STM32_HAS_SPI6          FALSE
#endif /* !(defined(STM32F427_437xx) || defined(STM32F429_439xx)) */

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          TRUE
#define STM32_HAS_TIM5          TRUE
#if !defined(STM32F401xx)
#define STM32_HAS_TIM6          TRUE
#define STM32_HAS_TIM7          TRUE
#define STM32_HAS_TIM8          TRUE
#else /* defined(STM32F401xx) */
#define STM32_HAS_TIM6          FALSE
#define STM32_HAS_TIM7          FALSE
#define STM32_HAS_TIM8          FALSE
#endif /* defined(STM32F401xx) */
#define STM32_HAS_TIM9          TRUE
#define STM32_HAS_TIM10         TRUE
#define STM32_HAS_TIM11         TRUE
#if !defined(STM32F401xx)
#define STM32_HAS_TIM12         TRUE
#define STM32_HAS_TIM13         TRUE
#define STM32_HAS_TIM14         TRUE
#else /* defined(STM32F401xx) */
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         FALSE
#endif /* defined(STM32F401xx) */
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         FALSE
#define STM32_HAS_TIM17         FALSE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(2, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_USART1_RX_DMA_CHN 0x00400400
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(2, 7))
#define STM32_USART1_TX_DMA_CHN 0x40000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART2_RX_DMA_CHN 0x00400000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_USART2_TX_DMA_CHN 0x04000000

#if !defined(STM32F401xx)
#define STM32_HAS_USART3        TRUE
#define STM32_USART3_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 1))
#define STM32_USART3_RX_DMA_CHN 0x00000040
#define STM32_USART3_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART3_TX_DMA_CHN 0x00074000

#define STM32_HAS_UART4         TRUE
#define STM32_UART4_RX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_UART4_RX_DMA_CHN  0x00000400
#define STM32_UART4_TX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_UART4_TX_DMA_CHN  0x00040000

#define STM32_HAS_UART5         TRUE
#define STM32_UART5_RX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(1, 0))
#define STM32_UART5_RX_DMA_CHN  0x00000004
#define STM32_UART5_TX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_UART5_TX_DMA_CHN  0x40000000

#else /* defined(STM32F401xx) */
#define STM32_HAS_USART3        FALSE
#define STM32_HAS_UART4         FALSE
#define STM32_HAS_UART5         FALSE
#endif /* defined(STM32F401xx) */

#define STM32_HAS_USART6        TRUE
#define STM32_USART6_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(2, 1) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 2))
#define STM32_USART6_RX_DMA_CHN 0x00000550
#define STM32_USART6_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(2, 6) |            \
                                 STM32_DMA_STREAM_ID_MSK(2, 7))
#define STM32_USART6_TX_DMA_CHN 0x55000000

/* USB attributes.*/
#define STM32_HAS_USB           FALSE
#define STM32_HAS_OTG1          TRUE
#if !defined(STM32F401xx)
#define STM32_HAS_OTG2          TRUE
#else /* defined(STM32F401xx) */
#define STM32_HAS_OTG2          FALSE
#endif /* defined(STM32F401xx) */
/** @} */

/*===========================================================================*/
/* Platform specific friendly IRQ names.                                     */
/*===========================================================================*/

/**
 * @name    IRQ VECTOR names
 * @{
 */
#define WWDG_IRQHandler         Vector40    /**< Window Watchdog.           */
#define PVD_IRQHandler          Vector44    /**< PVD through EXTI Line
                                                 detect.                    */
#define TAMP_STAMP_IRQHandler   Vector48    /**< Tamper and TimeStamp
                                                 through EXTI Line.         */
#define RTC_WKUP_IRQHandler     Vector4C    /**< RTC wakeup EXTI Line.      */
#define FLASH_IRQHandler        Vector50    /**< Flash.                     */
#define RCC_IRQHandler          Vector54    /**< RCC.                       */
#define EXTI0_IRQHandler        Vector58    /**< EXTI Line 0.               */
#define EXTI1_IRQHandler        Vector5C    /**< EXTI Line 1.               */
#define EXTI2_IRQHandler        Vector60    /**< EXTI Line 2.               */
#define EXTI3_IRQHandler        Vector64    /**< EXTI Line 3.               */
#define EXTI4_IRQHandler        Vector68    /**< EXTI Line 4.               */
#define DMA1_Stream0_IRQHandler Vector6C    /**< DMA1 Stream 0.             */
#define DMA1_Stream1_IRQHandler Vector70    /**< DMA1 Stream 1.             */
#define DMA1_Stream2_IRQHandler Vector74    /**< DMA1 Stream 2.             */
#define DMA1_Stream3_IRQHandler Vector78    /**< DMA1 Stream 3.             */
#define DMA1_Stream4_IRQHandler Vector7C    /**< DMA1 Stream 4.             */
#define DMA1_Stream5_IRQHandler Vector80    /**< DMA1 Stream 5.             */
#define DMA1_Stream6_IRQHandler Vector84    /**< DMA1 Stream 6.             */
#define ADC1_2_3_IRQHandler     Vector88    /**< ADC1, ADC2 and ADC3.       */
#define CAN1_TX_IRQHandler      Vector8C    /**< CAN1 TX.                   */
#define CAN1_RX0_IRQHandler     Vector90    /**< CAN1 RX0.                  */
#define CAN1_RX1_IRQHandler     Vector94    /**< CAN1 RX1.                  */
#define CAN1_SCE_IRQHandler     Vector98    /**< CAN1 SCE.                  */
#define EXTI9_5_IRQHandler      Vector9C    /**< EXTI Line 9..5.            */
#define TIM1_BRK_IRQHandler     VectorA0    /**< TIM1 Break.                */
#define TIM1_UP_IRQHandler      VectorA4    /**< TIM1 Update.               */
#define TIM1_TRG_COM_IRQHandler VectorA8    /**< TIM1 Trigger and
                                                 Commutation.               */
#define TIM1_CC_IRQHandler      VectorAC    /**< TIM1 Capture Compare.      */
#define TIM2_IRQHandler         VectorB0    /**< TIM2.                      */
#define TIM3_IRQHandler         VectorB4    /**< TIM3.                      */
#define TIM4_IRQHandler         VectorB8    /**< TIM4.                      */
#define I2C1_EV_IRQHandler      VectorBC    /**< I2C1 Event.                */
#define I2C1_ER_IRQHandler      VectorC0    /**< I2C1 Error.                */
#define I2C2_EV_IRQHandler      VectorC4    /**< I2C2 Event.                */
#define I2C2_ER_IRQHandler      VectorC8    /**< I2C1 Error.                */
#define SPI1_IRQHandler         VectorCC    /**< SPI1.                      */
#define SPI2_IRQHandler         VectorD0    /**< SPI2.                      */
#define USART1_IRQHandler       VectorD4    /**< USART1.                    */
#define USART2_IRQHandler       VectorD8    /**< USART2.                    */
#define USART3_IRQHandler       VectorDC    /**< USART3.                    */
#define EXTI15_10_IRQHandler    VectorE0    /**< EXTI Line 15..10.          */
#define RTC_Alarm_IRQHandler    VectorE4    /**< RTC alarms (A and B)
                                                 through EXTI line.         */
#define OTG_FS_WKUP_IRQHandler  VectorE8    /**< USB OTG FS Wakeup through
                                                 EXTI line.                 */
#define TIM8_BRK_IRQHandler     VectorEC    /**< TIM8 Break.                */
#define TIM8_UP_IRQHandler      VectorF0    /**< TIM8 Update.               */
#define TIM8_TRG_COM_IRQHandler VectorF4    /**< TIM8 Trigger and
                                                 Commutation.               */
#define TIM8_CC_IRQHandler      VectorF8    /**< TIM8 Capture Compare.      */
#define DMA1_Stream7_IRQHandler VectorFC    /**< DMA1 Stream 7.             */
#define FSMC_IRQHandler         Vector100   /**< FSMC.                      */
#define SDIO_IRQHandler         Vector104   /**< SDIO.                      */
#define TIM5_IRQHandler         Vector108   /**< TIM5.                      */
#define SPI3_IRQHandler         Vector10C   /**< SPI3.                      */
#define UART4_IRQHandler        Vector110   /**< UART4.                     */
#define UART5_IRQHandler        Vector114   /**< UART5.                     */
#define TIM6_IRQHandler         Vector118   /**< TIM6.                      */
#define TIM7_IRQHandler         Vector11C   /**< TIM7.                      */
#define DMA2_Stream0_IRQHandler Vector120   /**< DMA2 Stream0.              */
#define DMA2_Stream1_IRQHandler Vector124   /**< DMA2 Stream1.              */
#define DMA2_Stream2_IRQHandler Vector128   /**< DMA2 Stream2.              */
#define DMA2_Stream3_IRQHandler Vector12C   /**< DMA2 Stream3.              */
#define DMA2_Stream4_IRQHandler Vector130   /**< DMA2 Stream4.              */
#define ETH_IRQHandler          Vector134   /**< Ethernet.                  */
#define ETH_WKUP_IRQHandler     Vector138   /**< Ethernet Wakeup through
                                                 EXTI line.                 */
#define CAN2_TX_IRQHandler      Vector13C   /**< CAN2 TX.                   */
#define CAN2_RX0_IRQHandler     Vector140   /**< CAN2 RX0.                  */
#define CAN2_RX1_IRQHandler     Vector144   /**< CAN2 RX1.                  */
#define CAN2_SCE_IRQHandler     Vector148   /**< CAN2 SCE.                  */
#define OTG_FS_IRQHandler       Vector14C   /**< USB OTG FS.                */
#define DMA2_Stream5_IRQHandler Vector150   /**< DMA2 Stream5.              */
#define DMA2_Stream6_IRQHandler Vector154   /**< DMA2 Stream6.              */
#define DMA2_Stream7_IRQHandler Vector158   /**< DMA2 Stream7.              */
#define USART6_IRQHandler       Vector15C   /**< USART6.                    */
#define I2C3_EV_IRQHandler      Vector160   /**< I2C3 Event.                */
#define I2C3_ER_IRQHandler      Vector164   /**< I2C3 Error.                */
#define OTG_HS_EP1_OUT_IRQHandler Vector168 /**< USB OTG HS End Point 1 Out.*/
#define OTG_HS_EP1_IN_IRQHandler Vector16C  /**< USB OTG HS End Point 1 In. */
#define OTG_HS_WKUP_IRQHandler  Vector170   /**< USB OTG HS Wakeup through
                                                 EXTI line.                 */
#define OTG_HS_IRQHandler       Vector174   /**< USB OTG HS.                */
#define DCMI_IRQHandler         Vector178   /**< DCMI.                      */
#define CRYP_IRQHandler         Vector17C   /**< CRYP.                      */
#define HASH_RNG_IRQHandler     Vector180   /**< Hash and Rng.              */
#if defined(STM32F4XX) || defined(__DOXYGEN__)
#define FPU_IRQHandler          Vector184   /**< Floating Point Unit.       */
#endif
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
 * @brief   Enables the backup RAM regulator.
 */
#if !defined(STM32_BKPRAM_ENABLE) || defined(__DOXYGEN__)
#define STM32_BKPRAM_ENABLE         FALSE
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
 * @brief   USB/SDIO clock setting.
 */
#if !defined(STM32_CLOCK48_REQUIRED) || defined(__DOXYGEN__)
#define STM32_CLOCK48_REQUIRED      TRUE
#endif

/**
 * @brief   Main clock source selection.
 * @note    If the selected clock source is not the PLL then the PLL is not
 *          initialized and started.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_SW) || defined(__DOXYGEN__)
#define STM32_SW                    STM32_SW_PLL
#endif

#if defined(STM32F4XX) || defined(__DOXYGEN__)
/**
 * @brief   Clock source for the PLLs.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLSRC) || defined(__DOXYGEN__)
#define STM32_PLLSRC                STM32_PLLSRC_HSE
#endif

/**
 * @brief   PLLM divider value.
 * @note    The allowed values are 2..63.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLM_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLM_VALUE            8
#endif

/**
 * @brief   PLLN multiplier value.
 * @note    The allowed values are 192..432.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLN_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLN_VALUE            336
#endif

/**
 * @brief   PLLP divider value.
 * @note    The allowed values are 2, 4, 6, 8.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLP_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLP_VALUE            2
#endif

/**
 * @brief   PLLQ multiplier value.
 * @note    The allowed values are 2..15.
 * @note    The default value is calculated for a 168MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLQ_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLQ_VALUE            7
#endif

#else /* !defined(STM32F4XX) */
/**
 * @brief   Clock source for the PLLs.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 120MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLSRC) || defined(__DOXYGEN__)
#define STM32_PLLSRC                STM32_PLLSRC_HSE
#endif

/**
 * @brief   PLLM divider value.
 * @note    The allowed values are 2..63.
 * @note    The default value is calculated for a 120MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLM_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLM_VALUE            8
#endif

/**
 * @brief   PLLN multiplier value.
 * @note    The allowed values are 192..432.
 * @note    The default value is calculated for a 120MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLN_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLN_VALUE            240
#endif

/**
 * @brief   PLLP divider value.
 * @note    The allowed values are 2, 4, 6, 8.
 * @note    The default value is calculated for a 120MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLP_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLP_VALUE            2
#endif

/**
 * @brief   PLLQ multiplier value.
 * @note    The allowed values are 2..15.
 * @note    The default value is calculated for a 120MHz system clock from
 *          an external 8MHz HSE clock.
 */
#if !defined(STM32_PLLQ_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLQ_VALUE            5
#endif
#endif /* !defined(STM32F4XX) */

/**
 * @brief   AHB prescaler value.
 */
#if !defined(STM32_HPRE) || defined(__DOXYGEN__)
#define STM32_HPRE                  STM32_HPRE_DIV1
#endif

/**
 * @brief   APB1 prescaler value.
 */
#if !defined(STM32_PPRE1) || defined(__DOXYGEN__)
#define STM32_PPRE1                 STM32_PPRE1_DIV4
#endif

/**
 * @brief   APB2 prescaler value.
 */
#if !defined(STM32_PPRE2) || defined(__DOXYGEN__)
#define STM32_PPRE2                 STM32_PPRE2_DIV2
#endif

/**
 * @brief   RTC clock source.
 */
#if !defined(STM32_RTCSEL) || defined(__DOXYGEN__)
#define STM32_RTCSEL                STM32_RTCSEL_LSE
#endif

/**
 * @brief   RTC HSE prescaler value.
 */
#if !defined(STM32_RTCPRE_VALUE) || defined(__DOXYGEN__)
#define STM32_RTCPRE_VALUE          8
#endif

/**
 * @brief   MC01 clock source value.
 * @note    The default value outputs HSI clock on MC01 pin.
 */
#if !defined(STM32_MCO1SEL) || defined(__DOXYGEN__)
#define STM32_MCO1SEL               STM32_MCO1SEL_HSI
#endif

/**
 * @brief   MC01 prescaler value.
 * @note    The default value outputs HSI clock on MC01 pin.
 */
#if !defined(STM32_MCO1PRE) || defined(__DOXYGEN__)
#define STM32_MCO1PRE               STM32_MCO1PRE_DIV1
#endif

/**
 * @brief   MC02 clock source value.
 * @note    The default value outputs SYSCLK / 5 on MC02 pin.
 */
#if !defined(STM32_MCO2SEL) || defined(__DOXYGEN__)
#define STM32_MCO2SEL               STM32_MCO2SEL_SYSCLK
#endif

/**
 * @brief   MC02 prescaler value.
 * @note    The default value outputs SYSCLK / 5 on MC02 pin.
 */
#if !defined(STM32_MCO2PRE) || defined(__DOXYGEN__)
#define STM32_MCO2PRE               STM32_MCO2PRE_DIV5
#endif

/**
 * @brief   I2S clock source.
 */
#if !defined(STM32_I2SSRC) || defined(__DOXYGEN__)
#define STM32_I2SSRC                STM32_I2SSRC_CKIN
#endif

/**
 * @brief   PLLI2SN multiplier value.
 * @note    The allowed values are 192..432.
 */
#if !defined(STM32_PLLI2SN_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLI2SN_VALUE         192
#endif

/**
 * @brief   PLLI2SR multiplier value.
 * @note    The allowed values are 2..7.
 */
#if !defined(STM32_PLLI2SR_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLI2SR_VALUE         5
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if defined(STM32F4XX) || defined(__DOXYGEN__)
/*
 * Configuration-related checks.
 */
#if !defined(STM32F4xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32F4xx_MCUCONF not defined"
#endif

#else /* !defined(STM32F4XX) */
/*
 * Configuration-related checks.
 */
#if !defined(STM32F2xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32F2xx_MCUCONF not defined"
#endif
#endif /* !defined(STM32F4XX) */

/**
 * @brief   Maximum frequency thresholds and wait states for flash access.
 * @note    The values are valid for 2.7V to 3.6V supply range.
 */
#if defined(STM32F429_439xx) || defined(STM32F427_437xx) ||                 \
    defined(STM32F40_41xxx) || defined(__DOXYGEN__)
#if ((STM32_VDD >= 270) && (STM32_VDD <= 360)) || defined(__DOXYGEN__)
#define STM32_0WS_THRESHOLD         30000000
#define STM32_1WS_THRESHOLD         60000000
#define STM32_2WS_THRESHOLD         90000000
#define STM32_3WS_THRESHOLD         120000000
#define STM32_4WS_THRESHOLD         150000000
#define STM32_5WS_THRESHOLD         180000000
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#define STM32_8WS_THRESHOLD         0
#elif (STM32_VDD >= 240) && (STM32_VDD < 270)
#define STM32_0WS_THRESHOLD         24000000
#define STM32_1WS_THRESHOLD         48000000
#define STM32_2WS_THRESHOLD         72000000
#define STM32_3WS_THRESHOLD         96000000
#define STM32_4WS_THRESHOLD         120000000
#define STM32_5WS_THRESHOLD         144000000
#define STM32_6WS_THRESHOLD         168000000
#define STM32_7WS_THRESHOLD         180000000
#define STM32_8WS_THRESHOLD         0
#elif (STM32_VDD >= 210) && (STM32_VDD < 240)
#define STM32_0WS_THRESHOLD         22000000
#define STM32_1WS_THRESHOLD         44000000
#define STM32_2WS_THRESHOLD         66000000
#define STM32_3WS_THRESHOLD         88000000
#define STM32_4WS_THRESHOLD         110000000
#define STM32_5WS_THRESHOLD         132000000
#define STM32_6WS_THRESHOLD         154000000
#define STM32_7WS_THRESHOLD         176000000
#define STM32_8WS_THRESHOLD         180000000
#elif (STM32_VDD >= 180) && (STM32_VDD < 210)
#define STM32_0WS_THRESHOLD         20000000
#define STM32_1WS_THRESHOLD         40000000
#define STM32_2WS_THRESHOLD         60000000
#define STM32_3WS_THRESHOLD         80000000
#define STM32_4WS_THRESHOLD         100000000
#define STM32_5WS_THRESHOLD         120000000
#define STM32_6WS_THRESHOLD         140000000
#define STM32_7WS_THRESHOLD         168000000
#define STM32_8WS_THRESHOLD         0
#else
#error "invalid VDD voltage specified"
#endif

#elif defined(STM32F401xx)
#if (STM32_VDD >= 270) && (STM32_VDD <= 360)
#define STM32_0WS_THRESHOLD         30000000
#define STM32_1WS_THRESHOLD         60000000
#define STM32_2WS_THRESHOLD         84000000
#define STM32_3WS_THRESHOLD         0
#define STM32_4WS_THRESHOLD         0
#define STM32_5WS_THRESHOLD         0
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#define STM32_8WS_THRESHOLD         0
#elif (STM32_VDD >= 240) && (STM32_VDD < 270)
#define STM32_0WS_THRESHOLD         24000000
#define STM32_1WS_THRESHOLD         48000000
#define STM32_2WS_THRESHOLD         72000000
#define STM32_3WS_THRESHOLD         84000000
#define STM32_4WS_THRESHOLD         0
#define STM32_5WS_THRESHOLD         0
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#define STM32_8WS_THRESHOLD         0
#elif (STM32_VDD >= 210) && (STM32_VDD < 240)
#define STM32_0WS_THRESHOLD         18000000
#define STM32_1WS_THRESHOLD         36000000
#define STM32_2WS_THRESHOLD         54000000
#define STM32_3WS_THRESHOLD         72000000
#define STM32_4WS_THRESHOLD         84000000
#define STM32_5WS_THRESHOLD         0
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#define STM32_8WS_THRESHOLD         0
#elif (STM32_VDD >= 180) && (STM32_VDD < 210)
#define STM32_0WS_THRESHOLD         16000000
#define STM32_1WS_THRESHOLD         32000000
#define STM32_2WS_THRESHOLD         48000000
#define STM32_3WS_THRESHOLD         64000000
#define STM32_4WS_THRESHOLD         80000000
#define STM32_5WS_THRESHOLD         84000000
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#define STM32_8WS_THRESHOLD         0
#else
#error "invalid VDD voltage specified"
#endif

#else /* STM32F2XX */
#if (STM32_VDD >= 270) && (STM32_VDD <= 360)
#define STM32_0WS_THRESHOLD         30000000
#define STM32_1WS_THRESHOLD         60000000
#define STM32_2WS_THRESHOLD         90000000
#define STM32_3WS_THRESHOLD         120000000
#define STM32_4WS_THRESHOLD         0
#define STM32_5WS_THRESHOLD         0
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#elif (STM32_VDD >= 240) && (STM32_VDD < 270)
#define STM32_0WS_THRESHOLD         24000000
#define STM32_1WS_THRESHOLD         48000000
#define STM32_2WS_THRESHOLD         72000000
#define STM32_3WS_THRESHOLD         96000000
#define STM32_4WS_THRESHOLD         120000000
#define STM32_5WS_THRESHOLD         0
#define STM32_6WS_THRESHOLD         0
#define STM32_7WS_THRESHOLD         0
#elif (STM32_VDD >= 210) && (STM32_VDD < 240)
#define STM32_0WS_THRESHOLD         18000000
#define STM32_1WS_THRESHOLD         36000000
#define STM32_2WS_THRESHOLD         54000000
#define STM32_3WS_THRESHOLD         72000000
#define STM32_4WS_THRESHOLD         90000000
#define STM32_5WS_THRESHOLD         108000000
#define STM32_6WS_THRESHOLD         120000000
#define STM32_7WS_THRESHOLD         0
#elif (STM32_VDD >= 180) && (STM32_VDD < 210)
#define STM32_0WS_THRESHOLD         16000000
#define STM32_1WS_THRESHOLD         32000000
#define STM32_2WS_THRESHOLD         48000000
#define STM32_3WS_THRESHOLD         64000000
#define STM32_4WS_THRESHOLD         80000000
#define STM32_5WS_THRESHOLD         96000000
#define STM32_6WS_THRESHOLD         112000000
#define STM32_7WS_THRESHOLD         120000000
#else
#error "invalid VDD voltage specified"
#endif
#endif /* STM32F2XX */

/*
 * HSI related checks.
 */
#if STM32_HSI_ENABLED
#else /* !STM32_HSI_ENABLED */

#if STM32_SW == STM32_SW_HSI
#error "HSI not enabled, required by STM32_SW"
#endif

#if (STM32_SW == STM32_SW_PLL) && (STM32_PLLSRC == STM32_PLLSRC_HSI)
#error "HSI not enabled, required by STM32_SW and STM32_PLLSRC"
#endif

#if (STM32_MCO1SEL == STM32_MCO1SEL_HSI) ||                                 \
    ((STM32_MCO1SEL == STM32_MCO1SEL_PLL) &&                                \
     (STM32_PLLSRC == STM32_PLLSRC_HSI))
#error "HSI not enabled, required by STM32_MCO1SEL"
#endif

#if (STM32_MCO2SEL == STM32_MCO2SEL_PLL) && (STM32_PLLSRC == STM32_PLLSRC_HSI)
#error "HSI not enabled, required by STM32_MCO2SEL"
#endif

#if (STM32_I2SSRC == STM32_I2SSRC_PLLI2S) &&                                \
    (STM32_PLLSRC == STM32_PLLSRC_HSI)
#error "HSI not enabled, required by STM32_I2SSRC"
#endif

#endif /* !STM32_HSI_ENABLED */

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

#if (STM32_MCO1SEL == STM32_MCO1SEL_HSE) ||                                 \
    ((STM32_MCO1SEL == STM32_MCO1SEL_PLL) &&                                \
     (STM32_PLLSRC == STM32_PLLSRC_HSE))
#error "HSE not enabled, required by STM32_MCO1SEL"
#endif

#if (STM32_MCO2SEL == STM32_MCO2SEL_HSE) ||                                 \
    ((STM32_MCO2SEL == STM32_MCO2SEL_PLL) &&                                \
     (STM32_PLLSRC == STM32_PLLSRC_HSE))
#error "HSE not enabled, required by STM32_MCO2SEL"
#endif

#if (STM32_I2SSRC == STM32_I2SSRC_PLLI2S) &&                                \
    (STM32_PLLSRC == STM32_PLLSRC_HSE)
#error "HSE not enabled, required by STM32_I2SSRC"
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

#else /* !STM32_LSE_ENABLED */

#if STM32_RTCSEL == STM32_RTCSEL_LSE
#error "LSE not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSE_ENABLED */

/**
 * @brief   STM32_PLLM field.
 */
#if ((STM32_PLLM_VALUE >= 2) && (STM32_PLLM_VALUE <= 63)) ||                \
    defined(__DOXYGEN__)
#define STM32_PLLM                  (STM32_PLLM_VALUE << 0)
#else
#error "invalid STM32_PLLM_VALUE value specified"
#endif

/**
 * @brief   PLLs input clock frequency.
 */
#if (STM32_PLLSRC == STM32_PLLSRC_HSE) || defined(__DOXYGEN__)
#define STM32_PLLCLKIN              (STM32_HSECLK / STM32_PLLM_VALUE)
#elif STM32_PLLSRC == STM32_PLLSRC_HSI
#define STM32_PLLCLKIN              (STM32_HSICLK / STM32_PLLM_VALUE)
#else
#error "invalid STM32_PLLSRC value specified"
#endif

/*
 * PLLs input frequency range check.
 */
#if (STM32_PLLCLKIN < STM32_PLLIN_MIN) || (STM32_PLLCLKIN > STM32_PLLIN_MAX)
#error "STM32_PLLCLKIN outside acceptable range (STM32_PLLIN_MIN...STM32_PLLIN_MAX)"
#endif

/*
 * PLL enable check.
 */
#if STM32_CLOCK48_REQUIRED ||                                               \
    (STM32_SW == STM32_SW_PLL) ||                                           \
    (STM32_MCO1SEL == STM32_MCO1SEL_PLL) ||                                 \
    (STM32_MCO2SEL == STM32_MCO2SEL_PLL) ||                                 \
    defined(__DOXYGEN__)
/**
 * @brief   PLL activation flag.
 */
#define STM32_ACTIVATE_PLL          TRUE
#else
#define STM32_ACTIVATE_PLL          FALSE
#endif

/**
 * @brief   STM32_PLLN field.
 */
#if ((STM32_PLLN_VALUE >= 64) && (STM32_PLLN_VALUE <= 432)) ||              \
    defined(__DOXYGEN__)
#define STM32_PLLN                  (STM32_PLLN_VALUE << 6)
#else
#error "invalid STM32_PLLN_VALUE value specified"
#endif

/**
 * @brief   STM32_PLLP field.
 */
#if (STM32_PLLP_VALUE == 2) || defined(__DOXYGEN__)
#define STM32_PLLP                  (0 << 16)
#elif STM32_PLLP_VALUE == 4
#define STM32_PLLP                  (1 << 16)
#elif STM32_PLLP_VALUE == 6
#define STM32_PLLP                  (2 << 16)
#elif STM32_PLLP_VALUE == 8
#define STM32_PLLP                  (3 << 16)
#else
#error "invalid STM32_PLLP_VALUE value specified"
#endif

/**
 * @brief   STM32_PLLQ field.
 */
#if ((STM32_PLLQ_VALUE >= 2) && (STM32_PLLQ_VALUE <= 15)) ||                \
    defined(__DOXYGEN__)
#define STM32_PLLQ                  (STM32_PLLQ_VALUE << 24)
#else
#error "invalid STM32_PLLQ_VALUE value specified"
#endif

/**
 * @brief   PLL VCO frequency.
 */
#define STM32_PLLVCO                (STM32_PLLCLKIN * STM32_PLLN_VALUE)

/*
 * PLL VCO frequency range check.
 */
#if (STM32_PLLVCO < STM32_PLLVCO_MIN) || (STM32_PLLVCO > STM32_PLLVCO_MAX)
#error "STM32_PLLVCO outside acceptable range (STM32_PLLVCO_MIN...STM32_PLLVCO_MAX)"
#endif

/**
 * @brief   PLL output clock frequency.
 */
#define STM32_PLLCLKOUT             (STM32_PLLVCO / STM32_PLLP_VALUE)

/*
 * PLL output frequency range check.
 */
#if (STM32_PLLCLKOUT < STM32_PLLOUT_MIN) || (STM32_PLLCLKOUT > STM32_PLLOUT_MAX)
#error "STM32_PLLCLKOUT outside acceptable range (STM32_PLLOUT_MIN...STM32_PLLOUT_MAX)"
#endif

/**
 * @brief   System clock source.
 */
#if STM32_NO_INIT || defined(__DOXYGEN__)
#define STM32_SYSCLK                STM32_HSICLK
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

/* Calculating VOS settings, it is different for each sub-platform.*/
#if defined(STM32F429_439xx) || defined(STM32F427_437xx) ||                 \
    defined(__DOXYGEN__)
#if STM32_SYSCLK <= 120000000
#define STM32_VOS                   STM32_VOS_SCALE3
#define STM32_OVERDRIVE_REQUIRED    FALSE
#elif STM32_SYSCLK <= 144000000
#define STM32_VOS                   STM32_VOS_SCALE2
#define STM32_OVERDRIVE_REQUIRED    FALSE
#elif STM32_SYSCLK <= 168000000
#define STM32_VOS                   STM32_VOS_SCALE1
#define STM32_OVERDRIVE_REQUIRED    FALSE
#else
#define STM32_VOS                   STM32_VOS_SCALE1
#define STM32_OVERDRIVE_REQUIRED    TRUE
#endif

#elif defined(STM32F40_41xxx)
#if STM32_SYSCLK <= 144000000
#define STM32_VOS                   STM32_VOS_SCALE2
#else
#define STM32_VOS                   STM32_VOS_SCALE1
#endif
#define STM32_OVERDRIVE_REQUIRED    FALSE

#elif defined(STM32F401xx)
#if STM32_SYSCLK <= 60000000
#define STM32_VOS                   STM32_VOS_SCALE3
#else
#define STM32_VOS                   STM32_VOS_SCALE2
#endif
#define STM32_OVERDRIVE_REQUIRED    FALSE

#else /* STM32F2XX */
#define STM32_OVERDRIVE_REQUIRED    FALSE
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

/*
 * AHB frequency check.
 */
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

/*
 * APB1 frequency check.
 */
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

/*
 * APB2 frequency check.
 */
#if STM32_PCLK2 > STM32_PCLK2_MAX
#error "STM32_PCLK2 exceeding maximum frequency (STM32_PCLK2_MAX)"
#endif

/*
 * PLLI2S enable check.
 */
#if (STM32_I2SSRC == STM32_I2SSRC_PLLI2S) || defined(__DOXYGEN__)
/**
 * @brief   PLL activation flag.
 */
#define STM32_ACTIVATE_PLLI2S       TRUE
#else
#define STM32_ACTIVATE_PLLI2S       FALSE
#endif

/**
 * @brief   STM32_PLLI2SN field.
 */
#if ((STM32_PLLI2SN_VALUE >= 192) && (STM32_PLLI2SN_VALUE <= 432)) ||       \
    defined(__DOXYGEN__)
#define STM32_PLLI2SN               (STM32_PLLI2SN_VALUE << 6)
#else
#error "invalid STM32_PLLI2SN_VALUE value specified"
#endif

/**
 * @brief   STM32_PLLI2SR field.
 */
#if ((STM32_PLLI2SR_VALUE >= 2) && (STM32_PLLI2SR_VALUE <= 7)) ||           \
    defined(__DOXYGEN__)
#define STM32_PLLI2SR               (STM32_PLLI2SR_VALUE << 28)
#else
#error "invalid STM32_PLLI2SR_VALUE value specified"
#endif

/**
 * @brief   PLL VCO frequency.
 */
#define STM32_PLLI2SVCO             (STM32_PLLCLKIN * STM32_PLLI2SN_VALUE)

/*
 * PLLI2S VCO frequency range check.
 */
#if (STM32_PLLI2SVCO < STM32_PLLVCO_MIN) ||                                 \
    (STM32_PLLI2SVCO > STM32_PLLVCO_MAX)
#error "STM32_PLLI2SVCO outside acceptable range (STM32_PLLVCO_MIN...STM32_PLLVCO_MAX)"
#endif

/**
 * @brief   PLLI2S output clock frequency.
 */
#define STM32_PLLI2SCLKOUT          (STM32_PLLI2SVCO / STM32_PLLI2SR_VALUE)

/**
 * @brief   MCO1 divider clock.
 */
#if (STM32_MCO1SEL == STM32_MCO1SEL_HSI) || defined(__DOXYGEN__)
#define STM32_MCO1DIVCLK             STM32_HSICLK
#elif STM32_MCO1SEL == STM32_MCO1SEL_LSE
#define STM32_MCO1DIVCLK             STM32_LSECLK
#elif STM32_MCO1SEL == STM32_MCO1SEL_HSE
#define STM32_MCO1DIVCLK             STM32_HSECLK
#elif STM32_MCO1SEL == STM32_MCO1SEL_PLL
#define STM32_MCO1DIVCLK             STM32_PLLCLKOUT
#else
#error "invalid STM32_MCO1SEL value specified"
#endif

/**
 * @brief   MCO1 output pin clock.
 */
#if (STM32_MCO1PRE == STM32_MCO1PRE_DIV1) || defined(__DOXYGEN__)
#define STM32_MCO1CLK                STM32_MCO1DIVCLK
#elif STM32_MCO1PRE == STM32_MCO1PRE_DIV2
#define STM32_MCO1CLK                (STM32_MCO1DIVCLK / 2)
#elif STM32_MCO1PRE == STM32_MCO1PRE_DIV3
#define STM32_MCO1CLK                (STM32_MCO1DIVCLK / 3)
#elif STM32_MCO1PRE == STM32_MCO1PRE_DIV4
#define STM32_MCO1CLK                (STM32_MCO1DIVCLK / 4)
#elif STM32_MCO1PRE == STM32_MCO1PRE_DIV5
#define STM32_MCO1CLK                (STM32_MCO1DIVCLK / 5)
#else
#error "invalid STM32_MCO1PRE value specified"
#endif

/**
 * @brief   MCO2 divider clock.
 */
#if (STM32_MCO2SEL == STM32_MCO2SEL_HSE) || defined(__DOXYGEN__)
#define STM32_MCO2DIVCLK             STM32_HSECLK
#elif STM32_MCO2SEL == STM32_MCO2SEL_PLL
#define STM32_MCO2DIVCLK             STM32_PLLCLKOUT
#elif STM32_MCO2SEL == STM32_MCO2SEL_SYSCLK
#define STM32_MCO2DIVCLK             STM32_SYSCLK
#elif STM32_MCO2SEL == STM32_MCO2SEL_PLLI2S
#define STM32_MCO2DIVCLK             STM32_PLLI2S
#else
#error "invalid STM32_MCO2SEL value specified"
#endif

/**
 * @brief   MCO2 output pin clock.
 */
#if (STM32_MCO2PRE == STM32_MCO2PRE_DIV1) || defined(__DOXYGEN__)
#define STM32_MCO2CLK                STM32_MCO2DIVCLK
#elif STM32_MCO2PRE == STM32_MCO2PRE_DIV2
#define STM32_MCO2CLK                (STM32_MCO2DIVCLK / 2)
#elif STM32_MCO2PRE == STM32_MCO2PRE_DIV3
#define STM32_MCO2CLK                (STM32_MCO2DIVCLK / 3)
#elif STM32_MCO2PRE == STM32_MCO2PRE_DIV4
#define STM32_MCO2CLK                (STM32_MCO2DIVCLK / 4)
#elif STM32_MCO2PRE == STM32_MCO2PRE_DIV5
#define STM32_MCO2CLK                (STM32_MCO2DIVCLK / 5)
#else
#error "invalid STM32_MCO2PRE value specified"
#endif

/**
 * @brief   RTC HSE divider setting.
 */
#if ((STM32_RTCPRE_VALUE >= 2) && (STM32_RTCPRE_VALUE <= 31)) ||            \
    defined(__DOXYGEN__)
#define STM32_RTCPRE                (STM32_RTCPRE_VALUE << 16)
#else
#error "invalid STM32_RTCPRE value specified"
#endif

/**
 * @brief   HSE divider toward RTC clock.
 */
#if ((STM32_RTCPRE_VALUE >= 2) && (STM32_RTCPRE_VALUE <= 31))  ||           \
    defined(__DOXYGEN__)
#define STM32_HSEDIVCLK             (STM32_HSECLK / STM32_RTCPRE_VALUE)
#else
#error "invalid STM32_RTCPRE value specified"
#endif

/**
 * @brief   RTC clock.
 */
#if (STM32_RTCSEL == STM32_RTCSEL_NOCLOCK) || defined(__DOXYGEN__)
#define STM32_RTCCLK                0
#elif STM32_RTCSEL == STM32_RTCSEL_LSE
#define STM32_RTCCLK                STM32_LSECLK
#elif STM32_RTCSEL == STM32_RTCSEL_LSI
#define STM32_RTCCLK                STM32_LSICLK
#elif STM32_RTCSEL == STM32_RTCSEL_HSEDIV
#define STM32_RTCCLK                STM32_HSEDIVCLK
#else
#error "invalid STM32_RTCSEL value specified"
#endif

/**
 * @brief   48MHz frequency.
 */
#if STM32_CLOCK48_REQUIRED || defined(__DOXYGEN__)
#define STM32_PLL48CLK              (STM32_PLLVCO / STM32_PLLQ_VALUE)
#else
#define STM32_PLL48CLK              0
#endif

/**
 * @brief   Timers 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14 clock.
 */
#if (STM32_PPRE1 == STM32_PPRE1_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK1               (STM32_PCLK1 * 1)
#else
#define STM32_TIMCLK1               (STM32_PCLK1 * 2)
#endif

/**
 * @brief   Timers 1, 8 clock.
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
#define STM32_FLASHBITS             0x00000000
#elif STM32_HCLK <= STM32_1WS_THRESHOLD
#define STM32_FLASHBITS             0x00000001
#elif STM32_HCLK <= STM32_2WS_THRESHOLD
#define STM32_FLASHBITS             0x00000002
#elif STM32_HCLK <= STM32_3WS_THRESHOLD
#define STM32_FLASHBITS             0x00000003
#elif STM32_HCLK <= STM32_4WS_THRESHOLD
#define STM32_FLASHBITS             0x00000004
#elif STM32_HCLK <= STM32_5WS_THRESHOLD
#define STM32_FLASHBITS             0x00000005
#elif STM32_HCLK <= STM32_6WS_THRESHOLD
#define STM32_FLASHBITS             0x00000006
#elif STM32_HCLK <= STM32_7WS_THRESHOLD
#define STM32_FLASHBITS             0x00000007
#else
#define STM32_FLASHBITS             0x00000008
#endif

/* There are differences in vector names in the various sub-families,
   normalizing.*/
#define TIM1_BRK_IRQn       TIM1_BRK_TIM9_IRQn
#define TIM1_UP_IRQn        TIM1_UP_TIM10_IRQn
#define TIM1_TRG_COM_IRQn   TIM1_TRG_COM_TIM11_IRQn
#define TIM8_BRK_IRQn       TIM8_BRK_TIM12_IRQn
#define TIM8_UP_IRQn        TIM8_UP_TIM13_IRQn
#define TIM8_TRG_COM_IRQn   TIM8_TRG_COM_TIM14_IRQn

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

/* STM32 helpers and custom drivers.*/
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
