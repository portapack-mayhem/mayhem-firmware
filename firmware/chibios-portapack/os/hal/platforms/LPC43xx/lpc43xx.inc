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

#ifndef __LPC43XX_H__
#define __LPC43XX_H__

/** @addtogroup NXP
  * @{
  */

/** @addtogroup LPC43xx
  * @{
  */

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __CC_ARM   )
  #pragma anon_unions
#endif

// ------------------------------------------------------------------------------------------------
// -----                                         CREG                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Configuration Registers (CREG) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t ETHMODE            : 3;
       uint32_t RESERVED0          : 1;
  __IO uint32_t CTOUTCTRL          : 1;
       uint32_t RESERVED1          : 7;
  __IO uint32_t I2S0_TX_SCK_IN_SEL : 1;
  __IO uint32_t I2S0_RX_SCK_IN_SEL : 1;
  __IO uint32_t I2S1_TX_SCK_IN_SEL : 1;
  __IO uint32_t I2S1_RX_SCK_IN_SEL : 1;
  __IO uint32_t EMC_CLK_SEL        : 1;
       uint32_t RESERVED2          : 15;
} LPC_CREG_CREG6_Type;

typedef struct {
       uint32_t RESERVED0;
  __IO uint32_t CREG0;
       uint32_t RESERVED1[62];
  __IO uint32_t M4MEMMAP;
       uint32_t RESERVED2[5];
  __IO uint32_t CREG5;
  __IO uint32_t DMAMUX;
  __IO uint32_t FLASHCFGA;
  __IO uint32_t FLASHCFGB;
  __IO uint32_t ETBCFG;
  LPC_CREG_CREG6_Type CREG6;
  __IO uint32_t M4TXEVENT;
       uint32_t RESERVED3[51];
  __I  uint32_t CHIPID;
       uint32_t RESERVED4[65];
  __IO uint32_t M0SUBMEMMAP;
       uint32_t RESERVED5[2];
  __IO uint32_t M0SUBTXEVENT;
       uint32_t RESERVED6[58];
  __IO uint32_t M0APPTXEVENT;
  __IO uint32_t M0APPMEMMAP;
       uint32_t RESERVED7[62];
  __IO uint32_t USB0FLADJ;
       uint32_t RESERVED8[63];
  __IO uint32_t USB1FLADJ;
} LPC_CREG_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          CGU                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Clock Generation Unit (CGU) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t RCNT        : 9;
  __I  uint32_t FCNT        : 14;
  __IO uint32_t MEAS        : 1;
  __IO uint32_t CLK_SEL     : 5;
       uint32_t RESERVED0   : 3;
} LPC_CGU_FREQ_MON_Type;

typedef union {
  struct {
    __IO uint32_t ENABLE      : 1;
    __IO uint32_t BYPASS      : 1;
    __IO uint32_t HF          : 1;
         uint32_t RESERVED0   : 29;
  };
  __IO uint32_t word;
} LPC_CGU_XTAL_OSC_CTRL_Type;

typedef struct {
  __I  uint32_t LOCK        : 1;
  __I  uint32_t FR          : 1;
       uint32_t RESERVED0   : 30;
} LPC_CGU_PLL0USB_STAT_Type;

typedef struct {
  __IO uint32_t PD          : 1;
  __IO uint32_t BYPASS      : 1;
  __IO uint32_t DIRECTI     : 1;
  __IO uint32_t DIRECTO     : 1;
  __IO uint32_t CLKEN       : 1;
       uint32_t RESERVED0   : 1;
  __IO uint32_t FRM         : 1;
       uint32_t RESERVED1   : 4;
  __IO uint32_t AUTOBLOCK   : 1;
       uint32_t RESERVED2   : 12;
  __IO uint32_t CLK_SEL     : 5;
       uint32_t RESERVED3   : 3;
} LPC_CGU_PLL0USB_CTRL_Type;

typedef struct {
  __I  uint32_t LOCK        : 1;
       uint32_t RESERVED0   : 31;
} LPC_CGU_PLL1_STAT_Type;

typedef struct {
  __IO uint32_t PD          : 1;
  __IO uint32_t BYPASS      : 1;
       uint32_t RESERVED0   : 4;
  __IO uint32_t FBSEL       : 1;
  __IO uint32_t DIRECT      : 1;
  __IO uint32_t PSEL        : 2;
       uint32_t RESERVED1   : 1;
  __IO uint32_t AUTOBLOCK   : 1;
  __IO uint32_t NSEL        : 2;
       uint32_t RESERVED2   : 2;
  __IO uint32_t MSEL        : 8;
  __IO uint32_t CLK_SEL     : 5;
       uint32_t RESERVED3   : 3;
} LPC_CGU_PLL1_CTRL_Type;

typedef union {
  struct {
    __IO uint32_t PD          : 1;
         uint32_t RESERVED0   : 1;
    __IO uint32_t IDIV        : 2;
         uint32_t RESERVED1   : 7;
    __IO uint32_t AUTOBLOCK   : 1;
         uint32_t RESERVED2   : 12;
    __IO uint32_t CLK_SEL     : 5;
         uint32_t RESERVED3   : 3;
  };
  __IO uint32_t word;  
} LPC_CGU_IDIVx_CTRL_Type;

typedef struct {
  __I  uint32_t PD          : 1;
       uint32_t RESERVED0   : 10;
  __I  uint32_t AUTOBLOCK   : 1;
       uint32_t RESERVED1   : 12;
  __I  uint32_t CLK_SEL     : 5;
       uint32_t RESERVED2   : 3;
} LPC_CGU_BASE_SAFE_CLK_Type;

typedef union {
  struct {
    __IO uint32_t PD          : 1;
         uint32_t RESERVED0   : 10;
    __IO uint32_t AUTOBLOCK   : 1;
         uint32_t RESERVED1   : 12;
    __IO uint32_t CLK_SEL     : 5;
         uint32_t RESERVED2   : 3;
  };
  __IO uint32_t word;
} LPC_CGU_BASE_CLK_Type;

typedef struct {
       uint32_t RESERVED[5];        /* +0x000 */
  LPC_CGU_FREQ_MON_Type       FREQ_MON;
  LPC_CGU_XTAL_OSC_CTRL_Type  XTAL_OSC_CTRL;
  LPC_CGU_PLL0USB_STAT_Type   PLL0USB_STAT;
  LPC_CGU_PLL0USB_CTRL_Type   PLL0USB_CTRL;       /* +0x020 */
  __IO uint32_t               PLL0USB_MDIV;
  __IO uint32_t               PLL0USB_NP_DIV;
  __I  uint32_t               PLL0AUDIO_STAT;
  __IO uint32_t               PLL0AUDIO_CTRL;     /* +0x030 */
  __IO uint32_t               PLL0AUDIO_MDIV;
  __IO uint32_t               PLL0AUDIO_NP_DIV;
  __IO uint32_t               PLL0AUDIO_FRAC;
  __I  uint32_t               PLL1_STAT;          /* +0x040 */
  __IO uint32_t               PLL1_CTRL;
  LPC_CGU_IDIVx_CTRL_Type     IDIVA_CTRL;
  LPC_CGU_IDIVx_CTRL_Type     IDIVB_CTRL;
  LPC_CGU_IDIVx_CTRL_Type     IDIVC_CTRL;         /* +0x050 */
  LPC_CGU_IDIVx_CTRL_Type     IDIVD_CTRL;
  LPC_CGU_IDIVx_CTRL_Type     IDIVE_CTRL;
  LPC_CGU_BASE_SAFE_CLK_Type  BASE_SAFE_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_USB0_CLK;      /* +0x060 */
  LPC_CGU_BASE_CLK_Type       BASE_PERIPH_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_USB1_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_M4_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_SPIFI_CLK;     /* +0x070 */
  LPC_CGU_BASE_CLK_Type       BASE_SPI_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_PHY_RX_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_PHY_TX_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_APB1_CLK;      /* +0x080 */
  LPC_CGU_BASE_CLK_Type       BASE_APB3_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_LCD_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_ADCHS_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_SDIO_CLK;      /* +0x090 */
  LPC_CGU_BASE_CLK_Type       BASE_SSP0_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_SSP1_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_UART0_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_UART1_CLK;     /* +0x0a0 */
  LPC_CGU_BASE_CLK_Type       BASE_UART2_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_UART3_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_OUT_CLK;
       uint32_t OUTCLK_21_CTRL;     /* +0x0b0 */
       uint32_t OUTCLK_22_CTRL;
       uint32_t OUTCLK_23_CTRL;
       uint32_t OUTCLK_24_CTRL;
  LPC_CGU_BASE_CLK_Type       BASE_AUDIO_CLK;     /* +0x0c0 */
  LPC_CGU_BASE_CLK_Type       BASE_CGU_OUT0_CLK;
  LPC_CGU_BASE_CLK_Type       BASE_CGU_OUT1_CLK;
} LPC_CGU_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          CCU                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Clock Control Unit (CCU) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t RUN       : 1;
  __IO uint32_t AUTO      : 1;
  __IO uint32_t WAKEUP    : 1;
       uint32_t RESERVED0 : 29;
} LPC_CCU1_CFG_160_Type;

