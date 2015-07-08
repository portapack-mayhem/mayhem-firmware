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
 * @file    STM32L1xx/stm32_registry.h
 * @brief   STM32L1xx capabilities registry.
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
 * @name    STM32L1xx capabilities
 * @{
 */
#if defined(STM32L1XX_MD) || defined(STM32L1XX_MDP) || defined(__DOXYGEN__)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      FALSE
#define STM32_HAS_ADC3                      FALSE
#define STM32_HAS_ADC4                      FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1                      FALSE
#define STM32_HAS_CAN2                      FALSE
#define STM32_CAN_MAX_FILTERS               0

/* DAC attributes.*/
#define STM32_HAS_DAC1                      TRUE
#define STM32_HAS_DAC2                      FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA                  FALSE
#define STM32_HAS_DMA1                      TRUE
#define STM32_HAS_DMA2                      FALSE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS             23

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_I2C1_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 7)
#define STM32_I2C1_RX_DMA_CHN               0x00000000
#define STM32_I2C1_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 6)
#define STM32_I2C1_TX_DMA_CHN               0x00000000

#define STM32_HAS_I2C2                      TRUE
#define STM32_I2C2_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_I2C2_RX_DMA_CHN               0x00000000
#define STM32_I2C2_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_I2C2_TX_DMA_CHN               0x00000000

#define STM32_HAS_I2C3                      FALSE

/* RTC attributes.*/
#define STM32_HAS_RTC                       TRUE
#if defined(STM32L1XX_MDP)
#define STM32_RTC_HAS_SUBSECONDS            TRUE
#else
#define STM32_RTC_HAS_SUBSECONDS            FALSE
#endif
#define STM32_RTC_IS_CALENDAR               TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_SPI1_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN               0x00000000
#define STM32_SPI1_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN               0x00000000

#define STM32_HAS_SPI2                      TRUE
#define STM32_SPI2_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_SPI2_RX_DMA_CHN               0x00000000
#define STM32_SPI2_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_SPI2_TX_DMA_CHN               0x00000000

#define STM32_HAS_SPI3                      FALSE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      FALSE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      TRUE
#define STM32_HAS_TIM4                      TRUE
#define STM32_HAS_TIM5                      FALSE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      FALSE
#define STM32_HAS_TIM9                      TRUE
#define STM32_HAS_TIM10                     TRUE
#define STM32_HAS_TIM11                     TRUE
#define STM32_HAS_TIM12                     FALSE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     FALSE
#define STM32_HAS_TIM16                     FALSE
#define STM32_HAS_TIM17                     FALSE
#define STM32_HAS_TIM18                     FALSE
#define STM32_HAS_TIM19                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_USART1_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_USART1_RX_DMA_CHN             0x00000000
#define STM32_USART1_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_USART1_TX_DMA_CHN             0x00000000

#define STM32_HAS_USART2                    TRUE
#define STM32_USART2_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 6)
#define STM32_USART2_RX_DMA_CHN             0x00000000
#define STM32_USART2_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 7)
#define STM32_USART2_TX_DMA_CHN             0x00000000

#define STM32_HAS_USART3                    TRUE
#define STM32_USART3_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_USART3_RX_DMA_CHN             0x00000000
#define STM32_USART3_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_USART3_TX_DMA_CHN             0x00000000

#define STM32_HAS_UART4                     FALSE
#define STM32_HAS_UART5                     FALSE
#define STM32_HAS_USART6                    FALSE

/* USB attributes.*/
#define STM32_HAS_USB                       TRUE
#define STM32_HAS_OTG1                      FALSE
#define STM32_HAS_OTG2                      FALSE

#else /* STM32L1XX_HD */

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      FALSE
#define STM32_HAS_ADC3                      FALSE
#define STM32_HAS_ADC4                      FALSE

/* CAN attributes.*/
#define STM32_HAS_CAN1                      FALSE
#define STM32_HAS_CAN2                      FALSE
#define STM32_CAN_MAX_FILTERS               0

/* DAC attributes.*/
#define STM32_HAS_DAC1                      TRUE
#define STM32_HAS_DAC2                      FALSE

/* DMA attributes.*/
#define STM32_ADVANCED_DMA                  FALSE
#define STM32_HAS_DMA1                      TRUE
#define STM32_HAS_DMA2                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

/* EXTI attributes.*/
#define STM32_EXTI_NUM_CHANNELS             24

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     TRUE
#define STM32_HAS_GPIOG                     TRUE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_I2C1_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 7)
#define STM32_I2C1_RX_DMA_CHN               0x00000000
#define STM32_I2C1_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 6)
#define STM32_I2C1_TX_DMA_CHN               0x00000000

#define STM32_HAS_I2C2                      TRUE
#define STM32_I2C2_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_I2C2_RX_DMA_CHN               0x00000000
#define STM32_I2C2_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_I2C2_TX_DMA_CHN               0x00000000

#define STM32_HAS_I2C3                      FALSE

