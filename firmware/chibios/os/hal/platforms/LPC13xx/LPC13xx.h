/****************************************************************************
*   $Id:: LPC13xx.h 7402 2011-05-25 18:48:12Z usb00175                     $
*   Project: NXP LPC13xx software example  
 *
*   Description:
*     CMSIS Cortex-M0 Core Peripheral Access Layer Header File for 
 *     NXP LPC13xx Device Series 
 *
****************************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
****************************************************************************/



#ifndef __LPC13xx_H__
#define __LPC13xx_H__

#ifdef __cplusplus
 extern "C" {
#endif 

/** @addtogroup LPC13xx_Definitions LPC13xx Definitions
  This file defines all structures and symbols for LPC13xx:
    - Registers and bitfields
    - peripheral base address
    - peripheral ID
    - PIO definitions
  @{
*/


/******************************************************************************/
/*                Processor and Core Peripherals                              */
/******************************************************************************/
/** @addtogroup LPC13xx_CMSIS LPC13xx CMSIS Definitions
  Configuration of the Cortex-M3 Processor and Core Peripherals
  @{
*/

/*
 * ==========================================================================
 * ---------- Interrupt Number Definition -----------------------------------
 * ==========================================================================
 */
typedef enum IRQn
{
/******  Cortex-M3 Processor Exceptions Numbers ***************************************************/
  NonMaskableInt_IRQn           = -14,      /*!< 2 Non Maskable Interrupt                         */
  MemoryManagement_IRQn         = -12,      /*!< 4 Cortex-M3 Memory Management Interrupt          */
  BusFault_IRQn                 = -11,      /*!< 5 Cortex-M3 Bus Fault Interrupt                  */
  UsageFault_IRQn               = -10,      /*!< 6 Cortex-M3 Usage Fault Interrupt                */
  SVCall_IRQn                   = -5,       /*!< 11 Cortex-M3 SV Call Interrupt                   */
  DebugMonitor_IRQn             = -4,       /*!< 12 Cortex-M3 Debug Monitor Interrupt             */
  PendSV_IRQn                   = -2,       /*!< 14 Cortex-M3 Pend SV Interrupt                   */
  SysTick_IRQn                  = -1,       /*!< 15 Cortex-M3 System Tick Interrupt               */

/******  LPC13xx Specific Interrupt Numbers *******************************************************/
  WAKEUP0_IRQn                  = 0,        /*!< All I/O pins can be used as wakeup source.       */
  WAKEUP1_IRQn                  = 1,        /*!< There are 40 pins in total for LPC17xx           */
  WAKEUP2_IRQn                  = 2,
  WAKEUP3_IRQn                  = 3,
  WAKEUP4_IRQn                  = 4,   
  WAKEUP5_IRQn                  = 5,        
  WAKEUP6_IRQn                  = 6,        
  WAKEUP7_IRQn                  = 7,        
  WAKEUP8_IRQn                  = 8,        
  WAKEUP9_IRQn                  = 9,        
  WAKEUP10_IRQn                 = 10,       
  WAKEUP11_IRQn                 = 11,       
  WAKEUP12_IRQn                 = 12,       
  WAKEUP13_IRQn                 = 13,       
  WAKEUP14_IRQn                 = 14,       
  WAKEUP15_IRQn                 = 15,       
  WAKEUP16_IRQn                 = 16,       
  WAKEUP17_IRQn                 = 17,       
  WAKEUP18_IRQn                 = 18,       
  WAKEUP19_IRQn                 = 19,       
  WAKEUP20_IRQn                 = 20,       
  WAKEUP21_IRQn                 = 21,       
  WAKEUP22_IRQn                 = 22,       
  WAKEUP23_IRQn                 = 23,       
  WAKEUP24_IRQn                 = 24,       
  WAKEUP25_IRQn                 = 25,       
  WAKEUP26_IRQn                 = 26,       
  WAKEUP27_IRQn                 = 27,       
  WAKEUP28_IRQn                 = 28,       
  WAKEUP29_IRQn                 = 29,       
  WAKEUP30_IRQn                 = 30,       
  WAKEUP31_IRQn                 = 31,       
  WAKEUP32_IRQn                 = 32,       
  WAKEUP33_IRQn                 = 33,       
  WAKEUP34_IRQn                 = 34,       
  WAKEUP35_IRQn                 = 35,       
  WAKEUP36_IRQn                 = 36,       
  WAKEUP37_IRQn                 = 37,       
  WAKEUP38_IRQn                 = 38,       
  WAKEUP39_IRQn                 = 39,       
  I2C_IRQn                      = 40,       /*!< I2C Interrupt                                    */
  TIMER_16_0_IRQn               = 41,       /*!< 16-bit Timer0 Interrupt                          */
  TIMER_16_1_IRQn               = 42,       /*!< 16-bit Timer1 Interrupt                          */
  TIMER_32_0_IRQn               = 43,       /*!< 32-bit Timer0 Interrupt                          */
  TIMER_32_1_IRQn               = 44,       /*!< 32-bit Timer1 Interrupt                          */
  SSP0_IRQn                     = 45,       /*!< SSP Interrupt                                    */
  UART_IRQn                     = 46,       /*!< UART Interrupt                                   */
  USB_IRQn                      = 47,       /*!< USB Regular Interrupt                            */
  USB_FIQn                      = 48,       /*!< USB Fast Interrupt                               */
  ADC_IRQn                      = 49,       /*!< A/D Converter Interrupt                          */
  WDT_IRQn                      = 50,       /*!< Watchdog timer Interrupt                         */  
  BOD_IRQn                      = 51,       /*!< Brown Out Detect(BOD) Interrupt                  */
  EINT3_IRQn                    = 53,       /*!< External Interrupt 3 Interrupt                   */
  EINT2_IRQn                    = 54,       /*!< External Interrupt 2 Interrupt                   */
  EINT1_IRQn                    = 55,       /*!< External Interrupt 1 Interrupt                   */
  EINT0_IRQn                    = 56,       /*!< External Interrupt 0 Interrupt                   */
  SSP1_IRQn                     = 57,       /*!< SSP1 Interrupt                                   */
} IRQn_Type;

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */

/* Configuration of the Cortex-M3 Processor and Core Peripherals */
#define __MPU_PRESENT             1         /*!< MPU present or not                               */
#define __NVIC_PRIO_BITS          3         /*!< Number of Bits used for Priority Levels          */
#define __Vendor_SysTickConfig    0         /*!< Set to 1 if different SysTick Config is used     */

/*@}*/ /* end of group LPC13xx_CMSIS */


#include "core_cm3.h"                       /* Cortex-M3 processor and core peripherals           */
#include "system_LPC13xx.h"                 /* System Header                                      */


/******************************************************************************/
/*                Device Specific Peripheral Registers structures             */
/******************************************************************************/

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/*------------- System Control (SYSCON) --------------------------------------*/
/** @addtogroup LPC13xx_SYSCON LPC13xx System Control Block 
  @{
*/
typedef struct
{
  __IO uint32_t SYSMEMREMAP;            /*!< Offset: 0x000 (R/W)  System memory remap Register */
  __IO uint32_t PRESETCTRL;             /*!< Offset: 0x004 (R/W)  Peripheral reset control Register */
  __IO uint32_t SYSPLLCTRL;             /*!< Offset: 0x008 (R/W)  System PLL control Register */
  __I  uint32_t SYSPLLSTAT;             /*!< Offset: 0x00C (R/ )  System PLL status Register */
  __IO uint32_t USBPLLCTRL;      /* USB PLL control, offset 0x10 */
  __IO uint32_t USBPLLSTAT;
       uint32_t RESERVED0[2];

  __IO uint32_t SYSOSCCTRL;             /*!< Offset: 0x020 (R/W)  System oscillator control Register */
  __IO uint32_t WDTOSCCTRL;             /*!< Offset: 0x024 (R/W)  Watchdog oscillator control Register */
  __IO uint32_t IRCCTRL;                /*!< Offset: 0x028 (R/W)  IRC control Register */
       uint32_t RESERVED1[1];
  __IO uint32_t SYSRSTSTAT;             /*!< Offset: 0x030 (R/W)  System reset status Register */
       uint32_t RESERVED2[3];
  __IO uint32_t SYSPLLCLKSEL;           /*!< Offset: 0x040 (R/W)  System PLL clock source select Register */	
  __IO uint32_t SYSPLLCLKUEN;           /*!< Offset: 0x044 (R/W)  System PLL clock source update enable Register */
  __IO uint32_t USBPLLCLKSEL;
  __IO uint32_t USBPLLCLKUEN;
       uint32_t RESERVED3[8];

  __IO uint32_t MAINCLKSEL;             /*!< Offset: 0x070 (R/W)  Main clock source select Register */
  __IO uint32_t MAINCLKUEN;             /*!< Offset: 0x074 (R/W)  Main clock source update enable Register */
  __IO uint32_t SYSAHBCLKDIV;           /*!< Offset: 0x078 (R/W)  System AHB clock divider Register */
       uint32_t RESERVED4[1];

  __IO uint32_t SYSAHBCLKCTRL;          /*!< Offset: 0x080 (R/W)  System AHB clock control Register */
       uint32_t RESERVED5[4];
  __IO uint32_t SSP0CLKDIV;                 
  __IO uint32_t UARTCLKDIV;
  __IO uint32_t SSP1CLKDIV;             /*!< Offset: 0x09C SSP1 clock divider (R/W) */
       uint32_t RESERVED6[3];

  __IO uint32_t TRACECLKDIV;

  __IO uint32_t SYSTICKCLKDIV;   /* Offset 0xB0 */           
       uint32_t RESERVED7[3];

  __IO uint32_t USBCLKSEL;       /* Offset 0xC0 */ 
  __IO uint32_t USBCLKUEN;
  __IO uint32_t USBCLKDIV;
       uint32_t RESERVED8[1];
  __IO uint32_t WDTCLKSEL;              /*!< Offset: 0x0D0 (R/W)  WDT clock source select Register */
  __IO uint32_t WDTCLKUEN;              /*!< Offset: 0x0D4 (R/W)  WDT clock source update enable Register */
  __IO uint32_t WDTCLKDIV;              /*!< Offset: 0x0D8 (R/W)  WDT clock divider Register */
       uint32_t RESERVED9[1];

  __IO uint32_t CLKOUTCLKSEL;           /*!< Offset: 0x0E0 (R/W)  CLKOUT clock source select Register */
  __IO uint32_t CLKOUTUEN;              /*!< Offset: 0x0E4 (R/W)  CLKOUT clock source update enable Register */
  __IO uint32_t CLKOUTDIV;              /*!< Offset: 0x0E8 (R/W)  CLKOUT clock divider Register */
       uint32_t RESERVED10[5];

  __I  uint32_t PIOPORCAP0;             /*!< Offset: 0x100 (R/ )  POR captured PIO status 0 Register */
  __I  uint32_t PIOPORCAP1;             /*!< Offset: 0x104 (R/ )  POR captured PIO status 1 Register */
       uint32_t RESERVED11[11];
       uint32_t RESERVED12[7];
  __IO uint32_t BODCTRL;                /*!< Offset: 0x150 (R/W)  BOD control Register */
       uint32_t RESERVED13[1];
  __IO uint32_t SYSTCKCAL;              /*!< Offset: 0x158 (R/W)  System tick counter calibration Register */
       uint32_t RESERVED14[41];          

  __IO uint32_t STARTAPRP0;             /*!< Offset: 0x200 (R/W)  Start logic edge control Register 0 */     
  __IO uint32_t STARTERP0;              /*!< Offset: 0x204 (R/W)  Start logic signal enable Register 0 */
  __O  uint32_t STARTRSRP0CLR;          /*!< Offset: 0x208 ( /W)  Start logic reset Register 0 */
  __I  uint32_t STARTSRP0;              /*!< Offset: 0x20C (R/ )  Start logic status Register 0 */
  __IO uint32_t STARTAPRP1;             /*!< Offset: 0x210 (R/W)  Start logic edge control Register 1   (LPC11UXX only) */     
  __IO uint32_t STARTERP1;              /*!< Offset: 0x214 (R/W)  Start logic signal enable Register 1  (LPC11UXX only) */      
  __O  uint32_t STARTRSRP1CLR;          /*!< Offset: 0x218 ( /W)  Start logic reset Register 1          (LPC11UXX only) */
  __I  uint32_t STARTSRP1;              /*!< Offset: 0x21C (R/ )  Start logic status Register 1         (LPC11UXX only) */
       uint32_t RESERVED17[4];

  __IO uint32_t PDSLEEPCFG;             /*!< Offset: 0x230 (R/W)  Power-down states in Deep-sleep mode Register  */
  __IO uint32_t PDAWAKECFG;             /*!< Offset: 0x234 (R/W)  Power-down states after wake-up from Deep-sleep mode Register*/       
  __IO uint32_t PDRUNCFG;               /*!< Offset: 0x238 (R/W)  Power-down configuration Register*/
       uint32_t RESERVED18[110];
  __I  uint32_t DEVICE_ID;              /*!< Offset: 0x3F4 (R/ )  Device ID Register */
} LPC_SYSCON_TypeDef;
/*@}*/ /* end of group LPC13xx_SYSCON */


/*------------- Pin Connect Block (IOCON) --------------------------------*/
/** @addtogroup LPC13xx_IOCON LPC13xx I/O Configuration Block 
  @{
*/
typedef struct
{
  __IO uint32_t PIO2_6;                 /*!< Offset: 0x000 (R/W) I/O configuration for pin PIO2_6 */
       uint32_t RESERVED0[1];
  __IO uint32_t PIO2_0;                 /*!< Offset: 0x008 (R/W)  I/O configuration for pin PIO2_0/DTR/SSEL1 */
  __IO uint32_t RESET_PIO0_0;           /*!< Offset: 0x00C (R/W)  I/O configuration for pin RESET/PIO0_0  */
  __IO uint32_t PIO0_1;                 /*!< Offset: 0x010 (R/W)  I/O configuration for pin PIO0_1/CLKOUT/CT32B0_MAT2 */
  __IO uint32_t PIO1_8;                 /*!< Offset: 0x014 (R/W)  I/O configuration for pin PIO1_8/CT16B1_CAP0 */
       uint32_t RESERVED1[1];
  __IO uint32_t PIO0_2;                 /*!< Offset: 0x01C (R/W)  I/O configuration for pin PIO0_2/SSEL0/CT16B0_CAP0 */

  __IO uint32_t PIO2_7;                 /*!< Offset: 0x020 (R/W)  I/O configuration for pin PIO2_7 */
  __IO uint32_t PIO2_8;                 /*!< Offset: 0x024 (R/W)  I/O configuration for pin PIO2_8 */
  __IO uint32_t PIO2_1;                 /*!< Offset: 0x028 (R/W)  I/O configuration for pin PIO2_1/nDSR/SCK1 */
  __IO uint32_t PIO0_3;                 /*!< Offset: 0x02C (R/W)  I/O configuration for pin PIO0_3 */
  __IO uint32_t PIO0_4;                 /*!< Offset: 0x030 (R/W)  I/O configuration for pin PIO0_4/SCL */
  __IO uint32_t PIO0_5;                 /*!< Offset: 0x034 (R/W)  I/O configuration for pin PIO0_5/SDA */
  __IO uint32_t PIO1_9;                 /*!< Offset: 0x038 (R/W)  I/O configuration for pin PIO1_9/CT16B1_MAT0 */
  __IO uint32_t PIO3_4;                 /*!< Offset: 0x03C (R/W)  I/O configuration for pin PIO3_4 */

  __IO uint32_t PIO2_4;                 /*!< Offset: 0x040 (R/W)  I/O configuration for pin PIO2_4 */
  __IO uint32_t PIO2_5;                 /*!< Offset: 0x044 (R/W)  I/O configuration for pin PIO2_5 */
  __IO uint32_t PIO3_5;                 /*!< Offset: 0x048 (R/W)  I/O configuration for pin PIO3_5 */
  __IO uint32_t PIO0_6;                 /*!< Offset: 0x04C (R/W)  I/O configuration for pin PIO0_6/SCK0 */
  __IO uint32_t PIO0_7;                 /*!< Offset: 0x050 (R/W)  I/O configuration for pin PIO0_7/nCTS */
  __IO uint32_t PIO2_9;                 /*!< Offset: 0x054 (R/W)  I/O configuration for pin PIO2_9 */
  __IO uint32_t PIO2_10;                /*!< Offset: 0x058 (R/W)  I/O configuration for pin PIO2_10 */
  __IO uint32_t PIO2_2;                 /*!< Offset: 0x05C (R/W)  I/O configuration for pin PIO2_2/DCD/MISO1 */

  __IO uint32_t PIO0_8;                 /*!< Offset: 0x060 (R/W)  I/O configuration for pin PIO0_8/MISO0/CT16B0_MAT0 */
  __IO uint32_t PIO0_9;                 /*!< Offset: 0x064 (R/W)  I/O configuration for pin PIO0_9/MOSI0/CT16B0_MAT1 */
  __IO uint32_t SWCLK_PIO0_10;          /*!< Offset: 0x068 (R/W)  I/O configuration for pin SWCLK/PIO0_10/SCK0/CT16B0_MAT2 */
  __IO uint32_t PIO1_10;                /*!< Offset: 0x06C (R/W)  I/O configuration for pin PIO1_10/AD6/CT16B1_MAT1 */
  __IO uint32_t PIO2_11;                /*!< Offset: 0x070 (R/W)  I/O configuration for pin PIO2_11/SCK0 */
  __IO uint32_t R_PIO0_11;              /*!< Offset: 0x074 (R/W)  I/O configuration for pin TDI/PIO0_11/AD0/CT32B0_MAT3 */
  __IO uint32_t R_PIO1_0;               /*!< Offset: 0x078 (R/W)  I/O configuration for pin TMS/PIO1_0/AD1/CT32B1_CAP0 */
  __IO uint32_t R_PIO1_1;               /*!< Offset: 0x07C (R/W)  I/O configuration for pin TDO/PIO1_1/AD2/CT32B1_MAT0 */

  __IO uint32_t R_PIO1_2;               /*!< Offset: 0x080 (R/W)  I/O configuration for pin nTRST/PIO1_2/AD3/CT32B1_MAT1 */
  __IO uint32_t PIO3_0;                 /*!< Offset: 0x084 (R/W)  I/O configuration for pin PIO3_0/nDTR */
  __IO uint32_t PIO3_1;                 /*!< Offset: 0x088 (R/W)  I/O configuration for pin PIO3_1/nDSR */
  __IO uint32_t PIO2_3;                 /*!< Offset: 0x08C (R/W)  I/O configuration for pin PIO2_3/RI/MOSI1 */
  __IO uint32_t SWDIO_PIO1_3;           /*!< Offset: 0x090 (R/W)  I/O configuration for pin SWDIO/PIO1_3/AD4/CT32B1_MAT2 */
  __IO uint32_t PIO1_4;                 /*!< Offset: 0x094 (R/W)  I/O configuration for pin PIO1_4/AD5/CT32B1_MAT3 */
  __IO uint32_t PIO1_11;                /*!< Offset: 0x098 (R/W)  I/O configuration for pin PIO1_11/AD7 */
  __IO uint32_t PIO3_2;                 /*!< Offset: 0x09C (R/W)  I/O configuration for pin PIO3_2/nDCD */

  __IO uint32_t PIO1_5;
  __IO uint32_t PIO1_6;
  __IO uint32_t PIO1_7;
  __IO uint32_t PIO3_3;
  __IO uint32_t SCK_LOC;                /*!< Offset: 0x0B0 SCK pin location select Register (R/W) */
  __IO uint32_t DSR_LOC;                /*!< Offset: 0x0B4 DSR pin location select Register (R/W) */
  __IO uint32_t DCD_LOC;                /*!< Offset: 0x0B8 DCD pin location select Register (R/W) */
  __IO uint32_t RI_LOC;                 /*!< Offset: 0x0BC RI pin location Register (R/W) */
} LPC_IOCON_TypeDef;
/*@}*/ /* end of group LPC13xx_IOCON */


/*------------- Power Management Unit (PMU) --------------------------*/
/** @addtogroup LPC13xx_PMU LPC13xx Power Management Unit
  @{
*/
typedef struct
{
  __IO uint32_t PCON;                   /*!< Offset: 0x000 (R/W)  Power control Register */
  __IO uint32_t GPREG0;                 /*!< Offset: 0x004 (R/W)  General purpose Register 0 */
  __IO uint32_t GPREG1;                 /*!< Offset: 0x008 (R/W)  General purpose Register 1 */
  __IO uint32_t GPREG2;                 /*!< Offset: 0x00C (R/W)  General purpose Register 2 */
  __IO uint32_t GPREG3;                 /*!< Offset: 0x010 (R/W)  General purpose Register 3 */
  __IO uint32_t GPREG4;                 /*!< Offset: 0x014 (R/W)  General purpose Register 4 */
} LPC_PMU_TypeDef;
/*@}*/ /* end of group LPC13xx_PMU */


/*------------- General Purpose Input/Output (GPIO) --------------------------*/
/** @addtogroup LPC13xx_GPIO LPC13xx General Purpose Input/Output 
  @{
*/
typedef struct
{
  union {
    __IO uint32_t MASKED_ACCESS[4096];  /*!< Offset: 0x0000 (R/W) Port data Register for pins PIOn_0 to PIOn_11 */
    struct {
         uint32_t RESERVED0[4095];
    __IO uint32_t DATA;                 /*!< Offset: 0x3FFC (R/W) Port data Register */
    };
  };
       uint32_t RESERVED1[4096];
  __IO uint32_t DIR;                    /*!< Offset: 0x8000 (R/W)  Data direction Register */
  __IO uint32_t IS;                     /*!< Offset: 0x8004 (R/W)  Interrupt sense Register */
  __IO uint32_t IBE;                    /*!< Offset: 0x8008 (R/W)  Interrupt both edges Register */
  __IO uint32_t IEV;                    /*!< Offset: 0x800C (R/W)  Interrupt event Register */
  __IO uint32_t IE;                     /*!< Offset: 0x8010 (R/W)  Interrupt mask Register */
  __I  uint32_t RIS;                    /*!< Offset: 0x8014 (R/ )  Raw interrupt status Register */
  __I  uint32_t MIS;                    /*!< Offset: 0x8018 (R/ )  Masked interrupt status Register */
  __O  uint32_t IC;                     /*!< Offset: 0x801C ( /W)  Interrupt clear Register */
} LPC_GPIO_TypeDef;
/*@}*/ /* end of group LPC13xx_GPIO */


/*------------- Timer (TMR) --------------------------------------------------*/
/** @addtogroup LPC13xx_TMR LPC13xx 16/32-bit Counter/Timer 
  @{
*/
typedef struct
{
  __IO uint32_t IR;                     /*!< Offset: 0x000 (R/W)  Interrupt Register */
  __IO uint32_t TCR;                    /*!< Offset: 0x004 (R/W)  Timer Control Register */
  __IO uint32_t TC;                     /*!< Offset: 0x008 (R/W)  Timer Counter Register */
  __IO uint32_t PR;                     /*!< Offset: 0x00C (R/W)  Prescale Register */
  __IO uint32_t PC;                     /*!< Offset: 0x010 (R/W)  Prescale Counter Register */
  __IO uint32_t MCR;                    /*!< Offset: 0x014 (R/W)  Match Control Register */
  __IO uint32_t MR0;                    /*!< Offset: 0x018 (R/W)  Match Register 0 */
  __IO uint32_t MR1;                    /*!< Offset: 0x01C (R/W)  Match Register 1 */
  __IO uint32_t MR2;                    /*!< Offset: 0x020 (R/W)  Match Register 2 */
  __IO uint32_t MR3;                    /*!< Offset: 0x024 (R/W)  Match Register 3 */
  __IO uint32_t CCR;                    /*!< Offset: 0x028 (R/W)  Capture Control Register */
  __I  uint32_t CR0;                    /*!< Offset: 0x02C (R/ )  Capture Register 0 */
       uint32_t RESERVED1[3];
  __IO uint32_t EMR;                    /*!< Offset: 0x03C (R/W)  External Match Register */
       uint32_t RESERVED2[12];
  __IO uint32_t CTCR;                   /*!< Offset: 0x070 (R/W)  Count Control Register */
  __IO uint32_t PWMC;                   /*!< Offset: 0x074 (R/W)  PWM Control Register */
} LPC_TMR_TypeDef;
/*@}*/ /* end of group LPC13xx_TMR */


/*------------- Universal Asynchronous Receiver Transmitter (UART) -----------*/
/** @addtogroup LPC13xx_UART LPC13xx Universal Asynchronous Receiver/Transmitter 
  @{
*/
typedef struct
{
  union {
  __I  uint32_t  RBR;                   /*!< Offset: 0x000 (R/ )  Receiver Buffer  Register */
  __O  uint32_t  THR;                   /*!< Offset: 0x000 ( /W)  Transmit Holding Register */
  __IO uint32_t  DLL;                   /*!< Offset: 0x000 (R/W)  Divisor Latch LSB */
  };
  union {
  __IO uint32_t  DLM;                   /*!< Offset: 0x004 (R/W)  Divisor Latch MSB */
  __IO uint32_t  IER;                   /*!< Offset: 0x000 (R/W)  Interrupt Enable Register */
  };
  union {
  __I  uint32_t  IIR;                   /*!< Offset: 0x008 (R/ )  Interrupt ID Register */
  __O  uint32_t  FCR;                   /*!< Offset: 0x008 ( /W)  FIFO Control Register */
  };
  __IO uint32_t  LCR;                   /*!< Offset: 0x00C (R/W)  Line Control Register */
  __IO uint32_t  MCR;                   /*!< Offset: 0x010 (R/W)  Modem control Register */
  __I  uint32_t  LSR;                   /*!< Offset: 0x014 (R/ )  Line Status Register */
  __I  uint32_t  MSR;                   /*!< Offset: 0x018 (R/ )  Modem status Register */
  __IO uint32_t  SCR;                   /*!< Offset: 0x01C (R/W)  Scratch Pad Register */
  __IO uint32_t  ACR;                   /*!< Offset: 0x020 (R/W)  Auto-baud Control Register */
       uint32_t  RESERVED0[1];
  __IO uint32_t  FDR;                   /*!< Offset: 0x028 (R/W)  Fractional Divider Register */
       uint32_t  RESERVED1[1];
  __IO uint32_t  TER;                   /*!< Offset: 0x030 (R/W)  Transmit Enable Register */
       uint32_t  RESERVED2[6];
  __IO uint32_t  RS485CTRL;             /*!< Offset: 0x04C (R/W)  RS-485/EIA-485 Control Register */
  __IO uint32_t  ADRMATCH;              /*!< Offset: 0x050 (R/W)  RS-485/EIA-485 address match Register */
  __IO uint32_t  RS485DLY;              /*!< Offset: 0x054 (R/W)  RS-485/EIA-485 direction control delay Register */

} LPC_UART_TypeDef;
/*@}*/ /* end of group LPC13xx_UART */


/*------------- Synchronous Serial Communication (SSP) -----------------------*/
/** @addtogroup LPC13xx_SSP LPC13xx Synchronous Serial Port 
  @{
*/
typedef struct
{
  __IO uint32_t CR0;                    /*!< Offset: 0x000 (R/W)  Control Register 0 */
  __IO uint32_t CR1;                    /*!< Offset: 0x004 (R/W)  Control Register 1 */
  __IO uint32_t DR;                     /*!< Offset: 0x008 (R/W)  Data Register */
  __I  uint32_t SR;                     /*!< Offset: 0x00C (R/ )  Status Register */
  __IO uint32_t CPSR;                   /*!< Offset: 0x010 (R/W)  Clock Prescale Register */
  __IO uint32_t IMSC;                   /*!< Offset: 0x014 (R/W)  Interrupt Mask Set and Clear Register */
  __I  uint32_t RIS;                    /*!< Offset: 0x018 (R/ )  Raw Interrupt Status Register */
  __I  uint32_t MIS;                    /*!< Offset: 0x01C (R/ )  Masked Interrupt Status Register */
  __O  uint32_t ICR;                    /*!< Offset: 0x020 ( /W)  SSPICR Interrupt Clear Register */
} LPC_SSP_TypeDef;
/*@}*/ /* end of group LPC13xx_SSP */


/*------------- Inter-Integrated Circuit (I2C) -------------------------------*/
/** @addtogroup LPC13xx_I2C LPC13xx I2C-Bus Interface 
  @{
*/
typedef struct
{
  __IO uint32_t CONSET;                 /*!< Offset: 0x000 (R/W)  I2C Control Set Register */
  __I  uint32_t STAT;                   /*!< Offset: 0x004 (R/ )  I2C Status Register */
  __IO uint32_t DAT;                    /*!< Offset: 0x008 (R/W)  I2C Data Register */
  __IO uint32_t ADR0;                   /*!< Offset: 0x00C (R/W)  I2C Slave Address Register 0 */
  __IO uint32_t SCLH;                   /*!< Offset: 0x010 (R/W)  SCH Duty Cycle Register High Half Word */
  __IO uint32_t SCLL;                   /*!< Offset: 0x014 (R/W)  SCL Duty Cycle Register Low Half Word */
  __O  uint32_t CONCLR;                 /*!< Offset: 0x018 ( /W)  I2C Control Clear Register */
  __IO uint32_t MMCTRL;                 /*!< Offset: 0x01C (R/W)  Monitor mode control register */
  __IO uint32_t ADR1;                   /*!< Offset: 0x020 (R/W)  I2C Slave Address Register 1 */
  __IO uint32_t ADR2;                   /*!< Offset: 0x024 (R/W)  I2C Slave Address Register 2 */
  __IO uint32_t ADR3;                   /*!< Offset: 0x028 (R/W)  I2C Slave Address Register 3 */
  __I  uint32_t DATA_BUFFER;            /*!< Offset: 0x02C (R/ )  Data buffer Register */
  __IO uint32_t MASK0;                  /*!< Offset: 0x030 (R/W)  I2C Slave address mask register 0 */
  __IO uint32_t MASK1;                  /*!< Offset: 0x034 (R/W)  I2C Slave address mask register 1 */
  __IO uint32_t MASK2;                  /*!< Offset: 0x038 (R/W)  I2C Slave address mask register 2 */
  __IO uint32_t MASK3;                  /*!< Offset: 0x03C (R/W)  I2C Slave address mask register 3 */
} LPC_I2C_TypeDef;
/*@}*/ /* end of group LPC13xx_I2C */


/*------------- Windowed Watchdog Timer (WWDT) -----------------------------------------*/
/** @addtogroup LPC13xx_WWDT LPC13xx Windowed WatchDog Timer 
  @{
*/
typedef struct
{
  __IO uint32_t MOD;
  __IO uint32_t TC;
  __O  uint32_t FEED;
  __I  uint32_t TV;
       uint32_t RESERVED0;
  __IO uint32_t WARNINT;				/*!< Offset: 0x014 Watchdog timer warning int. register (R/W) */
  __IO uint32_t WINDOW;					/*!< Offset: 0x018 Watchdog timer window value register (R/W) */
} LPC_WWDT_TypeDef;
/*@}*/ /* end of group LPC13xx_WWDT */


/*------------- Analog-to-Digital Converter (ADC) ----------------------------*/
/** @addtogroup LPC13xx_ADC LPC13xx Analog-to-Digital Converter 
  @{
*/
typedef struct
{
  __IO uint32_t CR;                     /*!< Offset: 0x000 (R/W)  A/D Control Register */
  __IO uint32_t GDR;                    /*!< Offset: 0x004 (R/W)  A/D Global Data Register */
       uint32_t RESERVED0[1];
  __IO uint32_t INTEN;                  /*!< Offset: 0x00C (R/W)  A/D Interrupt Enable Register */
  __IO uint32_t DR[8];                  /*!< Offset: 0x010 (R/W)  A/D Channel 0..7 Data Register */
  __I  uint32_t STAT;                   /*!< Offset: 0x030 (R/ )  A/D Status Register */
} LPC_ADC_TypeDef;
/*@}*/ /* end of group LPC13xx_ADC */


/*------------- Universal Serial Bus (USB) -----------------------------------*/
/** @addtogroup LPC13xx_USB LPC13xx Universal Serial Bus 
  @{
*/
typedef struct
{
  __I  uint32_t DevIntSt;               /*!< Offset: 0x000 (R/ )  USB Device Interrupt Status Register */
  __IO uint32_t DevIntEn;               /*!< Offset: 0x004 (R/W)  USB Device Interrupt Enable Register */
  __O  uint32_t DevIntClr;              /*!< Offset: 0x008 ( /W)  USB Device Interrupt Clear Register */
  __O  uint32_t DevIntSet;              /*!< Offset: 0x00C ( /W)  USB Device Interrupt Set Register */

  __O  uint32_t CmdCode;                /*!< Offset: 0x010 ( /W)  USB Command Code Register */
  __I  uint32_t CmdData;                /*!< Offset: 0x014 (R/ )  USB Command Data Register */

  __I  uint32_t RxData;                 /*!< Offset: 0x018 (R/ )  USB Receive Data Register */
  __O  uint32_t TxData;                 /*!< Offset: 0x01C ( /W)  USB Transmit Data Register */
  __I  uint32_t RxPLen;                 /*!< Offset: 0x020 (R/ )  USB Receive Packet Length Register */
  __O  uint32_t TxPLen;                 /*!< Offset: 0x024 ( /W)  USB Transmit Packet Length Register */
  __IO uint32_t Ctrl;                   /*!< Offset: 0x028 (R/ )  USB Control Register */
  __O  uint32_t DevFIQSel;              /*!< Offset: 0x02C ( /W)  USB Device FIQ select Register */
} LPC_USB_TypeDef;
/*@}*/ /* end of group LPC13xx_USB */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/* Base addresses                                                             */
#define LPC_FLASH_BASE        (0x00000000UL)
#define LPC_RAM_BASE          (0x10000000UL)
#define LPC_APB0_BASE         (0x40000000UL)
#define LPC_AHB_BASE          (0x50000000UL)

/* APB0 peripherals                                                           */
#define LPC_I2C_BASE          (LPC_APB0_BASE + 0x00000)
#define LPC_WWDT_BASE         (LPC_APB0_BASE + 0x04000)
#define LPC_UART_BASE         (LPC_APB0_BASE + 0x08000)
#define LPC_CT16B0_BASE       (LPC_APB0_BASE + 0x0C000)
#define LPC_CT16B1_BASE       (LPC_APB0_BASE + 0x10000)
#define LPC_CT32B0_BASE       (LPC_APB0_BASE + 0x14000)
#define LPC_CT32B1_BASE       (LPC_APB0_BASE + 0x18000)
#define LPC_ADC_BASE          (LPC_APB0_BASE + 0x1C000)
#define LPC_USB_BASE          (LPC_APB0_BASE + 0x20000)
#define LPC_PMU_BASE          (LPC_APB0_BASE + 0x38000)
#define LPC_SSP0_BASE         (LPC_APB0_BASE + 0x40000)
#define LPC_IOCON_BASE        (LPC_APB0_BASE + 0x44000)
#define LPC_SYSCON_BASE       (LPC_APB0_BASE + 0x48000)
#define LPC_SSP1_BASE         (LPC_APB0_BASE + 0x58000)

/* AHB peripherals                                                            */	
#define LPC_GPIO_BASE         (LPC_AHB_BASE  + 0x00000)
#define LPC_GPIO0_BASE        (LPC_AHB_BASE  + 0x00000)
#define LPC_GPIO1_BASE        (LPC_AHB_BASE  + 0x10000)
#define LPC_GPIO2_BASE        (LPC_AHB_BASE  + 0x20000)
#define LPC_GPIO3_BASE        (LPC_AHB_BASE  + 0x30000)

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
#define LPC_I2C               ((LPC_I2C_TypeDef    *) LPC_I2C_BASE   )
#define LPC_WWDT              ((LPC_WWDT_TypeDef   *) LPC_WWDT_BASE  )
#define LPC_UART              ((LPC_UART_TypeDef   *) LPC_UART_BASE  )
#define LPC_TMR16B0           ((LPC_TMR_TypeDef    *) LPC_CT16B0_BASE)
#define LPC_TMR16B1           ((LPC_TMR_TypeDef    *) LPC_CT16B1_BASE)
#define LPC_TMR32B0           ((LPC_TMR_TypeDef    *) LPC_CT32B0_BASE)
#define LPC_TMR32B1           ((LPC_TMR_TypeDef    *) LPC_CT32B1_BASE)
#define LPC_ADC               ((LPC_ADC_TypeDef    *) LPC_ADC_BASE   )
#define LPC_PMU               ((LPC_PMU_TypeDef    *) LPC_PMU_BASE   )
#define LPC_SSP0              ((LPC_SSP_TypeDef    *) LPC_SSP0_BASE  )
#define LPC_SSP1              ((LPC_SSP_TypeDef    *) LPC_SSP1_BASE  )
#define LPC_IOCON             ((LPC_IOCON_TypeDef  *) LPC_IOCON_BASE )
#define LPC_SYSCON            ((LPC_SYSCON_TypeDef *) LPC_SYSCON_BASE)
#define LPC_USB               ((LPC_USB_TypeDef    *) LPC_USB_BASE   )
#define LPC_GPIO0             ((LPC_GPIO_TypeDef   *) LPC_GPIO0_BASE )
#define LPC_GPIO1             ((LPC_GPIO_TypeDef   *) LPC_GPIO1_BASE )
#define LPC_GPIO2             ((LPC_GPIO_TypeDef   *) LPC_GPIO2_BASE )
#define LPC_GPIO3             ((LPC_GPIO_TypeDef   *) LPC_GPIO3_BASE )

#ifdef __cplusplus
}
#endif

#endif  /* __LPC13xx_H__ */