typedef struct {
  __IO uint32_t RUN       : 1;
  __IO uint32_t AUTO      : 1;
  __IO uint32_t WAKEUP    : 1;
       uint32_t RESERVED0 : 2;
  __O  uint32_t DIV       : 3;
       uint32_t RESERVED1 : 19;
  __I  uint32_t DIVSTAT   : 3;
       uint32_t RESERVED2 : 2;
} LPC_CCU1_CFG_161_Type;

typedef struct {
  __I  uint32_t RUN       : 1;
  __I  uint32_t AUTO      : 1;
  __I  uint32_t WAKEUP    : 1;
       uint32_t RESERVED0 : 2;
  __I  uint32_t RUN_N     : 1;
       uint32_t RESERVED1 : 26;
} LPC_CCU_STAT_Type;

typedef struct {
  __IO uint32_t PM;                           /* +0x000 */
  __I  uint32_t BASE_STAT;                    /* +0x004 */
       uint32_t RESERVED0[62];                /* +0x008 */
  LPC_CCU1_CFG_160_Type CLK_APB3_BUS_CFG;             /* +0x100 */
  LPC_CCU_STAT_Type CLK_APB3_BUS_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB3_I2C1_CFG;
  LPC_CCU_STAT_Type CLK_APB3_I2C1_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB3_DAC_CFG;
  LPC_CCU_STAT_Type CLK_APB3_DAC_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB3_ADC0_CFG;
  LPC_CCU_STAT_Type CLK_APB3_ADC0_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB3_ADC1_CFG;
  LPC_CCU_STAT_Type CLK_APB3_ADC1_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB3_CAN0_CFG;
  LPC_CCU_STAT_Type CLK_APB3_CAN0_STAT;
       uint32_t RESERVED1[52];
  LPC_CCU1_CFG_160_Type CLK_APB1_BUS_CFG;             /* +0x200 */
  LPC_CCU_STAT_Type CLK_APB1_BUS_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB1_MOTOCON_PWM_CFG;
  LPC_CCU_STAT_Type CLK_APB1_MOTOCON_PWM_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB1_I2C0_CFG;
  LPC_CCU_STAT_Type CLK_APB1_I2C0_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB1_I2S_CFG;
  LPC_CCU_STAT_Type CLK_APB1_I2S_STAT;
  LPC_CCU1_CFG_160_Type CLK_APB1_CAN1_CFG;
  LPC_CCU_STAT_Type CLK_APB1_CAN1_STAT;
       uint32_t RESERVED2[54];
  LPC_CCU1_CFG_160_Type CLK_SPIFI_CFG;                /* +0x300 */
  LPC_CCU_STAT_Type CLK_SPIFI_STAT;
       uint32_t RESERVED3[62];
  LPC_CCU1_CFG_160_Type CLK_M4_BUS_CFG;               /* +0x400 */
  LPC_CCU_STAT_Type CLK_M4_BUS_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_SPIFI_CFG;
  LPC_CCU_STAT_Type CLK_M4_SPIFI_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_GPIO_CFG;
  LPC_CCU_STAT_Type CLK_M4_GPIO_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_LCD_CFG;
  LPC_CCU_STAT_Type CLK_M4_LCD_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_ETHERNET_CFG;
  LPC_CCU_STAT_Type CLK_M4_ETHERNET_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_USB0_CFG;
  LPC_CCU_STAT_Type CLK_M4_USB0_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_EMC_CFG;
  LPC_CCU_STAT_Type CLK_M4_EMC_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_SDIO_CFG;
  LPC_CCU_STAT_Type CLK_M4_SDIO_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_DMA_CFG;
  LPC_CCU_STAT_Type CLK_M4_DMA_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_M4CORE_CFG;
  LPC_CCU_STAT_Type CLK_M4_M4CORE_STAT;
       uint32_t RESERVED4[6];
  LPC_CCU1_CFG_160_Type CLK_M4_SCT_CFG;               /* +0x468 */
  LPC_CCU_STAT_Type CLK_M4_SCT_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_USB1_CFG;
  LPC_CCU_STAT_Type CLK_M4_USB1_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_EMCDIV_CFG;
  LPC_CCU_STAT_Type CLK_M4_EMCDIV_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_FLASHA_CFG;
  LPC_CCU_STAT_Type CLK_M4_FLASHA_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_FLASHB_CFG;
  LPC_CCU_STAT_Type CLK_M4_FLASHB_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_M0APP_CFG;
  LPC_CCU_STAT_Type CLK_M4_M0APP_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_ADCHS_CFG;
  LPC_CCU_STAT_Type CLK_M4_ADCHS_STAT;
  LPC_CCU1_CFG_161_Type CLK_M4_EEPROM_CFG;
  LPC_CCU_STAT_Type CLK_M4_EEPROM_STAT;
       uint32_t RESERVED5[22];
  LPC_CCU1_CFG_160_Type CLK_M4_WWDT_CFG;              /* +0x500 */
  LPC_CCU_STAT_Type CLK_M4_WWDT_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_USART0_CFG;
  LPC_CCU_STAT_Type CLK_M4_USART0_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_UART1_CFG;
  LPC_CCU_STAT_Type CLK_M4_UART1_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_SSP0_CFG;
  LPC_CCU_STAT_Type CLK_M4_SSP0_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_TIMER0_CFG;
  LPC_CCU_STAT_Type CLK_M4_TIMER0_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_TIMER1_CFG;
  LPC_CCU_STAT_Type CLK_M4_TIMER1_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_SCU_CFG;
  LPC_CCU_STAT_Type CLK_M4_SCU_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_CREG_CFG;
  LPC_CCU_STAT_Type CLK_M4_CREG_STAT;
       uint32_t RESERVED6[48];
  LPC_CCU1_CFG_160_Type CLK_M4_RITIMER_CFG;           /* +0x600 */
  LPC_CCU_STAT_Type CLK_M4_RITIMER_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_USART2_CFG;
  LPC_CCU_STAT_Type CLK_M4_USART2_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_USART3_CFG;
  LPC_CCU_STAT_Type CLK_M4_USART3_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_TIMER2_CFG;
  LPC_CCU_STAT_Type CLK_M4_TIMER2_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_TIMER3_CFG;
  LPC_CCU_STAT_Type CLK_M4_TIMER3_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_SSP1_CFG;
  LPC_CCU_STAT_Type CLK_M4_SSP1_STAT;
  LPC_CCU1_CFG_160_Type CLK_M4_QEI_CFG;
  LPC_CCU_STAT_Type CLK_M4_QEI_STAT;
       uint32_t RESERVED7[50];
  LPC_CCU1_CFG_160_Type CLK_PERIPH_BUS_CFG;           /* +0x700 */
  LPC_CCU_STAT_Type CLK_PERIPH_BUS_STAT;
       uint32_t RESERVED8[2];
  LPC_CCU1_CFG_160_Type CLK_PERIPH_CORE_CFG;          /* +0x710 */
  LPC_CCU_STAT_Type CLK_PERIPH_CORE_STAT;
  LPC_CCU1_CFG_160_Type CLK_PERIPH_SGPIO_CFG;
  LPC_CCU_STAT_Type CLK_PERIPH_SGPIO_STAT;
       uint32_t RESERVED9[56];
  LPC_CCU1_CFG_160_Type CLK_USB0_CFG;                 /* +0x800 */
  LPC_CCU_STAT_Type CLK_USB0_STAT;
       uint32_t RESERVED10[62];
  LPC_CCU1_CFG_160_Type CLK_USB1_CFG;                 /* +0x900 */
  LPC_CCU_STAT_Type CLK_USB1_STAT;
       uint32_t RESERVED11[62];
  LPC_CCU1_CFG_160_Type CLK_SPI_CFG;                  /* +0xa00 */
  LPC_CCU_STAT_Type CLK_SPI_STAT;
       uint32_t RESERVED12[62];
  LPC_CCU1_CFG_160_Type CLK_ADCHS_CFG;                /* +0xb00 */
  LPC_CCU_STAT_Type CLK_ADCHS_STAT;
} LPC_CCU1_Type;

