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
 * Setup for STMicroelectronics STM32F429I-Discovery board.
 */

/*
 * Board identifier.
 */
#define BOARD_ST_STM32F429I_DISCOVERY
#define BOARD_NAME                  "STMicroelectronics STM32F429I-Discovery"


/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0
#endif

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                8000000
#endif

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   300

/*
 * MCU type as defined in the ST header.
 */
#define STM32F429_439xx

/*
 * IO pins assignments.
 */
#define GPIOA_BUTTON                0
#define GPIOA_MEMS_INT1             1
#define GPIOA_MEMS_INT2             2
#define GPIOA_LCD_B5                3
#define GPIOA_LCD_VSYNC             4
#define GPIOA_PIN5                  5
#define GPIOA_LCD_G2                6
#define GPIOA_ACP_RST               7
#define GPIOA_I2C3_SCL              8
#define GPIOA_PIN9                  9
#define GPIOA_PIN10                 10
#define GPIOA_LCD_R4                11
#define GPIOA_LCD_R5                12
#define GPIOA_SWDIO                 13
#define GPIOA_SWCLK                 14
#define GPIOA_TP_INT                15

#define GPIOB_LCD_R3                0
#define GPIOB_LCD_R6                1
#define GPIOB_BOOT1                 2
#define GPIOB_SWO                   3
#define GPIOB_PIN4                  4
#define GPIOB_FMC_SDCKE1            5
#define GPIOB_FMC_SDNE1             6
#define GPIOB_PIN7                  7
#define GPIOB_LCD_B6                8
#define GPIOB_LCD_B7                9
#define GPIOB_LCD_G4                10
#define GPIOB_LCD_G5                11
#define GPIOB_OTG_HS_ID             12
#define GPIOB_OTG_HS_VBUS           13
#define GPIOB_OTG_HS_DM             14
#define GPIOB_OTG_HS_DP             15

#define GPIOC_FMC_SDNWE             0
#define GPIOC_SPI5_MEMS_CS          1
#define GPIOC_SPI5_LCD_CS           2
#define GPIOC_PIN3                  3
#define GPIOC_OTG_HS_PSO            4
#define GPIOC_OTG_HS_OC             5
#define GPIOC_LCD_HSYNC             6
#define GPIOC_LCD_G6                7
#define GPIOC_PIN8                  8
#define GPIOC_I2C3_SDA              9
#define GPIOC_LCD_R2                10
#define GPIOC_PIN11                 11
#define GPIOC_PIN12                 12
#define GPIOC_PIN13                 13
#define GPIOC_OSC32_IN              14
#define GPIOC_OSC32_OUT             15

#define GPIOD_FMC_D2                0
#define GPIOD_FMC_D3                1
#define GPIOD_PIN2                  2
#define GPIOD_LCD_G7                3
#define GPIOD_PIN4                  4
#define GPIOD_PIN5                  5
#define GPIOD_LCD_B2                6
#define GPIOD_PIN7                  7
#define GPIOD_FMC_D13               8
#define GPIOD_FMC_D14               9
#define GPIOD_FMC_D15               10
#define GPIOD_LCD_TE                11
#define GPIOD_LCD_RDX               12
#define GPIOD_LCD_WRX               13
#define GPIOD_FMC_D0                14
#define GPIOD_FMC_D1                15

#define GPIOE_FMC_NBL0              0
#define GPIOE_FMC_NBL1              1
#define GPIOE_PIN2                  2
#define GPIOE_PIN3                  3
#define GPIOE_PIN4                  4
#define GPIOE_PIN5                  5
#define GPIOE_PIN6                  6
#define GPIOE_FMC_D4                7
#define GPIOE_FMC_D5                8
#define GPIOE_FMC_D6                9
#define GPIOE_FMC_D7                10
#define GPIOE_FMC_D8                11
#define GPIOE_FMC_D9                12
#define GPIOE_FMC_D10               13
#define GPIOE_FMC_D11               14
#define GPIOE_FMC_D12               15

#define GPIOF_FMC_A0                0
#define GPIOF_FMC_A1                1
#define GPIOF_FMC_A2                2
#define GPIOF_FMC_A3                3
#define GPIOF_FMC_A4                4
#define GPIOF_FMC_A5                5
#define GPIOF_PIN6                  6
#define GPIOF_LCD_DCX               7
#define GPIOF_SPI5_MISO             8
#define GPIOF_SPI5_MOSI             9
#define GPIOF_LCD_DE                10
#define GPIOF_FMC_SDNRAS            11
#define GPIOF_FMC_A6                12
#define GPIOF_FMC_A7                13
#define GPIOF_FMC_A8                14
#define GPIOF_FMC_A9                15

#define GPIOG_FMC_A10               0
#define GPIOG_FMC_A11               1
#define GPIOG_PIN2                  2
#define GPIOG_PIN3                  3
#define GPIOG_FMC_BA0               4
#define GPIOG_FMC_BA1               5
#define GPIOG_LCD_R7                6
#define GPIOG_LCD_CLK               7
#define GPIOG_FMC_SDCLK             8
#define GPIOG_PIN9                  9
#define GPIOG_LCD_G3                10
#define GPIOG_LCD_B3                11
#define GPIOG_LCD_B4                12
#define GPIOG_LED3_GREEN            13
#define GPIOG_LED4_RED              14
#define GPIOG_FMC_SDNCAS            15

#define GPIOH_OSC_IN                0
#define GPIOH_OSC_OUT               1
#define GPIOH_PIN2                  2
#define GPIOH_PIN3                  3
#define GPIOH_PIN4                  4
#define GPIOH_PIN5                  5
#define GPIOH_PIN6                  6
#define GPIOH_PIN7                  7
#define GPIOH_PIN8                  8
#define GPIOH_PIN9                  9
#define GPIOH_PIN10                 10
#define GPIOH_PIN11                 11
#define GPIOH_PIN12                 12
#define GPIOH_PIN13                 13
#define GPIOH_PIN14                 14
#define GPIOH_PIN15                 15

