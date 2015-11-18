/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    LPC43xx_M4/hal_lld.h
 * @brief   HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "lpc43xx_m4.h"

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
#define PLATFORM_NAME           "LPC43xx M4"
/** @} */

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Platform specific friendly IRQ names.                                     */
/*===========================================================================*/

/**
 * @name    IRQ VECTOR names
 * @{
 */
#define DAC_IRQHandler          Vector40    /**< DAC                        */
#define MAPP_IRQHandler         Vector44    /**< Cortex-M0APP               */
#define DMA_IRQHandler          Vector48    /**< DMA                        */
#define Ethernet_IRQHandler     Vector54    /**< Ethernet                   */
#define SDIO_IRQHandler         Vector58    /**< SD/MMC                     */
#define LCD_IRQHandler          Vector5C    /**< LCD                        */
#define USB0_IRQHandler         Vector60    /**< USB0: OTG                  */
#define USB1_IRQHandler         Vector64    /**< USB1                       */
#define SCT_IRQHandler          Vector68    /**< SCT combined               */
#define RITimer_IRQHandler      Vector6C    /**< RI Timer                   */
#define Timer0_IRQHandler       Vector70    /**< Timer 0                    */
#define Timer1_IRQHandler       Vector74    /**< Timer 1                    */
#define Timer2_IRQHandler       Vector78    /**< Timer 2                    */
#define Timer3_IRQHandler       Vector7C    /**< Timer 3                    */
#define MCPWM_IRQHandler        Vector80    /**< Motor control PWM          */
#define ADC0_IRQHandler         Vector84    /**< ADC0                       */
#define I2C0_IRQHandler         Vector88    /**< I2C0                       */
#define I2C1_IRQHandler         Vector8C    /**< I2C1                       */
#define SPI_IRQHandler          Vector90    /**< SPI                        */
#define ADC1_IRQHandler         Vector94    /**< ADC1                       */
#define SSP0_IRQHandler         Vector98    /**< SSP0                       */
#define SSP1_IRQHandler         Vector9C    /**< SSP1                       */
#define USART0_IRQHandler       VectorA0    /**< USART0                     */
#define UART1_IRQHandler        VectorA4    /**< UART1                      */
#define USART2_IRQHandler       VectorA8    /**< USART2                     */
#define USART3_IRQHandler       VectorAC    /**< USART3                     */
#define I2S0_IRQHandler         VectorB0    /**< I2S0                       */
#define I2S1_IRQHandler         VectorB4    /**< I2S1                       */
#define SPIFI_IRQHandler        VectorB8    /**< SPIFI                      */
#define SGPIO_IRQHandler        VectorBC    /**< SGPIO                      */
#define PIN_INT0_IRQHandler     VectorC0    /**< GPIO pin interrupt 0       */
#define PIN_INT1_IRQHandler     VectorC4    /**< GPIO pin interrupt 1       */
#define PIN_INT2_IRQHandler     VectorC8    /**< GPIO pin interrupt 2       */
#define PIN_INT3_IRQHandler     VectorCC    /**< GPIO pin interrupt 3       */
#define PIN_INT4_IRQHandler     VectorD0    /**< GPIO pin interrupt 4       */
#define PIN_INT5_IRQHandler     VectorD4    /**< GPIO pin interrupt 5       */
#define PIN_INT6_IRQHandler     VectorD8    /**< GPIO pin interrupt 6       */
#define PIN_INT7_IRQHandler     VectorDC    /**< GPIO pin interrupt 7       */
#define GINT0_IRQHandler        VectorE0    /**< GPIO global interrupt 0    */
#define GINT1_IRQHandler        VectorE4    /**< GPIO global interrupt 1    */
#define EventRouter_IRQHandler  VectorE8    /**< Event router               */
#define C_CAN1_IRQHandler       VectorEC    /**< C_CAN1                     */
#define ATimer_IRQHandler       VectorF8    /**< Alarm timer                */
#define RTC_IRQHandler          VectorFC    /**< RTC                        */
#define WWDT_IRQHandler         Vector104   /**< WWDT                       */
#define C_CAN0_IRQHandler       Vector10C   /**< C_CAN0                     */
#define QEI_IRQHandler          Vector110   /**< QEI                        */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#define LPC43XX_M4_CLK_IRC              12000000

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
#define hal_lld_get_counter_frequency()     halLPCGetSystemClock()

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void systick_stop(void);
  void systick_adjust_period(const uint32_t counts_per_tick);
  halclock_t halLPCGetSystemClock(void);
  void halLPCSetSystemClock(const halclock_t new_frequency);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
