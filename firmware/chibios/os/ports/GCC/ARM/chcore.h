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
 * @file    ARM/chcore.h
 * @brief   ARM7/9 architecture port macros and structures.
 *
 * @addtogroup ARM_CORE
 * @{
 */

#ifndef _CHCORE_H_
#define _CHCORE_H_

/*===========================================================================*/
/* Port constants.                                                           */
/*===========================================================================*/

/* Core variants identifiers.*/
#define ARM_CORE_ARM7TDMI           7   /**< ARM77TDMI core identifier.     */
#define ARM_CORE_ARM9               9   /**< ARM9 core identifier.          */

/* Inclusion of the ARM implementation specific parameters.*/
#include "armparams.h"

/* ARM core check, only ARM7TDMI and ARM9 supported right now.*/
#if (ARM_CORE == ARM_CORE_ARM7TDMI) || (ARM_CORE == ARM_CORE_ARM9)
#else
#error "unknown or unsupported ARM core"
#endif

/*===========================================================================*/
/* Port statically derived parameters.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Port macros.                                                              */
/*===========================================================================*/

/*===========================================================================*/
/* Port configurable parameters.                                             */
/*===========================================================================*/

/**
 * @brief   If enabled allows the idle thread to enter a low power mode.
 */
#ifndef ARM_ENABLE_WFI_IDLE
#define ARM_ENABLE_WFI_IDLE             FALSE
#endif

/*===========================================================================*/
/* Port exported info.                                                       */
/*===========================================================================*/

/**
 * @brief   Macro defining a generic ARM architecture.
 */
#define CH_ARCHITECTURE_ARM

#if defined(__DOXYGEN__)
/**
 * @brief   Macro defining the specific ARM architecture.
 * @note    This macro is for documentation only, the real name changes
 *          depending on the selected architecture, the possible names are:
 *          - CH_ARCHITECTURE_ARM7TDMI.
 *          - CH_ARCHITECTURE_ARM9.
 *          .
 */
#define CH_ARCHITECTURE_ARMx

/**
 * @brief   Name of the implemented architecture.
 * @note    The value is for documentation only, the real value changes
 *          depending on the selected architecture, the possible values are:
 *          - "ARM7".
 *          - "ARM9".
 *          .
 */
#define CH_ARCHITECTURE_NAME            "ARMx"

/**
 * @brief   Name of the architecture variant (optional).
 * @note    The value is for documentation only, the real value changes
 *          depending on the selected architecture, the possible values are:
 *          - "ARM7TDMI"
 *          - "ARM9"
 *          .
 */
#define CH_CORE_VARIANT_NAME            "ARMxy"

/**
 * @brief   Port-specific information string.
 * @note    The value is for documentation only, the real value changes
 *          depending on the selected options, the possible values are:
 *          - "Pure ARM"
 *          - "Pure THUMB"
 *          - "Interworking"
 *          .
 */
#define CH_PORT_INFO                    "ARM|THUMB|Interworking"

#elif ARM_CORE == ARM_CORE_ARM7TDMI
#define CH_ARCHITECTURE_ARM7TDMI
#define CH_ARCHITECTURE_NAME            "ARM7"
#define CH_CORE_VARIANT_NAME            "ARM7TDMI"

#elif ARM_MODEL == ARM_VARIANT_ARM9
#define CH_ARCHITECTURE_ARM9
#define CH_ARCHITECTURE_NAME            "ARM9"
#define CH_CORE_VARIANT_NAME            "ARM9"
#endif

#if THUMB_PRESENT
#if THUMB_NO_INTERWORKING
#define CH_PORT_INFO                    "Pure THUMB mode"
#else /* !THUMB_NO_INTERWORKING */
#define CH_PORT_INFO                    "Interworking mode"
#endif /* !THUMB_NO_INTERWORKING */
#else /* !THUMB_PRESENT */
#define CH_PORT_INFO                    "Pure ARM mode"
#endif /* !THUMB_PRESENT */

/**
 * @brief   Name of the compiler supported by this port.
 */
#define CH_COMPILER_NAME                "GCC " __VERSION__

/*===========================================================================*/
/* Port implementation part (common).                                        */
/*===========================================================================*/

/**
 * @brief   32 bits stack and memory alignment enforcement.
 */
typedef uint32_t stkalign_t;

/**
 * @brief   Generic ARM register.
 */
typedef void *regarm_t;

/**
 * @brief   Interrupt saved context.
 * @details This structure represents the stack frame saved during a
 *          preemption-capable interrupt handler.
 */
struct extctx {
  regarm_t      spsr_irq;
  regarm_t      lr_irq;
  regarm_t      r0;
  regarm_t      r1;
  regarm_t      r2;
  regarm_t      r3;
  regarm_t      r12;
  regarm_t      lr_usr;
};

/**
 * @brief   System saved context.
 * @details This structure represents the inner stack frame during a context
 *          switching.
 */
struct intctx {
  regarm_t      r4;
  regarm_t      r5;
  regarm_t      r6;
  regarm_t      r7;
  regarm_t      r8;
  regarm_t      r9;
  regarm_t      r10;
  regarm_t      r11;
  regarm_t      lr;
};

/**
 * @brief   Platform dependent part of the @p Thread structure.
 * @details In this port the structure just holds a pointer to the @p intctx
 *          structure representing the stack pointer at context switch time.
 */
struct context {
  struct intctx *r13;
};

/**
 * @brief   Platform dependent part of the @p chThdCreateI() API.
 * @details This code usually setup the context switching frame represented
 *          by an @p intctx structure.
 */
#define SETUP_CONTEXT(workspace, wsize, pf, arg) {                          \
  tp->p_ctx.r13 = (struct intctx *)((uint8_t *)workspace +                  \
                                     wsize -                                \
                                     sizeof(struct intctx));                \
  tp->p_ctx.r13->r4 = pf;                                                   \
  tp->p_ctx.r13->r5 = arg;                                                  \
  tp->p_ctx.r13->lr = _port_thread_start;                                   \
}