#define GPIOI_PIN0                  0
#define GPIOI_PIN1                  1
#define GPIOI_PIN2                  2
#define GPIOI_PIN3                  3
#define GPIOI_PIN4                  4
#define GPIOI_PIN5                  5
#define GPIOI_PIN6                  6
#define GPIOI_PIN7                  7
#define GPIOI_PIN8                  8
#define GPIOI_PIN9                  9
#define GPIOI_PIN10                 10
#define GPIOI_PIN11                 11
#define GPIOI_PIN12                 12
#define GPIOI_PIN13                 13
#define GPIOI_PIN14                 14
#define GPIOI_PIN15                 15

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
 * PA0  - BUTTON                    (input floating).
 * PA1  - MEMS_INT1                 (input floating).
 * PA2  - MEMS_INT2                 (input floating).
 * PA3  - LCD_B5                    (alternate 14).
 * PA4  - LCD_VSYNC                 (alternate 14).
 * PA5  - PIN5                      (input pullup).
 * PA6  - LCD_G2                    (alternate 14).
 * PA7  - ACP_RST                   (input pullup).
 * PA8  - I2C3_SCL                  (alternate 4).
 * PA9  - PIN9                      (input pullup).
 * PA10 - PIN10                     (input pullup).
 * PA11 - LCD_R4                    (alternate 14).
 * PA12 - LCD_R5                    (alternate 14).
 * PA13 - SWDIO                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - TP_INT                    (input floating).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(GPIOA_BUTTON) |         \
                                     PIN_MODE_INPUT(GPIOA_MEMS_INT1) |      \
                                     PIN_MODE_INPUT(GPIOA_MEMS_INT2) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_LCD_B5) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_LCD_VSYNC) |  \
                                     PIN_MODE_INPUT(GPIOA_PIN5) |           \
                                     PIN_MODE_ALTERNATE(GPIOA_LCD_G2) |     \
                                     PIN_MODE_INPUT(GPIOA_ACP_RST) |        \
                                     PIN_MODE_ALTERNATE(GPIOA_I2C3_SCL) |   \
                                     PIN_MODE_INPUT(GPIOA_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOA_LCD_R4) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_LCD_R5) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_INPUT(GPIOA_TP_INT))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_BUTTON) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_MEMS_INT1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_MEMS_INT2) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LCD_B5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LCD_VSYNC) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LCD_G2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACP_RST) |    \
                                     PIN_OTYPE_OPENDRAIN(GPIOA_I2C3_SCL) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LCD_R4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LCD_R5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_TP_INT))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_2M(GPIOA_BUTTON) |          \
                                     PIN_OSPEED_2M(GPIOA_MEMS_INT1) |       \
                                     PIN_OSPEED_2M(GPIOA_MEMS_INT2) |       \
                                     PIN_OSPEED_100M(GPIOA_LCD_B5) |        \
                                     PIN_OSPEED_100M(GPIOA_LCD_VSYNC) |     \
                                     PIN_OSPEED_2M(GPIOA_PIN5) |            \
                                     PIN_OSPEED_100M(GPIOA_LCD_G2) |        \
                                     PIN_OSPEED_2M(GPIOA_ACP_RST) |         \
                                     PIN_OSPEED_100M(GPIOA_I2C3_SCL) |      \
                                     PIN_OSPEED_2M(GPIOA_PIN9) |            \
                                     PIN_OSPEED_2M(GPIOA_PIN10) |           \
                                     PIN_OSPEED_100M(GPIOA_LCD_R4) |        \
                                     PIN_OSPEED_100M(GPIOA_LCD_R5) |        \
                                     PIN_OSPEED_100M(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_100M(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_2M(GPIOA_TP_INT))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_FLOATING(GPIOA_BUTTON) |     \
                                     PIN_PUPDR_FLOATING(GPIOA_MEMS_INT1) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_MEMS_INT2) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_LCD_B5) |     \
                                     PIN_PUPDR_FLOATING(GPIOA_LCD_VSYNC) |  \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN5) |         \
                                     PIN_PUPDR_FLOATING(GPIOA_LCD_G2) |     \
                                     PIN_PUPDR_PULLUP(GPIOA_ACP_RST) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_I2C3_SCL) |   \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOA_LCD_R4) |     \
                                     PIN_PUPDR_FLOATING(GPIOA_LCD_R5) |     \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_TP_INT))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_BUTTON) |           \
                                     PIN_ODR_HIGH(GPIOA_MEMS_INT1) |        \
                                     PIN_ODR_HIGH(GPIOA_MEMS_INT2) |        \
                                     PIN_ODR_HIGH(GPIOA_LCD_B5) |           \
                                     PIN_ODR_HIGH(GPIOA_LCD_VSYNC) |        \
                                     PIN_ODR_HIGH(GPIOA_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOA_LCD_G2) |           \
                                     PIN_ODR_HIGH(GPIOA_ACP_RST) |          \
                                     PIN_ODR_HIGH(GPIOA_I2C3_SCL) |         \
                                     PIN_ODR_HIGH(GPIOA_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOA_LCD_R4) |           \
                                     PIN_ODR_HIGH(GPIOA_LCD_R5) |           \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_HIGH(GPIOA_TP_INT))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_BUTTON, 0) |         \
                                     PIN_AFIO_AF(GPIOA_MEMS_INT1, 0) |      \
                                     PIN_AFIO_AF(GPIOA_MEMS_INT2, 0) |      \
                                     PIN_AFIO_AF(GPIOA_LCD_B5, 14) |        \
                                     PIN_AFIO_AF(GPIOA_LCD_VSYNC, 14) |     \
                                     PIN_AFIO_AF(GPIOA_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOA_LCD_G2, 14) |        \
                                     PIN_AFIO_AF(GPIOA_ACP_RST, 0))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_I2C3_SCL, 4) |       \
                                     PIN_AFIO_AF(GPIOA_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOA_LCD_R4, 14) |        \
                                     PIN_AFIO_AF(GPIOA_LCD_R5, 14) |        \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0) |          \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0) |          \
                                     PIN_AFIO_AF(GPIOA_TP_INT, 0))

