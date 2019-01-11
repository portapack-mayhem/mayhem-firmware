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

/** @addtogroup lpc43xx_m4
  * @{
  */

#ifndef __LPC43XX_M4_H
#define __LPC43XX_M4_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup Configuration_section_for_CMSIS
  * @{
  */

/**
 * @brief Configuration of the Cortex-M4 Processor and Core Peripherals
 */
#define __CM4_REV                 0x0001  /*!< Core revision r0p1                            */
#define __MPU_PRESENT             1       /*!< LPC43XX M4 provides an MPU                    */
#define __NVIC_PRIO_BITS          3       /*!< LPC43XX M4 uses 3 Bits for the Priority Levels*/
#define __Vendor_SysTickConfig    0       /*!< Set to 1 if different SysTick Config is used  */
#define __FPU_PRESENT             1       /*!< FPU present                                   */

/**
 * @brief LPC43XX M4 Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */
typedef enum IRQn {
/******  Cortex-M4 Processor Exceptions Numbers ****************************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                          */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt                           */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                                   */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt                                 */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                                    */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt                              */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                                    */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt                                */
/******  LPC43xx M4 specific Interrupt Numbers *****************************************************************/
  DAC_IRQn                    = 0,      /*!< 16 DAC                                                            */
  M0CORE_IRQn                 = 1,      /*!< 17 Cortex-M0APP; Latched TXEV; for M4-M0APP communication         */
  DMA_IRQn                    = 2,      /*!< 18 DMA                                                            */
  /* 3: 19 Reserved */
  /* 4: 20 ORed flash bank A, flash bank B, EEPROM interrupts */
  ETHERNET_IRQn               = 5,      /*!< 21 Ethernet                                                       */
  SDIO_IRQn                   = 6,      /*!< 22 SD/MMC                                                         */
  LCD_IRQn                    = 7,      /*!< 23 LCD                                                            */
  USB0_IRQn                   = 8,      /*!< 24 OTG interrupt                                                  */
  USB1_IRQn                   = 9,      /*!< 25 USB1                                                           */
  SCT_IRQn                    = 10,     /*!< 26 SCT combined interrupt                                         */
  RITIMER_IRQn                = 11,     /*!< 27 RITIMER                                                        */
  TIMER0_IRQn                 = 12,     /*!< 28 TIMER0                                                         */
  TIMER1_IRQn                 = 13,     /*!< 29 TIMER1                                                         */
  TIMER2_IRQn                 = 14,     /*!< 30 TIMER2                                                         */
  TIMER3_IRQn                 = 15,     /*!< 31 TIMER3                                                         */
  MCPWM_IRQn                  = 16,     /*!< 32 Motor control PWM                                              */
  ADC0_IRQn                   = 17,     /*!< 33 ADC0                                                           */
  I2C0_IRQn                   = 18,     /*!< 34 I2C0                                                           */
  I2C1_IRQn                   = 19,     /*!< 35 I2C1                                                           */
  SPI_IRQn                    = 20,     /*!< 36 SPI                                                            */
  ADC1_IRQn                   = 21,     /*!< 37 ADC1                                                           */
  SSP0_IRQn                   = 22,     /*!< 38 SSP0                                                           */
  SSP1_IRQn                   = 23,     /*!< 39 SSP1                                                           */
  USART0_IRQn                 = 24,     /*!< 40 USART0                                                         */
  UART1_IRQn                  = 25,     /*!< 41 Combined UART interrupt with Modem interrupt                   */
  USART2_IRQn                 = 26,     /*!< 42 USART2                                                         */
  USART3_IRQn                 = 27,     /*!< 43 Combined USART interrupt with IrDA interrupt                   */
  I2S0_IRQn                   = 28,     /*!< 44 I2S0                                                           */
  I2S1_IRQn                   = 29,     /*!< 45 I2S1                                                           */
  SPIFI_IRQn                  = 30,     /*!< 46 SPIFI                                                          */
  SGPIO_IRQn                  = 31,     /*!< 47 SGPIO                                                          */
  PIN_INT0_IRQn               = 32,     /*!< 48 GPIO pin interrupt 0                                           */
  PIN_INT1_IRQn               = 33,     /*!< 49 GPIO pin interrupt 1                                           */
  PIN_INT2_IRQn               = 34,     /*!< 50 GPIO pin interrupt 2                                           */
  PIN_INT3_IRQn               = 35,     /*!< 51 GPIO pin interrupt 3                                           */
  PIN_INT4_IRQn               = 36,     /*!< 52 GPIO pin interrupt 4                                           */
  PIN_INT5_IRQn               = 37,     /*!< 53 GPIO pin interrupt 5                                           */
  PIN_INT6_IRQn               = 38,     /*!< 54 GPIO pin interrupt 6                                           */
  PIN_INT7_IRQn               = 39,     /*!< 55 GPIO pin interrupt 7                                           */
  GINT0_IRQn                  = 40,     /*!< 56 GPIO global interrupt 0                                        */
  GINT1_IRQn                  = 41,     /*!< 57 GPIO global interrupt 1                                        */
  EVENTROUTER_IRQn            = 42,     /*!< 58 Event router                                                   */
  C_CAN_IRQn                  = 43,     /*!< 59 C_CAN1                                                         */
  /* 44: 60 Reserved */
  /* 45: 61 ADCHS combined */
  ATIMER_IRQn                 = 46,     /*!< 62 Alarm timer                                                    */
  RTC_IRQn                    = 47,     /*!< 63 RTC                                                            */
  /* 48: 64 Reserved */
  WWDT_IRQn                   = 49,     /*!< 65 WWDT                                                           */
  /* 50: 66 TXEV instruction from the M0 subsystem core */
  C_CAN0_IRQn                 = 51,     /*!< 67 C_CAN0                                                         */
  QEI_IRQn                    = 52,     /*!< 68 QEI                                                            */
} IRQn_Type;

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "core_cm4.h"             /* Cortex-M4 processor and core peripherals */
#include "lpc43xx.inc"
#include "lpc43xx.h"

#ifdef __cplusplus

/* NOTE: Override old, misbehaving SIMD #defines */

#define __SIMD32_TYPE int32_t
#define __SIMD32(addr)  (*(__SIMD32_TYPE **) & (addr))
#define _SIMD32_OFFSET(addr) (*(__SIMD32_TYPE *) (addr))

/* Overload of __SXTB16() to add ROR argument, since using __ROR() as an
 * argument to the existing __SXTB16() doesn't produce optimum/sane code.
 */
__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SXTB16(uint32_t rm, uint32_t ror)
{
  int32_t rd;
  __ASM volatile ("sxtb16 %0, %1, ror %2" : "=r" (rd) : "r" (rm), "I" (ror));
  return rd;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SXTH(uint32_t rm, uint32_t ror)
{
  int32_t rd;
  __ASM volatile ("sxth %0, %1, ror %2" : "=r" (rd) : "r" (rm), "I" (ror));
  return rd;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMLATB(uint32_t rm, uint32_t rs, uint32_t rn) {
  int32_t rd;
  __ASM volatile ("smlatb %0, %1, %2, %3" : "=r" (rd) : "r" (rm), "r" (rs), "r" (rn));
  return rd;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMLABB(uint32_t rm, uint32_t rs, uint32_t rn) {
  int32_t rd;
  __ASM volatile("smlabb %0, %1, %2, %3" : "=r" (rd) : "r" (rm), "r" (rs), "r" (rn));
  return rd;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SXTAH(uint32_t rn, uint32_t rm, uint32_t ror) {
  int32_t rd;
  __ASM volatile("sxtah %0, %1, %2, ror %3" : "=r" (rd) : "r" (rn), "r" (rm), "I" (ror));
  return rd;
}

/* NOTE: BFI is kinda weird because it modifies RD, copy __SMLALD style? */
__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __BFI(uint32_t rd, uint32_t rn, uint32_t lsb, uint32_t width) {
  __ASM volatile("bfi %0, %1, %2, %3" : "+r" (rd) : "r" (rn), "I" (lsb), "I" (width));
  return rd;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMULBB(uint32_t op1, uint32_t op2) {
  int32_t result;
  __ASM volatile ("smulbb %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
  return result;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMULBT(uint32_t op1, uint32_t op2) {
  int32_t result;
  __ASM volatile ("smulbt %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
  return result;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMULTB(uint32_t op1, uint32_t op2) {
  int32_t result;
  __ASM volatile ("smultb %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
  return result;
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMULTT(uint32_t op1, uint32_t op2) {
  int32_t result;
  __ASM volatile ("smultt %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
  return result;
}

#undef __SMULL

__attribute__( ( always_inline ) ) static inline int64_t __SMULL (int32_t op1, int32_t op2)
{
  union llreg_u{
    uint32_t w32[2];
    int64_t w64;
  } llr;

  __asm volatile ("smull %0, %1, %2, %3" : "=r" (llr.w32[0]), "=r" (llr.w32[1]): "r" (op1), "r" (op2));

  return(llr.w64);
}

#undef __SMLALD

__attribute__( ( always_inline ) ) static inline int64_t __SMLALD (uint32_t op1, uint32_t op2, int64_t acc)
{
  union llreg_u{
    uint32_t w32[2];
    int64_t w64;
  } llr;
  llr.w64 = acc;

  __asm volatile ("smlald %0, %1, %2, %3" : "=r" (llr.w32[0]), "=r" (llr.w32[1]): "r" (op1), "r" (op2) , "0" (llr.w32[0]), "1" (llr.w32[1]) );

  return(llr.w64);
}

#undef __SMLALDX

__attribute__( ( always_inline ) ) static inline int64_t __SMLALDX (uint32_t op1, uint32_t op2, int64_t acc)
{
  union llreg_u{
    uint32_t w32[2];
    int64_t w64;
  } llr;
  llr.w64 = acc;

  __asm volatile ("smlaldx %0, %1, %2, %3" : "=r" (llr.w32[0]), "=r" (llr.w32[1]): "r" (op1), "r" (op2) , "0" (llr.w32[0]), "1" (llr.w32[1]) );

  return(llr.w64);
}

#undef __SMLSLD

__attribute__( ( always_inline ) ) static inline int64_t __SMLSLD (uint32_t op1, uint32_t op2, int64_t acc)
{
  union llreg_u{
    uint32_t w32[2];
    int64_t w64;
  } llr;
  llr.w64 = acc;

  __asm volatile ("smlsld %0, %1, %2, %3" : "=r" (llr.w32[0]), "=r" (llr.w32[1]): "r" (op1), "r" (op2) , "0" (llr.w32[0]), "1" (llr.w32[1]) );

  return(llr.w64);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE int32_t __SMMULR (int32_t op1, int32_t op2)
{
  uint32_t result;

  __ASM volatile ("smmulr %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
  return(result);
}

#endif /* __cplusplus */

#endif /* __LPC43XX_M4_H */

/**
  * @}
  */

/**
  * @}
  */
