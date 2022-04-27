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
 * @file    LPC43xx_M0/hal_lld.h
 * @brief   HAL subsystem low level driver header template.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "lpc43xx_m0.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS TRUE

/**
 * @brief   Platform name.
 */
#define PLATFORM_NAME           "LPC43xx M0"

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
#define RTC_IRQHandler          Vector40    /**< RTC                        */
#define M4Core_IRQHandler       Vector44    /**< Cortex-M4                  */
#define DMA_IRQHandler          Vector48    /**< DMA                        */
#define Ethernet_IRQHandler     Vector54    /**< Ethernet                   */
#define SDIO_IRQHandler         Vector58    /**< SD/MMC                     */
#define LCD_IRQHandler          Vector5C    /**< LCD                        */
#define USB0_IRQHandler         Vector60    /**< USB0: OTG                  */
#define USB1_IRQHandler         Vector64    /**< USB1                       */
#define SCT_IRQHandler          Vector68    /**< SCT combined               */
#define RITimer_Or_WWDT_IRQHandler  Vector6C    /**< RI Timer or WWDT       */
#define Timer0_IRQHandler       Vector70    /**< Timer 0                    */
#define GINT1_IRQHandler        Vector74    /**< GPIO global interrupt 1    */
#define PIN_INT4_IRQHandler     Vector78    /**< GPIO pin interrupt 4       */
#define Timer3_IRQHandler       Vector7C    /**< Timer 3                    */
#define MCPWM_IRQHandler        Vector80    /**< Motor control PWM          */
#define ADC0_IRQHandler         Vector84    /**< ADC0                       */
#define I2C0_Or_I2C1_IRQHandler Vector88    /**< I2C0 or I2C1               */
#define SGPIO_IRQHandler        Vector8C    /**< SGPIO                      */
#define SPI_Or_DAC_IRQHandler   Vector90    /**< SPI or DAC                 */
#define ADC1_IRQHandler         Vector94    /**< ADC1                       */
#define SSP0_Or_SSP1_IRQHandler Vector98    /**< SSP0 or SSP1               */
#define EventRouter_IRQHandler  Vector9C    /**< Event router               */
#define USART0_IRQHandler       VectorA0    /**< USART0                     */
#define UART1_IRQHandler        VectorA4    /**< UART1                      */
#define USART2_Or_C_CAN1_IRQHandler VectorA8    /**< USART2 or C_CAN1       */
#define USART3_IRQHandler       VectorAC    /**< USART3                     */
#define I2S0_Or_I2S1_QEI_IRQHandler VectorB0    /**< I2S0 or I2S1 or QEI    */
#define C_CAN0_IRQHandler       VectorB4    /**< C_CAN0                     */
#define SPIFI_Or_ADCHS_IRQHandler   VectorB8    /**< SPIFI or ADCHS         */
#define M0SUB_IRQHandler        VectorBC    /**< M0SUB                      */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#define LPC43XX_M0_CLK_PLL1_AT_BOOT     96000000

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
 * @note    This service is implemented by returning the content of timer 3's
 *          TC (counter value) register.
 *
 * @return              The value of the system free running counter of
 *                      type halrtcnt_t.
 *
 * @notapi
 */
#define hal_lld_get_counter_value()         (LPC_TIMER3->TC)

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
