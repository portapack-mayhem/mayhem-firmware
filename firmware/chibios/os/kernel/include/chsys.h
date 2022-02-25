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
 * @file    chsys.h
 * @brief   System related macros and structures.
 *
 * @addtogroup system
 * @{
 */

#ifndef _CHSYS_H_
#define _CHSYS_H_

/**
 * @name    Macro Functions
 * @{
 */
#if !CH_NO_IDLE_THREAD || defined(__DOXYGEN__)
/**
 * @brief   Returns a pointer to the idle thread.
 * @pre     In order to use this function the option @p CH_NO_IDLE_THREAD
 *          must be disabled.
 * @note    The reference counter of the idle thread is not incremented but
 *          it is not strictly required being the idle thread a static
 *          object.
 *
 * @return              Pointer to the idle thread.
 *
 * @api
 */
#define chSysGetIdleThread() (rlist.r_queue.p_prev)
#endif

/**
 * @brief   Halts the system.
 * @details This function is invoked by the operating system when an
 *          unrecoverable error is detected, for example because a programming
 *          error in the application code that triggers an assertion while
 *          in debug mode.
 * @note    Can be invoked from any system state.
 *
 * @special
 */
#if !defined(SYSTEM_HALT_HOOK) || defined(__DOXYGEN__)
#define chSysHalt() port_halt()
#else
#define chSysHalt() {                                                       \
  SYSTEM_HALT_HOOK();                                                       \
  port_halt();                                                              \
}
#endif

/**
 * @brief   Performs a context switch.
 * @note    Not a user function, it is meant to be invoked by the scheduler
 *          itself or from within the port layer.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 *
 * @special
 */
#define chSysSwitch(ntp, otp) {                                             \
  dbg_trace(otp);                                                           \
  THREAD_CONTEXT_SWITCH_HOOK(ntp, otp);                                     \
  port_switch(ntp, otp);                                                    \
}

/**
 * @brief   Raises the system interrupt priority mask to the maximum level.
 * @details All the maskable interrupt sources are disabled regardless their
 *          hardware priority.
 * @note    Do not invoke this API from within a kernel lock.
 *
 * @special
 */
#define chSysDisable() {                                                    \
  port_disable();                                                           \
  dbg_check_disable();                                                      \
}

/**
 * @brief   Raises the system interrupt priority mask to system level.
 * @details The interrupt sources that should not be able to preempt the kernel
 *          are disabled, interrupt sources with higher priority are still
 *          enabled.
 * @note    Do not invoke this API from within a kernel lock.
 * @note    This API is no replacement for @p chSysLock(), the @p chSysLock()
 *          could do more than just disable the interrupts.
 *
 * @special
 */
#define chSysSuspend() {                                                    \
  port_suspend();                                                           \
  dbg_check_suspend();                                                      \
}

/**
 * @brief   Lowers the system interrupt priority mask to user level.
 * @details All the interrupt sources are enabled.
 * @note    Do not invoke this API from within a kernel lock.
 * @note    This API is no replacement for @p chSysUnlock(), the
 *          @p chSysUnlock() could do more than just enable the interrupts.
 *
 * @special
 */
#define chSysEnable() {                                                     \
  dbg_check_enable();                                                       \
  port_enable();                                                            \
}

/**
 * @brief   Enters the kernel lock mode.
 *
 * @special
 */
#define chSysLock()  {                                                      \
  port_lock();                                                              \
  dbg_check_lock();                                                         \
}

/**
 * @brief   Leaves the kernel lock mode.
 *
 * @special
 */
#define chSysUnlock() {                                                     \
  dbg_check_unlock();                                                       \
  port_unlock();                                                            \
}

/**
 * @brief   Enters the kernel lock mode from within an interrupt handler.
 * @note    This API may do nothing on some architectures, it is required
 *          because on ports that support preemptable interrupt handlers
 *          it is required to raise the interrupt mask to the same level of
 *          the system mutual exclusion zone.<br>
 *          It is good practice to invoke this API before invoking any I-class
 *          syscall from an interrupt handler.
 * @note    This API must be invoked exclusively from interrupt handlers.
 *
 * @special
 */
#define chSysLockFromIsr() {                                                \
  port_lock_from_isr();                                                     \
  dbg_check_lock_from_isr();                                                \
}

/**
 * @brief   Leaves the kernel lock mode from within an interrupt handler.
 *
 * @note    This API may do nothing on some architectures, it is required
 *          because on ports that support preemptable interrupt handlers
 *          it is required to raise the interrupt mask to the same level of
 *          the system mutual exclusion zone.<br>
 *          It is good practice to invoke this API after invoking any I-class
 *          syscall from an interrupt handler.
 * @note    This API must be invoked exclusively from interrupt handlers.
 *
 * @special
 */
#define chSysUnlockFromIsr() {                                              \
  dbg_check_unlock_from_isr();                                              \
  port_unlock_from_isr();                                                   \
}
/** @} */

/**
 * @name    ISRs abstraction macros
 */
/**
 * @brief   IRQ handler enter code.
 * @note    Usually IRQ handlers functions are also declared naked.
 * @note    On some architectures this macro can be empty.
 *
 * @special
 */
#define CH_IRQ_PROLOGUE()                                                   \
  PORT_IRQ_PROLOGUE();                                                      \
  dbg_check_enter_isr();

/**
 * @brief   IRQ handler exit code.
 * @note    Usually IRQ handlers function are also declared naked.
 * @note    This macro usually performs the final reschedule by using
 *          @p chSchIsPreemptionRequired() and @p chSchDoReschedule().
 *
 * @special
 */
#define CH_IRQ_EPILOGUE()                                                   \
  dbg_check_leave_isr();                                                    \
  PORT_IRQ_EPILOGUE();

/**
 * @brief   Standard normal IRQ handler declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 *
 * @special
 */
#define CH_IRQ_HANDLER(id) PORT_IRQ_HANDLER(id)
/** @} */

/**
 * @name    Fast ISRs abstraction macros
 */
/**
 * @brief   Standard fast IRQ handler declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 * @note    Not all architectures support fast interrupts.
 *
 * @special
 */
#define CH_FAST_IRQ_HANDLER(id) PORT_FAST_IRQ_HANDLER(id)
/** @} */

#ifdef __cplusplus
extern "C" {
#endif
  void chSysInit(void);
  void chSysTimerHandlerI(void);
#ifdef __cplusplus
}
#endif

#endif /* _CHSYS_H_ */

/** @} */
