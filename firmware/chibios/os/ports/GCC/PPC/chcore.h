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
 * @file    PPC/chcore.h
 * @brief   PowerPC architecture port macros and structures.
 *
 * @addtogroup PPC_CORE
 * @{
 */

#ifndef _CHCORE_H_
#define _CHCORE_H_

#if CH_DBG_ENABLE_STACK_CHECK
#error "option CH_DBG_ENABLE_STACK_CHECK not supported by this port"
#endif

/*===========================================================================*/
/* Port constants (common).                                                  */
/*===========================================================================*/

/* Added to make the header stand-alone when included from asm.*/
#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE        (!FALSE)
#endif

/**
 * @name    Supported core variants
 * @{
 */
#define PPC_VARIANT_e200z0              200
#define PPC_VARIANT_e200z3              203
#define PPC_VARIANT_e200z4              204
/** @} */

#include "vectors.h"
#include "ppcparams.h"

/*===========================================================================*/
/* Port macros (common).                                                     */
/*===========================================================================*/

/*===========================================================================*/
/* Port configurable parameters (common).                                    */
/*===========================================================================*/

/**
 * @brief   Use VLE instruction set.
 * @note    This parameter is usually set in the Makefile.
 */
#if !defined(PPC_USE_VLE)
#define PPC_USE_VLE                     TRUE
#endif

/**
 * @brief   Enables the use of the @p WFI instruction.
 */
#if !defined(PPC_ENABLE_WFI_IDLE)
#define PPC_ENABLE_WFI_IDLE             FALSE
#endif

/*===========================================================================*/
/* Port derived parameters (common).                                         */
/*===========================================================================*/

#if PPC_USE_VLE && !PPC_SUPPORTS_VLE
#error "the selected MCU does not support VLE instructions set"
#endif

#if !PPC_USE_VLE && !PPC_SUPPORTS_BOOKE
#error "the selected MCU does not support BookE instructions set"
#endif

/*===========================================================================*/
/* Port exported info (common).                                              */
/*===========================================================================*/

/**
 * @brief   Unique macro for the implemented architecture.
 */
#define CH_ARCHITECTURE_PPC

/**
 * @brief   Name of the implemented architecture.
 */
#define CH_ARCHITECTURE_NAME            "Power Architecture"

/**
 * @brief   Name of the architecture variant.
 */
#if (PPC_VARIANT == PPC_VARIANT_e200z0) || defined(__DOXYGEN__)
#define CH_CORE_VARIANT_NAME            "e200z0"
#elif PPC_VARIANT == PPC_VARIANT_e200z3
#define CH_CORE_VARIANT_NAME            "e200z3"
#elif PPC_VARIANT == PPC_VARIANT_e200z4
#define CH_CORE_VARIANT_NAME            "e200z4"
#else
#error "unknown or unsupported PowerPC variant specified"
#endif

/**
 * @brief   Name of the compiler supported by this port.
 */
#define CH_COMPILER_NAME                "GCC " __VERSION__

/**
 * @brief   Port-specific information string.
 */
#if PPC_USE_VLE
#define CH_PORT_INFO                    "VLE mode"
#else
#define CH_PORT_INFO                    "Book-E mode"
#endif

/*===========================================================================*/
/* Port implementation part (common).                                        */
/*===========================================================================*/

#if !defined(_FROM_ASM_)

/**
 * @brief   Base type for stack and memory alignment.
 */
typedef struct {
  uint8_t a[8];
} stkalign_t __attribute__((aligned(8)));

/**
 * @brief   Generic PPC register.
 */
typedef void *regppc_t;

/**
 * @brief   Mandatory part of a stack frame.
 */
struct eabi_frame {
  regppc_t      slink;          /**< Stack back link.                       */
  regppc_t      shole;          /**< Stack hole for LR storage.             */
};

/**
 * @brief   Interrupt saved context.
 * @details This structure represents the stack frame saved during a
 *          preemption-capable interrupt handler.
 * @note    R2 and R13 are not saved because those are assumed to be immutable
 *          during the system life cycle.
 */
struct extctx {
  struct eabi_frame frame;
  /* Start of the e_stmvsrrw frame (offset 8).*/
  regppc_t      pc;
  regppc_t      msr;
  /* Start of the e_stmvsprw frame (offset 16).*/
  regppc_t      cr;
  regppc_t      lr;
  regppc_t      ctr;
  regppc_t      xer;
  /* Start of the e_stmvgprw frame (offset 32).*/
  regppc_t      r0;
  regppc_t      r3;
  regppc_t      r4;
  regppc_t      r5;
  regppc_t      r6;
  regppc_t      r7;
  regppc_t      r8;
  regppc_t      r9;
  regppc_t      r10;
  regppc_t      r11;
  regppc_t      r12;
  regppc_t      padding;
 };

 /**
 * @brief   System saved context.
 * @details This structure represents the inner stack frame during a context
 *          switching.
 * @note    R2 and R13 are not saved because those are assumed to be immutable
 *          during the system life cycle.
 * @note    LR is stored in the caller contex so it is not present in this
 *          structure.
 */
struct intctx {
  regppc_t      cr;                 /* Part of it is not volatile...        */
  regppc_t      r14;
  regppc_t      r15;
  regppc_t      r16;
  regppc_t      r17;
  regppc_t      r18;
  regppc_t      r19;
  regppc_t      r20;
  regppc_t      r21;
  regppc_t      r22;
  regppc_t      r23;
  regppc_t      r24;
  regppc_t      r25;
  regppc_t      r26;
  regppc_t      r27;
  regppc_t      r28;
  regppc_t      r29;
  regppc_t      r30;
  regppc_t      r31;
  regppc_t      padding;
};

/**
 * @brief   Platform dependent part of the @p Thread structure.
 * @details This structure usually contains just the saved stack pointer
 *          defined as a pointer to a @p intctx structure.
 */
struct context {
  struct intctx *sp;
};

/**
 * @brief   Platform dependent part of the @p chThdCreateI() API.
 * @details This code usually setup the context switching frame represented
 *          by an @p intctx structure.
 */
#define SETUP_CONTEXT(workspace, wsize, pf, arg) {                          \
  uint8_t *sp = (uint8_t *)workspace + wsize - sizeof(struct eabi_frame);   \
  ((struct eabi_frame *)sp)->slink = 0;                                     \
  ((struct eabi_frame *)sp)->shole = _port_thread_start;                    \
  tp->p_ctx.sp = (struct intctx *)(sp - sizeof(struct intctx));             \
  tp->p_ctx.sp->r31 = arg;                                                  \
  tp->p_ctx.sp->r30 = pf;                                                   \
}

