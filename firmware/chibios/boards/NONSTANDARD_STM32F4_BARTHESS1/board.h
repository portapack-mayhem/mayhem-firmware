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
 * Setup for STMicroelectronics STM32F4-Discovery board.
 */

/*
 * Board identifier.
 */
#define BOARD_NONSTANDARD_STM32F4_BARTHESS1
#define BOARD_NAME              "Hand made STM32F4x board"

/*
 * Board frequencies.
 * NOTE: The LSE crystal is not fitted by default on the board.
 */
#define STM32_LSECLK            32768
#define STM32_HSECLK            8000000

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD               300

/*
 * MCU type as defined in the ST header.
 */
#define STM32F40_41xxx

/*
 * IO pins assignments.
 */
#define GPIOA_USART2_CTS        0 /* xbee */
#define GPIOA_USART2_RTS        1 /* xbee */
#define GPIOA_USART2_TX         2 /* xbee */
#define GPIOA_USART2_RX         3 /* xbee */
#define GPIOA_SPI1_NSS          4
#define GPIOA_SPI1_SCK          5
#define GPIOA_SPI1_MISO         6
#define GPIOA_SPI1_MOSI         7
#define GPIOA_5V_DOMAIN_EN      8
#define GPIOA_USART1_TX         9 /* gps */
#define GPIOA_USART1_RX         10/* gps */
#define GPIOA_OTG_FS_DM         11
#define GPIOA_OTG_FS_DP         12
#define GPIOA_JTMS              13
#define GPIOA_JTCK              14
#define GPIOA_JTDI              15

#define GPIOB_RECEIVER_PPM      0
#define GPIOB_TACHOMETER        1
#define GPIOB_BOOT1             2
#define GPIOB_JTDO              3
#define GPIOB_NJTRST            4
#define GPIOB_LED_R             6
#define GPIOB_LED_G             7
#define GPIOB_LED_B             8
#define GPIOB_I2C2_SCL          10
#define GPIOB_I2C2_SDA          11


#define GPIOC_AN_CURRENT_SENS   0
#define GPIOC_AN_SUPPLY_SENS    1
#define GPIOC_AN_6V_SENS        2
#define GPIOC_AN33_0            3
#define GPIOC_AN33_1            4
#define GPIOC_AN33_2            5
#define GPIOC_SDIO_D0           8
#define GPIOC_SDIO_D1           9
#define GPIOC_SDIO_D2           10
#define GPIOC_SDIO_D3           11
#define GPIOC_SDIO_CK           12
#define GPIOC_TAMPER_RTC        13
#define GPIOC_OSC32_IN          14
#define GPIOC_OSC32_OUT         15

#define GPIOD_SDIO_CMD          2
#define GPIOD_PWM1              12
#define GPIOD_PWM2              13
#define GPIOD_PWM3              14
#define GPIOD_PWM4              15

#define GPIOE_GPS_PPS           0
#define GPIOE_XBEE_SLEEP        1
#define GPIOE_XBEE_RESET        2
#define GPIOE_SDIO_DETECT       3
#define GPIOE_USB_DISCOVERY     4
#define GPIOE_GPS_PWR_EN        5
#define GPIOE_BMP085_EOC        6
#define GPIOE_MAG_INT           7
#define GPIOE_MMA8451_INT1      8
#define GPIOE_PWM5              9
#define GPIOE_ITG3200_INT       10
#define GPIOE_PWM6              11
#define GPIOE_PWM7              13
#define GPIOE_PWM8              14
#define GPIOE_MMA8451_INT2      15

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 *
 * 1 for open drain outputs denotes hi-Z state
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_2M(n)            (0U << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1U << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2U << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3U << ((n) * 2))
#define PIN_PUDR_FLOATING(n)        (0U << ((n) * 2))
#define PIN_PUDR_PULLUP(n)          (1U << ((n) * 2))
#define PIN_PUDR_PULLDOWN(n)        (2U << ((n) * 2))
#define PIN_AFIO_AF(n, v)           ((v##U) << ((n % 8) * 4))

