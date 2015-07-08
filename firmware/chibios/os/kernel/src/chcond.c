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
/*
   Concepts and parts of this file have been contributed by Leon Woestenberg.
 */

/**
 * @file    chcond.c
 * @brief   Condition Variables code.
 *
 * @addtogroup condvars Condition Variables
 * @details This module implements the Condition Variables mechanism. Condition
 *          variables are an extensions to the Mutex subsystem and cannot
 *          work alone.
 *          <h2>Operation mode</h2>
 *          The condition variable is a synchronization object meant to be
 *          used inside a zone protected by a @p Mutex. Mutexes and CondVars
 *          together can implement a Monitor construct.
 * @pre     In order to use the condition variable APIs the @p CH_USE_CONDVARS
 *          option must be enabled in @p chconf.h.
 * @{
 */

#include "ch.h"

#if (CH_USE_CONDVARS && CH_USE_MUTEXES) || defined(__DOXYGEN__)

/**
 * @brief   Initializes s @p CondVar structure.
 *
 * @param[out] cp       pointer to a @p CondVar structure
 *
 * @init
 */
void chCondInit(CondVar *cp) {

  chDbgCheck(cp != NULL, "chCondInit");

  queue_init(&cp->c_queue);
}

/**
 * @brief   Signals one thread that is waiting on the condition variable.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 *
 * @api
 */
void chCondSignal(CondVar *cp) {

  chDbgCheck(cp != NULL, "chCondSignal");

  chSysLock();
  if (notempty(&cp->c_queue))
    chSchWakeupS(fifo_remove(&cp->c_queue), RDY_OK);
  chSysUnlock();
}

/**
 * @brief   Signals one thread that is waiting on the condition variable.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 *
 * @iclass
 */
void chCondSignalI(CondVar *cp) {

  chDbgCheckClassI();
  chDbgCheck(cp != NULL, "chCondSignalI");

  if (notempty(&cp->c_queue))
    chSchReadyI(fifo_remove(&cp->c_queue))->p_u.rdymsg = RDY_OK;
}

/**
 * @brief   Signals all threads that are waiting on the condition variable.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 *
 * @api
 */
