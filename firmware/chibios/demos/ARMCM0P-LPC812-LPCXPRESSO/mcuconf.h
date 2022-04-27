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
 * LPC812 drivers configuration.
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

/* Default: Run PLL @24MHz from 12MHz IRC
  #define LPC8xx_PLLCLK_SOURCE               SYSPLLCLKSEL_IRCOSC
  #define LPC8xx_SYSPLL_MUL                  4
  #define LPC8xx_SYSPLL_DIV                  4
  #define LPC8xx_MAINCLK_SOURCE              SYSMAINCLKSEL_PLLOUT
  #define LPC8xx_SYSABHCLK_DIV               1
*/

/*run directly from internal 12M osc...*/
#define LPC8xx_MAINCLK_SOURCE              SYSMAINCLKSEL_IRCOSC

/*
 * GPT driver system settings.
 */
/* Defaults:
  #define LPC8xx_GPT_USE_MRT0              TRUE
  #define LPC8xx_GPT_USE_MRT1              FALSE
  #define LPC8xx_GPT_USE_MRT2              FALSE
  #define LPC8xx_GPT_USE_MRT3              FALSE
  #define LPC8xx_GPT_MRT_IRQ_PRIORITY      2
*/

/*
 * PWM driver system settings.
 */

/*
 * SERIAL driver system settings.
 */
/* Defaults:
  #define LPC8xx_SERIAL_USE_UART0            TRUE
  #define LPC8xx_SERIAL_USE_UART1            FALSE
  #define LPC8xx_SERIAL_USE_UART2            FALSE
  #define LPC8xx_SERIAL_UART0_IRQ_PRIORITY   3
  #define LPC8xx_SERIAL_UART1_IRQ_PRIORITY   3
  #define LPC8xx_SERIAL_UART2_IRQ_PRIORITY   3

  #define LPC8xx_SERIAL_U_PCLK              11059200
  #define LPC8xx_SERIAL_UARTCLKDIV          !!Calculated!!
  #define LPC8xx_SERIAL_UARTFRGMUL          !!Calculated!!
  #define LPC8xx_SERIAL_UARTFRGDIV          !!Calculated!!
*/

/* change default baudrate to 9600 */
#define SERIAL_DEFAULT_BITRATE              9600