/*
 * Port A setup.
 * All input with pull-up except:
#define GPIOA_USART2_CTS        0 //xbee
#define GPIOA_USART2_RTS        1 //xbee
#define GPIOA_USART2_TX         2 //xbee
#define GPIOA_USART2_Rx         3 //xbee
#define GPIOA_SPI1_NSS          4
#define GPIOA_SPI1_SCK          5
#define GPIOA_SPI1_MISO         6
#define GPIOA_SPI1_MOSI         7
#define GPIOA_5V_DOMAIN_EN      8
#define GPIOA_USART1_TX         9
#define GPIOA_USART1_RX         10
#define GPIOA_OTG_FS_DM         11
#define GPIOA_OTG_FS_DP         12
#define GPIOA_JTMS              13
#define GPIOA_JTCK              14
#define GPIOA_JTDI              15
 */

/* default after reset 0xA8000000 */
#define VAL_GPIOA_MODER        (PIN_MODE_ALTERNATE(GPIOA_USART2_CTS) |        \
                                PIN_MODE_ALTERNATE(GPIOA_USART2_RTS) |        \
                                PIN_MODE_ALTERNATE(GPIOA_USART2_TX) |         \
                                PIN_MODE_ALTERNATE(GPIOA_USART2_RX) |         \
                                PIN_MODE_ALTERNATE(GPIOA_SPI1_NSS) |          \
                                PIN_MODE_ALTERNATE(GPIOA_SPI1_SCK) |          \
                                PIN_MODE_ALTERNATE(GPIOA_SPI1_MISO) |         \
                                PIN_MODE_ALTERNATE(GPIOA_SPI1_MOSI) |         \
                                PIN_MODE_OUTPUT(GPIOA_5V_DOMAIN_EN) |         \
                                PIN_MODE_ALTERNATE(GPIOA_USART1_TX) |         \
                                PIN_MODE_ALTERNATE(GPIOA_USART1_RX) |         \
                                PIN_MODE_ALTERNATE(GPIOA_OTG_FS_DM) |         \
                                PIN_MODE_ALTERNATE(GPIOA_OTG_FS_DP) |         \
                                PIN_MODE_ALTERNATE(GPIOA_JTMS) |              \
                                PIN_MODE_ALTERNATE(GPIOA_JTCK) |              \
                                PIN_MODE_ALTERNATE(GPIOA_JTDI))
/* default 0x00000000 */
#define VAL_GPIOA_OTYPER        0x00000000
/* default 0x00000000 */
#define VAL_GPIOA_OSPEEDR       0x00000000
/* 0x64000000 */
#define VAL_GPIOA_PUPDR        (PIN_PUDR_FLOATING(GPIOA_USART2_CTS) |         \
                                PIN_PUDR_FLOATING(GPIOA_USART2_RTS) |         \
                                PIN_PUDR_FLOATING(GPIOA_USART2_TX) |          \
                                PIN_PUDR_FLOATING(GPIOA_USART2_RX) |          \
                                PIN_PUDR_PULLUP(GPIOA_SPI1_NSS) |             \
                                PIN_PUDR_PULLUP(GPIOA_SPI1_SCK) |             \
                                PIN_PUDR_PULLUP(GPIOA_SPI1_MISO) |            \
                                PIN_PUDR_PULLUP(GPIOA_SPI1_MOSI) |            \
                                PIN_PUDR_FLOATING(GPIOA_5V_DOMAIN_EN) |       \
                                PIN_PUDR_FLOATING(GPIOA_USART1_TX) |          \
                                PIN_PUDR_FLOATING(GPIOA_USART1_RX) |          \
                                PIN_PUDR_FLOATING(GPIOA_OTG_FS_DM) |          \
                                PIN_PUDR_FLOATING(GPIOA_OTG_FS_DP) |          \
                                PIN_PUDR_PULLUP(GPIOA_JTMS) |                 \
                                PIN_PUDR_PULLDOWN(GPIOA_JTCK) |               \
                                PIN_PUDR_PULLUP(GPIOA_JTDI))
