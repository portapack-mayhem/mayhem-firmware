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
 * @file    RVCT/ARMCMx/chcore_v7m.h
 * @brief   ARMv7-M architecture port macros and structures.
 *
 * @addtogroup RVCT_ARMCMx_V7M_CORE
 * @{
 */

#ifndef _CHCORE_V7M_H_
#define _CHCORE_V7M_H_

/*===========================================================================*/
/* Port constants.                                                           */
/*===========================================================================*/

/**
 * @brief   Disabled value for BASEPRI register.
 */
#define CORTEX_BASEPRI_DISABLED         0

/*===========================================================================*/
/* Port macros.                                                              */
/*===========================================================================*/

/*===========================================================================*/
/* Port configurable parameters.                                             */
/*===========================================================================*/

/**
 * @brief   Stack size for the system idle thread.
 * @details This size depends on the idle thread implementation, usually
 *          the idle thread should take no more space than those reserved
 *          by @p PORT_INT_REQUIRED_STACK.
 * @note    In this port it is set to 16 because the idle thread does have
 *          a stack frame when compiling without optimizations. You may
 *          reduce this value to zero when compiling with optimizations.
 */
#if !defined(PORT_IDLE_THREAD_STACK_SIZE)
#define PORT_IDLE_THREAD_STACK_SIZE     16
#endif

/**
 * @brief   Per-thread stack overhead for interrupts servicing.
 * @details This constant is used in the calculation of the correct working
 *          area size.
 * @note    In this port this value is conservatively set to 32 because the
 *          function @p chSchDoReschedule() can have a stack frame, especially
 *          with compiler optimizations disabled. The value can be reduced
 *          when compiler optimizations are enabled.
 */
#if !defined(PORT_INT_REQUIRED_STACK)
#define PORT_INT_REQUIRED_STACK         32
#endif

/**
 * @brief   Enables the use of the WFI instruction in the idle thread loop.
 */
#if !defined(CORTEX_ENABLE_WFI_IDLE)
#define CORTEX_ENABLE_WFI_IDLE          FALSE
#endif

/**
 * @brief   SYSTICK handler priority.
 * @note    The default SYSTICK handler priority is calculated as the priority
 *          level in the middle of the numeric priorities range.
 */
#if !defined(CORTEX_PRIORITY_SYSTICK)
#define CORTEX_PRIORITY_SYSTICK         (CORTEX_PRIORITY_LEVELS >> 1)
#elif !CORTEX_IS_VALID_PRIORITY(CORTEX_PRIORITY_SYSTICK)
/* If it is externally redefined then better perform a validity check on it.*/
#error "invalid priority level specified for CORTEX_PRIORITY_SYSTICK"
#endif

/**
 * @brief   FPU support in context switch.
 * @details Activating this option activates the FPU support in the kernel.
 */
#if !defined(CORTEX_USE_FPU)
#define CORTEX_USE_FPU                  CORTEX_HAS_FPU
#elif CORTEX_USE_FPU && !CORTEX_HAS_FPU
/* This setting requires an FPU presence check in case it is externally
   redefined.*/
#error "the selected core does not have an FPU"
#endif

/**
 * @brief   Simplified priority handling flag.
 * @details Activating this option makes the Kernel work in compact mode.
 */
#if !defined(CORTEX_SIMPLIFIED_PRIORITY)
#define CORTEX_SIMPLIFIED_PRIORITY      FALSE
#endif

/**
 * @brief   SVCALL handler priority.
 * @note    The default SVCALL handler priority is defaulted to
 *          @p CORTEX_MAXIMUM_PRIORITY+1, this reserves the
 *          @p CORTEX_MAXIMUM_PRIORITY priority level as fast interrupts
 *          priority level.
 */
#if !defined(CORTEX_PRIORITY_SVCALL)
#define CORTEX_PRIORITY_SVCALL          (CORTEX_MAXIMUM_PRIORITY + 1)
#elif !CORTEX_IS_VALID_PRIORITY(CORTEX_PRIORITY_SVCALL)
/* If it is externally redefined then better perform a validity check on it.*/
#error "invalid priority level specified for CORTEX_PRIORITY_SVCALL"
#endif

/**
 * @brief   NVIC VTOR initialization expression.
 */
#if !defined(CORTEX_VTOR_INIT) || defined(__DOXYGEN__)
#define CORTEX_VTOR_INIT                0x00000000
#endif

/**
 * @brief   NVIC PRIGROUP initialization expression.
 * @details The default assigns all available priority bits as preemption
 *          priority with no sub-priority.
 */
#if !defined(CORTEX_PRIGROUP_INIT) || defined(__DOXYGEN__)
#define CORTEX_PRIGROUP_INIT            (7 - CORTEX_PRIORITY_BITS)
#endif

/*===========================================================================*/
/* Port derived parameters.                                                  */
/*===========================================================================*/

#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
/**
 * @brief   Maximum usable priority for normal ISRs.
 */
#define CORTEX_MAX_KERNEL_PRIORITY      (CORTEX_PRIORITY_SVCALL + 1)

/**
 * @brief   BASEPRI level within kernel lock.
 * @note    In compact kernel mode this constant value is enforced to zero.
 */
#define CORTEX_BASEPRI_KERNEL                                               \
  CORTEX_PRIORITY_MASK(CORTEX_MAX_KERNEL_PRIORITY)
#else

#define CORTEX_MAX_KERNEL_PRIORITY      1
#define CORTEX_BASEPRI_KERNEL           0
#endif

/**
 * @brief   PendSV priority level.
 * @note    This priority is enforced to be equal to
 *          @p CORTEX_MAX_KERNEL_PRIORITY, this handler always have the
 *          highest priority that cannot preempt the kernel.
 */
#define CORTEX_PRIORITY_PENDSV          CORTEX_MAX_KERNEL_PRIORITY

/*===========================================================================*/
/* Port exported info.                                                       */
/*===========================================================================*/

#if (CORTEX_MODEL == CORTEX_M3) || defined(__DOXYGEN__)
/**
 * @brief   Macro defining the specific ARM architecture.
 */
#define CH_ARCHITECTURE_ARM_v7M

/**
 * @brief   Name of the implemented architecture.
 */
#define CH_ARCHITECTURE_NAME            "ARMv7-M"

/**
 * @brief   Name of the architecture variant.
 */
#define CH_CORE_VARIANT_NAME            "Cortex-M3"

#elif (CORTEX_MODEL == CORTEX_M4)
#define CH_ARCHITECTURE_ARM_v7ME
#define CH_ARCHITECTURE_NAME            "ARMv7-ME"
#if CORTEX_USE_FPU
#define CH_CORE_VARIANT_NAME            "Cortex-M4F"
#else
#define CH_CORE_VARIANT_NAME            "Cortex-M4"
#endif
#endif

/**
 * @brief   Port-specific information string.
 */
#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
#define CH_PORT_INFO                    "Advanced kernel mode"
#else
#define CH_PORT_INFO                    "Compact kernel mode"
#endif

/*===========================================================================*/
/* Port implementation part.                                                 */
/*===========================================================================*/

#if !defined(_FROM_ASM_)

/**
 * @brief   Generic ARM register.
 */
typedef void *regarm_t;

/**
 * @brief   Stack and memory alignment enforcement.
 * @note    In this architecture the stack alignment is enforced to 64 bits,
 *          32 bits alignment is supported by hardware but deprecated by ARM,
 *          the implementation choice is to not offer the option.
 */
typedef uint64_t stkalign_t;

/* The documentation of the following declarations is in chconf.h in order
   to not have duplicated structure names into the documentation.*/
#if !defined(__DOXYGEN__)

struct extctx {
  regarm_t      r0;
  regarm_t      r1;
  regarm_t      r2;
  regarm_t      r3;
  regarm_t      r12;
  regarm_t      lr_thd;
  regarm_t      pc;
  regarm_t      xpsr;
#if CORTEX_USE_FPU
  regarm_t      s0;
  regarm_t      s1;
  regarm_t      s2;
  regarm_t      s3;
  regarm_t      s4;
  regarm_t      s5;
  regarm_t      s6;
  regarm_t      s7;
  regarm_t      s8;
  regarm_t      s9;
  regarm_t      s10;
  regarm_t      s11;
  regarm_t      s12;
  regarm_t      s13;
  regarm_t      s14;
  regarm_t      s15;
  regarm_t      fpscr;
  regarm_t      reserved;
#endif /* CORTEX_USE_FPU */
};

struct intctx {
#if CORTEX_USE_FPU
  regarm_t      s16;
  regarm_t      s17;
  regarm_t      s18;
  regarm_t      s19;
  regarm_t      s20;
  regarm_t      s21;
  regarm_t      s22;
  regarm_t      s23;
  regarm_t      s24;
  regarm_t      s25;
  regarm_t      s26;
  regarm_t      s27;
  regarm_t      s28;
  regarm_t      s29;
  regarm_t      s30;
  regarm_t      s31;
#endif /* CORTEX_USE_FPU */
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

#endif /* !defined(__DOXYGEN__) */

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
  tp->p_ctx.r13->r4 = (regarm_t)pf;                                         \
  tp->p_ctx.r13->r5 = (regarm_t)arg;                                        \
  tp->p_ctx.r13->lr = (regarm_t)_port_thread_start;                         \
}

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
#define PORT_IRQ_EPILOGUE() _port_irq_epilogue()

/**
 * @brief   IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_IRQ_HANDLER(id) void id(void)

/**
 * @brief   Fast IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_FAST_IRQ_HANDLER(id) void id(void)

/**
 * @brief   Port-related initialization code.
 */
#define port_init() _port_init()

/**
 * @brief   Kernel-lock action.
 * @details Usually this function just disables interrupts but may perform
 *          more actions.
 * @note    In this port this it raises the base priority to kernel level.
 */
#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
#define port_lock() {                                                       \
  register uint32_t basepri __asm("basepri");                               \
  basepri = CORTEX_BASEPRI_KERNEL;                                          \
}
#else /* CORTEX_SIMPLIFIED_PRIORITY */
#define port_lock() __disable_irq()
#endif /* CORTEX_SIMPLIFIED_PRIORITY */