/*
 * GPIOB setup:
 *
 * PB0  - LCD_R3                    (alternate 14).
 * PB1  - LCD_R6                    (alternate 14).
 * PB2  - BOOT1                     (input pullup).
 * PB3  - SWO                       (alternate 0).
 * PB4  - PIN4                      (input pullup).
 * PB5  - FMC_SDCKE1                (alternate 12).
 * PB6  - FMC_SDNE1                 (alternate 12).
 * PB7  - PIN7                      (input pullup).
 * PB8  - LCD_B6                    (alternate 14).
 * PB9  - LCD_B7                    (alternate 14).
 * PB10 - LCD_G4                    (alternate 14).
 * PB11 - LCD_G5                    (alternate 14).
 * PB12 - OTG_HS_ID                 (alternate 12).
 * PB13 - OTG_HS_VBUS               (input pulldown).
 * PB14 - OTG_HS_DM                 (alternate 12).
 * PB15 - OTG_HS_DP                 (alternate 12).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_ALTERNATE(GPIOB_LCD_R3) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_R6) |     \
                                     PIN_MODE_INPUT(GPIOB_BOOT1) |          \
                                     PIN_MODE_ALTERNATE(GPIOB_SWO) |        \
                                     PIN_MODE_INPUT(GPIOB_PIN4) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_FMC_SDCKE1) | \
                                     PIN_MODE_ALTERNATE(GPIOB_FMC_SDNE1) |  \
                                     PIN_MODE_INPUT(GPIOB_PIN7) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_B6) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_B7) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_G4) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_G5) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_ID) |  \
                                     PIN_MODE_INPUT(GPIOB_OTG_HS_VBUS) |    \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_DM) |  \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_LCD_R3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_R6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_BOOT1) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SWO) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_FMC_SDCKE1) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_FMC_SDNE1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_B6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_B7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_G4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_G5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_ID) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_VBUS) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_DM) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_100M(GPIOB_LCD_R3) |        \
                                     PIN_OSPEED_100M(GPIOB_LCD_R6) |        \
                                     PIN_OSPEED_100M(GPIOB_BOOT1) |         \
                                     PIN_OSPEED_100M(GPIOB_SWO) |           \
                                     PIN_OSPEED_2M(GPIOB_PIN4) |            \
                                     PIN_OSPEED_100M(GPIOB_FMC_SDCKE1) |    \
                                     PIN_OSPEED_100M(GPIOB_FMC_SDNE1) |     \
                                     PIN_OSPEED_2M(GPIOB_PIN7) |            \
                                     PIN_OSPEED_100M(GPIOB_LCD_B6) |        \
                                     PIN_OSPEED_100M(GPIOB_LCD_B7) |        \
                                     PIN_OSPEED_100M(GPIOB_LCD_G4) |        \
                                     PIN_OSPEED_100M(GPIOB_LCD_G5) |        \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_ID) |     \
                                     PIN_OSPEED_2M(GPIOB_OTG_HS_VBUS) |     \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_DM) |     \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_FLOATING(GPIOB_LCD_R3) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_R6) |     \
                                     PIN_PUPDR_PULLUP(GPIOB_BOOT1) |        \
                                     PIN_PUPDR_FLOATING(GPIOB_SWO) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_FMC_SDCKE1) | \
                                     PIN_PUPDR_FLOATING(GPIOB_FMC_SDNE1) |  \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN7) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_B6) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_B7) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_G4) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_G5) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_OTG_HS_ID) |  \
                                     PIN_PUPDR_PULLDOWN(GPIOB_OTG_HS_VBUS) |\
                                     PIN_PUPDR_FLOATING(GPIOB_OTG_HS_DM) |  \
                                     PIN_PUPDR_FLOATING(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_LCD_R3) |           \
                                     PIN_ODR_HIGH(GPIOB_LCD_R6) |           \
                                     PIN_ODR_HIGH(GPIOB_BOOT1) |            \
                                     PIN_ODR_HIGH(GPIOB_SWO) |              \
                                     PIN_ODR_HIGH(GPIOB_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOB_FMC_SDCKE1) |       \
                                     PIN_ODR_HIGH(GPIOB_FMC_SDNE1) |        \
                                     PIN_ODR_HIGH(GPIOB_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOB_LCD_B6) |           \
                                     PIN_ODR_HIGH(GPIOB_LCD_B7) |           \
                                     PIN_ODR_HIGH(GPIOB_LCD_G4) |           \
                                     PIN_ODR_HIGH(GPIOB_LCD_G5) |           \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_ID) |        \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_VBUS) |      \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_DM) |        \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_LCD_R3, 14) |        \
                                     PIN_AFIO_AF(GPIOB_LCD_R6, 14) |        \
                                     PIN_AFIO_AF(GPIOB_BOOT1, 0) |          \
                                     PIN_AFIO_AF(GPIOB_SWO, 0) |            \
                                     PIN_AFIO_AF(GPIOB_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOB_FMC_SDCKE1, 12) |    \
                                     PIN_AFIO_AF(GPIOB_FMC_SDNE1, 12) |     \
                                     PIN_AFIO_AF(GPIOB_PIN7, 0))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_LCD_B6, 14) |        \
                                     PIN_AFIO_AF(GPIOB_LCD_B7, 14) |        \
                                     PIN_AFIO_AF(GPIOB_LCD_G4, 14) |        \
                                     PIN_AFIO_AF(GPIOB_LCD_G5, 14) |        \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_ID, 12) |     \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_VBUS, 0) |    \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_DM, 12) |     \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_DP, 12))

/*
 * GPIOC setup:
 *
 * PC0  - FMC_SDNWE                 (alternate 12).
 * PC1  - SPI5_MEMS_CS              (output pushpull maximum).
 * PC2  - SPI5_LCD_CS               (output pushpull maximum).
 * PC3  - PIN3                      (input pullup).
 * PC4  - OTG_HS_PSO                (output pushpull maximum).
 * PC5  - OTG_HS_OC                 (input floating).
 * PC6  - LCD_HSYNC                 (alternate 14).
 * PC7  - LCD_G6                    (alternate 14).
 * PC8  - PIN8                      (input pullup).
 * PC9  - I2C3_SDA                  (alternate 4).
 * PC10 - LCD_R2                    (alternate 14).
 * PC11 - PIN11                     (input pullup).
 * PC12 - PIN12                     (input pullup).
 * PC13 - PIN13                     (input pullup).
 * PC14 - OSC32_IN                  (input floating).
 * PC15 - OSC32_OUT                 (input floating).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_ALTERNATE(GPIOC_FMC_SDNWE) |  \
                                     PIN_MODE_OUTPUT(GPIOC_SPI5_MEMS_CS) |  \
                                     PIN_MODE_OUTPUT(GPIOC_SPI5_LCD_CS) |   \
                                     PIN_MODE_INPUT(GPIOC_PIN3) |           \
                                     PIN_MODE_OUTPUT(GPIOC_OTG_HS_PSO) |    \
                                     PIN_MODE_INPUT(GPIOC_OTG_HS_OC) |      \
                                     PIN_MODE_ALTERNATE(GPIOC_LCD_HSYNC) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_LCD_G6) |     \
                                     PIN_MODE_INPUT(GPIOC_PIN8) |           \
                                     PIN_MODE_ALTERNATE(GPIOC_I2C3_SDA) |   \
                                     PIN_MODE_ALTERNATE(GPIOC_LCD_R2) |     \
                                     PIN_MODE_INPUT(GPIOC_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOC_OSC32_IN) |       \
                                     PIN_MODE_INPUT(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_FMC_SDNWE) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SPI5_MEMS_CS) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_SPI5_LCD_CS) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OTG_HS_PSO) | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OTG_HS_OC) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_LCD_HSYNC) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_LCD_G6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN8) |       \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_I2C3_SDA) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_LCD_R2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_IN) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_100M(GPIOC_FMC_SDNWE) |     \
                                     PIN_OSPEED_100M(GPIOC_SPI5_MEMS_CS) |  \
                                     PIN_OSPEED_100M(GPIOC_SPI5_LCD_CS) |   \
                                     PIN_OSPEED_2M(GPIOC_PIN3) |            \
                                     PIN_OSPEED_100M(GPIOC_OTG_HS_PSO) |    \
                                     PIN_OSPEED_100M(GPIOC_OTG_HS_OC) |     \
                                     PIN_OSPEED_100M(GPIOC_LCD_HSYNC) |     \
                                     PIN_OSPEED_100M(GPIOC_LCD_G6) |        \
                                     PIN_OSPEED_2M(GPIOC_PIN8) |            \
                                     PIN_OSPEED_100M(GPIOC_I2C3_SDA) |      \
                                     PIN_OSPEED_100M(GPIOC_LCD_R2) |        \
                                     PIN_OSPEED_2M(GPIOC_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOC_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOC_PIN13) |           \
                                     PIN_OSPEED_100M(GPIOC_OSC32_IN) |      \
                                     PIN_OSPEED_100M(GPIOC_OSC32_OUT))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_FLOATING(GPIOC_FMC_SDNWE) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_SPI5_MEMS_CS) |\
                                     PIN_PUPDR_FLOATING(GPIOC_SPI5_LCD_CS) |\
                                     PIN_PUPDR_PULLUP(GPIOC_PIN3) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_OTG_HS_PSO) | \
                                     PIN_PUPDR_FLOATING(GPIOC_OTG_HS_OC) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_LCD_HSYNC) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_LCD_G6) |     \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN8) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_I2C3_SDA) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_LCD_R2) |     \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN13) |        \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_IN) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_OUT))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_FMC_SDNWE) |        \
                                     PIN_ODR_HIGH(GPIOC_SPI5_MEMS_CS) |     \
                                     PIN_ODR_HIGH(GPIOC_SPI5_LCD_CS) |      \
                                     PIN_ODR_HIGH(GPIOC_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOC_OTG_HS_PSO) |       \
                                     PIN_ODR_HIGH(GPIOC_OTG_HS_OC) |        \
                                     PIN_ODR_HIGH(GPIOC_LCD_HSYNC) |        \
                                     PIN_ODR_HIGH(GPIOC_LCD_G6) |           \
                                     PIN_ODR_HIGH(GPIOC_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOC_I2C3_SDA) |         \
                                     PIN_ODR_HIGH(GPIOC_LCD_R2) |           \
                                     PIN_ODR_HIGH(GPIOC_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOC_OSC32_IN) |         \
                                     PIN_ODR_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_FMC_SDNWE, 12) |     \
                                     PIN_AFIO_AF(GPIOC_SPI5_MEMS_CS, 0) |   \
                                     PIN_AFIO_AF(GPIOC_SPI5_LCD_CS, 0) |    \
                                     PIN_AFIO_AF(GPIOC_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOC_OTG_HS_PSO, 0) |     \
                                     PIN_AFIO_AF(GPIOC_OTG_HS_OC, 0) |      \
                                     PIN_AFIO_AF(GPIOC_LCD_HSYNC, 14) |     \
                                     PIN_AFIO_AF(GPIOC_LCD_G6, 14))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOC_I2C3_SDA, 4) |       \
                                     PIN_AFIO_AF(GPIOC_LCD_R2, 14) |        \
                                     PIN_AFIO_AF(GPIOC_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOC_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOC_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOC_OSC32_IN, 0) |       \
                                     PIN_AFIO_AF(GPIOC_OSC32_OUT, 0))

/*
 * GPIOD setup:
 *
 * PD0  - FMC_D2                    (alternate 12).
 * PD1  - FMC_D3                    (alternate 12).
 * PD2  - PIN2                      (input pullup).
 * PD3  - LCD_G7                    (alternate 14).
 * PD4  - PIN4                      (input pullup).
 * PD5  - PIN5                      (input pullup).
 * PD6  - LCD_B2                    (alternate 14).
 * PD7  - PIN7                      (input pullup).
 * PD8  - FMC_D13                   (alternate 12).
 * PD9  - FMC_D14                   (alternate 12).
 * PD10 - FMC_D15                   (alternate 12).
 * PD11 - LCD_TE                    (input floating).
 * PD12 - LCD_RDX                   (output pushpull maximum).
 * PD13 - LCD_WRX                   (output pushpull maximum).
 * PD14 - FMC_D0                    (alternate 12).
 * PD15 - FMC_D1                    (alternate 12).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_ALTERNATE(GPIOD_FMC_D2) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D3) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN2) |           \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_G7) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN5) |           \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_B2) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN7) |           \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D13) |    \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D14) |    \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D15) |    \
                                     PIN_MODE_INPUT(GPIOD_LCD_TE) |         \
                                     PIN_MODE_OUTPUT(GPIOD_LCD_RDX) |       \
                                     PIN_MODE_OUTPUT(GPIOD_LCD_WRX) |       \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D0) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D1))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOD_FMC_D2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_G7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_B2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D13) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D14) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D15) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_TE) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_RDX) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_WRX) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D1))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_100M(GPIOD_FMC_D2) |        \
                                     PIN_OSPEED_100M(GPIOD_FMC_D3) |        \
                                     PIN_OSPEED_2M(GPIOD_PIN2) |            \
                                     PIN_OSPEED_100M(GPIOD_LCD_G7) |        \
                                     PIN_OSPEED_2M(GPIOD_PIN4) |            \
                                     PIN_OSPEED_2M(GPIOD_PIN5) |            \
                                     PIN_OSPEED_100M(GPIOD_LCD_B2) |        \
                                     PIN_OSPEED_2M(GPIOD_PIN7) |            \
                                     PIN_OSPEED_100M(GPIOD_FMC_D13) |       \
                                     PIN_OSPEED_100M(GPIOD_FMC_D14) |       \
                                     PIN_OSPEED_100M(GPIOD_FMC_D15) |       \
                                     PIN_OSPEED_100M(GPIOD_LCD_TE) |        \
                                     PIN_OSPEED_100M(GPIOD_LCD_RDX) |       \
                                     PIN_OSPEED_100M(GPIOD_LCD_WRX) |       \
                                     PIN_OSPEED_100M(GPIOD_FMC_D0) |        \
                                     PIN_OSPEED_100M(GPIOD_FMC_D1))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_FLOATING(GPIOD_FMC_D2) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D3) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN2) |         \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_G7) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN5) |         \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_B2) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN7) |         \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D13) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D14) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D15) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_TE) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_RDX) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_WRX) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D0) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D1))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(GPIOD_FMC_D2) |           \
                                     PIN_ODR_HIGH(GPIOD_FMC_D3) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOD_LCD_G7) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOD_LCD_B2) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOD_FMC_D13) |          \
                                     PIN_ODR_HIGH(GPIOD_FMC_D14) |          \
                                     PIN_ODR_HIGH(GPIOD_FMC_D15) |          \
                                     PIN_ODR_HIGH(GPIOD_LCD_TE) |           \
                                     PIN_ODR_HIGH(GPIOD_LCD_RDX) |          \
                                     PIN_ODR_HIGH(GPIOD_LCD_WRX) |          \
                                     PIN_ODR_HIGH(GPIOD_FMC_D0) |           \
                                     PIN_ODR_HIGH(GPIOD_FMC_D1))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_FMC_D2, 12) |        \
                                     PIN_AFIO_AF(GPIOD_FMC_D3, 12) |        \
                                     PIN_AFIO_AF(GPIOD_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOD_LCD_G7, 14) |        \
                                     PIN_AFIO_AF(GPIOD_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOD_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOD_LCD_B2, 14) |        \
                                     PIN_AFIO_AF(GPIOD_PIN7, 0))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(GPIOD_FMC_D13, 12) |       \
                                     PIN_AFIO_AF(GPIOD_FMC_D14, 12) |       \
                                     PIN_AFIO_AF(GPIOD_FMC_D15, 12) |       \
                                     PIN_AFIO_AF(GPIOD_LCD_TE, 0) |         \
                                     PIN_AFIO_AF(GPIOD_LCD_RDX, 0) |        \
                                     PIN_AFIO_AF(GPIOD_LCD_WRX, 0) |        \
                                     PIN_AFIO_AF(GPIOD_FMC_D0, 12) |        \
                                     PIN_AFIO_AF(GPIOD_FMC_D1, 12))

/*
 * GPIOE setup:
 *
 * PE0  - FMC_NBL0                  (alternate 12).
 * PE1  - FMC_NBL1                  (alternate 12).
 * PE2  - PIN2                      (input pullup).
 * PE3  - PIN3                      (input pullup).
 * PE4  - PIN4                      (input pullup).
 * PE5  - PIN5                      (input pullup).
 * PE6  - PIN6                      (input pullup).
 * PE7  - FMC_D4                    (alternate 12).
 * PE8  - FMC_D5                    (alternate 12).
 * PE9  - FMC_D6                    (alternate 12).
 * PE10 - FMC_D7                    (alternate 12).
 * PE11 - FMC_D8                    (alternate 12).
 * PE12 - FMC_D9                    (alternate 12).
 * PE13 - FMC_D10                   (alternate 12).
 * PE14 - FMC_D11                   (alternate 12).
 * PE15 - FMC_D12                   (alternate 12).
 */
