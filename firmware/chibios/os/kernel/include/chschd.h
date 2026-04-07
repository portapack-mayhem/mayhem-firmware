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
 * @file    chschd.h
 * @brief   Scheduler macros and structures.
 *
 * @addtogroup scheduler
 * @{
 */

#ifndef _CHSCHD_H_
#define _CHSCHD_H_

/**
 * @name    Wakeup status codes
 * @{
 */
#define RDY_OK          0           /**< @brief Normal wakeup message.      */
#define RDY_TIMEOUT     -1          /**< @brief Wakeup caused by a timeout
                                         condition.                         */
#define RDY_RESET       -2          /**< @brief Wakeup caused by a reset
                                         condition.                         */
/** @} */

/**
 * @name    Priority constants
 * @{
 */
#define NOPRIO          0           /**< @brief Ready list header priority. */
#define IDLEPRIO        1           /**< @brief Idle thread priority.       */
#define LOWPRIO         2           /**< @brief Lowest user priority.       */
#define NORMALPRIO      64          /**< @brief Normal user priority.       */
#define HIGHPRIO        127         /**< @brief Highest user priority.      */
#define ABSPRIO         255         /**< @brief Greatest possible priority. */
/** @} */

/**
 * @name    Special time constants
 * @{
 */
/**
 * @brief   Zero time specification for some functions with a timeout
 *          specification.
 * @note    Not all functions accept @p TIME_IMMEDIATE as timeout parameter,
 *          see the specific function documentation.
 */
#define TIME_IMMEDIATE  ((systime_t)0)

/**
 * @brief   Infinite time specification for all functions with a timeout
 *          specification.
 */
#define TIME_INFINITE   ((systime_t)-1)
/** @} */

/**
 * @brief   Returns the priority of the first thread on the given ready list.
 *
 * @notapi
 */
#define firstprio(rlp)  ((rlp)->p_next->p_prio)

/**
 * @extends ThreadsQueue
 *
 * @brief   Ready list header.
 */
#if !defined(PORT_OPTIMIZED_READYLIST_STRUCT) || defined(__DOXYGEN__)
typedef struct {
  ThreadsQueue          r_queue;    /**< @brief Threads queue.              */
  tprio_t               r_prio;     /**< @brief This field must be
                                                initialized to zero.        */
  struct context        r_ctx;      /**< @brief Not used, present because
                                                offsets.                    */
#if CH_USE_REGISTRY || defined(__DOXYGEN__)
  Thread                *r_newer;   /**< @brief Newer registry element.     */
  Thread                *r_older;   /**< @brief Older registry element.     */
#endif
  /* End of the fields shared with the Thread structure.*/
  Thread                *r_current; /**< @brief The currently running
                                                thread.                     */
} ReadyList;
#endif /* !defined(PORT_OPTIMIZED_READYLIST_STRUCT) */

#if !defined(PORT_OPTIMIZED_RLIST_EXT) && !defined(__DOXYGEN__)
extern ReadyList rlist;
#endif /* !defined(PORT_OPTIMIZED_RLIST_EXT) */

/**
 * @brief   Current thread pointer access macro.
 * @note    This macro is not meant to be used in the application code but
 *          only from within the kernel, use the @p chThdSelf() API instead.
 * @note    It is forbidden to use this macro in order to change the pointer
 *          (currp = something), use @p setcurrp() instead.
 */
#if !defined(PORT_OPTIMIZED_CURRP) || defined(__DOXYGEN__)
#define currp rlist.r_current
#endif /* !defined(PORT_OPTIMIZED_CURRP) */

/**
 * @brief   Current thread pointer change macro.
 * @note    This macro is not meant to be used in the application code but
 *          only from within the kernel.
 *
 * @notapi
 */
#if !defined(PORT_OPTIMIZED_SETCURRP) || defined(__DOXYGEN__)
#define setcurrp(tp) (currp = (tp))
#endif /* !defined(PORT_OPTIMIZED_SETCURRP) */

/*
 * Scheduler APIs.
 */
#ifdef __cplusplus
extern "C" {
#endif
  void _scheduler_init(void);
#if !defined(PORT_OPTIMIZED_READYI)
  Thread *chSchReadyI(Thread *tp);
#endif
#if !defined(PORT_OPTIMIZED_GOSLEEPS)
  void chSchGoSleepS(tstate_t newstate);
#endif
#if !defined(PORT_OPTIMIZED_GOSLEEPTIMEOUTS)
  msg_t chSchGoSleepTimeoutS(tstate_t newstate, systime_t time);
#endif
#if !defined(PORT_OPTIMIZED_WAKEUPS)
  void chSchWakeupS(Thread *tp, msg_t msg);
#endif
#if !defined(PORT_OPTIMIZED_RESCHEDULES)
  void chSchRescheduleS(void);
#endif
#if !defined(PORT_OPTIMIZED_ISPREEMPTIONREQUIRED)
  bool_t chSchIsPreemptionRequired(void);
#endif
#if !defined(PORT_OPTIMIZED_DORESCHEDULEBEHIND) || defined(__DOXYGEN__)
  void chSchDoRescheduleBehind(void);
#endif
#if !defined(PORT_OPTIMIZED_DORESCHEDULEAHEAD) || defined(__DOXYGEN__)
  void chSchDoRescheduleAhead(void);
#endif
#if !defined(PORT_OPTIMIZED_DORESCHEDULE)
  void chSchDoReschedule(void);
#endif
#ifdef __cplusplus
}
#endif

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Determines if the current thread must reschedule.
 * @details This function returns @p TRUE if there is a ready thread with
 *          higher priority.
 *
 * @iclass
 */
#if !defined(PORT_OPTIMIZED_ISRESCHREQUIREDI) || defined(__DOXYGEN__)
#define chSchIsRescRequiredI() (firstprio(&rlist.r_queue) > currp->p_prio)
#endif /* !defined(PORT_OPTIMIZED_ISRESCHREQUIREDI) */

/**
 * @brief   Determines if yielding is possible.
 * @details This function returns @p TRUE if there is a ready thread with
 *          equal or higher priority.
 *
 * @sclass
 */
#if !defined(PORT_OPTIMIZED_CANYIELDS) || defined(__DOXYGEN__)
#define chSchCanYieldS() (firstprio(&rlist.r_queue) >= currp->p_prio)
#endif /* !defined(PORT_OPTIMIZED_CANYIELDS) */

/**
 * @brief   Yields the time slot.
 * @details Yields the CPU control to the next thread in the ready list with
 *          equal or higher priority, if any.
 *
 * @sclass
 */
#if !defined(PORT_OPTIMIZED_DOYIELDS) || defined(__DOXYGEN__)
#define chSchDoYieldS() {                                                   \
  if (chSchCanYieldS())                                                     \
    chSchDoRescheduleBehind();                                              \
}
#endif /* !defined(PORT_OPTIMIZED_DOYIELDS) */

/**
 * @brief   Inline-able preemption code.
 * @details This is the common preemption code, this function must be invoked
 *          exclusively from the port layer.
 *
 * @special
 */
#if (CH_TIME_QUANTUM > 0) || defined(__DOXYGEN__)
#define chSchPreemption() {                                                 \
  tprio_t p1 = firstprio(&rlist.r_queue);                                   \
  tprio_t p2 = currp->p_prio;                                               \
  if (currp->p_preempt) {                                                   \
    if (p1 > p2)                                                            \
      chSchDoRescheduleAhead();                                             \
  }                                                                         \
  else {                                                                    \
    if (p1 >= p2)                                                           \
      chSchDoRescheduleBehind();                                            \
  }                                                                         \
}
#else /* CH_TIME_QUANTUM == 0 */
#define chSchPreemption() {                                                 \
  if (p1 >= p2)                                                             \
    chSchDoRescheduleAhead();                                               \
}
#endif /* CH_TIME_QUANTUM == 0 */
/** @} */

#endif /* _CHSCHD_H_ */

/** @} */
