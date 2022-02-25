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
 * @defgroup STM32F103_HAL STM32F103 HAL Support
 * @details HAL support for STM32 Performance Line LD, MD and HD sub-families.
 *
 * @ingroup HAL
 */

/**
 * @file    STM32F1xx/hal_lld_f103.h
 * @brief   STM32F103 Performance Line HAL subsystem low level driver header.
 *
 * @addtogroup STM32F103_HAL
 * @{
 */

#ifndef _HAL_LLD_F103_H_
#define _HAL_LLD_F103_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Platform identification
 * @{
 */
#if defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32F10x Performance Line"

#elif defined(STM32F10X_LD)
#define PLATFORM_NAME           "STM32F10x Performance Line Low Density"

#elif defined(STM32F10X_MD)
#define PLATFORM_NAME           "STM32F10x Performance Line Medium Density"

#elif defined(STM32F10X_HD)
#define PLATFORM_NAME           "STM32F10x Performance Line High Density"

#elif defined(STM32F10X_XL)
#define PLATFORM_NAME           "STM32F10x Performance Line eXtra Density"

#else
#error "unsupported STM32 Performance Line member"
#endif
/** @} */

/**
 * @name    Absolute Maximum Ratings
 * @{
 */
/**
 * @brief   Maximum system clock frequency.
 */
#define STM32_SYSCLK_MAX        72000000

/**
 * @brief   Maximum HSE clock frequency.
 */
#define STM32_HSECLK_MAX        25000000

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
 * @brief   Maximum PLLs input clock frequency.
 */
#define STM32_PLLIN_MIN         1000000

/**
 * @brief   Maximum PLL output clock frequency.
 */
#define STM32_PLLOUT_MAX        72000000

/**
 * @brief   Maximum PLL output clock frequency.
 */
#define STM32_PLLOUT_MIN        16000000

/**
 * @brief   Maximum APB1 clock frequency.
 */
#define STM32_PCLK1_MAX         36000000

/**
 * @brief   Maximum APB2 clock frequency.
 */
#define STM32_PCLK2_MAX         72000000

/**
 * @brief   Maximum ADC clock frequency.
 */
#define STM32_ADCCLK_MAX        14000000
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

#define STM32_ADCPRE_DIV2       (0 << 14)   /**< PPRE2 divided by 2.        */
#define STM32_ADCPRE_DIV4       (1 << 14)   /**< PPRE2 divided by 4.        */
#define STM32_ADCPRE_DIV6       (2 << 14)   /**< PPRE2 divided by 6.        */
#define STM32_ADCPRE_DIV8       (3 << 14)   /**< PPRE2 divided by 8.        */

#define STM32_PLLSRC_HSI        (0 << 16)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 16)   /**< PLL clock source is HSE.   */

#define STM32_PLLXTPRE_DIV1     (0 << 17)   /**< HSE divided by 1.          */
#define STM32_PLLXTPRE_DIV2     (1 << 17)   /**< HSE divided by 2.          */

#define STM32_USBPRE_DIV1P5     (0 << 22)   /**< PLLOUT divided by 1.5.     */
#define STM32_USBPRE_DIV1       (1 << 22)   /**< PLLOUT divided by 1.       */

#define STM32_MCOSEL_NOCLOCK    (0 << 24)   /**< No clock on MCO pin.       */
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
#define STM32_RTCSEL_HSEDIV     (3 << 8)    /**< HSE divided by 128 used as
                                                 RTC clock.                 */
/** @} */

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

#if defined(STM32F10X_LD) || defined(__DOXYGEN__)
/**
 * @name    STM32F103 LD capabilities
 * @{
 */
/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_HAS_ADC2          TRUE
#define STM32_HAS_ADC3          FALSE
#define STM32_HAS_ADC4          FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1          TRUE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   14

/* DAC attributes.*/
#define STM32_HAS_DAC           FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 19

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         FALSE
#define STM32_HAS_GPIOF         FALSE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          FALSE
#define STM32_I2C2_RX_DMA_MSK   0
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   0
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE
#define STM32_SPI3_RX_DMA_MSK   0
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   0
#define STM32_SPI3_TX_DMA_CHN   0x00000000

/* SDIO attributes.*/
#define STM32_HAS_SDIO          FALSE

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN   0x00000000
#define STM32_SPI1_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI2          FALSE
#define STM32_SPI2_RX_DMA_MSK   0
#define STM32_SPI2_RX_DMA_CHN   0x00000000
#define STM32_SPI2_TX_DMA_MSK   0
#define STM32_SPI2_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI3          FALSE
#define STM32_SPI3_RX_DMA_MSK   0
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   0
#define STM32_SPI3_TX_DMA_CHN   0x00000000

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          FALSE
#define STM32_HAS_TIM5          FALSE
#define STM32_HAS_TIM6          FALSE
#define STM32_HAS_TIM7          FALSE
#define STM32_HAS_TIM8          FALSE
#define STM32_HAS_TIM9          FALSE
#define STM32_HAS_TIM10         FALSE
#define STM32_HAS_TIM11         FALSE
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         FALSE
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         FALSE
#define STM32_HAS_TIM17         FALSE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        FALSE
#define STM32_USART3_RX_DMA_MSK 0
#define STM32_USART3_RX_DMA_CHN 0x00000000
#define STM32_USART3_TX_DMA_MSK 0
#define STM32_USART3_TX_DMA_CHN 0x00000000

#define STM32_HAS_UART4         FALSE
#define STM32_UART4_RX_DMA_MSK  0
#define STM32_UART4_RX_DMA_CHN  0x00000000
#define STM32_UART4_TX_DMA_MSK  0
#define STM32_UART4_TX_DMA_CHN  0x00000000

#define STM32_HAS_UART5         FALSE
#define STM32_UART5_RX_DMA_MSK  0
#define STM32_UART5_RX_DMA_CHN  0x00000000
#define STM32_UART5_TX_DMA_MSK  0
#define STM32_UART5_TX_DMA_CHN  0x00000000

#define STM32_HAS_USART6        FALSE
#define STM32_USART6_RX_DMA_MSK 0
#define STM32_USART6_RX_DMA_CHN 0x00000000
#define STM32_USART6_TX_DMA_MSK 0
#define STM32_USART6_TX_DMA_CHN 0x00000000

/* USB attributes.*/
#define STM32_HAS_USB           FALSE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE
/** @} */
#endif /* defined(STM32F10X_LD) */

#if defined(STM32F10X_MD) || defined(__DOXYGEN__)
/**
 * @name    STM32F103 MD capabilities
 * @{
 */
/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_HAS_ADC2          TRUE
#define STM32_HAS_ADC3          FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1          TRUE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   14

/* DAC attributes.*/
#define STM32_HAS_DAC           FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 19

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         TRUE
#define STM32_HAS_GPIOF         FALSE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE
#define STM32_I2C3_RX_DMA_MSK   0
#define STM32_I2C3_RX_DMA_CHN   0x00000000
#define STM32_I2C3_TX_DMA_MSK   0
#define STM32_I2C3_TX_DMA_CHN   0x00000000

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTCSEL_HAS_SUBSECONDS TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO          FALSE

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN   0x00000000
#define STM32_SPI1_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI2          TRUE
#define STM32_SPI2_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_SPI2_RX_DMA_CHN   0x00000000
#define STM32_SPI2_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_SPI2_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI3          FALSE
#define STM32_SPI3_RX_DMA_MSK   0
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   0
#define STM32_SPI3_TX_DMA_CHN   0x00000000

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          TRUE
#define STM32_HAS_TIM5          FALSE
#define STM32_HAS_TIM6          FALSE
#define STM32_HAS_TIM7          FALSE
#define STM32_HAS_TIM8          FALSE
#define STM32_HAS_TIM9          FALSE
#define STM32_HAS_TIM10         FALSE
#define STM32_HAS_TIM11         FALSE
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         FALSE
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         FALSE
#define STM32_HAS_TIM17         FALSE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        TRUE
#define STM32_USART3_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_USART3_RX_DMA_CHN 0x00000000
#define STM32_USART3_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_USART3_TX_DMA_CHN 0x00000000

#define STM32_HAS_UART4         FALSE
#define STM32_UART4_RX_DMA_MSK  0
#define STM32_UART4_RX_DMA_CHN  0x00000000
#define STM32_UART4_TX_DMA_MSK  0
#define STM32_UART4_TX_DMA_CHN  0x00000000

#define STM32_HAS_UART5         FALSE
#define STM32_UART5_RX_DMA_MSK  0
#define STM32_UART5_RX_DMA_CHN  0x00000000
#define STM32_UART5_TX_DMA_MSK  0
#define STM32_UART5_TX_DMA_CHN  0x00000000

#define STM32_HAS_USART6        FALSE
#define STM32_USART6_RX_DMA_MSK 0
#define STM32_USART6_RX_DMA_CHN 0x00000000
#define STM32_USART6_TX_DMA_MSK 0
#define STM32_USART6_TX_DMA_CHN 0x00000000

/* USB attributes.*/
#define STM32_HAS_USB           TRUE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE
/** @} */
#endif /* defined(STM32F10X_MD) */

#if defined(STM32F10X_HD) || defined(__DOXYGEN__)
/**
 * @name    STM32F103 HD capabilities
 * @{
 */
/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_HAS_ADC2          TRUE
#define STM32_HAS_ADC3          TRUE

/* CAN attributes.*/
#define STM32_HAS_CAN1          TRUE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   14

/* DAC attributes.*/
#define STM32_HAS_DAC           TRUE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 19

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         TRUE
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         TRUE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE
#define STM32_I2C3_RX_DMA_MSK   0
#define STM32_I2C3_RX_DMA_CHN   0x00000000
#define STM32_I2C3_TX_DMA_MSK   0
#define STM32_I2C3_TX_DMA_CHN   0x00000000

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTCSEL_HAS_SUBSECONDS TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO          TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN   0x00000000
#define STM32_SPI1_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI2          TRUE
#define STM32_SPI2_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_SPI2_RX_DMA_CHN   0x00000000
#define STM32_SPI2_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_SPI2_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI3          TRUE
#define STM32_SPI3_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(2, 1)
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(2, 2)
#define STM32_SPI3_TX_DMA_CHN   0x00000000

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          TRUE
#define STM32_HAS_TIM5          TRUE
#define STM32_HAS_TIM6          TRUE
#define STM32_HAS_TIM7          TRUE
#define STM32_HAS_TIM8          TRUE
#define STM32_HAS_TIM9          TRUE
#define STM32_HAS_TIM10         TRUE
#define STM32_HAS_TIM11         TRUE
#define STM32_HAS_TIM12         TRUE
#define STM32_HAS_TIM13         TRUE
#define STM32_HAS_TIM14         TRUE
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         FALSE
#define STM32_HAS_TIM17         FALSE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        TRUE
#define STM32_USART3_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_USART3_RX_DMA_CHN 0x00000000
#define STM32_USART3_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_USART3_TX_DMA_CHN 0x00000000

#define STM32_HAS_UART4         TRUE
#define STM32_UART4_RX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(2, 3))
#define STM32_UART4_RX_DMA_CHN  0x00000000
#define STM32_UART4_TX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_UART4_TX_DMA_CHN  0x00000000

#define STM32_HAS_UART5         TRUE
#define STM32_UART5_RX_DMA_MSK  0
#define STM32_UART5_RX_DMA_CHN  0x00000000
#define STM32_UART5_TX_DMA_MSK  0
#define STM32_UART5_TX_DMA_CHN  0x00000000

#define STM32_HAS_USART6        FALSE
#define STM32_USART6_RX_DMA_MSK 0
#define STM32_USART6_RX_DMA_CHN 0x00000000
#define STM32_USART6_TX_DMA_MSK 0
#define STM32_USART6_TX_DMA_CHN 0x00000000

/* USB attributes.*/
#define STM32_HAS_USB           TRUE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE
/** @} */
#endif /* defined(STM32F10X_HD) */

#if defined(STM32F10X_XL) || defined(__DOXYGEN__)
/**
 * @name    STM32F103 XL capabilities
 * @{
 */
/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_HAS_ADC2          TRUE
#define STM32_HAS_ADC3          TRUE

/* CAN attributes.*/
#define STM32_HAS_CAN1          TRUE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   14

/* DAC attributes.*/
#define STM32_HAS_DAC           TRUE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 19

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         TRUE
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         TRUE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE
#define STM32_I2C3_RX_DMA_MSK   0
#define STM32_I2C3_RX_DMA_CHN   0x00000000
#define STM32_I2C3_TX_DMA_MSK   0
#define STM32_I2C3_TX_DMA_CHN   0x00000000

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTCSEL_HAS_SUBSECONDS TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO          TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN   0x00000000
#define STM32_SPI1_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI2          TRUE
#define STM32_SPI2_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_SPI2_RX_DMA_CHN   0x00000000
#define STM32_SPI2_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_SPI2_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI3          TRUE
#define STM32_SPI3_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(2, 1)
#define STM32_SPI3_RX_DMA_CHN   0x00000000
#define STM32_SPI3_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(2, 2)
#define STM32_SPI3_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI4          FALSE
#define STM32_HAS_SPI5          FALSE
#define STM32_HAS_SPI6          FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          TRUE
#define STM32_HAS_TIM5          TRUE
#define STM32_HAS_TIM6          TRUE
#define STM32_HAS_TIM7          TRUE
#define STM32_HAS_TIM8          TRUE
#define STM32_HAS_TIM9          FALSE
#define STM32_HAS_TIM10         FALSE
#define STM32_HAS_TIM11         FALSE
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         FALSE
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         FALSE
#define STM32_HAS_TIM17         FALSE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 6))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 7))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        TRUE
#define STM32_USART3_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_USART3_RX_DMA_CHN 0x00000000
#define STM32_USART3_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_USART3_TX_DMA_CHN 0x00000000

#define STM32_HAS_UART4         TRUE
#define STM32_UART4_RX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(2, 3))
#define STM32_UART4_RX_DMA_CHN  0x00000000
#define STM32_UART4_TX_DMA_MSK  (STM32_DMA_STREAM_ID_MSK(2, 5))
#define STM32_UART4_TX_DMA_CHN  0x00000000

#define STM32_HAS_UART5         TRUE
#define STM32_UART5_RX_DMA_MSK  0
#define STM32_UART5_RX_DMA_CHN  0x00000000
#define STM32_UART5_TX_DMA_MSK  0
#define STM32_UART5_TX_DMA_CHN  0x00000000

#define STM32_HAS_USART6        FALSE
#define STM32_USART6_RX_DMA_MSK 0
#define STM32_USART6_RX_DMA_CHN 0x00000000
#define STM32_USART6_TX_DMA_MSK 0
#define STM32_USART6_TX_DMA_CHN 0x00000000

/* USB attributes.*/
#define STM32_HAS_USB           TRUE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE
/** @} */
#endif /* defined(STM32F10X_XL) */

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
#define TAMPER_IRQHandler       Vector48    /**< Tamper.                    */
#define RTC_IRQHandler          Vector4C    /**< RTC.                       */
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
#define ADC1_2_IRQHandler       Vector88    /**< ADC1_2.                    */
#define CAN1_TX_IRQHandler      Vector8C    /**< CAN1 TX.                   */
#define USB_HP_IRQHandler       Vector8C    /**< USB High Priority, CAN1 TX.*/
#define CAN1_RX0_IRQHandler     Vector90    /**< CAN1 RX0.                  */
#define USB_LP_IRQHandler       Vector90    /**< USB Low Priority, CAN1 RX0.*/
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
#define I2C2_ER_IRQHandler      VectorC8    /**< I2C2 Error.                */
#define SPI1_IRQHandler         VectorCC    /**< SPI1.                      */
#define SPI2_IRQHandler         VectorD0    /**< SPI2.                      */
#define USART1_IRQHandler       VectorD4    /**< USART1.                    */
#define USART2_IRQHandler       VectorD8    /**< USART2.                    */
#define USART3_IRQHandler       VectorDC    /**< USART3.                    */
#define EXTI15_10_IRQHandler    VectorE0    /**< EXTI Line 15..10.          */
#define RTC_Alarm_IRQHandler    VectorE4    /**< RTC Alarm through EXTI.    */
#define USB_FS_WKUP_IRQHandler  VectorE8    /**< USB Wakeup from suspend.   */
#define TIM8_BRK_IRQHandler     VectorEC    /**< TIM8 Break.                */
#define TIM8_UP_IRQHandler      VectorF0    /**< TIM8 Update.               */
#define TIM8_TRG_COM_IRQHandler VectorF4    /**< TIM8 Trigger and
                                                 Commutation.               */
#define TIM8_CC_IRQHandler      VectorF8    /**< TIM8 Capture Compare.      */
#define ADC3_IRQHandler         VectorFC    /**< ADC3.                      */
#define FSMC_IRQHandler         Vector100   /**< FSMC.                      */
#define SDIO_IRQHandler         Vector104   /**< SDIO.                      */
#define TIM5_IRQHandler         Vector108   /**< TIM5.                      */
#define SPI3_IRQHandler         Vector10C   /**< SPI3.                      */
#define UART4_IRQHandler        Vector110   /**< UART4.                     */
#define UART5_IRQHandler        Vector114   /**< UART5.                     */
#define TIM6_IRQHandler         Vector118   /**< TIM6.                      */
#define TIM7_IRQHandler         Vector11C   /**< TIM7.                      */
#define DMA2_Ch1_IRQHandler     Vector120   /**< DMA2 Channel1.             */
#define DMA2_Ch2_IRQHandler     Vector124   /**< DMA2 Channel2.             */
#define DMA2_Ch3_IRQHandler     Vector128   /**< DMA2 Channel3.             */
#define DMA2_Ch4_5_IRQHandler   Vector12C   /**< DMA2 Channel4 & Channel5.  */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Main clock source selection.
 * @note    If the selected clock source is not the PLL then the PLL is not
 *          initialized and started.
 * @note    The default value is calculated for a 72MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_SW) || defined(__DOXYGEN__)