#define VAL_GPIOE_MODER             (PIN_MODE_ALTERNATE(GPIOE_FMC_NBL0) |   \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_NBL1) |   \
                                     PIN_MODE_INPUT(GPIOE_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN6) |           \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D4) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D5) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D6) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D7) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D8) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D9) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D10) |    \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D11) |    \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D12))
#define VAL_GPIOE_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOE_FMC_NBL0) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_NBL1) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D8) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D9) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D11) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D12))
#define VAL_GPIOE_OSPEEDR           (PIN_OSPEED_100M(GPIOE_FMC_NBL0) |      \
                                     PIN_OSPEED_100M(GPIOE_FMC_NBL1) |      \
                                     PIN_OSPEED_2M(GPIOE_PIN2) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN3) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN4) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN5) |            \
                                     PIN_OSPEED_2M(GPIOE_PIN6) |            \
                                     PIN_OSPEED_100M(GPIOE_FMC_D4) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D5) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D6) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D7) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D8) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D9) |        \
                                     PIN_OSPEED_100M(GPIOE_FMC_D10) |       \
                                     PIN_OSPEED_100M(GPIOE_FMC_D11) |       \
                                     PIN_OSPEED_100M(GPIOE_FMC_D12))
#define VAL_GPIOE_PUPDR             (PIN_PUPDR_FLOATING(GPIOE_FMC_NBL0) |   \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_NBL1) |   \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN6) |         \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D4) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D5) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D6) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D7) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D8) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D9) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D10) |    \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D11) |    \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D12))
#define VAL_GPIOE_ODR               (PIN_ODR_HIGH(GPIOE_FMC_NBL0) |         \
                                     PIN_ODR_HIGH(GPIOE_FMC_NBL1) |         \
                                     PIN_ODR_HIGH(GPIOE_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOE_FMC_D4) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D5) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D6) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D7) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D8) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D9) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D10) |          \
                                     PIN_ODR_HIGH(GPIOE_FMC_D11) |          \
                                     PIN_ODR_HIGH(GPIOE_FMC_D12))
