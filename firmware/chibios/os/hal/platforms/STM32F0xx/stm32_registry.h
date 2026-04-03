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
 * @file    STM32F0xx/stm32_registry.h
 * @brief   STM32F0xx capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _STM32_REGISTRY_H_
#define _STM32_REGISTRY_H_

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    STM32F0xx capabilities
 * @{
 */
#if defined(STM32F0XX_MD) || defined(__DOXYGEN__)

/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_ADC1_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(1, 1) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_ADC1_DMA_CHN      0x00000000

#define STM32_HAS_ADC2          FALSE
#define STM32_HAS_ADC3          FALSE
#define STM32_HAS_ADC4          FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1          FALSE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   0

/* DAC attributes.*/
#define STM32_HAS_DAC           TRUE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 28

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         FALSE
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTC_HAS_SUBSECONDS FALSE
#define STM32_RTC_IS_CALENDAR   TRUE

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
#define STM32_HAS_SPI4          FALSE
#define STM32_HAS_SPI5          FALSE
#define STM32_HAS_SPI6          FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          TRUE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          FALSE
#define STM32_HAS_TIM5          FALSE
#define STM32_HAS_TIM6          TRUE
#define STM32_HAS_TIM7          FALSE
#define STM32_HAS_TIM8          FALSE
#define STM32_HAS_TIM9          FALSE
#define STM32_HAS_TIM10         FALSE
#define STM32_HAS_TIM11         FALSE
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         TRUE
#define STM32_HAS_TIM15         TRUE
#define STM32_HAS_TIM16         TRUE
#define STM32_HAS_TIM17         TRUE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        FALSE
#define STM32_HAS_UART4         FALSE
#define STM32_HAS_UART5         FALSE
#define STM32_HAS_USART6        FALSE

/* USB attributes.*/
#define STM32_HAS_USB           TRUE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE

#elif defined(STM32F0XX_LD)

/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_ADC1_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(1, 1) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_ADC1_DMA_CHN      0x00000000

#define STM32_HAS_ADC2          FALSE
#define STM32_HAS_ADC3          FALSE
#define STM32_HAS_ADC4          FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1          FALSE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   0

/* DAC attributes.*/
#define STM32_HAS_DAC           FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 28

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         FALSE
#define STM32_HAS_GPIOE         FALSE
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          FALSE
#define STM32_HAS_I2C3          FALSE

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTC_HAS_SUBSECONDS FALSE
#define STM32_RTC_IS_CALENDAR   TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO          FALSE

/* SPI attributes.*/
#define STM32_HAS_SPI1          TRUE
#define STM32_SPI1_RX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN   0x00000000
#define STM32_SPI1_TX_DMA_MSK   STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN   0x00000000

#define STM32_HAS_SPI2          FALSE
#define STM32_HAS_SPI3          FALSE
#define STM32_HAS_SPI4          FALSE
#define STM32_HAS_SPI5          FALSE
#define STM32_HAS_SPI6          FALSE

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
#define STM32_HAS_TIM14         TRUE
#define STM32_HAS_TIM15         FALSE
#define STM32_HAS_TIM16         TRUE
#define STM32_HAS_TIM17         TRUE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        FALSE
#define STM32_HAS_USART3        FALSE
#define STM32_HAS_UART4         FALSE
#define STM32_HAS_UART5         FALSE
#define STM32_HAS_USART6        FALSE

/* USB attributes.*/
#define STM32_HAS_USB           FALSE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE

#else /* STM32F030 */

/* ADC attributes.*/
#define STM32_HAS_ADC1          TRUE
#define STM32_ADC1_DMA_MSK      (STM32_DMA_STREAM_ID_MSK(1, 1) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_ADC1_DMA_CHN      0x00000000

#define STM32_HAS_ADC2          FALSE
#define STM32_HAS_ADC3          FALSE
#define STM32_HAS_ADC4          FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1          FALSE
#define STM32_HAS_CAN2          FALSE
#define STM32_CAN_MAX_FILTERS   0

/* DAC attributes.*/
#define STM32_HAS_DAC           FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA      FALSE
#define STM32_HAS_DMA1          TRUE
#define STM32_HAS_DMA2          FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH           FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS 28

/* GPIO attributes.*/
#define STM32_HAS_GPIOA         TRUE
#define STM32_HAS_GPIOB         TRUE
#define STM32_HAS_GPIOC         TRUE
#define STM32_HAS_GPIOD         TRUE
#define STM32_HAS_GPIOE         FALSE
#define STM32_HAS_GPIOF         TRUE
#define STM32_HAS_GPIOG         FALSE
#define STM32_HAS_GPIOH         FALSE
#define STM32_HAS_GPIOI         FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1          TRUE
#define STM32_I2C1_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 3))
#define STM32_I2C1_RX_DMA_CHN   0x00000000
#define STM32_I2C1_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 2))
#define STM32_I2C1_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C2          TRUE
#define STM32_I2C2_RX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_I2C2_RX_DMA_CHN   0x00000000
#define STM32_I2C2_TX_DMA_MSK   (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_I2C2_TX_DMA_CHN   0x00000000

#define STM32_HAS_I2C3          FALSE

/* RTC attributes.*/
#define STM32_HAS_RTC           TRUE
#define STM32_RTC_HAS_SUBSECONDS FALSE
#define STM32_RTC_IS_CALENDAR   TRUE

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
#define STM32_HAS_SPI4          FALSE
#define STM32_HAS_SPI5          FALSE
#define STM32_HAS_SPI6          FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1          TRUE
#define STM32_HAS_TIM2          FALSE
#define STM32_HAS_TIM3          TRUE
#define STM32_HAS_TIM4          FALSE
#define STM32_HAS_TIM5          FALSE
#define STM32_HAS_TIM6          TRUE
#define STM32_HAS_TIM7          FALSE
#define STM32_HAS_TIM8          FALSE
#define STM32_HAS_TIM9          FALSE
#define STM32_HAS_TIM10         FALSE
#define STM32_HAS_TIM11         FALSE
#define STM32_HAS_TIM12         FALSE
#define STM32_HAS_TIM13         FALSE
#define STM32_HAS_TIM14         TRUE
#define STM32_HAS_TIM15         TRUE
#define STM32_HAS_TIM16         TRUE
#define STM32_HAS_TIM17         TRUE
#define STM32_HAS_TIM18         FALSE
#define STM32_HAS_TIM19         FALSE

/* USART attributes.*/
#define STM32_HAS_USART1        TRUE
#define STM32_USART1_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 3) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART1_RX_DMA_CHN 0x00000000
#define STM32_USART1_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 2) |            \
                                 STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART1_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART2        TRUE
#define STM32_USART2_RX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 5))
#define STM32_USART2_RX_DMA_CHN 0x00000000
#define STM32_USART2_TX_DMA_MSK (STM32_DMA_STREAM_ID_MSK(1, 4))
#define STM32_USART2_TX_DMA_CHN 0x00000000

#define STM32_HAS_USART3        FALSE
#define STM32_HAS_UART4         FALSE
#define STM32_HAS_UART5         FALSE
#define STM32_HAS_USART6        FALSE

/* USB attributes.*/
#define STM32_HAS_USB           FALSE
#define STM32_HAS_OTG1          FALSE
#define STM32_HAS_OTG2          FALSE

#endif /* STM32F030 */

/** @} */

#endif /* _STM32_REGISTRY_H_ */

/** @} */