/**
 * @brief   Kernel-unlock action.
 * @details Usually this function just enables interrupts but may perform
 *          more actions.
 * @note    In this port this it lowers the base priority to user level.
 */
#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
#define port_unlock() {                                                     \
  register uint32_t basepri __asm("basepri");                               \
  basepri = CORTEX_BASEPRI_DISABLED;                                        \
}
#else /* CORTEX_SIMPLIFIED_PRIORITY */
#define port_unlock() __enable_irq()
#endif /* CORTEX_SIMPLIFIED_PRIORITY */

/**
 * @brief   Kernel-lock action from an interrupt handler.
 * @details This function is invoked before invoking I-class APIs from
 *          interrupt handlers. The implementation is architecture dependent,
 *          in its simplest form it is void.
 * @note    Same as @p port_lock() in this port.
 */
#define port_lock_from_isr() port_lock()

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 * @details This function is invoked after invoking I-class APIs from interrupt
 *          handlers. The implementation is architecture dependent, in its
 *          simplest form it is void.
 * @note    Same as @p port_unlock() in this port.
 */
#define port_unlock_from_isr() port_unlock()

/**
 * @brief   Disables all the interrupt sources.
 * @note    Of course non-maskable interrupt sources are not included.
 * @note    In this port it disables all the interrupt sources by raising
 *          the priority mask to level 0.
 */
