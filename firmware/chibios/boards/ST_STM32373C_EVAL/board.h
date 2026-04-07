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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for STMicroelectronics STM32373C-EVAL board.
 */

/*
 * Board identifier.
 */
#define BOARD_ST_STM32373C_EVAL
#define BOARD_NAME                  "STMicroelectronics STM32373C-EVAL"

/*
 * Board oscillators-related settings.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                32768
#endif

#define STM32_LSEDRV                (3 << 3)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                8000000
#endif

/*
 * MCU type as defined in the ST header.
 */
#define STM32F37X

/*
 * IO pins assignments.
 */
#define GPIOA_WKUP_BUTTON           0
#define GPIOA_LDR_OUT               1
#define GPIOA_KEY_BUTTON            2
#define GPIOA_PIN3                  3
#define GPIOA_PIN4                  4
#define GPIOA_PIN5                  5
#define GPIOA_PIN6                  6
#define GPIOA_COMP2_OUT             7
#define GPIOA_I2C2_SMB              8
#define GPIOA_I2C2_SCL              9
#define GPIOA_I2C2_SDA              10
#define GPIOA_USB_DM                11
#define GPIOA_USB_DP                12
#define GPIOA_SWDIO                 13
#define GPIOA_SWCLK                 14
#define GPIOA_JTDI                  15

#define GPIOB_MIC_IN                0
#define GPIOB_ADC_POT_IN            1
#define GPIOB_PIN2                  2
#define GPIOB_SWO                   3
#define GPIOB_JTRST                 4
#define GPIOB_PIN5                  5
#define GPIOB_I2C1_SCL              6
#define GPIOB_I2C1_SDA              7
#define GPIOB_PIN8                  8
#define GPIOB_PIN9                  9
#define GPIOB_PIN10                 10
#define GPIOB_PIN11                 11
#define GPIOB_PIN12                 12
#define GPIOB_PIN13                 13
#define GPIOB_PIN14                 14
#define GPIOB_PIN15                 15

#define GPIOC_LED1                  0
#define GPIOC_LED2                  1
#define GPIOC_LED3                  2
#define GPIOC_LED4                  3
#define GPIOC_PIN4                  4
#define GPIOC_USB_DISCONNECT        5
#define GPIOC_PIN6                  6
#define GPIOC_PIN7                  7
#define GPIOC_PIN8                  8
#define GPIOC_PIN9                  9
#define GPIOC_SPI3_SCK              10
#define GPIOC_SPI3_MISO             11
#define GPIOC_SPI3_MOSI             12
#define GPIOC_PIN13                 13
#define GPIOC_OSC32_IN              14
#define GPIOC_OSC32_OUT             15

#define GPIOD_CAN_RX                0
#define GPIOD_CAN_TX                1
#define GPIOD_LCD_CS                2
#define GPIOD_USART2_CTS            3
#define GPIOD_USART2_RST            4
#define GPIOD_USART2_TX             5
#define GPIOD_USART2_RX             6
#define GPIOD_PIN7                  7
#define GPIOD_PIN8                  8
#define GPIOD_PIN9                  9
#define GPIOD_PIN10                 10
#define GPIOD_AUDIO_RST             11
#define GPIOD_PIN12                 12
#define GPIOD_PIN13                 13
#define GPIOD_PIN14                 14
#define GPIOD_PIN15                 15

#define GPIOE_PIN0                  0
#define GPIOE_PIN1                  1
#define GPIOE_SD_CS                 2
#define GPIOE_SD_DETECT             3
#define GPIOE_PIN4                  4
#define GPIOE_PIN5                  5
#define GPIOE_JOY_SEL               6
#define GPIOE_RTD_IN                7
#define GPIOE_PRESSUREP             8
#define GPIOE_PRESSUREN             9
#define GPIOE_PIN10                 10
#define GPIOE_PIN11                 11
#define GPIOE_PIN12                 12
#define GPIOE_PIN13                 13
#define GPIOE_PRESSURE_TEPM         14
#define GPIOE_PIN15                 15