#define VAL_GPIOE_AFRL              (PIN_AFIO_AF(GPIOE_FMC_NBL0, 12) |      \
                                     PIN_AFIO_AF(GPIOE_FMC_NBL1, 12) |      \
                                     PIN_AFIO_AF(GPIOE_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOE_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOE_FMC_D4, 12))
#define VAL_GPIOE_AFRH              (PIN_AFIO_AF(GPIOE_FMC_D5, 12) |        \
                                     PIN_AFIO_AF(GPIOE_FMC_D6, 12) |        \
                                     PIN_AFIO_AF(GPIOE_FMC_D7, 12) |        \
                                     PIN_AFIO_AF(GPIOE_FMC_D8, 12) |        \
                                     PIN_AFIO_AF(GPIOE_FMC_D9, 12) |        \
                                     PIN_AFIO_AF(GPIOE_FMC_D10, 12) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D11, 12) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D12, 12))

/*
 * GPIOF setup:
 *
 * PF0  - FMC_A0                    (alternate 12).
 * PF1  - FMC_A1                    (alternate 12).
 * PF2  - FMC_A2                    (alternate 12).
 * PF3  - FMC_A3                    (alternate 12).
 * PF4  - FMC_A4                    (alternate 12).
 * PF5  - FMC_A5                    (alternate 12).
 * PF6  - PIN6                      (input pullup).
 * PF7  - LCD_DCX                   (output pushpull maximum).
 * PF8  - SPI5_MISO                 (alternate 5).
 * PF9  - SPI5_MOSI                 (alternate 5).
 * PF10 - LCD_DE                    (output pushpull maximum).
 * PF11 - FMC_SDNRAS                (alternate 12).
 * PF12 - FMC_A6                    (alternate 12).
 * PF13 - FMC_A7                    (alternate 12).
 * PF14 - FMC_A8                    (alternate 12).
 * PF15 - FMC_A9                    (alternate 12).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_ALTERNATE(GPIOF_FMC_A0) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A1) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A2) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A3) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A4) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A5) |     \
                                     PIN_MODE_INPUT(GPIOF_PIN6) |           \
                                     PIN_MODE_OUTPUT(GPIOF_LCD_DCX) |       \
                                     PIN_MODE_ALTERNATE(GPIOF_SPI5_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOF_SPI5_MOSI) |  \
                                     PIN_MODE_OUTPUT(GPIOF_LCD_DE) |        \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_SDNRAS) | \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A6) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A7) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A8) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A9))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOF_FMC_A0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_DCX) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOF_SPI5_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_SPI5_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_DE) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_SDNRAS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A8) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A9))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_100M(GPIOF_FMC_A0) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A1) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A2) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A3) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A4) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A5) |        \
                                     PIN_OSPEED_2M(GPIOF_PIN6) |            \
                                     PIN_OSPEED_100M(GPIOF_LCD_DCX) |       \
                                     PIN_OSPEED_100M(GPIOF_SPI5_MISO) |     \
                                     PIN_OSPEED_100M(GPIOF_SPI5_MOSI) |     \
                                     PIN_OSPEED_100M(GPIOF_LCD_DE) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_SDNRAS) |    \
                                     PIN_OSPEED_100M(GPIOF_FMC_A6) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A7) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A8) |        \
                                     PIN_OSPEED_100M(GPIOF_FMC_A9))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_FLOATING(GPIOF_FMC_A0) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A1) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A2) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A3) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A4) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A5) |     \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN6) |         \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_DCX) |    \
                                     PIN_PUPDR_FLOATING(GPIOF_SPI5_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_SPI5_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_DE) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_SDNRAS) | \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A6) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A7) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A8) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A9))
#define VAL_GPIOF_ODR               (PIN_ODR_HIGH(GPIOF_FMC_A0) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A1) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A2) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A3) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A4) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A5) |           \
                                     PIN_ODR_HIGH(GPIOF_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOF_LCD_DCX) |          \
                                     PIN_ODR_HIGH(GPIOF_SPI5_MISO) |        \
                                     PIN_ODR_HIGH(GPIOF_SPI5_MOSI) |        \
                                     PIN_ODR_HIGH(GPIOF_LCD_DE) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_SDNRAS) |       \
                                     PIN_ODR_HIGH(GPIOF_FMC_A6) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A7) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A8) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A9))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(GPIOF_FMC_A0, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A1, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A2, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A3, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A4, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A5, 12) |        \
                                     PIN_AFIO_AF(GPIOF_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOF_LCD_DCX, 0))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(GPIOF_SPI5_MISO, 5) |      \
                                     PIN_AFIO_AF(GPIOF_SPI5_MOSI, 5) |      \
                                     PIN_AFIO_AF(GPIOF_LCD_DE, 0) |         \
                                     PIN_AFIO_AF(GPIOF_FMC_SDNRAS, 12) |    \
                                     PIN_AFIO_AF(GPIOF_FMC_A6, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A7, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A8, 12) |        \
                                     PIN_AFIO_AF(GPIOF_FMC_A9, 12))

/*
 * GPIOG setup:
 *
 * PG0  - FMC_A10                   (alternate 12).
 * PG1  - FMC_A11                   (alternate 12).
 * PG2  - PIN2                      (input pullup).
 * PG3  - PIN3                      (input pullup).
 * PG4  - FMC_BA0                   (alternate 12).
 * PG5  - FMC_BA1                   (alternate 12).
 * PG6  - LCD_R7                    (alternate 14).
 * PG7  - LCD_CLK                   (alternate 14).
 * PG8  - FMC_SDCLK                 (alternate 12).
 * PG9  - PIN9                      (input pullup).
 * PG10 - LCD_G3                    (alternate 14).
 * PG11 - LCD_B3                    (alternate 14).
 * PG12 - LCD_B4                    (alternate 14).
 * PG13 - LED3_GREEN                (output pushpull maximum).
 * PG14 - LED4_RED                  (output pushpull maximum).
 * PG15 - FMC_SDNCAS                (alternate 12).
 */