/* 0x00000000 */
#define VAL_GPIOA_ODR           0x00000000
/* 0x00000000 */
#define VAL_GPIOA_AFRL         (PIN_AFIO_AF(GPIOA_USART2_CTS, 7) |            \
                                PIN_AFIO_AF(GPIOA_USART2_RTS, 7) |            \
                                PIN_AFIO_AF(GPIOA_USART2_TX, 7) |             \
                                PIN_AFIO_AF(GPIOA_USART2_RX, 7)|              \
                                PIN_AFIO_AF(GPIOA_SPI1_NSS, 5) |              \
                                PIN_AFIO_AF(GPIOA_SPI1_SCK, 5) |              \
                                PIN_AFIO_AF(GPIOA_SPI1_MISO, 5) |             \
                                PIN_AFIO_AF(GPIOA_SPI1_MOSI, 5))
/* 0x00000000 */
#define VAL_GPIOA_AFRH         (PIN_AFIO_AF(GPIOA_USART1_TX, 7) |             \
                                PIN_AFIO_AF(GPIOA_USART1_RX, 7) |             \
                                PIN_AFIO_AF(GPIOA_OTG_FS_DM, 10) |            \
                                PIN_AFIO_AF(GPIOA_OTG_FS_DP, 10) |            \
                                PIN_AFIO_AF(GPIOA_JTMS, 0) |                  \
                                PIN_AFIO_AF(GPIOA_JTCK, 0) |                  \
                                PIN_AFIO_AF(GPIOA_JTDI, 0))

/*
 * Port B setup.
#define GPIOB_RECEIVER_PPM      0
#define GPIOB_TACHOMETER        1
#define GPIOB_BOOT1             2
#define GPIOB_JTDO              3
#define GPIOB_NJTRST            4
#define GPIOB_LED_R             6
#define GPIOB_LED_G             7
#define GPIOB_LED_B             8
#define GPIOB_I2C2_SCL          10
#define GPIOB_I2C2_SDA          11
 */
/* 0x00000280 */
#define VAL_GPIOB_MODER        (PIN_MODE_INPUT(GPIOB_RECEIVER_PPM) |          \
                                PIN_MODE_INPUT(GPIOB_TACHOMETER) |            \
                                PIN_MODE_INPUT(GPIOB_BOOT1) |                 \
                                PIN_MODE_ALTERNATE(GPIOB_JTDO) |              \
                                PIN_MODE_ALTERNATE(GPIOB_NJTRST) |            \
                                PIN_MODE_INPUT(5) |                           \
                                PIN_MODE_OUTPUT(GPIOB_LED_R) |                \
                                PIN_MODE_OUTPUT(GPIOB_LED_G) |                \
                                PIN_MODE_OUTPUT(GPIOB_LED_B) |                \
                                PIN_MODE_INPUT(9) |                           \
                                PIN_MODE_ALTERNATE(GPIOB_I2C2_SCL) |          \
                                PIN_MODE_ALTERNATE(GPIOB_I2C2_SDA) |          \
                                PIN_MODE_INPUT(12) |                          \
                                PIN_MODE_INPUT(13) |                          \
                                PIN_MODE_INPUT(14) |                          \
                                PIN_MODE_INPUT(15))
/* 0x00000000 */
#define VAL_GPIOB_OTYPER       (PIN_OTYPE_OPENDRAIN(GPIOB_LED_R) |            \
                                PIN_OTYPE_OPENDRAIN(GPIOB_LED_G) |            \
                                PIN_OTYPE_OPENDRAIN(GPIOB_LED_B) |            \
                                PIN_OTYPE_OPENDRAIN(GPIOB_I2C2_SCL) |         \
                                PIN_OTYPE_OPENDRAIN(GPIOB_I2C2_SDA))