void chCondBroadcast(CondVar *cp) {

  chSysLock();
  chCondBroadcastI(cp);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Signals all threads that are waiting on the condition variable.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 *
 * @iclass
 */
void chCondBroadcastI(CondVar *cp) {

  chDbgCheckClassI();
  chDbgCheck(cp != NULL, "chCondBroadcastI");

  /* Empties the condition variable queue and inserts all the Threads into the
     ready list in FIFO order. The wakeup message is set to @p RDY_RESET in
     order to make a chCondBroadcast() detectable from a chCondSignal().*/
  while (cp->c_queue.p_next != (void *)&cp->c_queue)
    chSchReadyI(fifo_remove(&cp->c_queue))->p_u.rdymsg = RDY_RESET;
}

/**
 * @brief   Waits on the condition variable releasing the mutex lock.
 * @details Releases the currently owned mutex, waits on the condition
 *          variable, and finally acquires the mutex again. All the sequence
 *          is performed atomically.
 * @pre     The invoking thread <b>must</b> have at least one owned mutex.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the condition variable.
 * @retval RDY_OK       if the condvar has been signaled using
 *                      @p chCondSignal().
 * @retval RDY_RESET    if the condvar has been signaled using
 *                      @p chCondBroadcast().
 *
 * @api
 */
msg_t chCondWait(CondVar *cp) {
  msg_t msg;

  chSysLock();
  msg = chCondWaitS(cp);
  chSysUnlock();
  return msg;
}

/**
 * @brief   Waits on the condition variable releasing the mutex lock.
 * @details Releases the currently owned mutex, waits on the condition
 *          variable, and finally acquires the mutex again. All the sequence
 *          is performed atomically.
 * @pre     The invoking thread <b>must</b> have at least one owned mutex.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the condition variable.
 * @retval RDY_OK       if the condvar has been signaled using
 *                      @p chCondSignal().
 * @retval RDY_RESET    if the condvar has been signaled using
 *                      @p chCondBroadcast().
 *
 * @sclass
 */
msg_t chCondWaitS(CondVar *cp) {
  Thread *ctp = currp;
  Mutex *mp;
  msg_t msg;

  chDbgCheckClassS();
  chDbgCheck(cp != NULL, "chCondWaitS");
  chDbgAssert(ctp->p_mtxlist != NULL,
              "chCondWaitS(), #1",
              "not owning a mutex");

  mp = chMtxUnlockS();
  ctp->p_u.wtobjp = cp;
  prio_insert(ctp, &cp->c_queue);
  chSchGoSleepS(THD_STATE_WTCOND);
  msg = ctp->p_u.rdymsg;
  chMtxLockS(mp);
  return msg;
}

#if CH_USE_CONDVARS_TIMEOUT || defined(__DOXYGEN__)
/**
 * @brief   Waits on the condition variable releasing the mutex lock.
 * @details Releases the currently owned mutex, waits on the condition
 *          variable, and finally acquires the mutex again. All the sequence
 *          is performed atomically.
 * @pre     The invoking thread <b>must</b> have at least one owned mutex.
 * @pre     The configuration option @p CH_USE_CONDVARS_TIMEOUT must be enabled
 *          in order to use this function.
 * @post    Exiting the function because a timeout does not re-acquire the
 *          mutex, the mutex ownership is lost.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 * @param[in] time      the number of ticks before the operation timeouts, the
 *                      special values are handled as follow:
 *                      - @a TIME_INFINITE no timeout.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the condition variable.
 * @retval RDY_OK       if the condvar has been signaled using
 *                      @p chCondSignal().
 * @retval RDY_RESET    if the condvar has been signaled using
 *                      @p chCondBroadcast().
 * @retval RDY_TIMEOUT  if the condvar has not been signaled within the
 *                      specified timeout.
 *
 * @api
 */
msg_t chCondWaitTimeout(CondVar *cp, systime_t time) {
  msg_t msg;

  chSysLock();
  msg = chCondWaitTimeoutS(cp, time);
  chSysUnlock();
  return msg;
}

/**
 * @brief   Waits on the condition variable releasing the mutex lock.
 * @details Releases the currently owned mutex, waits on the condition
 *          variable, and finally acquires the mutex again. All the sequence
 *          is performed atomically.
 * @pre     The invoking thread <b>must</b> have at least one owned mutex.
 * @pre     The configuration option @p CH_USE_CONDVARS_TIMEOUT must be enabled
 *          in order to use this function.
 * @post    Exiting the function because a timeout does not re-acquire the
 *          mutex, the mutex ownership is lost.
 *
 * @param[in] cp        pointer to the @p CondVar structure
 * @param[in] time      the number of ticks before the operation timeouts, the
 *                      special values are handled as follow:
 *                      - @a TIME_INFINITE no timeout.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the condition variable.
 * @retval RDY_OK       if the condvar has been signaled using
 *                      @p chCondSignal().
 * @retval RDY_RESET    if the condvar has been signaled using
 *                      @p chCondBroadcast().
 * @retval RDY_TIMEOUT  if the condvar has not been signaled within the
 *                      specified timeout.
 *
 * @sclass
 */
msg_t chCondWaitTimeoutS(CondVar *cp, systime_t time) {
  Mutex *mp;
  msg_t msg;

  chDbgCheckClassS();
  chDbgCheck((cp != NULL) && (time != TIME_IMMEDIATE), "chCondWaitTimeoutS");
  chDbgAssert(currp->p_mtxlist != NULL,
              "chCondWaitTimeoutS(), #1",
              "not owning a mutex");

  mp = chMtxUnlockS();
  currp->p_u.wtobjp = cp;
  prio_insert(currp, &cp->c_queue);
  msg = chSchGoSleepTimeoutS(THD_STATE_WTCOND, time);
  if (msg != RDY_TIMEOUT)
    chMtxLockS(mp);
  return msg;
}
#endif /* CH_USE_CONDVARS_TIMEOUT */

#endif /* CH_USE_CONDVARS && CH_USE_MUTEXES */

/** @} */