/**
 * @brief   Stack size for the system idle thread.
 * @details This size depends on the idle thread implementation, usually
 *          the idle thread should take no more space than those reserved
 *          by @p PORT_INT_REQUIRED_STACK.
 * @note    In this port it is set to 4 because the idle thread does have
 *          a stack frame when compiling without optimizations.
 */
#ifndef PORT_IDLE_THREAD_STACK_SIZE
#define PORT_IDLE_THREAD_STACK_SIZE     4
#endif

/**
 * @brief   Per-thread stack overhead for interrupts servicing.
 * @details This constant is used in the calculation of the correct working
 *          area size.
 *          This value can be zero on those architecture where there is a
 *          separate interrupt stack and the stack space between @p intctx and
 *          @p extctx is known to be zero.
 * @note    In this port 0x10 is a safe value, it can be reduced after careful
 *          analysis of the generated code.
 */
#ifndef PORT_INT_REQUIRED_STACK
#define PORT_INT_REQUIRED_STACK         0x10
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
 * @note    This macro has a different implementation depending if compiled in
 *          ARM or THUMB mode.
 * @note    The THUMB implementation starts with ARM code because interrupt
 *          vectors are always invoked in ARM mode regardless the bit 0
 *          value. The switch in THUMB mode is done in the function prologue so
 *          it is transparent to the user code.
 */
#if !defined(PORT_IRQ_PROLOGUE)
#ifdef THUMB
#define PORT_IRQ_PROLOGUE() {                                               \
  asm volatile (".code 32                               \n\t"               \
                "stmfd   sp!, {r0-r3, r12, lr}          \n\t"               \
                "add     r0, pc, #1                     \n\t"               \
                "bx      r0                             \n\t"               \
                ".code 16" : : : "memory");                                 \
}
#else /* !THUMB */
#define PORT_IRQ_PROLOGUE() {                                               \
  asm volatile ("stmfd    sp!, {r0-r3, r12, lr}" : : : "memory");           \
}
#endif /* !THUMB */
#endif /* !defined(PORT_IRQ_PROLOGUE) */

/**
 * @brief   IRQ epilogue code.
 * @details This macro must be inserted at the end of all IRQ handlers
 *          enabled to invoke system APIs.
 * @note    This macro has a different implementation depending if compiled in
 *          ARM or THUMB mode.
 */
#if !defined(PORT_IRQ_EPILOGUE)
#ifdef THUMB
#define PORT_IRQ_EPILOGUE() {                                               \
  asm volatile ("ldr     r0, =_port_irq_common          \n\t"               \
                "bx      r0" : : : "memory");                               \
}
#else /* !THUMB */
#define PORT_IRQ_EPILOGUE() {                                               \
  asm volatile ("b       _port_irq_common" : : : "memory");                 \
}
#endif /* !THUMB */
#endif /* !defined(PORT_IRQ_EPILOGUE) */

/**
 * @brief   IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#if !defined(PORT_IRQ_HANDLER)
#define PORT_IRQ_HANDLER(id) __attribute__((naked)) void id(void)
#endif /* !defined(PORT_IRQ_HANDLER) */

/**
 * @brief   Fast IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#if !defined(PORT_FAST_IRQ_HANDLER)
#define PORT_FAST_IRQ_HANDLER(id)                                           \
  __attribute__((interrupt("FIQ"))) void id(void)
#endif /* !defined(PORT_FAST_IRQ_HANDLER) */

/**
 * @brief   Port-related initialization code.
 * @note    This function is empty in this port.
 */
#define port_init()

/**
 * @brief   Kernel-lock action.
 * @details Usually this function just disables interrupts but may perform
 *          more actions.
 * @note    In this port it disables the IRQ sources and keeps FIQ sources
 *          enabled.
 */
