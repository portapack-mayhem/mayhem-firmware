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
 * @file    chsem.c
 * @brief   Semaphores code.
 *
 * @addtogroup semaphores
 * @details Semaphores related APIs and services.
 *
 *          <h2>Operation mode</h2>
 *          Semaphores are a flexible synchronization primitive, ChibiOS/RT
 *          implements semaphores in their "counting semaphores" variant as
 *          defined by Edsger Dijkstra plus several enhancements like:
 *          - Wait operation with timeout.
 *          - Reset operation.
 *          - Atomic wait+signal operation.
 *          - Return message from the wait operation (OK, RESET, TIMEOUT).
 *          .
 *          The binary semaphores variant can be easily implemented using
 *          counting semaphores.<br>
 *          Operations defined for semaphores:
 *          - <b>Signal</b>: The semaphore counter is increased and if the
 *            result is non-positive then a waiting thread is removed from
 *            the semaphore queue and made ready for execution.
 *          - <b>Wait</b>: The semaphore counter is decreased and if the result
 *            becomes negative the thread is queued in the semaphore and
 *            suspended.
 *          - <b>Reset</b>: The semaphore counter is reset to a non-negative
 *            value and all the threads in the queue are released.
 *          .
 *          Semaphores can be used as guards for mutual exclusion zones
 *          (note that mutexes are recommended for this kind of use) but
 *          also have other uses, queues guards and counters for example.<br>
 *          Semaphores usually use a FIFO queuing strategy but it is possible
 *          to make them order threads by priority by enabling
 *          @p CH_USE_SEMAPHORES_PRIORITY in @p chconf.h.
 * @pre     In order to use the semaphore APIs the @p CH_USE_SEMAPHORES
 *          option must be enabled in @p chconf.h.
 * @{
 */

#include "ch.h"

#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)

#if CH_USE_SEMAPHORES_PRIORITY
#define sem_insert(tp, qp) prio_insert(tp, qp)
#else
#define sem_insert(tp, qp) queue_insert(tp, qp)
#endif

/**
 * @brief   Initializes a semaphore with the specified counter value.
 *
 * @param[out] sp       pointer to a @p Semaphore structure
 * @param[in] n         initial value of the semaphore counter. Must be
 *                      non-negative.
 *
 * @init
 */
void chSemInit(Semaphore *sp, cnt_t n) {

  chDbgCheck((sp != NULL) && (n >= 0), "chSemInit");

  queue_init(&sp->s_queue);
  sp->s_cnt = n;
}

/**
 * @brief   Performs a reset operation on the semaphore.
 * @post    After invoking this function all the threads waiting on the
 *          semaphore, if any, are released and the semaphore counter is set
 *          to the specified, non negative, value.
 * @note    The released threads can recognize they were waked up by a reset
 *          rather than a signal because the @p chSemWait() will return
 *          @p RDY_RESET instead of @p RDY_OK.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @param[in] n         the new value of the semaphore counter. The value must
 *                      be non-negative.
 *
 * @api
 */
void chSemReset(Semaphore *sp, cnt_t n) {

  chSysLock();
  chSemResetI(sp, n);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Performs a reset operation on the semaphore.
 * @post    After invoking this function all the threads waiting on the
 *          semaphore, if any, are released and the semaphore counter is set
 *          to the specified, non negative, value.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 * @note    The released threads can recognize they were waked up by a reset
 *          rather than a signal because the @p chSemWait() will return
 *          @p RDY_RESET instead of @p RDY_OK.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @param[in] n         the new value of the semaphore counter. The value must
 *                      be non-negative.
 *
 * @iclass
 */
void chSemResetI(Semaphore *sp, cnt_t n) {
  cnt_t cnt;

  chDbgCheckClassI();
  chDbgCheck((sp != NULL) && (n >= 0), "chSemResetI");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemResetI(), #1",
              "inconsistent semaphore");

  cnt = sp->s_cnt;
  sp->s_cnt = n;
  while (++cnt <= 0)
    chSchReadyI(lifo_remove(&sp->s_queue))->p_u.rdymsg = RDY_RESET;
}

/**
 * @brief   Performs a wait operation on a semaphore.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the thread has not stopped on the semaphore or the
 *                      semaphore has been signaled.
 * @retval RDY_RESET    if the semaphore has been reset using @p chSemReset().
 *
 * @api
 */
msg_t chSemWait(Semaphore *sp) {
  msg_t msg;

  chSysLock();
  msg = chSemWaitS(sp);
  chSysUnlock();
  return msg;
}

/**
 * @brief   Performs a wait operation on a semaphore.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the thread has not stopped on the semaphore or the
 *                      semaphore has been signaled.
 * @retval RDY_RESET    if the semaphore has been reset using @p chSemReset().
 *
 * @sclass
 */
msg_t chSemWaitS(Semaphore *sp) {

  chDbgCheckClassS();
  chDbgCheck(sp != NULL, "chSemWaitS");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemWaitS(), #1",
              "inconsistent semaphore");

  if (--sp->s_cnt < 0) {
    currp->p_u.wtobjp = sp;
    sem_insert(currp, &sp->s_queue);
    chSchGoSleepS(THD_STATE_WTSEM);
    return currp->p_u.rdymsg;
  }
  return RDY_OK;
}