#define VAL_GPIOG_MODER             (PIN_MODE_ALTERNATE(GPIOG_FMC_A10) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_A11) |    \
                                     PIN_MODE_INPUT(GPIOG_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOG_PIN3) |           \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_BA0) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_BA1) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_LCD_R7) |     \
                                     PIN_MODE_ALTERNATE(GPIOG_LCD_CLK) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_SDCLK) |  \
                                     PIN_MODE_INPUT(GPIOG_PIN9) |           \
                                     PIN_MODE_ALTERNATE(GPIOG_LCD_G3) |     \
                                     PIN_MODE_ALTERNATE(GPIOG_LCD_B3) |     \
                                     PIN_MODE_ALTERNATE(GPIOG_LCD_B4) |     \
                                     PIN_MODE_OUTPUT(GPIOG_LED3_GREEN) |    \
                                     PIN_MODE_OUTPUT(GPIOG_LED4_RED) |      \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOG_FMC_A10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_A11) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_BA0) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_BA1) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LCD_R7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LCD_CLK) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_SDCLK) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LCD_G3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LCD_B3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LCD_B4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LED3_GREEN) | \
                                     PIN_OTYPE_PUSHPULL(GPIOG_LED4_RED) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_OSPEEDR           (PIN_OSPEED_100M(GPIOG_FMC_A10) |       \
                                     PIN_OSPEED_100M(GPIOG_FMC_A11) |       \
                                     PIN_OSPEED_2M(GPIOG_PIN2) |            \
                                     PIN_OSPEED_2M(GPIOG_PIN3) |            \
                                     PIN_OSPEED_100M(GPIOG_FMC_BA0) |       \
                                     PIN_OSPEED_100M(GPIOG_FMC_BA1) |       \
                                     PIN_OSPEED_100M(GPIOG_LCD_R7) |        \
                                     PIN_OSPEED_100M(GPIOG_LCD_CLK) |       \
                                     PIN_OSPEED_100M(GPIOG_FMC_SDCLK) |     \
                                     PIN_OSPEED_2M(GPIOG_PIN9) |            \
                                     PIN_OSPEED_100M(GPIOG_LCD_G3) |        \
                                     PIN_OSPEED_100M(GPIOG_LCD_B3) |        \
                                     PIN_OSPEED_100M(GPIOG_LCD_B4) |        \
                                     PIN_OSPEED_100M(GPIOG_LED3_GREEN) |    \
                                     PIN_OSPEED_100M(GPIOG_LED4_RED) |      \
                                     PIN_OSPEED_100M(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_PUPDR             (PIN_PUPDR_FLOATING(GPIOG_FMC_A10) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_A11) |    \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN3) |         \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_BA0) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_BA1) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_LCD_R7) |     \
                                     PIN_PUPDR_FLOATING(GPIOG_LCD_CLK) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_SDCLK) |  \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN9) |         \
                                     PIN_PUPDR_FLOATING(GPIOG_LCD_G3) |     \
                                     PIN_PUPDR_FLOATING(GPIOG_LCD_B3) |     \
                                     PIN_PUPDR_FLOATING(GPIOG_LCD_B4) |     \
                                     PIN_PUPDR_FLOATING(GPIOG_LED3_GREEN) | \
                                     PIN_PUPDR_FLOATING(GPIOG_LED4_RED) |   \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_ODR               (PIN_ODR_HIGH(GPIOG_FMC_A10) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_A11) |          \
                                     PIN_ODR_HIGH(GPIOG_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOG_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOG_FMC_BA0) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_BA1) |          \
                                     PIN_ODR_HIGH(GPIOG_LCD_R7) |           \
                                     PIN_ODR_HIGH(GPIOG_LCD_CLK) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_SDCLK) |        \
                                     PIN_ODR_HIGH(GPIOG_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOG_LCD_G3) |           \
                                     PIN_ODR_HIGH(GPIOG_LCD_B3) |           \
                                     PIN_ODR_HIGH(GPIOG_LCD_B4) |           \
                                     PIN_ODR_LOW(GPIOG_LED3_GREEN) |        \
                                     PIN_ODR_LOW(GPIOG_LED4_RED) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_AFRL              (PIN_AFIO_AF(GPIOG_FMC_A10, 12) |       \
                                     PIN_AFIO_AF(GPIOG_FMC_A11, 12) |       \
                                     PIN_AFIO_AF(GPIOG_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOG_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOG_FMC_BA0, 12) |       \
                                     PIN_AFIO_AF(GPIOG_FMC_BA1, 12) |       \
                                     PIN_AFIO_AF(GPIOG_LCD_R7, 14) |        \
                                     PIN_AFIO_AF(GPIOG_LCD_CLK, 14))