#define STM32_SW                    STM32_SW_PLL
#endif

/**
 * @brief   Clock source for the PLL.
 * @note    This setting has only effect if the PLL is selected as the
 *          system clock source.
 * @note    The default value is calculated for a 72MHz system clock from
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
#if !defined(STM32_PLLXTPRE) || defined(__DOXYGEN__)
#define STM32_PLLXTPRE              STM32_PLLXTPRE_DIV1
#endif

/**
 * @brief   PLL multiplier value.
 * @note    The allowed range is 2...16.
 * @note    The default value is calculated for a 72MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_PLLMUL_VALUE) || defined(__DOXYGEN__)
#define STM32_PLLMUL_VALUE          9
#endif

/**
 * @brief   AHB prescaler value.
 * @note    The default value is calculated for a 72MHz system clock from
 *          a 8MHz crystal using the PLL.
 */
#if !defined(STM32_HPRE) || defined(__DOXYGEN__)
#define STM32_HPRE                  STM32_HPRE_DIV1
#endif

/**
 * @brief   APB1 prescaler value.
 */
#if !defined(STM32_PPRE1) || defined(__DOXYGEN__)
#define STM32_PPRE1                 STM32_PPRE1_DIV2
#endif

/**
 * @brief   APB2 prescaler value.
 */
