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
 * @file    AVR/chcore.h
 * @brief   AVR architecture port macros and structures.
 *
 * @addtogroup AVR_CORE
 * @{
 */

#ifndef _CHCORE_H_
#define _CHCORE_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#if CH_DBG_ENABLE_STACK_CHECK
#error "option CH_DBG_ENABLE_STACK_CHECK not supported by this port"
#endif

/**
 * @brief   If enabled allows the idle thread to enter a low power mode.
 */
#ifndef ENABLE_WFI_IDLE
#define ENABLE_WFI_IDLE                 0
#endif

/**
 * @brief   Macro defining the AVR architecture.
 */
#define CH_ARCHITECTURE_AVR

/**
 * @brief   Name of the implemented architecture.
 */
#define CH_ARCHITECTURE_NAME            "AVR"

/**
 * @brief   Name of the architecture variant (optional).
 */
#define CH_CORE_VARIANT_NAME            "MegaAVR"

/**
 * @brief   Name of the compiler supported by this port.
 */
#define CH_COMPILER_NAME                "GCC " __VERSION__

/**
 * @brief   Port-specific information string.
 */
#define CH_PORT_INFO                    "None"

/**
 * @brief   8 bits stack and memory alignment enforcement.
 */
typedef uint8_t stkalign_t;

/**
 * @brief   Interrupt saved context.
 * @details This structure represents the stack frame saved during a
 *          preemption-capable interrupt handler.
 * @note    The field @p _next is not part of the context, it represents the
 *          offset of the structure relative to the stack pointer.
 */
struct extctx {
  uint8_t       _next;
  uint8_t       r31;
  uint8_t       r30;
  uint8_t       r27;
  uint8_t       r26;
  uint8_t       r25;
  uint8_t       r24;
  uint8_t       r23;
  uint8_t       r22;
  uint8_t       r21;
  uint8_t       r20;
  uint8_t       r19;
  uint8_t       r18;
  uint8_t       sr;
  uint8_t       r1;
  uint8_t       r0;
  uint16_t      pc;
};

/**
 * @brief   System saved context.
 * @details This structure represents the inner stack frame during a context
 *          switching.
 * @note    The field @p _next is not part of the context, it represents the
 *          offset of the structure relative to the stack pointer.
 */
struct intctx {
  uint8_t       _next;
  uint8_t       r29;
  uint8_t       r28;
  uint8_t       r17;
  uint8_t       r16;
  uint8_t       r15;
  uint8_t       r14;
  uint8_t       r13;
  uint8_t       r12;
  uint8_t       r11;
  uint8_t       r10;
  uint8_t       r9;
  uint8_t       r8;
  uint8_t       r7;
  uint8_t       r6;
  uint8_t       r5;
  uint8_t       r4;
  uint8_t       r3;
  uint8_t       r2;
  uint8_t       pcl;
  uint8_t       pch;
};

/**
 * @brief   Platform dependent part of the @p Thread structure.
 * @details In the AVR port this structure just holds a pointer to the
 *          @p intctx structure representing the stack pointer at the time
 *          of the context switch.
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
  tp->p_ctx.sp = (struct intctx*)((uint8_t *)workspace + wsize  -           \
                                  sizeof(struct intctx));                   \
  tp->p_ctx.sp->r2  = (int)pf;                                              \
  tp->p_ctx.sp->r3  = (int)pf >> 8;                                         \
  tp->p_ctx.sp->r4  = (int)arg;                                             \
  tp->p_ctx.sp->r5  = (int)arg >> 8;                                        \
  tp->p_ctx.sp->pcl = (int)_port_thread_start >> 8;                         \
  tp->p_ctx.sp->pch = (int)_port_thread_start;                              \
}

/**
 * @brief   Stack size for the system idle thread.
 * @details This size depends on the idle thread implementation, usually
 *          the idle thread should take no more space than those reserved
 *          by @p PORT_INT_REQUIRED_STACK.
 * @note    In this port it is set to 8.
 */
#ifndef PORT_IDLE_THREAD_STACK_SIZE
#define PORT_IDLE_THREAD_STACK_SIZE     8
#endif

/**
 * @brief   Per-thread stack overhead for interrupts servicing.
 * @details This constant is used in the calculation of the correct working
 *          area size.
 *          This value can be zero on those architecture where there is a
 *          separate interrupt stack and the stack space between @p intctx and
 *          @p extctx is known to be zero.
 * @note    In this port the default is 32 bytes per thread.
 */
#ifndef PORT_INT_REQUIRED_STACK
#define PORT_INT_REQUIRED_STACK     32
#endif

/**
 * @brief   Enforces a correct alignment for a stack area size value.
 */
#define STACK_ALIGN(n) ((((n) - 1) | (sizeof(stkalign_t) - 1)) + 1)

/**
 * @brief   Computes the thread working area global size.
 */
#define THD_WA_SIZE(n) STACK_ALIGN(sizeof(Thread) +                         \
                                   (sizeof(struct intctx) - 1) +            \
                                   (sizeof(struct extctx) - 1) +            \
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
 * @note    This code tricks the compiler to save all the specified registers
 *          by "touching" them.
 */
#define PORT_IRQ_PROLOGUE() {                                               \
  asm ("" : : : "r18", "r19", "r20", "r21", "r22", "r23", "r24",            \
                "r25", "r26", "r27", "r30", "r31");                         \
}