/* 0x000000C0 */
#define VAL_GPIOB_OSPEEDR       0x000000C0//0xAAAAAAEA
/* 0x00000100 */
#define VAL_GPIOB_PUPDR        (PIN_PUDR_PULLDOWN(GPIOB_RECEIVER_PPM) |       \
                                PIN_PUDR_PULLDOWN(GPIOB_TACHOMETER) |         \
                                PIN_PUDR_FLOATING(GPIOB_BOOT1) |              \
                                PIN_PUDR_FLOATING(GPIOB_JTDO) |               \
                                PIN_PUDR_PULLUP(GPIOB_NJTRST) |               \
                                PIN_PUDR_FLOATING(5) |                        \
                                PIN_PUDR_FLOATING(GPIOB_LED_R) |              \
                                PIN_PUDR_FLOATING(GPIOB_LED_G) |              \
                                PIN_PUDR_FLOATING(GPIOB_LED_B) |              \
                                PIN_PUDR_FLOATING(9) |                        \
                                PIN_PUDR_FLOATING(GPIOB_I2C2_SCL) |           \
                                PIN_PUDR_FLOATING(GPIOB_I2C2_SDA) |           \
                                PIN_PUDR_PULLUP(12) |                         \
                                PIN_PUDR_PULLUP(13) |                         \
                                PIN_PUDR_PULLUP(14) |                         \
                                PIN_PUDR_PULLUP(15))
/* 0x00000000 */
#define VAL_GPIOB_ODR           0x000001C0
/* 0x00000000 */
#define VAL_GPIOB_AFRL         (PIN_AFIO_AF(GPIOB_RECEIVER_PPM, 0) |          \
                                PIN_AFIO_AF(GPIOB_JTDO, 0))
/* 0x00000000 */
#define VAL_GPIOB_AFRH         (PIN_AFIO_AF(GPIOB_I2C2_SCL, 4) |              \
                                PIN_AFIO_AF(GPIOB_I2C2_SDA, 4))


/*
 * Port C setup.
#define GPIOC_AN_CURRENT_SENS   0
#define GPIOC_AN_SUPPLY_SENS    1
#define GPIOC_AN_6V_SENS        2
#define GPIOC_AN33_0            3
#define GPIOC_AN33_1            4
#define GPIOC_AN33_2            5

#define GPIOC_SDIO_D0           8
#define GPIOC_SDIO_D1           9
#define GPIOC_SDIO_D2           10
#define GPIOC_SDIO_D3           11
#define GPIOC_SDIO_CK           12

#define GPIOC_TAMPER_RTC        13
#define GPIOC_OSC32_IN          14
#define GPIOC_OSC32_OUT         15
 */
/* 0x00000000 */
#define VAL_GPIOC_MODER        (PIN_MODE_ANALOG(GPIOC_AN_CURRENT_SENS) |      \
                                PIN_MODE_ANALOG(GPIOC_AN_SUPPLY_SENS) |       \
                                PIN_MODE_ANALOG(GPIOC_AN_6V_SENS) |           \
                                PIN_MODE_ANALOG(GPIOC_AN33_0) |               \
                                PIN_MODE_ANALOG(GPIOC_AN33_1) |               \
                                PIN_MODE_ANALOG(GPIOC_AN33_2) |               \
                                PIN_MODE_INPUT(6) |                           \
                                PIN_MODE_INPUT(7) |                           \
                                PIN_MODE_ALTERNATE(GPIOC_SDIO_D0) |           \
                                PIN_MODE_ALTERNATE(GPIOC_SDIO_D1) |           \
                                PIN_MODE_ALTERNATE(GPIOC_SDIO_D2) |           \
                                PIN_MODE_ALTERNATE(GPIOC_SDIO_D3) |           \
                                PIN_MODE_ALTERNATE(GPIOC_SDIO_CK) |           \
                                PIN_MODE_INPUT(GPIOC_TAMPER_RTC) |            \
                                PIN_MODE_INPUT(GPIOC_OSC32_IN) |              \
                                PIN_MODE_INPUT(GPIOC_OSC32_OUT))