typedef LPC_CCU1_CFG_160_Type LPC_CCU2_CFG_164_Type ;

typedef struct {
  __IO uint32_t PM;
  __I  uint32_t BASE_STAT;
  __I  uint32_t RESERVED_x008[62];
  LPC_CCU2_CFG_164_Type CLK_AUDIO_CFG;
  LPC_CCU_STAT_Type CLK_AUDIO_STAT;
  __I  uint32_t RESERVED_x108[62];
  LPC_CCU2_CFG_164_Type CLK_APB2_USART3_CFG;
  LPC_CCU_STAT_Type CLK_APB2_USART3_STAT;
  __I  uint32_t RESERVED_x208[62];
  LPC_CCU2_CFG_164_Type CLK_APB2_USART2_CFG;
  LPC_CCU_STAT_Type CLK_APB2_USART2_STAT;
  __I  uint32_t RESERVED_x308[62];
  LPC_CCU2_CFG_164_Type CLK_APB0_UART1_CFG;
  LPC_CCU_STAT_Type CLK_APB0_UART1_STAT;
  __I  uint32_t RESERVED_x408[62];
  LPC_CCU2_CFG_164_Type CLK_APB0_USART0_CFG;
  LPC_CCU_STAT_Type CLK_APB0_USART0_STAT;
  __I  uint32_t RESERVED_x508[62];
  LPC_CCU2_CFG_164_Type CLK_APB2_SSP1_CFG;
  LPC_CCU_STAT_Type CLK_APB2_SSP1_STAT;
  __I  uint32_t RESERVED_x608[62];
  LPC_CCU2_CFG_164_Type CLK_APB0_SSP0_CFG;
  LPC_CCU_STAT_Type CLK_APB0_SSP0_STAT;
  __I  uint32_t RESERVED_x708[62];
  LPC_CCU2_CFG_164_Type CLK_SDIO_CFG;
  LPC_CCU_STAT_Type CLK_SDIO_STAT;
} LPC_CCU2_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          RGU                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Reset Generation Unit (RGU) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
       uint32_t RESERVED0[64];          /* 0x000 */
  __O  uint32_t RESET_CTRL[2];          /* 0x100 */
       uint32_t RESERVED1[2];
  __IO uint32_t RESET_STATUS[4];        /* 0x110 */
       uint32_t RESERVED2[12];
  __I  uint32_t RESET_ACTIVE_STATUS[2]; /* 0x150 */
       uint32_t RESERVED3[170];
  __IO uint32_t RESET_EXT_STAT[64];     /* 0x400 */
} LPC_RGU_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          SCU                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx System Control Unit (SCU)/IO configuration Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  union {
    struct {
      /* 0x000 */
      union {
        struct {
          __IO uint32_t SFSP0_0;
          __IO uint32_t SFSP0_1;
        };
        __IO uint32_t SFSP0[2];
      };
      uint32_t RESERVED0[30];

      /* 0x080 */
      union {
        struct {
          __IO uint32_t SFSP1_0;
          __IO uint32_t SFSP1_1;
          __IO uint32_t SFSP1_2;
          __IO uint32_t SFSP1_3;
          __IO uint32_t SFSP1_4;
          __IO uint32_t SFSP1_5;
          __IO uint32_t SFSP1_6;
          __IO uint32_t SFSP1_7;
          __IO uint32_t SFSP1_8;
          __IO uint32_t SFSP1_9;
          __IO uint32_t SFSP1_10;
          __IO uint32_t SFSP1_11;
          __IO uint32_t SFSP1_12;
          __IO uint32_t SFSP1_13;
          __IO uint32_t SFSP1_14;
          __IO uint32_t SFSP1_15;
          __IO uint32_t SFSP1_16;
          __IO uint32_t SFSP1_17;
          __IO uint32_t SFSP1_18;
          __IO uint32_t SFSP1_19;
          __IO uint32_t SFSP1_20;
        };
        __IO uint32_t SFSP1[21];
      };
      uint32_t RESERVED1[11];

      /* 0x100 */
      union {
        struct {
          __IO uint32_t SFSP2_0;
          __IO uint32_t SFSP2_1;
          __IO uint32_t SFSP2_2;
          __IO uint32_t SFSP2_3;
          __IO uint32_t SFSP2_4;
          __IO uint32_t SFSP2_5;
          __IO uint32_t SFSP2_6;
          __IO uint32_t SFSP2_7;
          __IO uint32_t SFSP2_8;
          __IO uint32_t SFSP2_9;
          __IO uint32_t SFSP2_10;
          __IO uint32_t SFSP2_11;
          __IO uint32_t SFSP2_12;
          __IO uint32_t SFSP2_13;
        };
        __IO uint32_t SFSP2[14];
      };
      uint32_t RESERVED2[18];

      /* 0x180 */
      union {
        struct {
          __IO uint32_t SFSP3_0;
          __IO uint32_t SFSP3_1;
          __IO uint32_t SFSP3_2;
          __IO uint32_t SFSP3_3;
          __IO uint32_t SFSP3_4;
          __IO uint32_t SFSP3_5;
          __IO uint32_t SFSP3_6;
          __IO uint32_t SFSP3_7;
          __IO uint32_t SFSP3_8;
        };
        __IO uint32_t SFSP3[9];
      };
      uint32_t RESERVED3[23];

      /* 0x200 */
      union {
        struct {
          __IO uint32_t SFSP4_0;
          __IO uint32_t SFSP4_1;
          __IO uint32_t SFSP4_2;
          __IO uint32_t SFSP4_3;
          __IO uint32_t SFSP4_4;
          __IO uint32_t SFSP4_5;
          __IO uint32_t SFSP4_6;
          __IO uint32_t SFSP4_7;
          __IO uint32_t SFSP4_8;
          __IO uint32_t SFSP4_9;
          __IO uint32_t SFSP4_10;
        };
        __IO uint32_t SFSP4[11];
      };
      uint32_t RESERVED4[21];

      /* 0x280 */
      union {
        struct {
          __IO uint32_t SFSP5_0;
          __IO uint32_t SFSP5_1;
          __IO uint32_t SFSP5_2;
          __IO uint32_t SFSP5_3;
          __IO uint32_t SFSP5_4;
          __IO uint32_t SFSP5_5;
          __IO uint32_t SFSP5_6;
          __IO uint32_t SFSP5_7;
        };
        __IO uint32_t SFSP5[8];
      };
      uint32_t RESERVED5[24];

      /* 0x300 */
      union {
        struct {
          __IO uint32_t SFSP6_0;
          __IO uint32_t SFSP6_1;
          __IO uint32_t SFSP6_2;
          __IO uint32_t SFSP6_3;
          __IO uint32_t SFSP6_4;
          __IO uint32_t SFSP6_5;
          __IO uint32_t SFSP6_6;
          __IO uint32_t SFSP6_7;
          __IO uint32_t SFSP6_8;
          __IO uint32_t SFSP6_9;
          __IO uint32_t SFSP6_10;
          __IO uint32_t SFSP6_11;
          __IO uint32_t SFSP6_12;
        };
        __IO uint32_t SFSP6[13];
      };
      uint32_t RESERVED6[19];

      /* 0x380 */
      union {
        struct {
          __IO uint32_t SFSP7_0;
          __IO uint32_t SFSP7_1;
          __IO uint32_t SFSP7_2;
          __IO uint32_t SFSP7_3;
          __IO uint32_t SFSP7_4;
          __IO uint32_t SFSP7_5;
          __IO uint32_t SFSP7_6;
          __IO uint32_t SFSP7_7;
        };
        __IO uint32_t SFSP7[8];
      };
      uint32_t RESERVED7[24];

      /* 0x400 */
      union {
        struct {
          __IO uint32_t SFSP8_0;
          __IO uint32_t SFSP8_1;
          __IO uint32_t SFSP8_2;
          __IO uint32_t SFSP8_3;
          __IO uint32_t SFSP8_4;
          __IO uint32_t SFSP8_5;
          __IO uint32_t SFSP8_6;
          __IO uint32_t SFSP8_7;
          __IO uint32_t SFSP8_8;
        };
        __IO uint32_t SFSP8[9];
      };
      uint32_t RESERVED8[23];

      /* 0x480 */
      union {
        struct {
          __IO uint32_t SFSP9_0;
          __IO uint32_t SFSP9_1;
          __IO uint32_t SFSP9_2;
          __IO uint32_t SFSP9_3;
          __IO uint32_t SFSP9_4;
          __IO uint32_t SFSP9_5;
          __IO uint32_t SFSP9_6;
        };
        __IO uint32_t SFSP9[7];
      };
      uint32_t RESERVED9[25];

      /* 0x500 */
      union {
        struct {
          __IO uint32_t SFSPA_0;
          __IO uint32_t SFSPA_1;
          __IO uint32_t SFSPA_2;
          __IO uint32_t SFSPA_3;
          __IO uint32_t SFSPA_4;
        };
        __IO uint32_t SFSPA[5];
      };
      uint32_t RESERVEDA[27];

      /* 0x580 */
      union {
        struct {
          __IO uint32_t SFSPB_0;
          __IO uint32_t SFSPB_1;
          __IO uint32_t SFSPB_2;
          __IO uint32_t SFSPB_3;
          __IO uint32_t SFSPB_4;
          __IO uint32_t SFSPB_5;
          __IO uint32_t SFSPB_6;
        };
        __IO uint32_t SFSPB[7];
      };
      uint32_t RESERVEDB[25];

      /* 0x600 */
      union {
        struct {
          __IO uint32_t SFSPC_0;
          __IO uint32_t SFSPC_1;
          __IO uint32_t SFSPC_2;
          __IO uint32_t SFSPC_3;
          __IO uint32_t SFSPC_4;
          __IO uint32_t SFSPC_5;
          __IO uint32_t SFSPC_6;
          __IO uint32_t SFSPC_7;
          __IO uint32_t SFSPC_8;
          __IO uint32_t SFSPC_9;
          __IO uint32_t SFSPC_10;
          __IO uint32_t SFSPC_11;
          __IO uint32_t SFSPC_12;
          __IO uint32_t SFSPC_13;
          __IO uint32_t SFSPC_14;
        };
        __IO uint32_t SFSPC[15];
      };
      uint32_t RESERVEDC[17];

      /* 0x680 */
      union {
        struct {
          __IO uint32_t SFSPD_0;
          __IO uint32_t SFSPD_1;
          __IO uint32_t SFSPD_2;
          __IO uint32_t SFSPD_3;
          __IO uint32_t SFSPD_4;
          __IO uint32_t SFSPD_5;
          __IO uint32_t SFSPD_6;
          __IO uint32_t SFSPD_7;
          __IO uint32_t SFSPD_8;
          __IO uint32_t SFSPD_9;
          __IO uint32_t SFSPD_10;
          __IO uint32_t SFSPD_11;
          __IO uint32_t SFSPD_12;
          __IO uint32_t SFSPD_13;
          __IO uint32_t SFSPD_14;
          __IO uint32_t SFSPD_15;
          __IO uint32_t SFSPD_16;
        };
        __IO uint32_t SFSPD[17];
      };
      uint32_t RESERVEDD[15];

      /* 0x700 */
      union {
        struct {
          __IO uint32_t SFSPE_0;
          __IO uint32_t SFSPE_1;
          __IO uint32_t SFSPE_2;
          __IO uint32_t SFSPE_3;
          __IO uint32_t SFSPE_4;
          __IO uint32_t SFSPE_5;
          __IO uint32_t SFSPE_6;
          __IO uint32_t SFSPE_7;
          __IO uint32_t SFSPE_8;
          __IO uint32_t SFSPE_9;
          __IO uint32_t SFSPE_10;
          __IO uint32_t SFSPE_11;
          __IO uint32_t SFSPE_12;
          __IO uint32_t SFSPE_13;
          __IO uint32_t SFSPE_14;
          __IO uint32_t SFSPE_15;
        };
        __IO uint32_t SFSPE[16];
      };
      uint32_t RESERVEDE[16];

      /* 0x780 */
      union {
        struct {
          __IO uint32_t SFSPF_0;
          __IO uint32_t SFSPF_1;
          __IO uint32_t SFSPF_2;
          __IO uint32_t SFSPF_3;
          __IO uint32_t SFSPF_4;
          __IO uint32_t SFSPF_5;
          __IO uint32_t SFSPF_6;
          __IO uint32_t SFSPF_7;
          __IO uint32_t SFSPF_8;
          __IO uint32_t SFSPF_9;
          __IO uint32_t SFSPF_10;
          __IO uint32_t SFSPF_11;
        };
        __IO uint32_t SFSPF[12];
      };
      uint32_t RESERVEDF[20];
    };
    __IO uint32_t SFSP[16][32];
    __IO uint32_t SFS[512];
  };

  /* 0x800 */
  uint32_t RESERVEDG[256];

  /* 0xc00 */
  union {
    struct {
      __IO uint32_t SFSCLK0;
      __IO uint32_t SFSCLK1;
      __IO uint32_t SFSCLK2;
      __IO uint32_t SFSCLK3;
    };
    __IO uint32_t SFSCLK[4];
  };
  uint32_t RESERVEDH[28];

  /* 0xc80 */
  __IO uint32_t SFSUSB;
  __IO uint32_t SFSI2C0;
  union {
    struct {
      __IO uint32_t ENAIO0;
      __IO uint32_t ENAIO1;
      __IO uint32_t ENAIO2;
    };
    __IO uint32_t ENAIO[3];
  };
  uint32_t RESERVEDI[27];

  /* 0xd00 */
  __IO uint32_t EMCDELAYCLK;
  uint32_t RESERVEDJ[31];

  /* 0xd80 */
  __IO uint32_t SDDELAY;
  uint32_t RESERVEDK[31];

  /* 0xe00 */
  union {
    struct {
      __IO uint32_t PINTSEL0;
      __IO uint32_t PINTSEL1;
    };
    __IO uint32_t PINTSEL[2];
  };
} LPC_SCU_Type;

