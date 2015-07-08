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

/*
 * LPC1227 drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the driver
 * is enabled in halconf.h.
 *
 * IRQ priorities:
 * 3...0        Lowest...highest.
 */

/*
 * HAL driver system settings.
 */
#define LPC122x_PLLCLK_SOURCE               SYSPLLCLKSEL_SYSOSC
#define LPC122x_SYSPLL_MUL                  3
#define LPC122x_SYSPLL_DIV                  8
#define LPC122x_MAINCLK_SOURCE              SYSMAINCLKSEL_PLLOUT
#define LPC122x_SYSABHCLK_DIV               1

/*
 * ADC driver system settings.
 */

/*
 * CAN driver system settings.
 */

/*
 * GPT driver system settings.
 */
#define LPC122x_GPT_USE_CT16B0              TRUE
#define LPC122x_GPT_USE_CT16B1              TRUE
#define LPC122x_GPT_USE_CT32B0              TRUE
#define LPC122x_GPT_USE_CT32B1              TRUE
#define LPC122x_GPT_CT16B0_IRQ_PRIORITY     1
#define LPC122x_GPT_CT16B1_IRQ_PRIORITY     3
#define LPC122x_GPT_CT32B0_IRQ_PRIORITY     2
#define LPC122x_GPT_CT32B1_IRQ_PRIORITY     2

/*
 * PWM driver system settings.
 */

/*
 * SERIAL driver system settings.
 */
#define LPC122x_SERIAL_USE_UART0            TRUE
#define LPC122x_SERIAL_FIFO_PRELOAD         16
#define LPC122x_SERIAL_UART0CLKDIV          1
#define LPC122x_SERIAL_UART0_IRQ_PRIORITY   3
#define LPC122x_SERIAL_TXD0_SELECTOR        TXD0_IS_PIO0_2
#define LPC122x_SERIAL_RXD0_SELECTOR        RXD0_IS_PIO0_1

#define LPC122x_SERIAL_USE_UART1            FALSE
#define LPC122x_SERIAL_FIFO_PRELOAD         16
#define LPC122x_SERIAL_UART1CLKDIV          1
#define LPC122x_SERIAL_UART1_IRQ_PRIORITY   3
#define LPC122x_SERIAL_RXD1_SELECTOR        RXD1_IS_PIO0_8
#define LPC122x_SERIAL_TXD1_SELECTOR        TXD1_IS_PIO0_9

/*
 * SPI driver system settings.
 */
#define LPC122x_SPI_USE_SSP0                TRUE
#define LPC122x_SPI_SSP0CLKDIV              1
#define LPC122x_SPI_SSP0_IRQ_PRIORITY       1
#define LPC122x_SPI_SSP_ERROR_HOOK(spip)    chSysHalt()