#ifdef THUMB
#define port_lock() {                                                       \
  asm volatile ("bl     _port_lock_thumb" : : : "r3", "lr", "memory");      \
}
#else /* !THUMB */
#define port_lock() asm volatile ("msr     CPSR_c, #0x9F" : : : "memory")
#endif /* !THUMB */

/**
 * @brief   Kernel-unlock action.
 * @details Usually this function just enables interrupts but may perform
 *          more actions.
 * @note    In this port it enables both the IRQ and FIQ sources.
 */
#ifdef THUMB
#define port_unlock() {                                                     \
  asm volatile ("bl     _port_unlock_thumb" : : : "r3", "lr", "memory");    \
}
#else /* !THUMB */
#define port_unlock() asm volatile ("msr     CPSR_c, #0x1F" : : : "memory")
#endif /* !THUMB */

/**
 * @brief   Kernel-lock action from an interrupt handler.
 * @details This function is invoked before invoking I-class APIs from
 *          interrupt handlers. The implementation is architecture dependent,
 *          in its simplest form it is void.
 * @note    Empty in this port.
 */
#define port_lock_from_isr()

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 * @details This function is invoked after invoking I-class APIs from interrupt
 *          handlers. The implementation is architecture dependent, in its
 *          simplest form it is void.
 * @note    Empty in this port.
 */
#define port_unlock_from_isr()

/**
 * @brief   Disables all the interrupt sources.
 * @note    Of course non-maskable interrupt sources are not included.
 * @note    In this port it disables both the IRQ and FIQ sources.
 * @note    Implements a workaround for spurious interrupts taken from the NXP
 *          LPC214x datasheet.
 */
#ifdef THUMB
#define port_disable() {                                                    \
  asm volatile ("bl     _port_disable_thumb" : : : "r3", "lr", "memory");   \
}
#else /* !THUMB */
#define port_disable() {                                                    \
  asm volatile ("mrs     r3, CPSR                       \n\t"               \
                "orr     r3, #0x80                      \n\t"               \
                "msr     CPSR_c, r3                     \n\t"               \
                "orr     r3, #0x40                      \n\t"               \
                "msr     CPSR_c, r3" : : : "r3", "memory");                 \
}
#endif /* !THUMB */

/**
 * @brief   Disables the interrupt sources below kernel-level priority.
 * @note    Interrupt sources above kernel level remains enabled.
 * @note    In this port it disables the IRQ sources and enables the
 *          FIQ sources.
 */
#ifdef THUMB
#define port_suspend() {                                                    \
  asm volatile ("bl     _port_suspend_thumb" : : : "r3", "lr", "memory");   \
}
#else /* !THUMB */
#define port_suspend() asm volatile ("msr     CPSR_c, #0x9F" : : : "memory")
#endif /* !THUMB */

/**
 * @brief   Enables all the interrupt sources.
 * @note    In this port it enables both the IRQ and FIQ sources.
 */
#ifdef THUMB
#define port_enable() {                                                     \
  asm volatile ("bl     _port_enable_thumb" : : : "r3", "lr", "memory");    \
}
#else /* !THUMB */
#define port_enable() asm volatile ("msr     CPSR_c, #0x1F" : : : "memory")
#endif /* !THUMB */

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 * @note    Implemented as inlined code for performance reasons.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 */
#ifdef THUMB
#if CH_DBG_ENABLE_STACK_CHECK
#define port_switch(ntp, otp) {                                             \
  register struct intctx *r13 asm ("r13");                                  \
  if ((stkalign_t *)(r13 - 1) < otp->p_stklimit)                            \
    chDbgPanic("stack overflow");                                           \
  _port_switch_thumb(ntp, otp);                                             \
}
#else /* !CH_DBG_ENABLE_STACK_CHECK */
#define port_switch(ntp, otp) _port_switch_thumb(ntp, otp)
#endif /* !CH_DBG_ENABLE_STACK_CHECK */
#else /* !THUMB */
#if CH_DBG_ENABLE_STACK_CHECK
#define port_switch(ntp, otp) {                                             \
  register struct intctx *r13 asm ("r13");                                  \
  if ((stkalign_t *)(r13 - 1) < otp->p_stklimit)                            \
    chDbgPanic("stack overflow");                                           \
  _port_switch_arm(ntp, otp);                                               \
}
#else /* !CH_DBG_ENABLE_STACK_CHECK */
#define port_switch(ntp, otp) _port_switch_arm(ntp, otp)
#endif /* !CH_DBG_ENABLE_STACK_CHECK */
#endif /* !THUMB */

#ifdef __cplusplus
extern "C" {
#endif
  void port_halt(void);
#ifdef THUMB
  void _port_switch_thumb(Thread *ntp, Thread *otp);
#else /* !THUMB */
  void _port_switch_arm(Thread *ntp, Thread *otp);
#endif /* !THUMB */
  void _port_thread_start(void);
#ifdef __cplusplus
}
#endif

#endif /* _CHCORE_H_ */

/** @} */