#if !defined(STM32_PPRE2) || defined(__DOXYGEN__)
#define STM32_PPRE2                 STM32_PPRE2_DIV2
#endif

/**
 * @brief   ADC prescaler value.
 */
#if !defined(STM32_ADCPRE) || defined(__DOXYGEN__)
#define STM32_ADCPRE                STM32_ADCPRE_DIV4
#endif

/**
 * @brief   USB clock setting.
 */
#if !defined(STM32_USB_CLOCK_REQUIRED) || defined(__DOXYGEN__)
#define STM32_USB_CLOCK_REQUIRED    TRUE
#endif

/**
 * @brief   USB prescaler initialization.
 */
#if !defined(STM32_USBPRE) || defined(__DOXYGEN__)
#define STM32_USBPRE                STM32_USBPRE_DIV1P5
#endif

/**
 * @brief   MCO pin setting.
 */
#if !defined(STM32_MCOSEL) || defined(__DOXYGEN__)
#define STM32_MCOSEL                STM32_MCOSEL_NOCLOCK
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
#if !defined(STM32F103_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32F103_MCUCONF not defined"
#endif

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

#if (STM32_MCOSEL == STM32_MCOSEL_HSI) ||                                   \
    ((STM32_MCOSEL == STM32_MCOSEL_PLLDIV2) &&                              \
     (STM32_PLLSRC == STM32_PLLSRC_HSI))