/* RTC attributes.*/
#define STM32_HAS_RTC                       TRUE
#define STM32_RTC_HAS_SUBSECONDS            TRUE
#define STM32_RTC_IS_CALENDAR               TRUE

/* SDIO attributes.*/
#define STM32_HAS_SDIO                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_SPI1_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_SPI1_RX_DMA_CHN               0x00000000
#define STM32_SPI1_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_SPI1_TX_DMA_CHN               0x00000000

#define STM32_HAS_SPI2                      TRUE
#define STM32_SPI2_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_SPI2_RX_DMA_CHN               0x00000000
#define STM32_SPI2_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_SPI2_TX_DMA_CHN               0x00000000

#define STM32_HAS_SPI3                      TRUE
#define STM32_SPI3_RX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(2, 1)
#define STM32_SPI3_RX_DMA_CHN               0x00000000
#define STM32_SPI3_TX_DMA_MSK               STM32_DMA_STREAM_ID_MSK(2, 2)
#define STM32_SPI3_TX_DMA_CHN               0x00000000

#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_TIM_MAX_CHANNELS              4

#define STM32_HAS_TIM2                      TRUE
#define STM32_TIM2_IS_32BITS                FALSE
#define STM32_TIM2_CHANNELS                 4

#define STM32_HAS_TIM3                      TRUE
#define STM32_TIM3_IS_32BITS                FALSE
#define STM32_TIM3_CHANNELS                 4

#define STM32_HAS_TIM4                      TRUE
#define STM32_TIM4_IS_32BITS                FALSE
#define STM32_TIM4_CHANNELS                 4

#define STM32_HAS_TIM5                      TRUE
#define STM32_TIM5_IS_32BITS                TRUE
#define STM32_TIM5_CHANNELS                 4

#define STM32_HAS_TIM6                      TRUE
#define STM32_TIM6_IS_32BITS                FALSE
#define STM32_TIM6_CHANNELS                 0

#define STM32_HAS_TIM7                      TRUE
#define STM32_TIM7_IS_32BITS                FALSE
#define STM32_TIM7_CHANNELS                 0

#define STM32_HAS_TIM9                      TRUE
#define STM32_TIM15_IS_32BITS               FALSE
#define STM32_TIM15_CHANNELS                2

#define STM32_HAS_TIM10                     TRUE
#define STM32_TIM15_IS_32BITS               FALSE
#define STM32_TIM15_CHANNELS                2

#define STM32_HAS_TIM11                     TRUE
#define STM32_TIM15_IS_32BITS               FALSE
#define STM32_TIM15_CHANNELS                2

#define STM32_HAS_TIM1                      FALSE
#define STM32_HAS_TIM8                      FALSE
#define STM32_HAS_TIM12                     FALSE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     FALSE
#define STM32_HAS_TIM16                     FALSE
#define STM32_HAS_TIM17                     FALSE
#define STM32_HAS_TIM18                     FALSE
#define STM32_HAS_TIM19                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_USART1_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 5)
#define STM32_USART1_RX_DMA_CHN             0x00000000
#define STM32_USART1_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 4)
#define STM32_USART1_TX_DMA_CHN             0x00000000

#define STM32_HAS_USART2                    TRUE
#define STM32_USART2_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 6)
#define STM32_USART2_RX_DMA_CHN             0x00000000
#define STM32_USART2_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 7)
#define STM32_USART2_TX_DMA_CHN             0x00000000

#define STM32_HAS_USART3                    TRUE
#define STM32_USART3_RX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 3)
#define STM32_USART3_RX_DMA_CHN             0x00000000
#define STM32_USART3_TX_DMA_MSK             STM32_DMA_STREAM_ID_MSK(1, 2)
#define STM32_USART3_TX_DMA_CHN             0x00000000

#define STM32_HAS_UART4                     TRUE
#define STM32_UART4_RX_DMA_MSK              STM32_DMA_STREAM_ID_MSK(2, 3)
#define STM32_UART4_RX_DMA_CHN              0x00000000
#define STM32_UART4_TX_DMA_MSK              STM32_DMA_STREAM_ID_MSK(2, 5)
#define STM32_UART4_TX_DMA_CHN              0x00000000

#define STM32_HAS_UART5                     TRUE
#define STM32_UART5_RX_DMA_MSK              STM32_DMA_STREAM_ID_MSK(2, 2)
#define STM32_UART5_RX_DMA_CHN              0x00000000
#define STM32_UART5_TX_DMA_MSK              STM32_DMA_STREAM_ID_MSK(2, 1)
#define STM32_UART5_TX_DMA_CHN              0x00000000

#define STM32_HAS_USART6                    FALSE

/* USB attributes.*/
#define STM32_HAS_USB                       TRUE
#define STM32_HAS_OTG1                      FALSE
#define STM32_HAS_OTG2                      FALSE

#endif /* STM32L1XX_HD */

/** @} */

#endif /* _STM32_REGISTRY_H_ */

/** @} */