// ------------------------------------------------------------------------------------------------
// -----                                       GPIO_PORT                                      -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx GPIO Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  union {
    struct {
      __IO uint8_t B0[32];                      /*!< (@ +0x0000) Byte pin registers port 0; pins PIO0_0 to PIO0_31 */
      __IO uint8_t B1[32];                      /*!< (@ +0x0020) Byte pin registers port 1 */
      __IO uint8_t B2[32];                      /*!< (@ +0x0040) Byte pin registers port 2 */
      __IO uint8_t B3[32];                      /*!< (@ +0x0060) Byte pin registers port 3 */
      __IO uint8_t B4[32];                      /*!< (@ +0x0080) Byte pin registers port 4 */
      __IO uint8_t B5[32];                      /*!< (@ +0x00a0) Byte pin registers port 5 */
      __IO uint8_t B6[32];                      /*!< (@ +0x00c0) Byte pin registers port 6 */
      __IO uint8_t B7[32];                      /*!< (@ +0x00e0) Byte pin registers port 7 */
    };
    __IO uint8_t B[256];                        /*!< (@ +0x0000) Byte pin registers port 0-7 */
  };
  __I  uint32_t RESERVED0[960];                 /*!< (@ +0x0100) */
  union {
    struct {
      __IO uint32_t W0[32];                     /*!< (@ +0x1000) Word pin registers port 0 */
      __IO uint32_t W1[32];                     /*!< (@ +0x1080) Word pin registers port 1 */
      __IO uint32_t W2[32];                     /*!< (@ +0x1100) Word pin registers port 2 */
      __IO uint32_t W3[32];                     /*!< (@ +0x1180) Word pin registers port 3 */
      __IO uint32_t W4[32];                     /*!< (@ +0x1200) Word pin registers port 4 */
      __IO uint32_t W5[32];                     /*!< (@ +0x1280) Word pin registers port 5 */
      __IO uint32_t W6[32];                     /*!< (@ +0x1300) Word pin registers port 6 */
      __IO uint32_t W7[32];                     /*!< (@ +0x1380) Word pin registers port 7 */
    };
    __IO uint32_t W[256];                       /*!< (@ +0x1000) Word pin registers port 0-7 */
  };
       uint32_t RESERVED1[768];
  __IO uint32_t DIR[8];     /* 0x2000 */
       uint32_t RESERVED2[24];
  __IO uint32_t MASK[8];    /* 0x2080 */
       uint32_t RESERVED3[24];
  __IO uint32_t PIN[8];     /* 0x2100 */
       uint32_t RESERVED4[24];
  __IO uint32_t MPIN[8];    /* 0x2180 */
       uint32_t RESERVED5[24];
  __IO uint32_t SET[8];     /* 0x2200 */
       uint32_t RESERVED6[24];
  __O  uint32_t CLR[8];     /* 0x2280 */
       uint32_t RESERVED7[24];
  __O  uint32_t NOT[8];     /* 0x2300 */
} LPC_GPIO_Type;

