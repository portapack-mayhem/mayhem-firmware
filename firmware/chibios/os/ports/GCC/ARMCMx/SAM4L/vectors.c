/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    GCC/ARMCMx/SAM4L/vectors.c
 * @brief   Interrupt vectors for the ATSAM4L family.
 *
 * @defgroup ARMCMx_SAM4L_VECTORS ATSAM4L Interrupt Vectors
 * @ingroup ARMCMx_SPECIFIC
 * @details Interrupt vectors for the ATSAM4L family.
 * @{
 */

#include "ch.h"

/**
 * @brief   Type of an IRQ vector.
 */
typedef void  (*irq_vector_t)(void);

/**
 * @brief   Type of a structure representing the whole vectors table.
 */
typedef struct {
  uint32_t      *init_stack;
  irq_vector_t  reset_vector;
  irq_vector_t  nmi_vector;
  irq_vector_t  hardfault_vector;
  irq_vector_t  memmanage_vector;
  irq_vector_t  busfault_vector;
  irq_vector_t  usagefault_vector;
  irq_vector_t  vector1c;
  irq_vector_t  vector20;
  irq_vector_t  vector24;
  irq_vector_t  vector28;
  irq_vector_t  svcall_vector;
  irq_vector_t  debugmonitor_vector;
  irq_vector_t  vector34;
  irq_vector_t  pendsv_vector;
  irq_vector_t  systick_vector;
  irq_vector_t  vectors[80];
} vectors_t;

#if !defined(__DOXYGEN__)
extern uint32_t __main_stack_end__;
extern void ResetHandler(void);
extern void NMIVector(void);
extern void HardFaultVector(void);
extern void MemManageVector(void);
extern void BusFaultVector(void);
extern void UsageFaultVector(void);
extern void Vector1C(void);
extern void Vector20(void);
extern void Vector24(void);
extern void Vector28(void);
extern void SVCallVector(void);
extern void DebugMonitorVector(void);
extern void Vector34(void);
extern void PendSVVector(void);
extern void SysTickVector(void);
extern void HFLASHC_Handler(void);
extern void PDCA_0_Handler(void);
extern void PDCA_1_Handler(void);
extern void PDCA_2_Handler(void);
extern void PDCA_3_Handler(void);
extern void PDCA_4_Handler(void);
extern void PDCA_5_Handler(void);
extern void PDCA_6_Handler(void);
extern void PDCA_7_Handler(void);
extern void PDCA_8_Handler(void);
extern void PDCA_9_Handler(void);
extern void PDCA_10_Handler(void);
extern void PDCA_11_Handler(void);
extern void PDCA_12_Handler(void);
extern void PDCA_13_Handler(void);
extern void PDCA_14_Handler(void);
extern void PDCA_15_Handler(void);
extern void CRCCU_Handler(void);
extern void USBC_Handler(void);
extern void PEVC_TR_Handler(void);
extern void PEVC_OV_Handler(void);
extern void AESA_Handler(void);
extern void PM_Handler(void);
extern void SCIF_Handler(void);
extern void FREQM_Handler(void);
extern void GPIO_0_Handler(void);
extern void GPIO_1_Handler(void);
extern void GPIO_2_Handler(void);
extern void GPIO_3_Handler(void);
extern void GPIO_4_Handler(void);
extern void GPIO_5_Handler(void);
extern void GPIO_6_Handler(void);
extern void GPIO_7_Handler(void);
extern void GPIO_8_Handler(void);
extern void GPIO_9_Handler(void);
extern void GPIO_10_Handler(void);
extern void GPIO_11_Handler(void);
extern void BPM_Handler(void);
extern void BSCIF_Handler(void);
extern void AST_ALARM_Handler(void);
extern void AST_PER_Handler(void);
extern void AST_OVF_Handler(void);
extern void AST_READY_Handler(void);
extern void AST_CLKREADY_Handler(void);
extern void WDT_Handler(void);
extern void EIC_1_Handler(void);
extern void EIC_2_Handler(void);
extern void EIC_3_Handler(void);
extern void EIC_4_Handler(void);
extern void EIC_5_Handler(void);
extern void EIC_6_Handler(void);
extern void EIC_7_Handler(void);
extern void EIC_8_Handler(void);
extern void IISC_Handler(void);
extern void SPI_Handler(void);
extern void TC00_Handler(void);
extern void TC01_Handler(void);
extern void TC02_Handler(void);
extern void TC010_Handler(void);
extern void TC011_Handler(void);
extern void TC012_Handler(void);
extern void TWIM0_Handler(void);
extern void TWIS0_Handler(void);
extern void TWIM1_Handler(void);
extern void TWIS1_Handler(void);
extern void USART0_Handler(void);
extern void USART1_Handler(void);
extern void USART2_Handler(void);
extern void USART3_Handler(void);
extern void ADCIFE_Handler(void);
extern void DACC_Handler(void);
extern void ACIFC_Handler(void);
extern void ABDACB_Handler(void);
extern void TRNG_Handler(void);
extern void PARC_Handler(void);
extern void CATB_Handler(void);
extern void Dummy_Handler(void);
extern void TWIM2_Handler(void);
extern void TWIM3_Handler(void);
extern void LCDCA_Handler(void);
#endif

/**
 * @brief   STM32 vectors table.
 */
#if !defined(__DOXYGEN__)
__attribute__ ((section("vectors")))
#endif
vectors_t _vectors = {
  &__main_stack_end__,ResetHandler,       NMIVector,          HardFaultVector,
  MemManageVector,    BusFaultVector,     UsageFaultVector,   Vector1C,
  Vector20,           Vector24,           Vector28,           SVCallVector,
  DebugMonitorVector, Vector34,           PendSVVector,       SysTickVector,
  {
    HFLASHC_Handler,    PDCA_0_Handler,     PDCA_1_Handler,     PDCA_2_Handler,
    PDCA_3_Handler,	    PDCA_4_Handler,     PDCA_5_Handler,     PDCA_6_Handler,
    PDCA_7_Handler,     PDCA_8_Handler,     PDCA_9_Handler,     PDCA_10_Handler,
    PDCA_11_Handler,    PDCA_12_Handler,    PDCA_13_Handler,    PDCA_14_Handler,
    PDCA_15_Handler,    CRCCU_Handler,      USBC_Handler,       PEVC_TR_Handler,
    PEVC_OV_Handler,    AESA_Handler,       PM_Handler,         SCIF_Handler,
    FREQM_Handler,      GPIO_0_Handler,     GPIO_1_Handler,     GPIO_2_Handler,
    GPIO_3_Handler,     GPIO_4_Handler,     GPIO_5_Handler,     GPIO_6_Handler,
    GPIO_7_Handler,     GPIO_8_Handler,     GPIO_9_Handler,     GPIO_10_Handler,
    GPIO_11_Handler,    BPM_Handler,        BSCIF_Handler,      AST_ALARM_Handler,
    AST_PER_Handler,    AST_OVF_Handler,    AST_READY_Handler,  AST_CLKREADY_Handler,
    WDT_Handler,        EIC_1_Handler,      EIC_2_Handler,      EIC_3_Handler,
    EIC_4_Handler,      EIC_5_Handler,      EIC_6_Handler,      EIC_7_Handler,
    EIC_8_Handler,      IISC_Handler,       SPI_Handler,        TC00_Handler,
    TC01_Handler,       TC02_Handler,       TC010_Handler,      TC011_Handler,
    TC012_Handler,      TWIM0_Handler,      TWIS0_Handler,      TWIM1_Handler,
    TWIS1_Handler,      USART0_Handler,     USART1_Handler,     USART2_Handler,
    USART3_Handler,     ADCIFE_Handler,     DACC_Handler,       ACIFC_Handler,
    ABDACB_Handler,     TRNG_Handler,       PARC_Handler,       CATB_Handler,
    Dummy_Handler,      TWIM2_Handler,      TWIM3_Handler,      LCDCA_Handler
  }
};

/**
 * @brief   Unhandled exceptions handler.
 * @details Any undefined exception vector points to this function by default.
 *          This function simply stops the system into an infinite loop.
 *
 * @notapi
 */
#if !defined(__DOXYGEN__)
__attribute__ ((naked))
#endif
void _unhandled_exception(void) {

  while (TRUE)
    ;
}

void NMIVector(void) __attribute__((weak, alias("_unhandled_exception")));
void HardFaultVector(void) __attribute__((weak, alias("_unhandled_exception")));
void MemManageVector(void) __attribute__((weak, alias("_unhandled_exception")));
void BusFaultVector(void) __attribute__((weak, alias("_unhandled_exception")));
void UsageFaultVector(void) __attribute__((weak, alias("_unhandled_exception")));
void Vector1C(void) __attribute__((weak, alias("_unhandled_exception")));
void Vector20(void) __attribute__((weak, alias("_unhandled_exception")));
void Vector24(void) __attribute__((weak, alias("_unhandled_exception")));
void Vector28(void) __attribute__((weak, alias("_unhandled_exception")));
void SVCallVector(void) __attribute__((weak, alias("_unhandled_exception")));
void DebugMonitorVector(void) __attribute__((weak, alias("_unhandled_exception")));
void Vector34(void) __attribute__((weak, alias("_unhandled_exception")));
void PendSVVector(void) __attribute__((weak, alias("_unhandled_exception")));
void SysTickVector(void) __attribute__((weak, alias("_unhandled_exception")));
void HFLASHC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_0_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_2_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_3_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_4_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_5_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_6_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_7_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_8_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_9_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_10_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_11_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_12_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_13_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_14_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PDCA_15_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void CRCCU_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void USBC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PEVC_TR_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PEVC_OV_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AESA_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PM_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void SCIF_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void FREQM_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_0_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_2_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_3_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_4_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_5_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_6_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_7_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_8_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_9_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_10_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void GPIO_11_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void BPM_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void BSCIF_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AST_ALARM_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AST_PER_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AST_OVF_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AST_READY_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void AST_CLKREADY_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void WDT_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_2_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_3_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_4_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_5_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_6_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_7_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void EIC_8_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void IISC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void SPI_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC00_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC01_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC02_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC010_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC011_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TC012_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIM0_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIS0_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIM1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIS1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void USART0_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void USART1_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void USART2_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void USART3_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void ADCIFE_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void DACC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void ACIFC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void ABDACB_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TRNG_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void PARC_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void CATB_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void Dummy_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIM2_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void TWIM3_Handler(void) __attribute__((weak, alias("_unhandled_exception")));
void LCDCA_Handler(void) __attribute__((weak, alias("_unhandled_exception")));

/** @} */
