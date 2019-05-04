/*
    ChibiOS/RT - Copyright (C) 2014 Jared Boone, ShareBrained Technology

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

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup lpc43xx_m0
  * @{
  */

#ifndef __LPC43XX_M0_H
#define __LPC43XX_M0_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup Configuration_section_for_CMSIS
  * @{
  */

/**
 * @brief Configuration of the Cortex-M0 Processor and Core Peripherals
 */
#define __CM0_REV                 0       /*!< Core revision r0p0                             */
#define __MPU_PRESENT             0       /*!< LPC43XX M0 does not provide MPU                */
#define __NVIC_PRIO_BITS          2       /*!< LPC43XX M0 uses 2 Bits for the Priority Levels */
#define __Vendor_SysTickConfig    0       /*!< Set to 1 if different SysTick Config is used   */


/**
 * @brief LPC43XX M0 Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */
typedef enum IRQn {
/******  Cortex-M0 Processor Exceptions Numbers ****************************************************************/
  Reset_IRQn                  = -15,    /*!< 1 Reset Vector, invoked on Power up and warm reset      */
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                */
  HardFault_IRQn              = -13,    /*!< 3 Cortex-M0 Hard Fault Interrupt                        */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M0 SV Call Interrupt                          */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M0 Pend SV Interrupt                          */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M0 System Tick Interrupt                      */

/******  LPC43xx M0 specific Interrupt Numbers *****************************************************************/
  RTC_IRQn                    = 0,      /*!< 16 RTC                                                            */
  M4CORE_IRQn                 = 1,      /*!< 17 Cortex-M4; Latched TXEV; for M0APP-M4 communication            */
  DMA_IRQn                    = 2,      /*!< 18 DMA                                                            */
  /* 3: 19 Reserved */
  /* 4: 20 ORed flash bank A, flash bank B, EEPROM interrupts */
  ETHERNET_IRQn               = 5,      /*!< 21 Ethernet                                                       */
  SDIO_IRQn                   = 6,      /*!< 22 SD/MMC                                                         */
  LCD_IRQn                    = 7,      /*!< 23 LCD                                                            */
  USB0_IRQn                   = 8,      /*!< 24 OTG interrupt                                                  */
  USB1_IRQn                   = 9,      /*!< 25 USB1                                                           */
  SCT_IRQn                    = 10,     /*!< 26 SCT combined interrupt                                         */
  RITIMER_OR_WWDT_IRQn        = 11,     /*!< 27 RITIMER or WWDT                                                */
  TIMER0_IRQn                 = 12,     /*!< 28 TIMER0                                                         */
  GINT1_IRQn                  = 13,     /*!< 29 GPIO globak interrupt 1                                        */
  PIN_INT4_IRQn               = 14,     /*!< 30 GPIO pin interrupt 4                                           */
  TIMER3_IRQn                 = 15,     /*!< 31 TIMER3                                                         */
  MCPWM_IRQn                  = 16,     /*!< 32 Motor control PWM                                              */
  ADC0_IRQn                   = 17,     /*!< 33 ADC0                                                           */
  I2C0_OR_I2C1_IRQn           = 18,     /*!< 34 I2C0 or I2C1                                                   */
  SGPIO_IRQn                  = 19,     /*!< 35 SGPIO                                                          */
  SPI_OR_DAC_IRQn             = 20,     /*!< 36 SPI interrupt ORed with DAC interrupt                          */
  ADC1_IRQn                   = 21,     /*!< 37 ADC1                                                           */
  SSP0_OR_SSP1_IRQn           = 22,     /*!< 38 SSP0 interrupt ORed with SSP1 interrupt                        */
  EVENTROUTER_IRQn            = 23,     /*!< 39 Event router                                                   */
  USART0_IRQn                 = 24,     /*!< 40 USART0                                                         */
  UART1_IRQn                  = 25,     /*!< 41 Combined UART interrupt with Modem interrupt                   */
  USART2_OR_C_CAN1_IRQn       = 26,     /*!< 42 USART2 interrupt ORed with C_CAN1 interrupt                    */
  USART3_IRQn                 = 27,     /*!< 43 Combined USART interrupt with IrDA interrupt                   */
  I2S0_OR_I2S1_QEI_IRQn       = 28,     /*!< 44 I2S0 OR I2S1 OR QEI interrupt                                  */
  C_CAN0_IRQn                 = 29,     /*!< 45 C_CAN0                                                         */
  SPIFI_OR_ADCHS_IRQn         = 30,     /*!< 46 SPIFI OR ADCHS interrupt                                       */
  M0SUB_IRQn                  = 31,     /*!< 47 M0SUB core                                                     */
  TIMER1_IRQn                 = 32
} IRQn_Type;

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "core_cm0.h"             /* Cortex-M0 processor and core peripherals */
#include "lpc43xx.inc"
#include "lpc43xx.h"

#endif /* __LPC43XX_M0_H */

/**
  * @}
  */

/**
  * @}
  */
