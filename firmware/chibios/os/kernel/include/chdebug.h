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
 * @file    chdebug.h
 * @brief   Debug macros and structures.
 *
 * @addtogroup debug
 * @{
 */

#ifndef _CHDEBUG_H_
#define _CHDEBUG_H_

#if CH_DBG_ENABLE_ASSERTS     || CH_DBG_ENABLE_CHECKS      ||               \
    CH_DBG_ENABLE_STACK_CHECK || CH_DBG_SYSTEM_STATE_CHECK
#define CH_DBG_ENABLED              TRUE
#else
#define CH_DBG_ENABLED              FALSE
#endif

#define __QUOTE_THIS(p) #p

/*===========================================================================*/
/**
 * @name    Debug related settings
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Trace buffer entries.
 */
#ifndef CH_TRACE_BUFFER_SIZE
#define CH_TRACE_BUFFER_SIZE        64
#endif

/**
 * @brief   Fill value for thread stack area in debug mode.
 */
#ifndef CH_STACK_FILL_VALUE
#define CH_STACK_FILL_VALUE         0x55
#endif

/**
 * @brief   Fill value for thread area in debug mode.
 * @note    The chosen default value is 0xFF in order to make evident which
 *          thread fields were not initialized when inspecting the memory with
 *          a debugger. A uninitialized field is not an error in itself but it
 *          better to know it.
 */
#ifndef CH_THREAD_FILL_VALUE
#define CH_THREAD_FILL_VALUE        0xFF
#endif

/** @} */

/*===========================================================================*/
/* System state checker related code and variables.                          */
/*===========================================================================*/

#if !CH_DBG_SYSTEM_STATE_CHECK
#define dbg_enter_lock()
#define dbg_leave_lock()
#define dbg_check_disable()
#define dbg_check_suspend()
#define dbg_check_enable()
#define dbg_check_lock()
#define dbg_check_unlock()
#define dbg_check_lock_from_isr()
#define dbg_check_unlock_from_isr()
#define dbg_check_enter_isr()
#define dbg_check_leave_isr()
#define chDbgCheckClassI()
#define chDbgCheckClassS()
#else
#define dbg_enter_lock() (dbg_lock_cnt = 1)
#define dbg_leave_lock() (dbg_lock_cnt = 0)
#endif

/*===========================================================================*/
/* Trace related structures and macros.                                      */
/*===========================================================================*/

#if CH_DBG_ENABLE_TRACE || defined(__DOXYGEN__)
/**
 * @brief   Trace buffer record.
 */
typedef struct {
  systime_t             se_time;    /**< @brief Time of the switch event.   */
  Thread                *se_tp;     /**< @brief Switched in thread.         */
  void                  *se_wtobjp; /**< @brief Object where going to sleep.*/
  uint8_t               se_state;   /**< @brief Switched out thread state.  */
} ch_swc_event_t;

/**
 * @brief   Trace buffer header.
 */
typedef struct {
  unsigned              tb_size;    /**< @brief Trace buffer size (entries).*/
  ch_swc_event_t        *tb_ptr;    /**< @brief Pointer to the buffer front.*/
  /** @brief Ring buffer.*/
  ch_swc_event_t        tb_buffer[CH_TRACE_BUFFER_SIZE];
} ch_trace_buffer_t;

#if !defined(__DOXYGEN__)
extern ch_trace_buffer_t dbg_trace_buffer;
#endif

#endif /* CH_DBG_ENABLE_TRACE */

#if !CH_DBG_ENABLE_TRACE
/* When the trace feature is disabled this function is replaced by an empty
   macro.*/
#define dbg_trace(otp)
#endif

/*===========================================================================*/
/* Parameters checking related macros.                                       */
/*===========================================================================*/

#if CH_DBG_ENABLE_CHECKS || defined(__DOXYGEN__)
/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Function parameters check.
 * @details If the condition check fails then the kernel panics and halts.
 * @note    The condition is tested only if the @p CH_DBG_ENABLE_CHECKS switch
 *          is specified in @p chconf.h else the macro does nothing.
 *
 * @param[in] c         the condition to be verified to be true
 * @param[in] func      the undecorated function name
 *
 * @api
 */
#if !defined(chDbgCheck)
#define chDbgCheck(c, func) {                                               \
  if (!(c))                                                                 \
    chDbgPanic(__QUOTE_THIS(func)"()");                                     \
}
#endif /* !defined(chDbgCheck) */
/** @} */
#else /* !CH_DBG_ENABLE_CHECKS */
#define chDbgCheck(c, func) {                                               \
  (void)(c), (void)__QUOTE_THIS(func)"()";                                  \
}
#endif /* !CH_DBG_ENABLE_CHECKS */

/*===========================================================================*/
/* Assertions related macros.                                                */
/*===========================================================================*/

#if CH_DBG_ENABLE_ASSERTS || defined(__DOXYGEN__)
/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Condition assertion.
 * @details If the condition check fails then the kernel panics with the
 *          specified message and halts.
 * @note    The condition is tested only if the @p CH_DBG_ENABLE_ASSERTS switch
 *          is specified in @p chconf.h else the macro does nothing.
 * @note    The convention for the message is the following:<br>
 *          @<function_name@>(), #@<assert_number@>
 * @note    The remark string is not currently used except for putting a
 *          comment in the code about the assertion.
 *
 * @param[in] c         the condition to be verified to be true
 * @param[in] m         the text message
 * @param[in] r         a remark string
 *
 * @api
 */
#if !defined(chDbgAssert)
#define chDbgAssert(c, m, r) {                                              \
  if (!(c))                                                                 \
    chDbgPanic(m);                                                          \
}
#endif /* !defined(chDbgAssert) */
/** @} */
#else /* !CH_DBG_ENABLE_ASSERTS */
#define chDbgAssert(c, m, r) {(void)(c);}
#endif /* !CH_DBG_ENABLE_ASSERTS */

/*===========================================================================*/
/* Panic related macros.                                                     */
/*===========================================================================*/

#if !CH_DBG_ENABLED
/* When the debug features are disabled this function is replaced by an empty
   macro.*/
#define chDbgPanic(msg) {}
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if CH_DBG_SYSTEM_STATE_CHECK
  extern cnt_t dbg_isr_cnt;
  extern cnt_t dbg_lock_cnt;
  void dbg_check_disable(void);
  void dbg_check_suspend(void);
  void dbg_check_enable(void);
  void dbg_check_lock(void);
  void dbg_check_unlock(void);
  void dbg_check_lock_from_isr(void);
  void dbg_check_unlock_from_isr(void);
  void dbg_check_enter_isr(void);
  void dbg_check_leave_isr(void);
  void chDbgCheckClassI(void);
  void chDbgCheckClassS(void);
#endif
#if CH_DBG_ENABLE_TRACE || defined(__DOXYGEN__)
  void _trace_init(void);
  void dbg_trace(Thread *otp);
#endif
#if CH_DBG_ENABLED
  extern const char *dbg_panic_msg;
  void chDbgPanic(const char *msg);
#endif
#ifdef __cplusplus
}
#endif

#endif /* _CHDEBUG_H_ */

/** @} */
