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
 * @file    STM32F4xx/stm32_isr.h
 * @brief   ISR remapper driver header.
 *
 * @addtogroup STM32F4xx_ISR
 * @{
 */

#ifndef _STM32_ISR_H_
#define _STM32_ISR_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    ISR names and numbers remapping
 * @{
 */
/*
 * CAN units.
 */
#define STM32_CAN1_TX_HANDLER       Vector8C
#define STM32_CAN1_RX0_HANDLER      Vector90
#define STM32_CAN1_RX1_HANDLER      Vector94
#define STM32_CAN1_SCE_HANDLER      Vector98
#define STM32_CAN2_TX_HANDLER       Vector13C
#define STM32_CAN2_RX0_HANDLER      Vector140
#define STM32_CAN2_RX1_HANDLER      Vector144
#define STM32_CAN2_SCE_HANDLER      Vector148

#define STM32_CAN1_TX_NUMBER        19
#define STM32_CAN1_RX0_NUMBER       20
#define STM32_CAN1_RX1_NUMBER       21
#define STM32_CAN1_SCE_NUMBER       22
#define STM32_CAN2_TX_NUMBER        63
#define STM32_CAN2_RX0_NUMBER       64
#define STM32_CAN2_RX1_NUMBER       65
#define STM32_CAN2_SCE_NUMBER       66

/*
 * I2C units.
 */
#define STM32_I2C1_EVENT_HANDLER    VectorBC
#define STM32_I2C1_ERROR_HANDLER    VectorC0
#define STM32_I2C1_EVENT_NUMBER     31
#define STM32_I2C1_ERROR_NUMBER     32

#define STM32_I2C2_EVENT_HANDLER    VectorC4
#define STM32_I2C2_ERROR_HANDLER    VectorC8
#define STM32_I2C2_EVENT_NUMBER     33
#define STM32_I2C2_ERROR_NUMBER     34

#define STM32_I2C3_EVENT_HANDLER    Vector160
#define STM32_I2C3_ERROR_HANDLER    Vector164
#define STM32_I2C3_EVENT_NUMBER     72
#define STM32_I2C3_ERROR_NUMBER     73

/*
 * OTG units.
 */
#define STM32_OTG1_HANDLER          Vector14C
#define STM32_OTG2_HANDLER          Vector174
#define STM32_OTG2_EP1OUT_HANDLER   Vector168
#define STM32_OTG2_EP1IN_HANDLER    Vector16C

#define STM32_OTG1_NUMBER           67
#define STM32_OTG2_NUMBER           77
#define STM32_OTG2_EP1OUT_NUMBER    74
#define STM32_OTG2_EP1IN_NUMBER     75

/*
 * SDIO unit.
 */
#define STM32_SDIO_HANDLER          Vector104

#define STM32_SDIO_NUMBER           49

/*
 * TIM units.
 */
#define STM32_TIM1_UP_HANDLER       VectorA4
#define STM32_TIM1_CC_HANDLER       VectorAC
#define STM32_TIM2_HANDLER          VectorB0
#define STM32_TIM3_HANDLER          VectorB4
#define STM32_TIM4_HANDLER          VectorB8
#define STM32_TIM5_HANDLER          Vector108
#define STM32_TIM6_HANDLER          Vector118
#define STM32_TIM7_HANDLER          Vector11C
#define STM32_TIM8_UP_HANDLER       VectorF0
#define STM32_TIM8_CC_HANDLER       VectorF8
#define STM32_TIM9_HANDLER          VectorA0
#define STM32_TIM11_HANDLER         VectorA8
#define STM32_TIM12_HANDLER         VectorEC
#define STM32_TIM14_HANDLER         VectorF4

#define STM32_TIM1_UP_NUMBER        25
#define STM32_TIM1_CC_NUMBER        27
#define STM32_TIM2_NUMBER           28
#define STM32_TIM3_NUMBER           29
#define STM32_TIM4_NUMBER           30
#define STM32_TIM5_NUMBER           50
#define STM32_TIM6_NUMBER           54
#define STM32_TIM7_NUMBER           55
#define STM32_TIM8_UP_NUMBER        44
#define STM32_TIM8_CC_NUMBER        46
#define STM32_TIM9_NUMBER           24
#define STM32_TIM11_NUMBER          26
#define STM32_TIM12_NUMBER          43
#define STM32_TIM14_NUMBER          45

/*
 * USART units.
 */
#define STM32_USART1_HANDLER        VectorD4
#define STM32_USART2_HANDLER        VectorD8
#define STM32_USART3_HANDLER        VectorDC
#define STM32_UART4_HANDLER         Vector110
#define STM32_UART5_HANDLER         Vector114
#define STM32_USART6_HANDLER        Vector15C

#define STM32_USART1_NUMBER         37
#define STM32_USART2_NUMBER         38
#define STM32_USART3_NUMBER         39
#define STM32_UART4_NUMBER          52
#define STM32_UART5_NUMBER          53
#define STM32_USART6_NUMBER         71

/*
 * Ethernet
 */
#define ETH_IRQHandler              Vector134

/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#endif /* _STM32_ISR_H_ */

/** @} */