// ------------------------------------------------------------------------------------------------
// -----                                       GPIO_INT                                       -----
// ------------------------------------------------------------------------------------------------

typedef struct {
  __IO uint32_t ISEL;
  __IO uint32_t IENR;
  __I  uint32_t SIENR;
  __I  uint32_t CIENR;
  __IO uint32_t IENF;
  __I  uint32_t SIENF;
  __I  uint32_t CIENF;
  __IO uint32_t RISE;
  __IO uint32_t FALL;
  __IO uint32_t IST;
} LPC_GPIO_INT_Type;

// ------------------------------------------------------------------------------------------------
// -----                                         SGPIO                                        -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Serial GPIO (SGPIO) date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t OUT_MUX_CFG[16];
  __IO uint32_t SGPIO_MUX_CFG[16];
  __IO uint32_t SLICE_MUX_CFG[16];
  __IO uint32_t REG[16];
  __IO uint32_t REG_SS[16];
  __IO uint32_t PRESET[16];
  __IO uint32_t COUNT[16];
  __IO uint32_t POS[16];
  __IO uint32_t MASK_A;
  __IO uint32_t MASK_H;
  __IO uint32_t MASK_I;
  __IO uint32_t MASK_P;
  __I  uint32_t GPIO_INREG;
  __IO uint32_t GPIO_OUTREG;
  __IO uint32_t GPIO_OENREG;
  __IO uint32_t CTRL_ENABLE;
  __IO uint32_t CTRL_DISABLE;
       uint32_t RESERVED0[823];
  __O  uint32_t CLR_EN_0;
  __O  uint32_t SET_EN_0;
  __I  uint32_t ENABLE_0;
  __I  uint32_t STATUS_0;
  __O  uint32_t CLR_STATUS_0;
  __O  uint32_t SET_STATUS_0;
       uint32_t RESERVED1[2];
  __O  uint32_t CLR_EN_1;
  __O  uint32_t SET_EN_1;
  __I  uint32_t ENABLE_1;
  __I  uint32_t STATUS_1;
  __O  uint32_t CLR_STATUS_1;
  __O  uint32_t SET_STATUS_1;
       uint32_t RESERVED2[2];
  __O  uint32_t CLR_EN_2;
  __O  uint32_t SET_EN_2;
  __I  uint32_t ENABLE_2;
  __I  uint32_t STATUS_2;
  __O  uint32_t CLR_STATUS_2;
  __O  uint32_t SET_STATUS_2;
       uint32_t RESERVED3[2];
  __O  uint32_t CLR_EN_3;
  __O  uint32_t SET_EN_3;
  __I  uint32_t ENABLE_3;
  __I  uint32_t STATUS_3;
  __O  uint32_t CLR_STATUS_3;
  __O  uint32_t SET_STATUS_3;
} LPC_SGPIO_Type;

// ------------------------------------------------------------------------------------------------
// -----                                         GPDMA                                        -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx General Purpose DMA (GPDMA) date=1/28/2014 Major revision=1 Minor revision=8
  */

struct LPC_GPDMA_LLI_Type;
typedef struct LPC_GPDMA_LLI_Type LPC_GPDMA_LLI_Type;

struct LPC_GPDMA_LLI_Type {
  uint32_t CSRCADDR;
  uint32_t CDESTADDR;
  uint32_t CLLI;
  uint32_t CCONTROL;
};

typedef struct {
  __IO uint32_t SRCADDR;      /* +0x00 */
  __IO uint32_t DESTADDR;
  __IO uint32_t LLI;
  __IO uint32_t CONTROL;
  __IO uint32_t CONFIG;       /* +0x10 */
       uint32_t RESERVED0[3];
} LPC_GPDMA_Channel_Type;

typedef struct {
  __I  uint32_t INTSTAT;        /* +0x000 */
  __I  uint32_t INTTCSTAT;
  __O  uint32_t INTTCCLR;
  __I  uint32_t INTERRSTAT;
  __O  uint32_t INTERRCLR;      /* +0x010 */
  __I  uint32_t RAWINTTCSTAT;
  __I  uint32_t RAWINTERRSTAT;
  __I  uint32_t ENBLDCHNS;
  __IO uint32_t SOFTBREQ;       /* +0x020 */
  __IO uint32_t SOFTSREQ;
  __IO uint32_t SOFTLBREQ;
  __IO uint32_t SOFTLSREQ;
  __IO uint32_t CONFIG;         /* +0x030 */
  __IO uint32_t SYNC;
       uint32_t RESERVED0[50];
  LPC_GPDMA_Channel_Type CH[8]; /* +0x100 */
} LPC_GPDMA_Type;

// ------------------------------------------------------------------------------------------------
// -----                                         SDMMC                                        -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx SD/MMC Interface Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  uint32_t DESC0;
  uint32_t DESC1;
  uint32_t DESC2;
  uint32_t DESC3;
} LPC_SDMMC_DESC_Type;

