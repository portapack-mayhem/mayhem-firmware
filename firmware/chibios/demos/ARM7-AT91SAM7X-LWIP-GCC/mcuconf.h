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
 * AT91SAM7 drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the driver
 * is enabled in halconf.h.
 */

/*
 * ADC driver system settings.
 */

/*
 * CAN driver system settings.
 */

/*
 * MAC driver system settings.
 */
#define MAC_TRANSMIT_BUFFERS        2
#define MAC_RECEIVE_BUFFERS         2
#define MAC_BUFFERS_SIZE            1518
#define EMAC_INTERRUPT_PRIORITY     (AT91C_AIC_PRIOR_HIGHEST - 3)

/*
 * PWM driver system settings.
 */

/*
 * SERIAL driver system settings.
 */
#define USE_SAM7_USART0             TRUE
#define USE_SAM7_USART1             TRUE
#define SAM7_USART0_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)
#define SAM7_USART1_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)

/*
 * SPI driver system settings.
 */
#define USE_AT91SAM7_SPI            FALSE
#define AT91SAM7_SPI_USE_SPI0       TRUE
#define AT91SAM7_SPI_USE_SPI1       FALSE
#define AT91SAM7_SPI0_PRIORITY      (AT91C_AIC_PRIOR_HIGHEST - 1)
#define AT91SAM7_SPI1_PRIORITY      (AT91C_AIC_PRIOR_HIGHEST - 1)