#error "HSI not enabled, required by STM32_MCOSEL"
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

#else /* !STM32_LSE_ENABLED */

#if STM32_RTCSEL == STM32_RTCSEL_LSE
#error "LSE not enabled, required by STM32_RTCSEL"
#endif

#endif /* !STM32_LSE_ENABLED */

/* PLL activation conditions.*/
#if STM32_USB_CLOCK_REQUIRED ||                                             \
    (STM32_SW == STM32_SW_PLL) ||                                           \
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
#if (STM32_PLLXTPRE != STM32_PLLXTPRE_DIV1) &&                              \
    (STM32_PLLXTPRE != STM32_PLLXTPRE_DIV2)
#error "invalid STM32_PLLXTPRE value specified"
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
#if STM32_PLLXTPRE == STM32_PLLXTPRE_DIV1
#define STM32_PLLCLKIN              (STM32_HSECLK / 1)
#else
#define STM32_PLLCLKIN              (STM32_HSECLK / 2)
#endif
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
 * @brief   RTC clock.
 */
#if (STM32_RTCSEL == STM32_RTCSEL_LSE) || defined(__DOXYGEN__)
#define STM32_RTCCLK                STM32_LSECLK
#elif STM32_RTCSEL == STM32_RTCSEL_LSI
#define STM32_RTCCLK                STM32_LSICLK
#elif STM32_RTCSEL == STM32_RTCSEL_HSEDIV
#define STM32_RTCCLK                (STM32_HSECLK / 128)
#elif STM32_RTCSEL == STM32_RTCSEL_NOCLOCK
#define STM32_RTCCLK                0
#else
#error "invalid source selected for RTC clock"
#endif