typedef struct {
  __IO uint32_t CTRL;
  __IO uint32_t PWREN;
  __IO uint32_t CLKDIV;
  __IO uint32_t CLKSRC;
  __IO uint32_t CLKENA;
  __IO uint32_t TMOUT;
  __IO uint32_t CTYPE;
  __IO uint32_t BLKSIZ;
  __IO uint32_t BYTCNT;
  __IO uint32_t INTMASK;
  __IO uint32_t CMDARG;
  __IO uint32_t CMD;
  __I  uint32_t RESP0;
  __I  uint32_t RESP1;
  __I  uint32_t RESP2;
  __I  uint32_t RESP3;
  __I  uint32_t MINTSTS;
  __IO uint32_t RINTSTS;
  __I  uint32_t STATUS;
  __IO uint32_t FIFOTH;
  __I  uint32_t CDETECT;
  __I  uint32_t WRTPRT;
       uint32_t RESERVED0;
  __I  uint32_t TCBCNT;
  __I  uint32_t TBBCNT;
  __IO uint32_t DEBNCE;
       uint32_t RESERVED1[4];
  __IO uint32_t RST_N;
       uint32_t RESERVED2;
  __IO uint32_t BMOD;
  __O  uint32_t PLDMND;
  __IO uint32_t DBADDR;
  __IO uint32_t IDSTS;
  __IO uint32_t IDINTEN;
  __I  uint32_t DSCADDR;
  __I  uint32_t BUFADDR;
       uint32_t RESERVED3[25];
  __IO uint32_t DATA;
} LPC_SDMMC_Type;

// ------------------------------------------------------------------------------------------------
// -----                                         TIMER                                        -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx SPI Flash Interface (SPIFI) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t CTRL;
  __IO uint32_t CMD;
  __IO uint32_t ADDR;
  __IO uint32_t IDATA;
  __IO uint32_t CLIMIT;
  __IO uint32_t DATA;
  __IO uint32_t MCMD;
  __IO uint32_t STAT;
} LPC_SPIFI_Type;

// ------------------------------------------------------------------------------------------------
// -----                                         TIMER                                        -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Timer0/1/2/3 Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t IR;
  __IO uint32_t TCR;
  __IO uint32_t TC;
  __IO uint32_t PR;
  __IO uint32_t PC;
  __IO uint32_t MCR;
  __IO uint32_t MR[4];
  __IO uint32_t CCR;
  __I  uint32_t CR[4];
  __IO uint32_t EMR;
       uint32_t RESERVED0[12];
  __IO uint32_t CTCR;
} LPC_TIMER_Type;

// ------------------------------------------------------------------------------------------------
// -----                                        MOTOCONPWM                                    -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Motor Control PWM (MOTOCONPWM) Modification date=7/26/2017 Major revision=2 Minor revision=3
  */

typedef struct {
  __I  uint32_t CON;
  __O  uint32_t CON_SET;
  __O  uint32_t CON_CLR;
  __I  uint32_t CAPCON;
  __O  uint32_t CAPCON_SET;
  __O  uint32_t CAPCON_CLR;
  __IO uint32_t TC0;
  __IO uint32_t TC1;
  __IO uint32_t TC2;
  __IO uint32_t LIM0;
  __IO uint32_t LIM1;
  __IO uint32_t LIM2;
  __IO uint32_t MAT0;
  __IO uint32_t MAT1;
  __IO uint32_t MAT2;
  __IO uint32_t DT;
  __IO uint32_t MCCP;
  __I  uint32_t CAP0;
  __I  uint32_t CAP1;
  __I  uint32_t CAP2;
  __I  uint32_t INTEN;
  __O  uint32_t INTEN_SET;
  __O  uint32_t INTEN_CLR;
  __I  uint32_t CNTCON;
  __O  uint32_t CNTCON_SET;
  __O  uint32_t CNTCON_CLR;
  __I  uint32_t INTF;
  __O  uint32_t INTF_SET;
  __O  uint32_t INTF_CLR;
  __O  uint32_t CAP_CLR;
} LPC_MCPWM_Type;

// ------------------------------------------------------------------------------------------------
// -----                                        RITIMER                                       -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Repetitive Interrupt Timer (RIT) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t COMPVAL;
  __IO uint32_t MASK;
  __IO uint32_t CTRL;
  __IO uint32_t COUNTER;
} LPC_RITIMER_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          RTC                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx Real-Time Clock (RTC) Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __O  uint32_t ILR;
       uint32_t RESERVED0;
  __IO uint32_t CCR;
  __IO uint32_t CIIR;
  __IO uint32_t AMR;
  __I  uint32_t CTIME0;
  __I  uint32_t CTIME1;
  __I  uint32_t CTIME2;
  __IO uint32_t SEC;
  __IO uint32_t MIN;
  __IO uint32_t HRS;
  __IO uint32_t DOM;
  __IO uint32_t DOW;
  __IO uint32_t DOY;
  __IO uint32_t MONTH;
  __IO uint32_t YEAR;
  __IO uint32_t CALIBRATION;
       uint32_t RESERVED1[7];
  __IO uint32_t ASEC;
  __IO uint32_t AMIN;
  __IO uint32_t AHRS;
  __IO uint32_t ADOM;
  __IO uint32_t ADOW;
  __IO uint32_t ADOY;
  __IO uint32_t AMON;
  __IO uint32_t AYRS;
} LPC_RTC_Type;

// ------------------------------------------------------------------------------------------------
// -----                                       USART0/2/3                                     -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx USART0/2/3 Modification date=2/18/2015 Major revision=1 Minor revision=9
  */

typedef struct {
  union {
    __I  uint32_t RBR;                      /*!< (@ 0x400xx000) Receiver Buffer Register. Contains the next received character to be read. (DLAB=0) */
    __O  uint32_t THR;                      /*!< (@ 0x400xx000) Transmit Holding Register. The next character to be transmitted is written here. (DLAB=0) */
    __IO uint32_t DLL;                      /*!< (@ 0x400xx000) Divisor Latch LSB. Least significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
  };

  union {
    __IO uint32_t DLM;                      /*!< (@ 0x400xx004) Divisor Latch MSB. Most significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
    __IO uint32_t IER;                      /*!< (@ 0x400xx004) Interrupt Enable Register. Contains individual interrupt enable bits for the 7 potential UART interrupts. (DLAB=0) */
  };

  union {
    __I  uint32_t IIR;                      /*!< (@ 0x400xx008) Interrupt ID Register. Identifies which interrupt(s) are pending. */
    __O  uint32_t FCR;                      /*!< (@ 0x400xx008) FIFO Control Register. Controls UART FIFO usage and modes. */
  };

  __IO uint32_t LCR;                        /*!< (@ 0x400xx00C) Line Control Register. Contains controls for frame formatting and break generation. */
  __I  uint32_t RESERVED0[1];
  __I  uint32_t LSR;                        /*!< (@ 0x400xx014) Line Status Register. Contains flags for transmit and receive status, including line errors. */
  __I  uint32_t RESERVED1[1];
  __IO uint32_t SCR;                        /*!< (@ 0x400xx01C) Scratch Pad Register. Eight-bit temporary storage for software. */
  __IO uint32_t ACR;                        /*!< (@ 0x400xx020) Auto-baud Control Register. Contains controls for the auto-baud feature. */
  __IO uint32_t ICR;                        /*!< (@ 0x400xx024) IrDA control register (USART3 only). */
  __IO uint32_t FDR;                        /*!< (@ 0x400xx028) Fractional Divider Register. Generates a clock input for the baud rate divider. */
  __IO uint32_t OSR;                        /*!< (@ 0x400xx02C) Oversampling register. Controls the degree of oversampling during each bit time. */
  __I  uint32_t RESERVED3[4];
  __IO uint32_t HDEN;                       /*!< (@ 0x400xx040) Half-duplex enable register. */
  __I  uint32_t RESERVED4[1];
  __IO uint32_t SCICTRL;                    /*!< (@ 0x400xx048) Smart card interface control register. */
  __IO uint32_t RS485CTRL;                  /*!< (@ 0x400xx04C) RS-485/EIA-485 Control. Contains controls to configure various aspects of RS-485/EIA-485 modes. */
  __IO uint32_t RS485ADRMATCH;              /*!< (@ 0x400xx050) RS-485/EIA-485 address match. Contains the address match value for RS-485/EIA-485 mode. */
  __IO uint32_t RS485DLY;                   /*!< (@ 0x400xx054) RS-485/EIA-485 direction control delay. */
  __IO uint32_t SYNCCTRL;                   /*!< (@ 0x400xx058) Synchronous mode control register. */
  __IO uint32_t TER;                        /*!< (@ 0x400xx05C) Transmit Enable Register. Turns off UART transmitter for use with software flow control. */
} LPC_USART_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          UART1                                       -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx UART1 Modification date=2/18/2015 Major revision=1 Minor revision=9
  */