/* 0x00000000 */
#define VAL_GPIOC_OTYPER        0x00000000
/* 0x00000000 */
#define VAL_GPIOC_OSPEEDR      (PIN_OSPEED_25M(GPIOC_SDIO_D0) |               \
                                PIN_OSPEED_25M(GPIOC_SDIO_D1) |               \
                                PIN_OSPEED_25M(GPIOC_SDIO_D2) |               \
                                PIN_OSPEED_25M(GPIOC_SDIO_D3) |               \
                                PIN_OSPEED_25M(GPIOC_SDIO_CK) |               \
                                PIN_OSPEED_2M(GPIOC_TAMPER_RTC))
/* 0x00000000 */
#define VAL_GPIOC_PUPDR        (PIN_PUDR_FLOATING(GPIOC_AN_CURRENT_SENS) |    \
                                PIN_PUDR_FLOATING(GPIOC_AN_SUPPLY_SENS) |     \
                                PIN_PUDR_FLOATING(GPIOC_AN_6V_SENS) |         \
                                PIN_PUDR_FLOATING(GPIOC_AN33_0) |             \
                                PIN_PUDR_FLOATING(GPIOC_AN33_1) |             \
                                PIN_PUDR_FLOATING(GPIOC_AN33_2) |             \
                                PIN_PUDR_PULLUP(6) |                          \
                                PIN_PUDR_PULLUP(7) |                          \
                                PIN_PUDR_PULLUP(GPIOC_SDIO_D0) |              \
                                PIN_PUDR_PULLUP(GPIOC_SDIO_D1) |              \
                                PIN_PUDR_PULLUP(GPIOC_SDIO_D2) |              \
                                PIN_PUDR_PULLUP(GPIOC_SDIO_D3) |              \
                                PIN_PUDR_PULLUP(GPIOC_SDIO_CK) |              \
                                PIN_PUDR_FLOATING(GPIOC_TAMPER_RTC) |         \
                                PIN_PUDR_FLOATING(GPIOC_OSC32_IN) |           \
                                PIN_PUDR_FLOATING(GPIOC_OSC32_OUT))
/* 0x00000000 */
#define VAL_GPIOC_ODR           0x00000000
/* 0x00000000 */
#define VAL_GPIOC_AFRL          0x00000000
/* 0x00000000 */
#define VAL_GPIOC_AFRH         (PIN_AFIO_AF(GPIOC_SDIO_D0, 12) |              \
                                PIN_AFIO_AF(GPIOC_SDIO_D1, 12) |              \
                                PIN_AFIO_AF(GPIOC_SDIO_D2, 12) |              \
                                PIN_AFIO_AF(GPIOC_SDIO_D3, 12) |              \
                                PIN_AFIO_AF(GPIOC_SDIO_CK, 12))

/*
 * Port D setup.
#define GPIOD_SDIO_CMD          2
#define GPIOD_PWM1              12
#define GPIOD_PWM2              13
#define GPIOD_PWM3              14
#define GPIOD_PWM4              15
 */
/* 0x00000000 */
#define VAL_GPIOD_MODER        (PIN_MODE_ALTERNATE(GPIOD_SDIO_CMD) |          \
                                PIN_MODE_ALTERNATE(GPIOD_PWM1) |              \
                                PIN_MODE_ALTERNATE(GPIOD_PWM2) |              \
                                PIN_MODE_ALTERNATE(GPIOD_PWM3) |              \
                                PIN_MODE_ALTERNATE(GPIOD_PWM4))
/* 0x00000000 */
#define VAL_GPIOD_OTYPER        0x00000000
/* 0x00000000 */
#define VAL_GPIOD_OSPEEDR      (PIN_OSPEED_25M(GPIOD_SDIO_CMD))
/* 0x00000000 */
#define VAL_GPIOD_PUPDR        (PIN_PUDR_PULLUP(GPIOD_SDIO_CMD) |             \
                                PIN_PUDR_PULLDOWN(GPIOD_PWM1) |               \
                                PIN_PUDR_PULLDOWN(GPIOD_PWM2) |               \
                                PIN_PUDR_PULLDOWN(GPIOD_PWM3) |               \
                                PIN_PUDR_PULLDOWN(GPIOD_PWM4))
