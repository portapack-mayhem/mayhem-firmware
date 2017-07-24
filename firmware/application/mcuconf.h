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

/*
 * LPC43xx drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the driver
 * is enabled in halconf.h.
 *
 * IRQ priorities:
 * 3...0        Lowest...highest.
 */

/* NOTE: Beware setting IRQ priorities < "2":
 * dbg_check_enter_isr "#SV8 means that probably you have some IRQ set at a
 * priority level above the kernel level (level 0 or 1 usually) so it is able
 * to preempt the kernel and mess things up.
 */

/*
 * I2C driver system settings.
 */


#define LPC43XX_I2C_USE_I2C0                TRUE

/*
 * SPI driver system settings.
 */

#define LPC_SPI_USE_SSP1                    TRUE


/*
 * DMA driver system settings.
 */

#define LPC_ADC0_IRQ_PRIORITY               1
//#define LPC_DMA_IRQ_PRIORITY                2
//#define LPC_ADC1_IRQ_PRIORITY               3
#define LPC43XX_GPT_TIMER0_IRQ_PRIORITY     2
//#define LPC43XX_GPT_TIMER1_IRQ_PRIORITY     2
#define LPC43XX_M0_I2C_I2C0_OR_I2C1_IRQ_PRIORITY    3
#define LPC43XX_PIN_INT4_IRQ_PRIORITY       3

#define LPC_SPI_SSP0_OR_SSP1_IRQ_PRIORITY           3

#define LPC_SDC_SDIO_IRQ_PRIORITY           3
#define LPC_RTC_IRQ_PRIORITY                3

#define LPC43XX_GPT_USE_TIMER0              TRUE
//#define LPC43XX_GPT_USE_TIMER1              TRUE

#define LPC43XX_M4TXEVENT_IRQ_PRIORITY      3