typedef struct {
  union {
    __I  uint32_t RBR;                      /*!< (@ 0x40082000) Receiver Buffer Register. Contains the next received character to be read. (DLAB=0) */
    __O  uint32_t THR;                      /*!< (@ 0x40082000) Transmit Holding Register. The next character to be transmitted is written here. (DLAB=0) */
    __IO uint32_t DLL;                      /*!< (@ 0x40082000) Divisor Latch LSB. Least significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
  };

  union {
    __IO uint32_t DLM;                      /*!< (@ 0x40082004) Divisor Latch MSB. Most significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
    __IO uint32_t IER;                      /*!< (@ 0x40082004) Interrupt Enable Register. Contains individual interrupt enable bits for the 7 potential UART interrupts. (DLAB=0) */
  };

  union {
    __I  uint32_t IIR;                      /*!< (@ 0x40082008) Interrupt ID Register. Identifies which interrupt(s) are pending. */
    __O  uint32_t FCR;                      /*!< (@ 0x40082008) FIFO Control Register. Controls UART FIFO usage and modes. */
  };

  __IO uint32_t LCR;                        /*!< (@ 0x4008200C) Line Control Register. Contains controls for frame formatting and break generation. */
  __IO uint32_t MCR;                        /*!< (@ 0x40082010) Modem Control Register. Contains controls for flow control handshaking and loopback mode. */
  __I  uint32_t LSR;                        /*!< (@ 0x40082014) Line Status Register. Contains flags for transmit and receive status, including line errors. */
  __I  uint32_t MSR;                        /*!< (@ 0x40082018) Modem Status Register. Contains handshake signal status flags. */
  __IO uint32_t SCR;                        /*!< (@ 0x4008201C) Scratch Pad Register. Eight-bit temporary storage for software. */
  __IO uint32_t ACR;                        /*!< (@ 0x40082020) Auto-baud Control Register. Contains controls for the auto-baud feature. */
  __I  uint32_t RESERVED0[1];
  __IO uint32_t FDR;                        /*!< (@ 0x40082028) Fractional Divider Register. Generates a clock input for the baud rate divider. */
  __I  uint32_t RESERVED1[8];
  __IO uint32_t RS485CTRL;                  /*!< (@ 0x4008204C) RS-485/EIA-485 Control. Contains controls to configure various aspects of RS-485/EIA-485 modes. */
  __IO uint32_t RS485ADRMATCH;              /*!< (@ 0x40082050) RS-485/EIA-485 address match. Contains the address match value for RS-485/EIA-485 mode. */
  __IO uint32_t RS485DLY;                   /*!< (@ 0x40082054) RS-485/EIA-485 direction control delay. */
  __I  uint32_t RESERVED2[1];
  __IO uint32_t TER;                        /*!< (@ 0x4008205C) Transmit Enable Register. Turns off UART transmitter for use with software flow control. */
} LPC_UART_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          SSP                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx SSP0/1 Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {                            /*!< SSP Structure         */
  __IO uint32_t CR0;                        /*!< Control Register 0. Selects the serial clock rate, bus type, and data size. */
  __IO uint32_t CR1;                        /*!< Control Register 1. Selects master/slave and other modes. */
  __IO uint32_t DR;                         /*!< Data Register. Writes fill the transmit FIFO, and reads empty the receive FIFO. */
  __I  uint32_t SR;                         /*!< Status Register        */
  __IO uint32_t CPSR;                       /*!< Clock Prescale Register */
  __IO uint32_t IMSC;                       /*!< Interrupt Mask Set and Clear Register */
  __I  uint32_t RIS;                        /*!< Raw Interrupt Status Register */
  __I  uint32_t MIS;                        /*!< Masked Interrupt Status Register */
  __IO uint32_t ICR;                        /*!< SSPICR Interrupt Clear Register */
  __IO uint32_t DMACR;                      /*!< SSP DMA control register */
} LPC_SSPx_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          I2S                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx I2S interface Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t DAO;
  __IO uint32_t DAI;
  __O  uint32_t TXFIFO;
  __I  uint32_t RXFIFO;
  __I  uint32_t STATE;
  __IO uint32_t DMA1;
  __IO uint32_t DMA2;
  __IO uint32_t IRQ;
  __IO uint32_t TXRATE;
  __IO uint32_t RXRATE;
  __IO uint32_t TXBITRATE;
  __IO uint32_t RXBITRATE;
  __IO uint32_t TXMODE;
  __IO uint32_t RXMODE;
} LPC_I2S_Type;

// ------------------------------------------------------------------------------------------------
// -----                                          I2C                                         -----
// ------------------------------------------------------------------------------------------------

/**
  * @brief Product name title=UM10503 Chapter title=LPC43xx I2C-bus interface Modification date=1/28/2014 Major revision=1 Minor revision=8
  */

typedef struct {
  __IO uint32_t CONSET;
  __I  uint32_t STAT;
  __IO uint32_t DAT;
  __IO uint32_t ADR0;
  __IO uint32_t SCLH;
  __IO uint32_t SCLL;
  __O  uint32_t CONCLR;
  __IO uint32_t MMCTRL;
  __IO uint32_t ADR1;
  __IO uint32_t ADR2;
  __IO uint32_t ADR3;
  __I  uint32_t DATA_BUFFER;
  __IO uint32_t MASK0;
  __IO uint32_t MASK1;
  __IO uint32_t MASK2;
  __IO uint32_t MASK3;
} LPC_I2Cx_Type;

#define I2C_CONSET_AA       (1 << 2)
#define I2C_CONSET_SI       (1 << 3)
#define I2C_CONSET_STO      (1 << 4)
#define I2C_CONSET_STA      (1 << 5)
#define I2C_CONSET_I2EN     (1 << 6)

#define I2C_CONCLR_AAC      (1 << 2)
#define I2C_CONCLR_SIC      (1 << 3)
#define I2C_CONCLR_STAC     (1 << 5)
#define I2C_CONCLR_I2ENC    (1 << 6)

// ------------------------------------------------------------------------------------------------
// -----                                          ADC                                         -----
// ------------------------------------------------------------------------------------------------

typedef struct {
  __IO uint32_t CR;
  __I  uint32_t GDR;
       uint32_t RESERVED0;
  __IO uint32_t INTEN;
  __I  uint32_t DR[8];
  __I  uint32_t STAT;
} LPC_ADCx_Type;

#if defined ( __CC_ARM   )
  #pragma no_anon_unions
#endif

// ------------------------------------------------------------------------------------------------
// -----                                 Peripheral memory map                                -----
// ------------------------------------------------------------------------------------------------

#define LPC_AHB_BASE              (0x40000000)
#define LPC_SCT_BASE              (LPC_AHB_BASE + 0x00000)
#define LPC_GPDMA_BASE            (LPC_AHB_BASE + 0x02000)
#define LPC_SPIFI_BASE            (LPC_AHB_BASE + 0x03000)
#define LPC_SDMMC_BASE            (LPC_AHB_BASE + 0x04000)
#define LPC_EMC_BASE              (LPC_AHB_BASE + 0x05000)
#define LPC_USB0_BASE             (LPC_AHB_BASE + 0x06000)
#define LPC_USB1_BASE             (LPC_AHB_BASE + 0x07000)
#define LPC_LCD_BASE              (LPC_AHB_BASE + 0x08000)
#define LPC_ETHERNET_BASE         (LPC_AHB_BASE + 0x10000)