/* 0x00000000 */
#define VAL_GPIOD_ODR           0x00000000
/* 0x00000000 */
#define VAL_GPIOD_AFRL         (PIN_AFIO_AF(GPIOD_SDIO_CMD, 12))
/* 0x00000000 */
#define VAL_GPIOD_AFRH         (PIN_AFIO_AF(GPIOD_PWM1, 2) |                  \
                                PIN_AFIO_AF(GPIOD_PWM2, 2) |                  \
                                PIN_AFIO_AF(GPIOD_PWM3, 2) |                  \
                                PIN_AFIO_AF(GPIOD_PWM4, 2))


/*
 * Port E setup.
#define GPIOE_GPS_PPS           0
#define GPIOE_XBEE_SLEEP        1
#define GPIOE_XBEE_RESET        2
#define GPIOE_SDIO_DETECT       3
#define GPIOE_USB_DISCOVERY     4
#define GPIOE_GPS_EN            5
#define GPIOE_BMP085_EOC        6
#define GPIOE_MAG_INT           7
#define GPIOE_MMA8451_INT1      8
#define GPIOE_PWM5              9
#define GPIOE_ITG3200_INT       10
#define GPIOE_PWM6              11
#define GPIOE_TACHOMETER        12
#define GPIOE_PWM7              13
#define GPIOE_PWM8              14
#define GPIOE_MMA8451_INT2      15
 */
/* 0x00000000 */
#define VAL_GPIOE_MODER        (PIN_MODE_INPUT(GPIOE_GPS_PPS) |               \
                                PIN_MODE_OUTPUT(GPIOE_XBEE_SLEEP) |           \
                                PIN_MODE_OUTPUT(GPIOE_XBEE_RESET) |           \
                                PIN_MODE_INPUT(GPIOE_SDIO_DETECT) |           \
                                PIN_MODE_OUTPUT(GPIOE_USB_DISCOVERY) |        \
                                PIN_MODE_OUTPUT(GPIOE_GPS_PWR_EN) |           \
                                PIN_MODE_INPUT(GPIOE_BMP085_EOC) |            \
                                PIN_MODE_INPUT(GPIOE_MAG_INT) |               \
                                PIN_MODE_INPUT(GPIOE_MMA8451_INT1) |          \
                                PIN_MODE_ALTERNATE(GPIOE_PWM5) |              \
                                PIN_MODE_INPUT(GPIOE_ITG3200_INT) |           \
                                PIN_MODE_ALTERNATE(GPIOE_PWM6) |              \
                                PIN_MODE_INPUT(12) |                          \
                                PIN_MODE_ALTERNATE(GPIOE_PWM7) |              \
                                PIN_MODE_ALTERNATE(GPIOE_PWM8) |              \
                                PIN_MODE_INPUT(GPIOE_MMA8451_INT2))
/* 0x00000000 */
#define VAL_GPIOE_OTYPER       (PIN_OTYPE_PUSHPULL(GPIOE_XBEE_SLEEP) |        \
                                PIN_OTYPE_PUSHPULL(GPIOE_XBEE_RESET) |        \
                                PIN_OTYPE_PUSHPULL(GPIOE_USB_DISCOVERY) |     \
                                PIN_OTYPE_OPENDRAIN(GPIOE_GPS_PWR_EN) |       \
                                PIN_OTYPE_PUSHPULL(GPIOE_PWM5) |              \
                                PIN_OTYPE_PUSHPULL(GPIOE_PWM6) |              \
                                PIN_OTYPE_PUSHPULL(GPIOE_PWM7) |              \
                                PIN_OTYPE_PUSHPULL(GPIOE_PWM8))