#define port_disable() __disable_irq()

/**
 * @brief   Disables the interrupt sources below kernel-level priority.
 * @note    Interrupt sources above kernel level remains enabled.
 * @note    In this port it raises/lowers the base priority to kernel level.
 */
#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
#define port_suspend() {                                                    \
  register uint32_t basepri __asm("basepri");                               \
  basepri = CORTEX_BASEPRI_KERNEL;                                          \
}
#else /* CORTEX_SIMPLIFIED_PRIORITY */
#define port_suspend() __disable_irq()
#endif /* CORTEX_SIMPLIFIED_PRIORITY */

/**
 * @brief   Enables all the interrupt sources.
 * @note    In this port it lowers the base priority to user level.
 */
#if !CORTEX_SIMPLIFIED_PRIORITY || defined(__DOXYGEN__)
#define port_enable() {                                                     \
  register uint32_t basepri __asm("basepri");                               \
  basepri = CORTEX_BASEPRI_DISABLED;                                        \
  __enable_irq();                                                           \
}
#else /* CORTEX_SIMPLIFIED_PRIORITY */
#define port_enable() __enable_irq()
#endif /* CORTEX_SIMPLIFIED_PRIORITY */

/**
 * @brief   Enters an architecture-dependent IRQ-waiting mode.
 * @details The function is meant to return when an interrupt becomes pending.
 *          The simplest implementation is an empty function or macro but this
 *          would not take advantage of architecture-specific power saving
 *          modes.
 * @note    Implemented as an inlined @p WFI instruction.
 */
#if CORTEX_ENABLE_WFI_IDLE || defined(__DOXYGEN__)
#define port_wait_for_interrupt() __wfi()
#else
#define port_wait_for_interrupt()
#endif

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
  uint8_t *r13 = (uint8_t *)__current_sp();                                 \
  if ((stkalign_t *)(r13 - sizeof(struct intctx)) < otp->p_stklimit)        \
    chDbgPanic("stack overflow");                                           \
  _port_switch(ntp, otp);                                                   \
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void port_halt(void);
  void _port_init(void);
  void _port_irq_epilogue(void);
  void _port_switch_from_isr(void);
  void _port_exit_from_isr(void);
  void _port_switch(Thread *ntp, Thread *otp);
  void _port_thread_start(void);
#ifdef __cplusplus
}
#endif

#endif /* _FROM_ASM_ */

#endif /* _CHCORE_V7M_H_ */

/** @} */