#define VAL_GPIOG_AFRH              (PIN_AFIO_AF(GPIOG_FMC_SDCLK, 12) |     \
                                     PIN_AFIO_AF(GPIOG_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOG_LCD_G3, 14) |        \
                                     PIN_AFIO_AF(GPIOG_LCD_B3, 14) |        \
                                     PIN_AFIO_AF(GPIOG_LCD_B4, 14) |        \
                                     PIN_AFIO_AF(GPIOG_LED3_GREEN, 0) |     \
                                     PIN_AFIO_AF(GPIOG_LED4_RED, 0) |       \
                                     PIN_AFIO_AF(GPIOG_FMC_SDNCAS, 12))

/*
 * GPIOH setup:
 *
 * PH0  - OSC_IN                    (input floating).
 * PH1  - OSC_OUT                   (input floating).
 * PH2  - PIN2                      (input pullup).
 * PH3  - PIN3                      (input pullup).
 * PH4  - PIN4                      (input pullup).
 * PH5  - PIN5                      (input pullup).
 * PH6  - PIN6                      (input pullup).
 * PH7  - PIN7                      (input pullup).
 * PH8  - PIN8                      (input pullup).
 * PH9  - PIN9                      (input pullup).
 * PH10 - PIN10                     (input pullup).
 * PH11 - PIN11                     (input pullup).
 * PH12 - PIN12                     (input pullup).
 * PH13 - PIN13                     (input pullup).
 * PH14 - PIN14                     (input pullup).
 * PH15 - PIN15                     (input pullup).
 */