#define GPIOF_OSC_IN                0
#define GPIOF_OSC_OUT               1
#define GPIOF_JOY_DOWN              2
#define GPIOF_PIN3                  3
#define GPIOF_JOY_LEFT              4
#define GPIOF_PIN5                  5
#define GPIOF_PIN6                  6
#define GPIOF_PIN7                  7
#define GPIOF_PIN8                  8
#define GPIOF_JOY_RIGHT             9
#define GPIOF_JOY_UP                10
#define GPIOF_PIN11                 11
#define GPIOF_PIN12                 12
#define GPIOF_PIN13                 13
#define GPIOF_PIN14                 14
#define GPIOF_PIN15                 15

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_2M(n)            (0U << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1U << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2U << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3U << ((n) * 2))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2))
#define PIN_AFIO_AF(n, v)           ((v##U) << ((n % 8) * 4))

/*
 * GPIOA setup:
 *
 * PA0  - WKUP_BUTTON               (input floating).
 * PA1  - LDR_OUT                   (analog).
 * PA2  - KEY_BUTTON                (input floating).
 * PA3  - PIN3                      (input pullup).
 * PA4  - PIN4                      (input pullup).
 * PA5  - PIN5                      (input floating).
 * PA6  - PIN6                      (input pullup).
 * PA7  - COMP2_OUT                 (output pushpull maximum).
 * PA8  - I2C2_SMB                  (input floating).
 * PA9  - I2C2_SCL                  (alternate 4).
 * PA10 - I2C2_SDA                  (alternate 4).
 * PA11 - USB_DM                    (alternate 14).
 * PA12 - USB_DP                    (alternate 14).
 * PA13 - SWDIO                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - JTDI                      (alternate 0).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(GPIOA_WKUP_BUTTON) |    \
                                     PIN_MODE_ANALOG(GPIOA_LDR_OUT) |       \
                                     PIN_MODE_INPUT(GPIOA_KEY_BUTTON) |     \
                                     PIN_MODE_INPUT(GPIOA_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN6) |           \
                                     PIN_MODE_OUTPUT(GPIOA_COMP2_OUT) |     \
                                     PIN_MODE_INPUT(GPIOA_I2C2_SMB) |       \
                                     PIN_MODE_ALTERNATE(GPIOA_I2C2_SCL) |   \
                                     PIN_MODE_ALTERNATE(GPIOA_I2C2_SDA) |   \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DM) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DP) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_JTDI))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_WKUP_BUTTON) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_LDR_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_KEY_BUTTON) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_COMP2_OUT) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_I2C2_SMB) |   \
                                     PIN_OTYPE_OPENDRAIN(GPIOA_I2C2_SCL) |  \
                                     PIN_OTYPE_OPENDRAIN(GPIOA_I2C2_SDA) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DM) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DP) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_JTDI))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_2M(GPIOA_WKUP_BUTTON) |     \
                                     PIN_OSPEED_2M(GPIOA_LDR_OUT) |         \
                                     PIN_OSPEED_2M(GPIOA_KEY_BUTTON) |      \
                                     PIN_OSPEED_2M(GPIOA_PIN3) |            \
                                     PIN_OSPEED_2M(GPIOA_PIN4) |            \
                                     PIN_OSPEED_100M(GPIOA_PIN5) |          \
                                     PIN_OSPEED_2M(GPIOA_PIN6) |            \
                                     PIN_OSPEED_100M(GPIOA_COMP2_OUT) |     \
                                     PIN_OSPEED_2M(GPIOA_I2C2_SMB) |        \
                                     PIN_OSPEED_100M(GPIOA_I2C2_SCL) |      \
                                     PIN_OSPEED_100M(GPIOA_I2C2_SDA) |      \
                                     PIN_OSPEED_100M(GPIOA_USB_DM) |        \
                                     PIN_OSPEED_2M(GPIOA_USB_DP) |          \
                                     PIN_OSPEED_100M(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_100M(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_100M(GPIOA_JTDI))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_FLOATING(GPIOA_WKUP_BUTTON) |\
                                     PIN_PUPDR_FLOATING(GPIOA_LDR_OUT) |    \
                                     PIN_PUPDR_FLOATING(GPIOA_KEY_BUTTON) | \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOA_PIN5) |       \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN6) |         \
                                     PIN_PUPDR_FLOATING(GPIOA_COMP2_OUT) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_I2C2_SMB) |   \
                                     PIN_PUPDR_FLOATING(GPIOA_I2C2_SCL) |   \
                                     PIN_PUPDR_FLOATING(GPIOA_I2C2_SDA) |   \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_DM) |     \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_DP) |     \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_JTDI))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_WKUP_BUTTON) |      \
                                     PIN_ODR_HIGH(GPIOA_LDR_OUT) |          \
                                     PIN_ODR_HIGH(GPIOA_KEY_BUTTON) |       \
                                     PIN_ODR_HIGH(GPIOA_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN6) |             \
                                     PIN_ODR_LOW(GPIOA_COMP2_OUT) |         \
                                     PIN_ODR_HIGH(GPIOA_I2C2_SMB) |         \
                                     PIN_ODR_HIGH(GPIOA_I2C2_SCL) |         \
                                     PIN_ODR_HIGH(GPIOA_I2C2_SDA) |         \
                                     PIN_ODR_HIGH(GPIOA_USB_DM) |           \
                                     PIN_ODR_HIGH(GPIOA_USB_DP) |           \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_HIGH(GPIOA_JTDI))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_WKUP_BUTTON, 0) |    \
                                     PIN_AFIO_AF(GPIOA_LDR_OUT, 0) |        \
                                     PIN_AFIO_AF(GPIOA_KEY_BUTTON, 0) |     \
                                     PIN_AFIO_AF(GPIOA_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN5, 5) |           \
                                     PIN_AFIO_AF(GPIOA_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOA_COMP2_OUT, 0))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_I2C2_SMB, 0) |       \
                                     PIN_AFIO_AF(GPIOA_I2C2_SCL, 4) |       \
                                     PIN_AFIO_AF(GPIOA_I2C2_SDA, 4) |       \
                                     PIN_AFIO_AF(GPIOA_USB_DM, 14) |        \
                                     PIN_AFIO_AF(GPIOA_USB_DP, 14) |        \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0) |          \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0) |          \
                                     PIN_AFIO_AF(GPIOA_JTDI, 0))

/*
 * GPIOB setup:
 *
 * PB0  - MIC_IN                    (analog).
 * PB1  - ADC_POT_IN                (analog).
 * PB2  - PIN2                      (input pullup).
 * PB3  - SWO                       (alternate 0).
 * PB4  - JTRST                     (input floating).
 * PB5  - PIN5                      (input pullup).
 * PB6  - I2C1_SCL                  (alternate 4).
 * PB7  - I2C1_SDA                  (alternate 4).
 * PB8  - PIN8                      (input pullup).
 * PB9  - PIN9                      (input pullup).
 * PB10 - PIN10                     (input pullup).
 * PB11 - PIN11                     (input pullup).
 * PB12 - PIN12                     (input pullup).
 * PB13 - PIN13                     (input pullup).
 * PB14 - PIN14                     (input pullup).
 * PB15 - PIN15                     (input pullup).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_ANALOG(GPIOB_MIC_IN) |        \
                                     PIN_MODE_ANALOG(GPIOB_ADC_POT_IN) |    \
                                     PIN_MODE_INPUT(GPIOB_PIN2) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_SWO) |        \
                                     PIN_MODE_INPUT(GPIOB_JTRST) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN5) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_I2C1_SCL) |   \
                                     PIN_MODE_ALTERNATE(GPIOB_I2C1_SDA) |   \
                                     PIN_MODE_INPUT(GPIOB_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOB_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOB_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN15))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_MIC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_ADC_POT_IN) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SWO) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_JTRST) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN5) |       \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_I2C1_SCL) |  \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_I2C1_SDA) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN15))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_2M(GPIOB_MIC_IN) |          \
                                     PIN_OSPEED_2M(GPIOB_ADC_POT_IN) |      \
                                     PIN_OSPEED_2M(GPIOB_PIN2) |            \
                                     PIN_OSPEED_100M(GPIOB_SWO) |           \
                                     PIN_OSPEED_100M(GPIOB_JTRST) |         \
                                     PIN_OSPEED_2M(GPIOB_PIN5) |            \
                                     PIN_OSPEED_100M(GPIOB_I2C1_SCL) |      \
                                     PIN_OSPEED_100M(GPIOB_I2C1_SDA) |      \
                                     PIN_OSPEED_2M(GPIOB_PIN8) |            \
                                     PIN_OSPEED_2M(GPIOB_PIN9) |            \
                                     PIN_OSPEED_2M(GPIOB_PIN10) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN14) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN15))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_FLOATING(GPIOB_MIC_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_ADC_POT_IN) | \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN2) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_SWO) |        \
                                     PIN_PUPDR_FLOATING(GPIOB_JTRST) |      \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN5) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_I2C1_SCL) |   \
                                     PIN_PUPDR_FLOATING(GPIOB_I2C1_SDA) |   \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN15))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_MIC_IN) |           \
                                     PIN_ODR_HIGH(GPIOB_ADC_POT_IN) |       \
                                     PIN_ODR_HIGH(GPIOB_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOB_SWO) |              \
                                     PIN_ODR_HIGH(GPIOB_JTRST) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOB_I2C1_SCL) |         \
                                     PIN_ODR_HIGH(GPIOB_I2C1_SDA) |         \
                                     PIN_ODR_HIGH(GPIOB_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN15))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_MIC_IN, 0) |         \
                                     PIN_AFIO_AF(GPIOB_ADC_POT_IN, 0) |     \
                                     PIN_AFIO_AF(GPIOB_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOB_SWO, 0) |            \
                                     PIN_AFIO_AF(GPIOB_JTRST, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOB_I2C1_SCL, 4) |       \
                                     PIN_AFIO_AF(GPIOB_I2C1_SDA, 4))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN15, 0))

/*
 * GPIOC setup:
 *
 * PC0  - LED1                      (output opendrain maximum).
 * PC1  - LED2                      (output opendrain maximum).
 * PC2  - LED3                      (output opendrain maximum).
 * PC3  - LED4                      (output opendrain maximum).
 * PC4  - PIN4                      (input pullup).
 * PC5  - USB_DISCONNECT            (output pushpull maximum).
 * PC6  - PIN6                      (input pullup).
 * PC7  - PIN7                      (input pullup).
 * PC8  - PIN8                      (input pullup).
 * PC9  - PIN9                      (input pullup).
 * PC10 - SPI3_SCK                  (alternate 6).
 * PC11 - SPI3_MISO                 (alternate 6).
 * PC12 - SPI3_MOSI                 (alternate 6).
 * PC13 - PIN13                     (input pullup).
 * PC14 - OSC32_IN                  (input floating).
 * PC15 - OSC32_OUT                 (input floating).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_OUTPUT(GPIOC_LED1) |          \
                                     PIN_MODE_OUTPUT(GPIOC_LED2) |          \
                                     PIN_MODE_OUTPUT(GPIOC_LED3) |          \
                                     PIN_MODE_OUTPUT(GPIOC_LED4) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN4) |           \
                                     PIN_MODE_OUTPUT(GPIOC_USB_DISCONNECT) |\
                                     PIN_MODE_INPUT(GPIOC_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN9) |           \
                                     PIN_MODE_ALTERNATE(GPIOC_SPI3_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOC_SPI3_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_SPI3_MOSI) |  \
                                     PIN_MODE_INPUT(GPIOC_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOC_OSC32_IN) |       \
                                     PIN_MODE_INPUT(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_OPENDRAIN(GPIOC_LED1) |      \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_LED2) |      \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_LED3) |      \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_LED4) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_USB_DISCONNECT) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SPI3_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SPI3_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SPI3_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_IN) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_100M(GPIOC_LED1) |          \
                                     PIN_OSPEED_100M(GPIOC_LED2) |          \
                                     PIN_OSPEED_100M(GPIOC_LED3) |          \
                                     PIN_OSPEED_100M(GPIOC_LED4) |          \
                                     PIN_OSPEED_2M(GPIOC_PIN4) |            \
                                     PIN_OSPEED_100M(GPIOC_USB_DISCONNECT) |\
                                     PIN_OSPEED_2M(GPIOC_PIN6) |            \
                                     PIN_OSPEED_2M(GPIOC_PIN7) |            \
                                     PIN_OSPEED_2M(GPIOC_PIN8) |            \
                                     PIN_OSPEED_2M(GPIOC_PIN9) |            \
                                     PIN_OSPEED_100M(GPIOC_SPI3_SCK) |      \
                                     PIN_OSPEED_100M(GPIOC_SPI3_MISO) |     \
                                     PIN_OSPEED_100M(GPIOC_SPI3_MOSI) |     \
                                     PIN_OSPEED_2M(GPIOC_PIN13) |           \
                                     PIN_OSPEED_100M(GPIOC_OSC32_IN) |      \
                                     PIN_OSPEED_100M(GPIOC_OSC32_OUT))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_FLOATING(GPIOC_LED1) |       \
                                     PIN_PUPDR_FLOATING(GPIOC_LED2) |       \
                                     PIN_PUPDR_FLOATING(GPIOC_LED3) |       \
                                     PIN_PUPDR_FLOATING(GPIOC_LED4) |       \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_USB_DISCONNECT) |\
                                     PIN_PUPDR_PULLUP(GPIOC_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN9) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_SPI3_SCK) |   \
                                     PIN_PUPDR_PULLUP(GPIOC_SPI3_MISO) |    \
                                     PIN_PUPDR_FLOATING(GPIOC_SPI3_MOSI) |  \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN13) |        \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_IN) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_OUT))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_LED1) |             \
                                     PIN_ODR_HIGH(GPIOC_LED2) |             \
                                     PIN_ODR_HIGH(GPIOC_LED3) |             \
                                     PIN_ODR_HIGH(GPIOC_LED4) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOC_USB_DISCONNECT) |   \
                                     PIN_ODR_HIGH(GPIOC_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOC_SPI3_SCK) |         \
                                     PIN_ODR_HIGH(GPIOC_SPI3_MISO) |        \
                                     PIN_ODR_HIGH(GPIOC_SPI3_MOSI) |        \
                                     PIN_ODR_HIGH(GPIOC_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOC_OSC32_IN) |         \
                                     PIN_ODR_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_LED1, 0) |           \
                                     PIN_AFIO_AF(GPIOC_LED2, 0) |           \
                                     PIN_AFIO_AF(GPIOC_LED3, 0) |           \
                                     PIN_AFIO_AF(GPIOC_LED4, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOC_USB_DISCONNECT, 0) | \
                                     PIN_AFIO_AF(GPIOC_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN7, 0))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOC_SPI3_SCK, 6) |       \
                                     PIN_AFIO_AF(GPIOC_SPI3_MISO, 6) |      \
                                     PIN_AFIO_AF(GPIOC_SPI3_MOSI, 6) |      \
                                     PIN_AFIO_AF(GPIOC_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOC_OSC32_IN, 0) |       \
                                     PIN_AFIO_AF(GPIOC_OSC32_OUT, 0))

/*
 * GPIOD setup:
 *
 * PD0  - CAN_RX                    (alternate 7).
 * PD1  - CAN_TX                    (alternate 7).
 * PD2  - LCD_CS                    (output pushpull maximum).
 * PD3  - USART2_CTS                (alternate 7).
 * PD4  - USART2_RST                (alternate 7).
 * PD5  - USART2_TX                 (alternate 7).
 * PD6  - USART2_RX                 (alternate 7).
 * PD7  - PIN7                      (input pullup).
 * PD8  - PIN8                      (input pullup).
 * PD9  - PIN9                      (input pullup).
 * PD10 - PIN10                     (input pullup).
 * PD11 - AUDIO_RST                 (output pushpull maximum).
 * PD12 - PIN12                     (input pullup).
 * PD13 - PIN13                     (input pullup).
 * PD14 - PIN14                     (input pullup).
 * PD15 - PIN15                     (input pullup).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_ALTERNATE(GPIOD_CAN_RX) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_CAN_TX) |     \
                                     PIN_MODE_OUTPUT(GPIOD_LCD_CS) |        \
                                     PIN_MODE_ALTERNATE(GPIOD_USART2_CTS) | \
                                     PIN_MODE_ALTERNATE(GPIOD_USART2_RST) | \
                                     PIN_MODE_ALTERNATE(GPIOD_USART2_TX) |  \
                                     PIN_MODE_ALTERNATE(GPIOD_USART2_RX) |  \
                                     PIN_MODE_INPUT(GPIOD_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN10) |          \
                                     PIN_MODE_OUTPUT(GPIOD_AUDIO_RST) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN15))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOD_CAN_RX) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_CAN_TX) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_CS) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_USART2_CTS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOD_USART2_RST) | \
                                     PIN_OTYPE_PUSHPULL(GPIOD_USART2_TX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOD_USART2_RX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_AUDIO_RST) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN15))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_100M(GPIOD_CAN_RX) |        \
                                     PIN_OSPEED_100M(GPIOD_CAN_TX) |        \
                                     PIN_OSPEED_100M(GPIOD_LCD_CS) |        \
                                     PIN_OSPEED_100M(GPIOD_USART2_CTS) |    \
                                     PIN_OSPEED_100M(GPIOD_USART2_RST) |    \
                                     PIN_OSPEED_100M(GPIOD_USART2_TX) |     \
                                     PIN_OSPEED_100M(GPIOD_USART2_RX) |     \
                                     PIN_OSPEED_2M(GPIOD_PIN7) |            \
                                     PIN_OSPEED_2M(GPIOD_PIN8) |            \
                                     PIN_OSPEED_2M(GPIOD_PIN9) |            \
                                     PIN_OSPEED_2M(GPIOD_PIN10) |           \
                                     PIN_OSPEED_100M(GPIOD_AUDIO_RST) |     \
                                     PIN_OSPEED_2M(GPIOD_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOD_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOD_PIN14) |           \
                                     PIN_OSPEED_2M(GPIOD_PIN15))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_FLOATING(GPIOD_CAN_RX) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_CAN_TX) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_CS) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_USART2_CTS) | \
                                     PIN_PUPDR_FLOATING(GPIOD_USART2_RST) | \
                                     PIN_PUPDR_FLOATING(GPIOD_USART2_TX) |  \
                                     PIN_PUPDR_FLOATING(GPIOD_USART2_RX) |  \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOD_AUDIO_RST) |  \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN15))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(GPIOD_CAN_RX) |           \
                                     PIN_ODR_HIGH(GPIOD_CAN_TX) |           \
                                     PIN_ODR_HIGH(GPIOD_LCD_CS) |           \
                                     PIN_ODR_HIGH(GPIOD_USART2_CTS) |       \
                                     PIN_ODR_HIGH(GPIOD_USART2_RST) |       \
                                     PIN_ODR_HIGH(GPIOD_USART2_TX) |        \
                                     PIN_ODR_HIGH(GPIOD_USART2_RX) |        \
                                     PIN_ODR_HIGH(GPIOD_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN10) |            \
                                     PIN_ODR_LOW(GPIOD_AUDIO_RST) |         \
                                     PIN_ODR_HIGH(GPIOD_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN15))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_CAN_RX, 7) |         \
                                     PIN_AFIO_AF(GPIOD_CAN_TX, 7) |         \
                                     PIN_AFIO_AF(GPIOD_LCD_CS, 0) |         \
                                     PIN_AFIO_AF(GPIOD_USART2_CTS, 7) |     \
                                     PIN_AFIO_AF(GPIOD_USART2_RST, 7) |     \
                                     PIN_AFIO_AF(GPIOD_USART2_TX, 7) |      \
                                     PIN_AFIO_AF(GPIOD_USART2_RX, 7) |      \
                                     PIN_AFIO_AF(GPIOD_PIN7, 0))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(GPIOD_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOD_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOD_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOD_AUDIO_RST, 0) |      \
                                     PIN_AFIO_AF(GPIOD_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOD_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOD_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOD_PIN15, 0))

/*
 * GPIOE setup:
 *
 * PE0  - PIN0                      (input pullup).
 * PE1  - PIN1                      (input pullup).
 * PE2  - SD_CS                     (output opendrain maximum).
 * PE3  - SD_DETECT                 (input pullup).
 * PE4  - PIN4                      (input pullup).
 * PE5  - PIN5                      (input pullup).
 * PE6  - JOY_SEL                   (input pulldown).
 * PE7  - RTD_IN                    (analog).
 * PE8  - PRESSUREP                 (analog).
 * PE9  - PRESSUREN                 (analog).
 * PE10 - PIN10                     (input pullup).
 * PE11 - PIN11                     (input pullup).
 * PE12 - PIN12                     (input pullup).
 * PE13 - PIN13                     (input pullup).
 * PE14 - PRESSURE_TEPM             (input floating).
 * PE15 - PIN15                     (input pullup).
 */