/* 0x00000000 */
#define VAL_GPIOE_OSPEEDR       0x00000000
/* 0x00000000 */
#define VAL_GPIOE_PUPDR        (PIN_PUDR_PULLDOWN(GPIOE_GPS_PPS) |            \
                                PIN_PUDR_PULLUP(GPIOE_XBEE_SLEEP) |           \
                                PIN_PUDR_FLOATING(GPIOE_XBEE_RESET) |         \
                                PIN_PUDR_PULLUP(GPIOE_SDIO_DETECT) |          \
                                PIN_PUDR_FLOATING(GPIOE_USB_DISCOVERY) |      \
                                PIN_PUDR_FLOATING(GPIOE_GPS_PWR_EN) |         \
                                PIN_PUDR_PULLDOWN(GPIOE_BMP085_EOC) |         \
                                PIN_PUDR_PULLDOWN(GPIOE_MAG_INT) |            \
                                PIN_PUDR_PULLDOWN(GPIOE_MMA8451_INT1) |       \
                                PIN_PUDR_PULLDOWN(GPIOE_PWM5) |               \
                                PIN_PUDR_PULLDOWN(GPIOE_ITG3200_INT) |        \
                                PIN_PUDR_PULLDOWN(GPIOE_PWM6) |               \
                                PIN_PUDR_PULLUP(12) |                         \
                                PIN_PUDR_PULLDOWN(GPIOE_PWM7) |               \
                                PIN_PUDR_PULLDOWN(GPIOE_PWM8) |               \
                                PIN_PUDR_PULLDOWN(GPIOE_MMA8451_INT2))
/* 0x00000000 */
#define VAL_GPIOE_ODR           0x30
/* 0x00000000 */
#define VAL_GPIOE_AFRL          0x00000000
/* 0x00000000 */
#define VAL_GPIOE_AFRH         (PIN_AFIO_AF(GPIOE_PWM5, 1) |                  \
                                PIN_AFIO_AF(GPIOE_PWM6, 1) |                  \
                                PIN_AFIO_AF(GPIOE_PWM7, 1) |                  \
                                PIN_AFIO_AF(GPIOE_PWM8, 1))

/*
 * Port F setup.
 */
#define VAL_GPIOF_MODER             0x00000000
#define VAL_GPIOF_OTYPER            0x00000000
#define VAL_GPIOF_OSPEEDR           0x00000000
#define VAL_GPIOF_PUPDR             0x00000000
#define VAL_GPIOF_ODR               0x00000000
#define VAL_GPIOF_AFRL              0x00000000
#define VAL_GPIOF_AFRH              0x00000000

/*
 * Port G setup.
 */
#define VAL_GPIOG_MODER             0x00000000
#define VAL_GPIOG_OTYPER            0x00000000
#define VAL_GPIOG_OSPEEDR           0x00000000
#define VAL_GPIOG_PUPDR             0x00000000
#define VAL_GPIOG_ODR               0x00000000
#define VAL_GPIOG_AFRL              0x00000000
#define VAL_GPIOG_AFRH              0x00000000

/*
 * Port H setup.
 */
#define VAL_GPIOH_MODER             0x00000000
#define VAL_GPIOH_OTYPER            0x00000000
#define VAL_GPIOH_OSPEEDR           0x00000000
#define VAL_GPIOH_PUPDR             0x00000000
#define VAL_GPIOH_ODR               0x00000000
#define VAL_GPIOH_AFRL              0x00000000
#define VAL_GPIOH_AFRH              0x00000000

/*
 * Port I setup.
 */
#define VAL_GPIOI_MODER             0x00000000
#define VAL_GPIOI_OTYPER            0x00000000
#define VAL_GPIOI_OSPEEDR           0x00000000
#define VAL_GPIOI_PUPDR             0x00000000
#define VAL_GPIOI_ODR               0x00000000
#define VAL_GPIOI_AFRL              0x00000000
#define VAL_GPIOI_AFRH              0x00000000

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