/**
 * @brief   IRQ epilogue code.
 * @details This macro must be inserted at the end of all IRQ handlers
 *          enabled to invoke system APIs.
 */
#define PORT_IRQ_EPILOGUE() {                                               \
  dbg_check_lock();                                                         \
  if (chSchIsPreemptionRequired())                                          \
    chSchDoReschedule();                                                    \
  dbg_check_unlock();                                                       \
}

/**
 * @brief   IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_IRQ_HANDLER(id) ISR(id)

/**
 * @brief   Port-related initialization code.
 * @note    This function is empty in this port.
 */
#define port_init()

/**
 * @brief   Kernel-lock action.
 * @details Usually this function just disables interrupts but may perform more
 *          actions.
 * @note    Implemented as global interrupt disable.
 */
#define port_lock() asm volatile ("cli" : : : "memory")

/**
 * @brief   Kernel-unlock action.
 * @details Usually this function just enables interrupts but may perform more
 *          actions.
 * @note    Implemented as global interrupt enable.
 */
#define port_unlock() asm volatile ("sei" : : : "memory")

/**
 * @brief   Kernel-lock action from an interrupt handler.
 * @details This function is invoked before invoking I-class APIs from
 *          interrupt handlers. The implementation is architecture dependent,
 *          in its simplest form it is void.
 * @note    This function is empty in this port.
 */
#define port_lock_from_isr()

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 * @details This function is invoked after invoking I-class APIs from interrupt
 *          handlers. The implementation is architecture dependent, in its
 *          simplest form it is void.
 * @note    This function is empty in this port.
 */
#define port_unlock_from_isr()

/**
 * @brief   Disables all the interrupt sources.
 * @note    Of course non-maskable interrupt sources are not included.
 * @note    Implemented as global interrupt disable.
 */
#define port_disable() asm volatile ("cli" : : : "memory")

/**
 * @brief   Disables the interrupt sources below kernel-level priority.
 * @note    Interrupt sources above kernel level remains enabled.
 * @note    Same as @p port_disable() in this port, there is no difference
 *          between the two states.
 */
#define port_suspend() asm volatile ("cli" : : : "memory")

/**
 * @brief   Enables all the interrupt sources.
 * @note    Implemented as global interrupt enable.
 */
#define port_enable() asm volatile ("sei" : : : "memory")

/**
 * @brief   Enters an architecture-dependent IRQ-waiting mode.
 * @details The function is meant to return when an interrupt becomes pending.
 *          The simplest implementation is an empty function or macro but this
 *          would not take advantage of architecture-specific power saving
 *          modes.
 * @note    This port function is implemented as inlined code for performance
 *          reasons.
 */
#if ENABLE_WFI_IDLE != 0
#define port_wait_for_interrupt() {                                         \
  asm volatile ("sleep" : : : "memory");                                    \
}
#else
#define port_wait_for_interrupt()
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void port_switch(Thread *ntp, Thread *otp);
  void port_halt(void);
  void _port_thread_start(void);
#ifdef __cplusplus
}
#endif

#endif /* _CHCORE_H_ */

/** @} */