#define VAL_GPIOE_MODER             (PIN_MODE_INPUT(GPIOE_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN1) |           \
                                     PIN_MODE_OUTPUT(GPIOE_SD_CS) |         \
                                     PIN_MODE_INPUT(GPIOE_SD_DETECT) |      \
                                     PIN_MODE_INPUT(GPIOE_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOE_JOY_SEL) |        \
                                     PIN_MODE_ANALOG(GPIOE_RTD_IN) |        \
                                     PIN_MODE_ANALOG(GPIOE_PRESSUREP) |     \
                                     PIN_MODE_ANALOG(GPIOE_PRESSUREN) |     \
                                     PIN_MODE_INPUT(GPIOE_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOE_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOE_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOE_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOE_PRESSURE_TEPM) |  \
                                     PIN_MODE_INPUT(GPIOE_PIN15))
#define VAL_GPIOE_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOE_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN1) |       \
                                     PIN_OTYPE_OPENDRAIN(GPIOE_SD_CS) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_SD_DETECT) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_JOY_SEL) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOE_RTD_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PRESSUREP) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PRESSUREN) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PRESSURE_TEPM) |\
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN15))
#define VAL_GPIOE_OSPEEDR           (PIN_OSPEED_2M(GPIOE_PIN0) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN1) |            \
                                     PIN_OSPEED_100M(GPIOE_SD_CS) |         \
                                     PIN_OSPEED_100M(GPIOE_SD_DETECT) |     \
                                     PIN_OSPEED_2M(GPIOE_PIN4) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN5) |            \
                                     PIN_OSPEED_100M(GPIOE_JOY_SEL) |       \
                                     PIN_OSPEED_2M(GPIOE_RTD_IN) |          \
                                     PIN_OSPEED_100M(GPIOE_PRESSUREP) |     \
                                     PIN_OSPEED_100M(GPIOE_PRESSUREN) |     \
                                     PIN_OSPEED_2M(GPIOE_PIN10) |           \
                                     PIN_OSPEED_2M(GPIOE_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOE_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOE_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOE_PRESSURE_TEPM) |   \
                                     PIN_OSPEED_2M(GPIOE_PIN15))
