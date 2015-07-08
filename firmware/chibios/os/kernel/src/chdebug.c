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
 * @file    chdebug.c
 * @brief   ChibiOS/RT Debug code.
 *
 * @addtogroup debug
 * @details Debug APIs and services:
 *          - Runtime system state and call protocol check. The following
 *            panic messages can be generated:
 *            - SV#1, misplaced @p chSysDisable().
 *            - SV#2, misplaced @p chSysSuspend()
 *            - SV#3, misplaced @p chSysEnable().
 *            - SV#4, misplaced @p chSysLock().
 *            - SV#5, misplaced @p chSysUnlock().
 *            - SV#6, misplaced @p chSysLockFromIsr().
 *            - SV#7, misplaced @p chSysUnlockFromIsr().
 *            - SV#8, misplaced @p CH_IRQ_PROLOGUE().
 *            - SV#9, misplaced @p CH_IRQ_EPILOGUE().
 *            - SV#10, misplaced I-class function.
 *            - SV#11, misplaced S-class function.
 *            .
 *          - Trace buffer.
 *          - Parameters check.
 *          - Kernel assertions.
 *          - Kernel panics.
 *          .
 * @note    Stack checks are not implemented in this module but in the port
 *          layer in an architecture-dependent way.
 * @{
 */

#include "ch.h"

/*===========================================================================*/
/* System state checker related code and variables.                          */
/*===========================================================================*/

#if CH_DBG_SYSTEM_STATE_CHECK || defined(__DOXYGEN__)

/**
 * @brief   ISR nesting level.
 */
cnt_t dbg_isr_cnt;

/**
 * @brief   Lock nesting level.
 */
cnt_t dbg_lock_cnt;

/**
 * @brief   Guard code for @p chSysDisable().
 *
 * @notapi
 */
void dbg_check_disable(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#1");
}

/**
 * @brief   Guard code for @p chSysSuspend().
 *
 * @notapi
 */
void dbg_check_suspend(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#2");
}

/**
 * @brief   Guard code for @p chSysEnable().
 *
 * @notapi
 */
void dbg_check_enable(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#3");
}

/**
 * @brief   Guard code for @p chSysLock().
 *
 * @notapi
 */
void dbg_check_lock(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#4");
  dbg_enter_lock();
}

/**
 * @brief   Guard code for @p chSysUnlock().
 *
 * @notapi
 */
void dbg_check_unlock(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt <= 0))
    chDbgPanic("SV#5");
  dbg_leave_lock();
}

/**
 * @brief   Guard code for @p chSysLockFromIsr().
 *
 * @notapi
 */
void dbg_check_lock_from_isr(void) {

  if ((dbg_isr_cnt <= 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#6");
  dbg_enter_lock();
}

/**
 * @brief   Guard code for @p chSysUnlockFromIsr().
 *
 * @notapi
 */
void dbg_check_unlock_from_isr(void) {

  if ((dbg_isr_cnt <= 0) || (dbg_lock_cnt <= 0))
    chDbgPanic("SV#7");
  dbg_leave_lock();
}

/**
 * @brief   Guard code for @p CH_IRQ_PROLOGUE().
 *
 * @notapi
 */
void dbg_check_enter_isr(void) {

  port_lock_from_isr();
  if ((dbg_isr_cnt < 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#8");
  dbg_isr_cnt++;
  port_unlock_from_isr();
}

/**
 * @brief   Guard code for @p CH_IRQ_EPILOGUE().
 *
 * @notapi
 */
void dbg_check_leave_isr(void) {

  port_lock_from_isr();
  if ((dbg_isr_cnt <= 0) || (dbg_lock_cnt != 0))
    chDbgPanic("SV#9");
  dbg_isr_cnt--;
  port_unlock_from_isr();
}

/**
 * @brief   I-class functions context check.
 * @details Verifies that the system is in an appropriate state for invoking
 *          an I-class API function. A panic is generated if the state is
 *          not compatible.
 *
 * @api
 */
void chDbgCheckClassI(void) {

  if ((dbg_isr_cnt < 0) || (dbg_lock_cnt <= 0))
    chDbgPanic("SV#10");
}

/**
 * @brief   S-class functions context check.
 * @details Verifies that the system is in an appropriate state for invoking
 *          an S-class API function. A panic is generated if the state is
 *          not compatible.
 *
 * @api
 */
void chDbgCheckClassS(void) {

  if ((dbg_isr_cnt != 0) || (dbg_lock_cnt <= 0))
    chDbgPanic("SV#11");
}

#endif /* CH_DBG_SYSTEM_STATE_CHECK */

/*===========================================================================*/
/* Trace related code and variables.                                         */
/*===========================================================================*/

#if CH_DBG_ENABLE_TRACE || defined(__DOXYGEN__)
/**
 * @brief   Public trace buffer.
 */
ch_trace_buffer_t dbg_trace_buffer;

/**
 * @brief   Trace circular buffer subsystem initialization.
 * @note    Internal use only.
 */
void _trace_init(void) {

  dbg_trace_buffer.tb_size = CH_TRACE_BUFFER_SIZE;
  dbg_trace_buffer.tb_ptr = &dbg_trace_buffer.tb_buffer[0];
}

/**
 * @brief   Inserts in the circular debug trace buffer a context switch record.
 *
 * @param[in] otp       the thread being switched out
 *
 * @notapi
 */
void dbg_trace(Thread *otp) {

  dbg_trace_buffer.tb_ptr->se_time   = chTimeNow();
  dbg_trace_buffer.tb_ptr->se_tp     = currp;
  dbg_trace_buffer.tb_ptr->se_wtobjp = otp->p_u.wtobjp;
  dbg_trace_buffer.tb_ptr->se_state  = (uint8_t)otp->p_state;
  if (++dbg_trace_buffer.tb_ptr >=
      &dbg_trace_buffer.tb_buffer[CH_TRACE_BUFFER_SIZE])
    dbg_trace_buffer.tb_ptr = &dbg_trace_buffer.tb_buffer[0];
}
#endif /* CH_DBG_ENABLE_TRACE */

/*===========================================================================*/
/* Panic related code and variables.                                         */
/*===========================================================================*/

#if CH_DBG_ENABLED || defined(__DOXYGEN__)
/**
 * @brief   Pointer to the panic message.
 * @details This pointer is meant to be accessed through the debugger, it is
 *          written once and then the system is halted.
 */
const char *dbg_panic_msg;

/**
 * @brief   Prints a panic message on the console and then halts the system.
 *
 * @param[in] msg       the pointer to the panic message string
 */
void chDbgPanic(const char *msg) {

  dbg_panic_msg = msg;
  chSysHalt();
}
#endif /* CH_DBG_ENABLED */

/** @} */
