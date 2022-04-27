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
 * @file    common/ARMCMx/nvic.h
 * @brief   Cortex-Mx NVIC support macros and structures.
 *
 * @addtogroup COMMON_ARMCMx_NVIC
 * @{
 */

#ifndef _NVIC_H_
#define _NVIC_H_

/**
 * @name System vector numbers
 * @{
 */
#define HANDLER_MEM_MANAGE      0       /**< MEM MANAGE vector id.          */
#define HANDLER_BUS_FAULT       1       /**< BUS FAULT vector id.           */
#define HANDLER_USAGE_FAULT     2       /**< USAGE FAULT vector id.         */
#define HANDLER_RESERVED_3      3
#define HANDLER_RESERVED_4      4
#define HANDLER_RESERVED_5      5
#define HANDLER_RESERVED_6      6
#define HANDLER_SVCALL          7       /**< SVCALL vector id.              */
#define HANDLER_DEBUG_MONITOR   8       /**< DEBUG MONITOR vector id.       */
#define HANDLER_RESERVED_9      9
#define HANDLER_PENDSV          10      /**< PENDSV vector id.              */
#define HANDLER_SYSTICK         11      /**< SYS TCK vector id.             */
/** @} */

typedef volatile uint8_t IOREG8;        /**< 8 bits I/O register type.      */
typedef volatile uint32_t IOREG32;      /**< 32 bits I/O register type.     */

/**
 * @brief NVIC ITCR register.
 */
#define NVIC_ITCR               (*((IOREG32 *)0xE000E004U))

/**
 * @brief Structure representing the SYSTICK I/O space.
 */
typedef struct {
  IOREG32       CSR;
  IOREG32       RVR;
  IOREG32       CVR;
  IOREG32       CBVR;
} CMx_ST;

/**
 * @brief SYSTICK peripheral base address.
 */
#define STBase                  ((CMx_ST *)0xE000E010U)
#define ST_CSR                  (STBase->CSR)
#define ST_RVR                  (STBase->RVR)
#define ST_CVR                  (STBase->CVR)
#define ST_CBVR                 (STBase->CBVR)

#define CSR_ENABLE_MASK         (0x1U << 0)
#define ENABLE_OFF_BITS         (0U << 0)
#define ENABLE_ON_BITS          (1U << 0)
#define CSR_TICKINT_MASK        (0x1U << 1)
#define   TICKINT_DISABLED_BITS (0U << 1)
#define   TICKINT_ENABLED_BITS  (1U << 1)
#define CSR_CLKSOURCE_MASK      (0x1U << 2)
#define   CLKSOURCE_EXT_BITS    (0U << 2)
#define   CLKSOURCE_CORE_BITS   (1U << 2)
#define CSR_COUNTFLAG_MASK      (0x1U << 16)

#define RVR_RELOAD_MASK         (0xFFFFFFU << 0)

#define CVR_CURRENT_MASK        (0xFFFFFFU << 0)

#define CBVR_TENMS_MASK         (0xFFFFFFU << 0)
#define CBVR_SKEW_MASK          (0x1U << 30)
#define CBVR_NOREF_MASK         (0x1U << 31)

/**
 * @brief Structure representing the NVIC I/O space.
 */
typedef struct {
  IOREG32       ISER[8];
  IOREG32       unused1[24];
  IOREG32       ICER[8];
  IOREG32       unused2[24];
  IOREG32       ISPR[8];
  IOREG32       unused3[24];
  IOREG32       ICPR[8];
  IOREG32       unused4[24];
  IOREG32       IABR[8];
  IOREG32       unused5[56];
  IOREG32       IPR[60];
  IOREG32       unused6[644];
  IOREG32       STIR;
} CMx_NVIC;

/**
 * @brief NVIC peripheral base address.
 */
#define NVICBase                ((CMx_NVIC *)0xE000E100U)
#define NVIC_ISER(n)            (NVICBase->ISER[n])
#define NVIC_ICER(n)            (NVICBase->ICER[n])
#define NVIC_ISPR(n)            (NVICBase->ISPR[n])
#define NVIC_ICPR(n)            (NVICBase->ICPR[n])
#define NVIC_IABR(n)            (NVICBase->IABR[n])
#define NVIC_IPR(n)             (NVICBase->IPR[n])
#define NVIC_STIR               (NVICBase->STIR)

/**
 * @brief Structure representing the System Control Block I/O space.
 */
typedef struct {
  IOREG32       CPUID;
  IOREG32       ICSR;
  IOREG32       VTOR;
  IOREG32       AIRCR;
  IOREG32       SCR;
  IOREG32       CCR;
  IOREG32       SHPR[3];
  IOREG32       SHCSR;
  IOREG32       CFSR;
  IOREG32       HFSR;
  IOREG32       DFSR;
  IOREG32       MMFAR;
  IOREG32       BFAR;
  IOREG32       AFSR;
  IOREG32       PFR[2];
  IOREG32       DFR;
  IOREG32       ADR;
  IOREG32       MMFR[4];
  IOREG32       SAR[5];
  IOREG32       unused1[5];
  IOREG32       CPACR;
} CMx_SCB;

/**
 * @brief SCB peripheral base address.
 */
#define SCBBase                 ((CMx_SCB *)0xE000ED00U)
#define SCB_CPUID               (SCBBase->CPUID)
#define SCB_ICSR                (SCBBase->ICSR)
#define SCB_VTOR                (SCBBase->VTOR)
#define SCB_AIRCR               (SCBBase->AIRCR)
#define SCB_SCR                 (SCBBase->SCR)
#define SCB_CCR                 (SCBBase->CCR)
#define SCB_SHPR(n)             (SCBBase->SHPR[n])
#define SCB_SHCSR               (SCBBase->SHCSR)
#define SCB_CFSR                (SCBBase->CFSR)
#define SCB_HFSR                (SCBBase->HFSR)
#define SCB_DFSR                (SCBBase->DFSR)
#define SCB_MMFAR               (SCBBase->MMFAR)
#define SCB_BFAR                (SCBBase->BFAR)
#define SCB_AFSR                (SCBBase->AFSR)
#define SCB_PFR(n)              (SCBBase->PFR[n])
#define SCB_DFR                 (SCBBase->DFR)
#define SCB_ADR                 (SCBBase->ADR)
#define SCB_MMFR(n)             (SCBBase->MMFR[n])
#define SCB_SAR(n)              (SCBBase->SAR[n])
#define SCB_CPACR               (SCBBase->CPACR)

#define ICSR_VECTACTIVE_MASK    (0x1FFU << 0)
#define ICSR_RETTOBASE          (0x1U << 11)
#define ICSR_VECTPENDING_MASK   (0x1FFU << 12)
#define ICSR_ISRPENDING         (0x1U << 22)
#define ICSR_ISRPREEMPT         (0x1U << 23)
#define ICSR_PENDSTCLR          (0x1U << 25)
#define ICSR_PENDSTSET          (0x1U << 26)
#define ICSR_PENDSVCLR          (0x1U << 27)
#define ICSR_PENDSVSET          (0x1U << 28)
#define ICSR_NMIPENDSET         (0x1U << 31)

#define AIRCR_VECTKEY           0x05FA0000U
#define AIRCR_PRIGROUP_MASK     (0x7U << 8)
#define AIRCR_PRIGROUP(n)       ((n) << 8)

/**
 * @brief Structure representing the FPU I/O space.
 */
typedef struct {
  IOREG32       unused1[1];
  IOREG32       FPCCR;
  IOREG32       FPCAR;
  IOREG32       FPDSCR;
  IOREG32       MVFR0;
  IOREG32       MVFR1;
} CMx_FPU;

/**
 * @brief FPU peripheral base address.
 */
#define FPUBase                 ((CMx_FPU *)0xE000EF30U)
#define SCB_FPCCR               (FPUBase->FPCCR)
#define SCB_FPCAR               (FPUBase->FPCAR)
#define SCB_FPDSCR              (FPUBase->FPDSCR)
#define SCB_MVFR0               (FPUBase->MVFR0)
#define SCB_MVFR1               (FPUBase->MVFR1)

#define FPCCR_ASPEN             (0x1U << 31)
#define FPCCR_LSPEN             (0x1U << 30)
#define FPCCR_MONRDY            (0x1U << 8)
#define FPCCR_BFRDY             (0x1U << 6)
#define FPCCR_MMRDY             (0x1U << 5)
#define FPCCR_HFRDY             (0x1U << 4)
#define FPCCR_THREAD            (0x1U << 3)
#define FPCCR_USER              (0x1U << 1)
#define FPCCR_LSPACT            (0x1U << 0)

#define FPDSCR_AHP              (0x1U << 26)
#define FPDSCR_DN               (0x1U << 25)
#define FPDSCR_FZ               (0x1U << 24)
#define FPDSCR_RMODE(n)         ((n##U) << 22)

/**
 * @brief Structure representing the SCS I/O space.
 */
typedef struct {
  IOREG32       DHCSR;
  IOREG32       DCRSR;
  IOREG32       DCRDR;
  IOREG32       DEMCR;
} CMx_SCS;

/**
 * @brief SCS peripheral base address.
 */
#define SCSBase                 ((CMx_SCS *)0xE000EDF0U)
#define SCS_DHCSR               (SCSBase->DHCSR)
#define SCS_DCRSR               (SCSBase->DCRSR)
#define SCS_DCRDR               (SCSBase->DCRDR)
#define SCS_DEMCR               (SCSBase->DEMCR)

#define SCS_DEMCR_TRCENA        (0x1U << 24)

/**
 * @brief Structure representing the DWT I/O space.
 */
typedef struct {
  IOREG32       CTRL;
  IOREG32       CYCCNT;
  IOREG32       CPICNT;
  IOREG32       EXCCNT;
  IOREG32       SLEEPCNT;
  IOREG32       LSUCNT;
  IOREG32       FOLDCNT;
  IOREG32       PCSR;
} CMx_DWT;

/**
 * @brief DWT peripheral base address.
 */
#define DWTBase                 ((CMx_DWT *)0xE0001000U)
#define DWT_CTRL                (DWTBase->CTRL)
#define DWT_CYCCNT              (DWTBase->CYCCNT)
#define DWT_CPICNT              (DWTBase->CPICNT)
#define DWT_EXCCNT              (DWTBase->EXCCNT)
#define DWT_SLEEPCNT            (DWTBase->SLEEPCNT)
#define DWT_LSUCNT              (DWTBase->LSUCNT)
#define DWT_FOLDCNT             (DWTBase->FOLDCNT)
#define DWT_PCSR                (DWTBase->PCSR)

#define DWT_CTRL_CYCCNTENA      (0x1U << 0)

#ifdef __cplusplus
extern "C" {
#endif
  void nvicEnableVector(uint32_t n, uint32_t prio);
  void nvicDisableVector(uint32_t n);
  void nvicSetSystemHandlerPriority(uint32_t handler, uint32_t prio);
#ifdef __cplusplus
}
#endif

#endif /* _NVIC_H_ */

/** @} */