#define VAL_GPIOH_MODER             (PIN_MODE_INPUT(GPIOH_OSC_IN) |         \
                                     PIN_MODE_INPUT(GPIOH_OSC_OUT) |        \
                                     PIN_MODE_INPUT(GPIOH_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN15))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOH_OSC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSC_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN15))
#define VAL_GPIOH_OSPEEDR           (PIN_OSPEED_100M(GPIOH_OSC_IN) |        \
                                     PIN_OSPEED_100M(GPIOH_OSC_OUT) |       \
                                     PIN_OSPEED_2M(GPIOH_PIN2) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN3) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN4) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN5) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN6) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN7) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN8) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN9) |            \
                                     PIN_OSPEED_2M(GPIOH_PIN10) |           \
                                     PIN_OSPEED_2M(GPIOH_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOH_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOH_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOH_PIN14) |           \
                                     PIN_OSPEED_2M(GPIOH_PIN15))
#define VAL_GPIOH_PUPDR             (PIN_PUPDR_FLOATING(GPIOH_OSC_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_OSC_OUT) |    \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN15))
#define VAL_GPIOH_ODR               (PIN_ODR_HIGH(GPIOH_OSC_IN) |           \
                                     PIN_ODR_HIGH(GPIOH_OSC_OUT) |          \
                                     PIN_ODR_HIGH(GPIOH_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN15))
#define VAL_GPIOH_AFRL              (PIN_AFIO_AF(GPIOH_OSC_IN, 0) |         \
                                     PIN_AFIO_AF(GPIOH_OSC_OUT, 0) |        \
                                     PIN_AFIO_AF(GPIOH_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN7, 0))
#define VAL_GPIOH_AFRH              (PIN_AFIO_AF(GPIOH_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOH_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOH_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOH_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOH_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOH_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOH_PIN15, 0))

/*
 * GPIOI setup:
 *
 * PI0  - PIN0                      (input pullup).
 * PI1  - PIN1                      (input pullup).
 * PI2  - PIN2                      (input pullup).
 * PI3  - PIN3                      (input pullup).
 * PI4  - PIN4                      (input pullup).
 * PI5  - PIN5                      (input pullup).
 * PI6  - PIN6                      (input pullup).
 * PI7  - PIN7                      (input pullup).
 * PI8  - PIN8                      (input pullup).
 * PI9  - PIN9                      (input pullup).
 * PI10 - PIN10                     (input pullup).
 * PI11 - PIN11                     (input pullup).
 * PI12 - PIN12                     (input pullup).
 * PI13 - PIN13                     (input pullup).
 * PI14 - PIN14                     (input pullup).
 * PI15 - PIN15                     (input pullup).
 */
#define VAL_GPIOI_MODER             (PIN_MODE_INPUT(GPIOI_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN15))
#define VAL_GPIOI_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOI_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN15))
#define VAL_GPIOI_OSPEEDR           (PIN_OSPEED_2M(GPIOI_PIN0) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN1) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN2) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN3) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN4) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN5) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN6) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN7) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN8) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN9) |            \
                                     PIN_OSPEED_2M(GPIOI_PIN10) |           \
                                     PIN_OSPEED_2M(GPIOI_PIN11) |           \
                                     PIN_OSPEED_2M(GPIOI_PIN12) |           \
                                     PIN_OSPEED_2M(GPIOI_PIN13) |           \
                                     PIN_OSPEED_2M(GPIOI_PIN14) |           \
                                     PIN_OSPEED_2M(GPIOI_PIN15))
#define VAL_GPIOI_PUPDR             (PIN_PUPDR_PULLUP(GPIOI_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN15))
#define VAL_GPIOI_ODR               (PIN_ODR_HIGH(GPIOI_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN15))
#define VAL_GPIOI_AFRL              (PIN_AFIO_AF(GPIOI_PIN0, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN7, 0))
#define VAL_GPIOI_AFRH              (PIN_AFIO_AF(GPIOI_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOI_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOI_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOI_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOI_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOI_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOI_PIN15, 0))


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