/**
 * @brief   Performs a wait operation on a semaphore with timeout specification.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the thread has not stopped on the semaphore or the
 *                      semaphore has been signaled.
 * @retval RDY_RESET    if the semaphore has been reset using @p chSemReset().
 * @retval RDY_TIMEOUT  if the semaphore has not been signaled or reset within
 *                      the specified timeout.
 *
 * @api
 */
msg_t chSemWaitTimeout(Semaphore *sp, systime_t time) {
  msg_t msg;

  chSysLock();
  msg = chSemWaitTimeoutS(sp, time);
  chSysUnlock();
  return msg;
}

/**
 * @brief   Performs a wait operation on a semaphore with timeout specification.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the thread has not stopped on the semaphore or the
 *                      semaphore has been signaled.
 * @retval RDY_RESET    if the semaphore has been reset using @p chSemReset().
 * @retval RDY_TIMEOUT  if the semaphore has not been signaled or reset within
 *                      the specified timeout.
 *
 * @sclass
 */
msg_t chSemWaitTimeoutS(Semaphore *sp, systime_t time) {

  chDbgCheckClassS();
  chDbgCheck(sp != NULL, "chSemWaitTimeoutS");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemWaitTimeoutS(), #1",
              "inconsistent semaphore");

  if (--sp->s_cnt < 0) {
    if (TIME_IMMEDIATE == time) {
      sp->s_cnt++;
      return RDY_TIMEOUT;
    }
    currp->p_u.wtobjp = sp;
    sem_insert(currp, &sp->s_queue);
    return chSchGoSleepTimeoutS(THD_STATE_WTSEM, time);
  }
  return RDY_OK;
}

/**
 * @brief   Performs a signal operation on a semaphore.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 *
 * @api
 */
void chSemSignal(Semaphore *sp) {

  chDbgCheck(sp != NULL, "chSemSignal");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemSignal(), #1",
              "inconsistent semaphore");

  chSysLock();
  if (++sp->s_cnt <= 0)
    chSchWakeupS(fifo_remove(&sp->s_queue), RDY_OK);
  chSysUnlock();
}

/**
 * @brief   Performs a signal operation on a semaphore.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] sp    pointer to a @p Semaphore structure
 *
 * @iclass
 */
void chSemSignalI(Semaphore *sp) {

  chDbgCheckClassI();
  chDbgCheck(sp != NULL, "chSemSignalI");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemSignalI(), #1",
              "inconsistent semaphore");

  if (++sp->s_cnt <= 0) {
    /* Note, it is done this way in order to allow a tail call on
             chSchReadyI().*/
    Thread *tp = fifo_remove(&sp->s_queue);
    tp->p_u.rdymsg = RDY_OK;
    chSchReadyI(tp);
  }
}

/**
 * @brief   Adds the specified value to the semaphore counter.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] sp        pointer to a @p Semaphore structure
 * @param[in] n         value to be added to the semaphore counter. The value
 *                      must be positive.
 *
 * @iclass
 */
void chSemAddCounterI(Semaphore *sp, cnt_t n) {

  chDbgCheckClassI();
  chDbgCheck((sp != NULL) && (n > 0), "chSemAddCounterI");
  chDbgAssert(((sp->s_cnt >= 0) && isempty(&sp->s_queue)) ||
              ((sp->s_cnt < 0) && notempty(&sp->s_queue)),
              "chSemAddCounterI(), #1",
              "inconsistent semaphore");

  while (n > 0) {
    if (++sp->s_cnt <= 0)
      chSchReadyI(fifo_remove(&sp->s_queue))->p_u.rdymsg = RDY_OK;
    n--;
  }
}

#if CH_USE_SEMSW || defined(__DOXYGEN__)
/**
 * @brief   Performs atomic signal and wait operations on two semaphores.
 * @pre     The configuration option @p CH_USE_SEMSW must be enabled in order
 *          to use this function.
 *
 * @param[in] sps       pointer to a @p Semaphore structure to be signaled
 * @param[in] spw       pointer to a @p Semaphore structure to wait on
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the thread has not stopped on the semaphore or the
 *                      semaphore has been signaled.
 * @retval RDY_RESET    if the semaphore has been reset using @p chSemReset().
 *
 * @api
 */
msg_t chSemSignalWait(Semaphore *sps, Semaphore *spw) {
  msg_t msg;

  chDbgCheck((sps != NULL) && (spw != NULL), "chSemSignalWait");
  chDbgAssert(((sps->s_cnt >= 0) && isempty(&sps->s_queue)) ||
              ((sps->s_cnt < 0) && notempty(&sps->s_queue)),
              "chSemSignalWait(), #1",
              "inconsistent semaphore");
  chDbgAssert(((spw->s_cnt >= 0) && isempty(&spw->s_queue)) ||
              ((spw->s_cnt < 0) && notempty(&spw->s_queue)),
              "chSemSignalWait(), #2",
              "inconsistent semaphore");

  chSysLock();
  if (++sps->s_cnt <= 0)
    chSchReadyI(fifo_remove(&sps->s_queue))->p_u.rdymsg = RDY_OK;
  if (--spw->s_cnt < 0) {
    Thread *ctp = currp;
    sem_insert(ctp, &spw->s_queue);
    ctp->p_u.wtobjp = spw;
    chSchGoSleepS(THD_STATE_WTSEM);
    msg = ctp->p_u.rdymsg;
  }
  else {
    chSchRescheduleS();
    msg = RDY_OK;
  }
  chSysUnlock();
  return msg;
}
#endif /* CH_USE_SEMSW */

#endif /* CH_USE_SEMAPHORES */

/** @} */