#define LPC_RTC_DOMAIN_BASE       (0x40040000)
#define LPC_ALARM_TIMER_BASE      (LPC_RTC_DOMAIN_BASE + 0x0000)
#define LPC_BACKUP_REG_BASE       (LPC_RTC_DOMAIN_BASE + 0x1000)
#define LPC_POWER_MODE_CTRL_BASE  (LPC_RTC_DOMAIN_BASE + 0x2000)
#define LPC_CREG_BASE             (LPC_RTC_DOMAIN_BASE + 0x3000)
#define LPC_EVENT_ROUTER_BASE     (LPC_RTC_DOMAIN_BASE + 0x4000)
#define LPC_OTP_CTRL_BASE         (LPC_RTC_DOMAIN_BASE + 0x5000)
#define LPC_RTC_BASE              (LPC_RTC_DOMAIN_BASE + 0x6000)

#define LPC_CLOCK_RESET_CTRL_BASE (0x40050000)
#define LPC_CGU_BASE              (LPC_CLOCK_RESET_CTRL_BASE + 0x0000)
#define LPC_CCU1_BASE             (LPC_CLOCK_RESET_CTRL_BASE + 0x1000)
#define LPC_CCU2_BASE             (LPC_CLOCK_RESET_CTRL_BASE + 0x2000)
#define LPC_RGU_BASE              (LPC_CLOCK_RESET_CTRL_BASE + 0x3000)

#define LPC_APB0_BASE             (0x40080000)
#define LPC_WWDT_BASE             (LPC_APB0_BASE + 0x0000)
#define LPC_USART0_BASE           (LPC_APB0_BASE + 0x1000)
#define LPC_UART1_BASE            (LPC_APB0_BASE + 0x2000)
#define LPC_SSP0_BASE             (LPC_APB0_BASE + 0x3000)
#define LPC_TIMER0_BASE           (LPC_APB0_BASE + 0x4000)
#define LPC_TIMER1_BASE           (LPC_APB0_BASE + 0x5000)
#define LPC_SCU_BASE              (LPC_APB0_BASE + 0x6000)
#define LPC_GPIO_INT_BASE         (LPC_APB0_BASE + 0x7000)
#define LPC_GPIO_GROUP0_INT_BASE  (LPC_APB0_BASE + 0x8000)
#define LPC_GPIO_GROUP1_INT_BASE  (LPC_APB0_BASE + 0x9000)

#define LPC_APB1_BASE             (0x400a0000)
#define LPC_MCPWM_BASE            (LPC_APB1_BASE + 0x0000)
#define LPC_I2C0_BASE             (LPC_APB1_BASE + 0x1000)
#define LPC_I2S0_BASE             (LPC_APB1_BASE + 0x2000)
#define LPC_I2S1_BASE             (LPC_APB1_BASE + 0x3000)
#define LPC_C_CAN1_BASE           (LPC_APB1_BASE + 0x4000)

#define LPC_APB2_BASE             (0x400c0000)
#define LPC_RITIMER_BASE          (LPC_APB2_BASE + 0x0000)
#define LPC_USART2_BASE           (LPC_APB2_BASE + 0x1000)
#define LPC_USART3_BASE           (LPC_APB2_BASE + 0x2000)
#define LPC_TIMER2_BASE           (LPC_APB2_BASE + 0x3000)
#define LPC_TIMER3_BASE           (LPC_APB2_BASE + 0x4000)
#define LPC_SSP1_BASE             (LPC_APB2_BASE + 0x5000)
#define LPC_QEI_BASE              (LPC_APB2_BASE + 0x6000)
#define LPC_GIMA_BASE             (LPC_APB2_BASE + 0x7000)

#define LPC_APB3_BASE             (0x400e0000)
#define LPC_I2C1_BASE             (LPC_APB3_BASE + 0x0000)
#define LPC_DAC_BASE              (LPC_APB3_BASE + 0x1000)
#define LPC_C_CAN0_BASE           (LPC_APB3_BASE + 0x2000)
#define LPC_ADC0_BASE             (LPC_APB3_BASE + 0x3000)
#define LPC_ADC1_BASE             (LPC_APB3_BASE + 0x4000)

#define LPC_ADCHS_BASE            (0x400f0000)
#define LPC_GPIO_BASE             (0x400f4000)
#define LPC_SPI_BASE              (0x40100000)
#define LPC_SGPIO_BASE            (0x40101000)

#define LPC_BITBAND_BASE          (0x42000000)

#define LPC_SPIFI_DATA_CACHED_BASE  (0x14000000)
#define LPC_SPIFI_DATA_BASE       (0x80000000)

// ------------------------------------------------------------------------------------------------
// -----                                Peripheral declaration                                -----
// ------------------------------------------------------------------------------------------------


#define LPC_GPDMA                 ((LPC_GPDMA_Type          *) LPC_GPDMA_BASE)
#define LPC_SPIFI                 ((LPC_SPIFI_Type          *) LPC_SPIFI_BASE)
#define LPC_SDMMC                 ((LPC_SDMMC_Type          *) LPC_SDMMC_BASE)
#define LPC_CREG                  ((LPC_CREG_Type           *) LPC_CREG_BASE)
#define LPC_RTC                   ((LPC_RTC_Type            *) LPC_RTC_BASE)
#define LPC_CGU                   ((LPC_CGU_Type            *) LPC_CGU_BASE)
#define LPC_CCU1                  ((LPC_CCU1_Type           *) LPC_CCU1_BASE)
#define LPC_CCU2                  ((LPC_CCU2_Type           *) LPC_CCU2_BASE)
#define LPC_RGU                   ((LPC_RGU_Type            *) LPC_RGU_BASE)
#define LPC_USART0                ((LPC_USART_Type          *) LPC_USART0_BASE)
#define LPC_UART1                 ((LPC_UART_Type           *) LPC_UART1_BASE)
#define LPC_SSP0                  ((LPC_SSPx_Type           *) LPC_SSP0_BASE)
#define LPC_TIMER0                ((LPC_TIMER_Type          *) LPC_TIMER0_BASE)
#define LPC_TIMER1                ((LPC_TIMER_Type          *) LPC_TIMER1_BASE)
#define LPC_SCU                   ((LPC_SCU_Type            *) LPC_SCU_BASE)
#define LPC_GPIO_INT              ((LPC_GPIO_INT_Type       *) LPC_GPIO_INT_BASE)
#define LPC_I2C0                  ((LPC_I2Cx_Type           *) LPC_I2C0_BASE)
#define LPC_I2S0                  ((LPC_I2S_Type            *) LPC_I2S0_BASE)
#define LPC_I2S1                  ((LPC_I2S_Type            *) LPC_I2S1_BASE)
#define LPC_RITIMER               ((LPC_RITIMER_Type        *) LPC_RITIMER_BASE)
#define LPC_USART2                ((LPC_USART_Type          *) LPC_USART2_BASE)
#define LPC_USART3                ((LPC_USART_Type          *) LPC_USART3_BASE)
#define LPC_TIMER2                ((LPC_TIMER_Type          *) LPC_TIMER2_BASE)
#define LPC_TIMER3                ((LPC_TIMER_Type          *) LPC_TIMER3_BASE)
#define LPC_MCPWM                 ((LPC_MCPWM_Type          *) LPC_MCPWM_BASE)
#define LPC_SSP1                  ((LPC_SSPx_Type           *) LPC_SSP1_BASE)
#define LPC_I2C1                  ((LPC_I2Cx_Type           *) LPC_I2C1_BASE)
#define LPC_ADC0                  ((LPC_ADCx_Type           *) LPC_ADC0_BASE)
#define LPC_ADC1                  ((LPC_ADCx_Type           *) LPC_ADC1_BASE)
#define LPC_GPIO                  ((LPC_GPIO_Type           *) LPC_GPIO_BASE)
#define LPC_SGPIO                 ((LPC_SGPIO_Type          *) LPC_SGPIO_BASE)

#ifdef __cplusplus
}
#endif

/** @} */ /* End of group LPC43xx */
/** @} */ /* End of group NXP */

#endif  // __LPC43XX_H__