#define VAL_GPIOE_PUPDR             (PIN_PUPDR_PULLUP(GPIOE_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN1) |         \
                                     PIN_PUPDR_FLOATING(GPIOE_SD_CS) |      \
                                     PIN_PUPDR_PULLUP(GPIOE_SD_DETECT) |    \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN5) |         \
                                     PIN_PUPDR_PULLDOWN(GPIOE_JOY_SEL) |    \
                                     PIN_PUPDR_FLOATING(GPIOE_RTD_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_PRESSUREP) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_PRESSUREN) |  \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN13) |        \
                                     PIN_PUPDR_FLOATING(GPIOE_PRESSURE_TEPM) |\
                                     PIN_PUPDR_PULLUP(GPIOE_PIN15))
#define VAL_GPIOE_ODR               (PIN_ODR_HIGH(GPIOE_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOE_SD_CS) |            \
                                     PIN_ODR_HIGH(GPIOE_SD_DETECT) |        \
                                     PIN_ODR_HIGH(GPIOE_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOE_JOY_SEL) |          \
                                     PIN_ODR_HIGH(GPIOE_RTD_IN) |           \
                                     PIN_ODR_HIGH(GPIOE_PRESSUREP) |        \
                                     PIN_ODR_HIGH(GPIOE_PRESSUREN) |        \
                                     PIN_ODR_HIGH(GPIOE_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOE_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOE_PIN12) |            \
                                     PIN_ODR_LOW(GPIOE_PIN13) |             \
                                     PIN_ODR_LOW(GPIOE_PRESSURE_TEPM) |     \
                                     PIN_ODR_LOW(GPIOE_PIN15))
