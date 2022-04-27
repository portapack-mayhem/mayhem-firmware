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
 * @file    STM32F30x/stm32_isr.h
 * @brief   ISR remapper driver header.
 *
 * @addtogroup STM32F30x_ISR
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

#define STM32_CAN1_TX_NUMBER        19
#define STM32_CAN1_RX0_NUMBER       20
#define STM32_CAN1_RX1_NUMBER       21
#define STM32_CAN1_SCE_NUMBER       22

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

/*
 * TIM units.
 */
#define STM32_TIM1_UP_HANDLER       VectorA4
#define STM32_TIM1_CC_HANDLER       VectorAC
#define STM32_TIM2_HANDLER          VectorB0
#define STM32_TIM3_HANDLER          VectorB4
#define STM32_TIM4_HANDLER          VectorB8
#define STM32_TIM6_HANDLER          Vector118
#define STM32_TIM7_HANDLER          Vector11C
#define STM32_TIM8_UP_HANDLER       VectorF0
#define STM32_TIM8_CC_HANDLER       VectorF8

#define STM32_TIM1_UP_NUMBER        25
#define STM32_TIM1_CC_NUMBER        27
#define STM32_TIM2_NUMBER           28
#define STM32_TIM3_NUMBER           29
#define STM32_TIM4_NUMBER           30
#define STM32_TIM6_NUMBER           54
#define STM32_TIM7_NUMBER           55
#define STM32_TIM8_UP_NUMBER        44
#define STM32_TIM8_CC_NUMBER        46

/*
 * USART units.
 */
#define STM32_USART1_HANDLER        VectorD4
#define STM32_USART2_HANDLER        VectorD8
#define STM32_USART3_HANDLER        VectorDC
#define STM32_UART4_HANDLER         Vector110
#define STM32_UART5_HANDLER         Vector114

#define STM32_USART1_NUMBER         37
#define STM32_USART2_NUMBER         38
#define STM32_USART3_NUMBER         39
#define STM32_UART4_NUMBER          52
#define STM32_UART5_NUMBER          53

/*
 * USB units.
 */
#define STM32_USB1_HP_HANDLER       Vector168
#define STM32_USB1_LP_HANDLER       Vector16C

#define STM32_USB1_HP_NUMBER        74
#define STM32_USB1_LP_NUMBER        75
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