/**
 * @brief   ADC frequency.
 */
#if (STM32_ADCPRE == STM32_ADCPRE_DIV2) || defined(__DOXYGEN__)
#define STM32_ADCCLK                (STM32_PCLK2 / 2)
#elif STM32_ADCPRE == STM32_ADCPRE_DIV4
#define STM32_ADCCLK                (STM32_PCLK2 / 4)
#elif STM32_ADCPRE == STM32_ADCPRE_DIV6
#define STM32_ADCCLK                (STM32_PCLK2 / 6)
#elif STM32_ADCPRE == STM32_ADCPRE_DIV8
#define STM32_ADCCLK                (STM32_PCLK2 / 8)
#else
#error "invalid STM32_ADCPRE value specified"
#endif

/* ADC frequency check.*/
#if STM32_ADCCLK > STM32_ADCCLK_MAX
#error "STM32_ADCCLK exceeding maximum frequency (STM32_ADCCLK_MAX)"
#endif

/**
 * @brief   USB frequency.
 */
#if (STM32_USBPRE == STM32_USBPRE_DIV1P5) || defined(__DOXYGEN__)
#define STM32_USBCLK                ((STM32_PLLCLKOUT * 2) / 3)
#elif (STM32_USBPRE == STM32_USBPRE_DIV1)
#define STM32_USBCLK                STM32_PLLCLKOUT
#else
#error "invalid STM32_USBPRE value specified"
#endif

/**
 * @brief   Timers 2, 3, 4, 5, 6, 7, 12, 13, 14 clock.
 */
#if (STM32_PPRE1 == STM32_PPRE1_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK1               (STM32_PCLK1 * 1)
#else
#define STM32_TIMCLK1               (STM32_PCLK1 * 2)
#endif

/**
 * @brief   Timers 1, 8, 9, 10, 11 clock.
 */
#if (STM32_PPRE2 == STM32_PPRE2_DIV1) || defined(__DOXYGEN__)
#define STM32_TIMCLK2               (STM32_PCLK2 * 1)
#else
#define STM32_TIMCLK2               (STM32_PCLK2 * 2)
#endif

/**
 * @brief   Flash settings.
 */
#if (STM32_HCLK <= 24000000) || defined(__DOXYGEN__)
#define STM32_FLASHBITS             0x00000010
#elif STM32_HCLK <= 48000000
#define STM32_FLASHBITS             0x00000011
#else
#define STM32_FLASHBITS             0x00000012
#endif

#endif /* _HAL_LLD_F103_H_ */

/** @} */