#define VAL_GPIOE_AFRL              (PIN_AFIO_AF(GPIOE_PIN0, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOE_SD_CS, 0) |          \
                                     PIN_AFIO_AF(GPIOE_SD_DETECT, 0) |      \
                                     PIN_AFIO_AF(GPIOE_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOE_JOY_SEL, 0) |        \
                                     PIN_AFIO_AF(GPIOE_RTD_IN, 0))
#define VAL_GPIOE_AFRH              (PIN_AFIO_AF(GPIOE_PRESSUREP, 0) |      \
                                     PIN_AFIO_AF(GPIOE_PRESSUREN, 0) |      \
                                     PIN_AFIO_AF(GPIOE_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOE_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOE_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOE_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOE_PRESSURE_TEPM, 0) |  \
                                     PIN_AFIO_AF(GPIOE_PIN15, 0))

/*
 * GPIOF setup:
 *
 * PF0  - OSC_IN                    (input floating).
 * PF1  - OSC_OUT                   (input floating).
 * PF2  - JOY_DOWN                  (input pulldown).
 * PF3  - PIN3                      (input pullup).
 * PF4  - JOY_LEFT                  (input pulldown).
 * PF5  - PIN5                      (input pullup).
 * PF6  - PIN6                      (input pullup).
 * PF7  - PIN7                      (input pullup).
 * PF8  - PIN8                      (input pullup).
 * PF9  - JOY_RIGHT                 (input pulldown).
 * PF10 - JOY_UP                    (input pulldown).
 * PF11 - PIN11                     (input pullup).
 * PF12 - PIN12                     (input pullup).
 * PF13 - PIN13                     (input pullup).
 * PF14 - PIN14                     (input pullup).
 * PF15 - PIN15                     (input pullup).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_INPUT(GPIOF_OSC_IN) |         \
                                     PIN_MODE_INPUT(GPIOF_OSC_OUT) |        \
                                     PIN_MODE_INPUT(GPIOF_JOY_DOWN) |       \
                                     PIN_MODE_INPUT(GPIOF_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOF_JOY_LEFT) |       \
                                     PIN_MODE_INPUT(GPIOF_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOF_JOY_RIGHT) |      \
                                     PIN_MODE_INPUT(GPIOF_JOY_UP) |         \
                                     PIN_MODE_INPUT(GPIOF_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN15))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOF_OSC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_OSC_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOF_JOY_DOWN) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_JOY_LEFT) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_JOY_RIGHT) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_JOY_UP) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN15))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_100M(GPIOF_OSC_IN) |        \
                                     PIN_OSPEED_100M(GPIOF_OSC_OUT) |       \
                                     PIN_OSPEED_100M(GPIOF_JOY_DOWN) |      \
                                     PIN_OSPEED_2M(GPIOF_PIN3) |            \
                                     PIN_OSPEED_100M(GPIOF_JOY_LEFT) |      \
                                     PIN_OSPEED_2M(GPIOF_PIN5) |            \
                                     PIN_OSPEED_2M(GPIOF_PIN6) |            \
                                     PIN_OSPEED_2M(GPIOF_PIN7) |            \
                                     PIN_OSPEED_2M(GPIOF_PIN8) |            \
                                     PIN_OSPEED_100M(GPIOF_JOY_RIGHT) |     \
                                     PIN_OSPEED_100M(GPIOF_JOY_UP) |        \
                                     PIN_OSPEED_2M(GPIOF_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOF_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOF_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOF_PIN14) |           \
                                     PIN_OSPEED_2M(GPIOF_PIN15))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_FLOATING(GPIOF_OSC_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_OSC_OUT) |    \
                                     PIN_PUPDR_PULLDOWN(GPIOF_JOY_DOWN) |   \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN3) |         \
                                     PIN_PUPDR_PULLDOWN(GPIOF_JOY_LEFT) |   \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN8) |         \
                                     PIN_PUPDR_PULLDOWN(GPIOF_JOY_RIGHT) |  \
                                     PIN_PUPDR_PULLDOWN(GPIOF_JOY_UP) |     \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN15))
#define VAL_GPIOF_ODR               (PIN_ODR_HIGH(GPIOF_OSC_IN) |           \
                                     PIN_ODR_HIGH(GPIOF_OSC_OUT) |          \
                                     PIN_ODR_HIGH(GPIOF_JOY_DOWN) |         \
                                     PIN_ODR_HIGH(GPIOF_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOF_JOY_LEFT) |         \
                                     PIN_ODR_HIGH(GPIOF_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOF_JOY_RIGHT) |        \
                                     PIN_ODR_HIGH(GPIOF_JOY_UP) |           \
                                     PIN_ODR_HIGH(GPIOF_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN15))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(GPIOF_OSC_IN, 0) |         \
                                     PIN_AFIO_AF(GPIOF_OSC_OUT, 0) |        \
                                     PIN_AFIO_AF(GPIOF_JOY_DOWN, 0) |       \
                                     PIN_AFIO_AF(GPIOF_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOF_JOY_LEFT, 0) |       \
                                     PIN_AFIO_AF(GPIOF_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOF_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOF_PIN7, 0))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(GPIOF_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOF_JOY_RIGHT, 0) |      \
                                     PIN_AFIO_AF(GPIOF_JOY_UP, 0) |         \
                                     PIN_AFIO_AF(GPIOF_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOF_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOF_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOF_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOF_PIN15, 0))


#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