/**
 * @brief   Stack size for the system idle thread.
 * @details This size depends on the idle thread implementation, usually
 *          the idle thread should take no more space than those reserved
 *          by @p PORT_INT_REQUIRED_STACK.
 */
#ifndef PORT_IDLE_THREAD_STACK_SIZE
#define PORT_IDLE_THREAD_STACK_SIZE     32
#endif

/**
 * @brief   Per-thread stack overhead for interrupts servicing.
 * @details This constant is used in the calculation of the correct working
 *          area size.
 */
#ifndef PORT_INT_REQUIRED_STACK
#define PORT_INT_REQUIRED_STACK         256
#endif

/**
 * @brief   Enforces a correct alignment for a stack area size value.
 */
#define STACK_ALIGN(n) ((((n) - 1) | (sizeof(stkalign_t) - 1)) + 1)

/**
 * @brief   Computes the thread working area global size.
 */
#define THD_WA_SIZE(n) STACK_ALIGN(sizeof(Thread) +                         \
                                   sizeof(struct intctx) +                  \
                                   sizeof(struct extctx) +                  \
                                   (n) + (PORT_INT_REQUIRED_STACK))

/**
 * @brief   Static working area allocation.
 * @details This macro is used to allocate a static thread working area
 *          aligned as both position and size.
 */
#define WORKING_AREA(s, n) stkalign_t s[THD_WA_SIZE(n) / sizeof(stkalign_t)]

/**
 * @brief   IRQ prologue code.
 * @details This macro must be inserted at the start of all IRQ handlers
 *          enabled to invoke system APIs.
 */
#define PORT_IRQ_PROLOGUE()

/**
 * @brief   IRQ epilogue code.
 * @details This macro must be inserted at the end of all IRQ handlers
 *          enabled to invoke system APIs.
 */
#define PORT_IRQ_EPILOGUE()

/**
 * @brief   IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_IRQ_HANDLER(id) void id(void)

/**
 * @details Implemented as global interrupt disable.
 */
#define port_lock() asm volatile ("wrteei  0" : : : "memory")

/**
 * @details Implemented as global interrupt enable.
 */
#define port_unlock() asm volatile("wrteei  1" : : : "memory")

/**
 * @details Implemented as global interrupt disable.
 */
#define port_lock_from_isr() /*asm ("wrteei  0")*/

/**
 * @details Implemented as global interrupt enable.
 */
#define port_unlock_from_isr() /*asm ("wrteei  1")*/

/**
 * @details Implemented as global interrupt disable.
 */
#define port_disable() asm volatile ("wrteei  0" : : : "memory")

/**
 * @details Same as @p port_disable() in this port, there is no difference
 *          between the two states.
 */
#define port_suspend() asm volatile ("wrteei  0" : : : "memory")

/**
 * @details Implemented as global interrupt enable.
 */
#define port_enable() asm volatile ("wrteei  1" : : : "memory")

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 */
#if !CH_DBG_ENABLE_STACK_CHECK || defined(__DOXYGEN__)
#define port_switch(ntp, otp) _port_switch(ntp, otp)
#else
#define port_switch(ntp, otp) {                                             \
  register struct intctx *sp asm ("%r1");                                   \
  if ((stkalign_t *)(sp - 1) < otp->p_stklimit)                             \
    chDbgPanic("stack overflow");                                           \
  _port_switch(ntp, otp);                                                   \
}
#endif

/**
 * @brief   Writes to a special register.
 *
 * @param[in] spr       special register number
 * @param[in] val       value to be written
 */
#define port_mtspr(spr, val)                                                \
  asm volatile ("mtspr %0,%1" : : "n" (spr), "r" (val))

/**
 * @details This port function is implemented as inlined code for performance
 *          reasons.
 */
#if PPC_ENABLE_WFI_IDLE
#if !defined(port_wait_for_interrupt)
#define port_wait_for_interrupt() {                                         \
  asm volatile ("wait" : : : "memory");                                     \
}
#endif
#else
#define port_wait_for_interrupt()
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void port_init(void);
  void port_halt(void);
  void _port_switch(Thread *ntp, Thread *otp);
  void _port_thread_start(void);
#ifdef __cplusplus
}
#endif

#endif /* _FROM_ASM_ */

#endif /* _CHCORE_H_ */

/** @} */
