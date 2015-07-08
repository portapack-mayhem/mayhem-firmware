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
 * @file    chschd.c
 * @brief   Scheduler code.
 *
 * @addtogroup scheduler
 * @details This module provides the default portable scheduler code,
 *          scheduler functions can be individually captured by the port
 *          layer in order to provide architecture optimized equivalents.
 *          When a function is captured its default code is not built into
 *          the OS image, the optimized version is included instead.
 * @{
 */

#include "ch.h"

/**
 * @brief   Ready list header.
 */
#if !defined(PORT_OPTIMIZED_RLIST_VAR) || defined(__DOXYGEN__)
ReadyList rlist;
#endif /* !defined(PORT_OPTIMIZED_RLIST_VAR) */

/**
 * @brief   Scheduler initialization.
 *
 * @notapi
 */
void _scheduler_init(void) {

  queue_init(&rlist.r_queue);
  rlist.r_prio = NOPRIO;
#if CH_USE_REGISTRY
  rlist.r_newer = rlist.r_older = (Thread *)&rlist;
#endif
}

/**
 * @brief   Inserts a thread in the Ready List.
 * @details The thread is positioned behind all threads with higher or equal
 *          priority.
 * @pre     The thread must not be already inserted in any list through its
 *          @p p_next and @p p_prev or list corruption would occur.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] tp        the thread to be made ready
 * @return              The thread pointer.
 *
 * @iclass
 */
#if !defined(PORT_OPTIMIZED_READYI) || defined(__DOXYGEN__)
Thread *chSchReadyI(Thread *tp) {
  Thread *cp;

  chDbgCheckClassI();

  /* Integrity checks.*/
  chDbgAssert((tp->p_state != THD_STATE_READY) &&
              (tp->p_state != THD_STATE_FINAL),
              "chSchReadyI(), #1",
              "invalid state");

  tp->p_state = THD_STATE_READY;
  cp = (Thread *)&rlist.r_queue;
  do {
    cp = cp->p_next;
  } while (cp->p_prio >= tp->p_prio);
  /* Insertion on p_prev.*/
  tp->p_next = cp;
  tp->p_prev = cp->p_prev;
  tp->p_prev->p_next = cp->p_prev = tp;
  return tp;
}
#endif /* !defined(PORT_OPTIMIZED_READYI) */

/**
 * @brief   Puts the current thread to sleep into the specified state.
 * @details The thread goes into a sleeping state. The possible
 *          @ref thread_states are defined into @p threads.h.
 *
 * @param[in] newstate  the new thread state
 *
 * @sclass
 */
#if !defined(PORT_OPTIMIZED_GOSLEEPS) || defined(__DOXYGEN__)
void chSchGoSleepS(tstate_t newstate) {
  Thread *otp;

  chDbgCheckClassS();

  (otp = currp)->p_state = newstate;
#if CH_TIME_QUANTUM > 0
  /* The thread is renouncing its remaining time slices so it will have a new
     time quantum when it will wakeup.*/
  otp->p_preempt = CH_TIME_QUANTUM;
#endif
  setcurrp(fifo_remove(&rlist.r_queue));
  currp->p_state = THD_STATE_CURRENT;
  chSysSwitch(currp, otp);
}
#endif /* !defined(PORT_OPTIMIZED_GOSLEEPS) */

#if !defined(PORT_OPTIMIZED_GOSLEEPTIMEOUTS) || defined(__DOXYGEN__)
/*
 * Timeout wakeup callback.
 */
static void wakeup(void *p) {
  Thread *tp = (Thread *)p;

  chSysLockFromIsr();
  switch (tp->p_state) {
  case THD_STATE_READY:
    /* Handling the special case where the thread has been made ready by
       another thread with higher priority.*/
    chSysUnlockFromIsr();
    return;
#if CH_USE_SEMAPHORES || CH_USE_QUEUES ||                                   \
    (CH_USE_CONDVARS && CH_USE_CONDVARS_TIMEOUT)
#if CH_USE_SEMAPHORES
  case THD_STATE_WTSEM:
    chSemFastSignalI((Semaphore *)tp->p_u.wtobjp);
    /* Falls into, intentional. */
#endif
#if CH_USE_QUEUES
  case THD_STATE_WTQUEUE:
#endif
#if CH_USE_CONDVARS && CH_USE_CONDVARS_TIMEOUT
  case THD_STATE_WTCOND:
#endif
    /* States requiring dequeuing.*/
    dequeue(tp);
#endif
  }
  tp->p_u.rdymsg = RDY_TIMEOUT;
  chSchReadyI(tp);
  chSysUnlockFromIsr();
}

/**
 * @brief   Puts the current thread to sleep into the specified state with
 *          timeout specification.
 * @details The thread goes into a sleeping state, if it is not awakened
 *          explicitly within the specified timeout then it is forcibly
 *          awakened with a @p RDY_TIMEOUT low level message. The possible
 *          @ref thread_states are defined into @p threads.h.
 *
 * @param[in] newstate  the new thread state
 * @param[in] time      the number of ticks before the operation timeouts, the
 *                      special values are handled as follow:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state, this is equivalent to invoking
 *                        @p chSchGoSleepS() but, of course, less efficient.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 * @return              The wakeup message.
 * @retval RDY_TIMEOUT if a timeout occurs.
 *
 * @sclass
 */
msg_t chSchGoSleepTimeoutS(tstate_t newstate, systime_t time) {

  chDbgCheckClassS();

  if (TIME_INFINITE != time) {
    VirtualTimer vt;

    chVTSetI(&vt, time, wakeup, currp);
    chSchGoSleepS(newstate);
    if (chVTIsArmedI(&vt))
      chVTResetI(&vt);
  }
  else
    chSchGoSleepS(newstate);
  return currp->p_u.rdymsg;
}
#endif /* !defined(PORT_OPTIMIZED_GOSLEEPTIMEOUTS) */

/**
 * @brief   Wakes up a thread.
 * @details The thread is inserted into the ready list or immediately made
 *          running depending on its relative priority compared to the current
 *          thread.
 * @pre     The thread must not be already inserted in any list through its
 *          @p p_next and @p p_prev or list corruption would occur.
 * @note    It is equivalent to a @p chSchReadyI() followed by a
 *          @p chSchRescheduleS() but much more efficient.
 * @note    The function assumes that the current thread has the highest
 *          priority.
 *
 * @param[in] ntp       the Thread to be made ready
 * @param[in] msg       message to the awakened thread
 *
 * @sclass
 */
#if !defined(PORT_OPTIMIZED_WAKEUPS) || defined(__DOXYGEN__)
void chSchWakeupS(Thread *ntp, msg_t msg) {

  chDbgCheckClassS();

  ntp->p_u.rdymsg = msg;
  /* If the waken thread has a not-greater priority than the current
     one then it is just inserted in the ready list else it made
     running immediately and the invoking thread goes in the ready
     list instead.*/
  if (ntp->p_prio <= currp->p_prio)
    chSchReadyI(ntp);
  else {
    Thread *otp = chSchReadyI(currp);
    setcurrp(ntp);
    ntp->p_state = THD_STATE_CURRENT;
    chSysSwitch(ntp, otp);
  }
}
#endif /* !defined(PORT_OPTIMIZED_WAKEUPS) */

/**
 * @brief   Performs a reschedule if a higher priority thread is runnable.
 * @details If a thread with a higher priority than the current thread is in
 *          the ready list then make the higher priority thread running.
 *
 * @sclass
 */
#if !defined(PORT_OPTIMIZED_RESCHEDULES) || defined(__DOXYGEN__)
void chSchRescheduleS(void) {

  chDbgCheckClassS();

  if (chSchIsRescRequiredI())
    chSchDoRescheduleAhead();
}
#endif /* !defined(PORT_OPTIMIZED_RESCHEDULES) */

/**
 * @brief   Evaluates if preemption is required.
 * @details The decision is taken by comparing the relative priorities and
 *          depending on the state of the round robin timeout counter.
 * @note    Not a user function, it is meant to be invoked by the scheduler
 *          itself or from within the port layer.
 *
 * @retval TRUE         if there is a thread that must go in running state
 *                      immediately.
 * @retval FALSE        if preemption is not required.
 *
 * @special
 */
#if !defined(PORT_OPTIMIZED_ISPREEMPTIONREQUIRED) || defined(__DOXYGEN__)
bool_t chSchIsPreemptionRequired(void) {
  tprio_t p1 = firstprio(&rlist.r_queue);
  tprio_t p2 = currp->p_prio;
#if CH_TIME_QUANTUM > 0
  /* If the running thread has not reached its time quantum, reschedule only
     if the first thread on the ready queue has a higher priority.
     Otherwise, if the running thread has used up its time quantum, reschedule
     if the first thread on the ready queue has equal or higher priority.*/
  return currp->p_preempt ? p1 > p2 : p1 >= p2;
#else
  /* If the round robin preemption feature is not enabled then performs a
     simpler comparison.*/
  return p1 > p2;
#endif
}
#endif /* !defined(PORT_OPTIMIZED_ISPREEMPTIONREQUIRED) */

/**
 * @brief   Switches to the first thread on the runnable queue.
 * @details The current thread is positioned in the ready list behind all
 *          threads having the same priority. The thread regains its time
 *          quantum.
 * @note    Not a user function, it is meant to be invoked by the scheduler
 *          itself or from within the port layer.
 *
 * @special
 */
#if !defined(PORT_OPTIMIZED_DORESCHEDULEBEHIND) || defined(__DOXYGEN__)
void chSchDoRescheduleBehind(void) {
  Thread *otp;

  otp = currp;
  /* Picks the first thread from the ready queue and makes it current.*/
  setcurrp(fifo_remove(&rlist.r_queue));
  currp->p_state = THD_STATE_CURRENT;
#if CH_TIME_QUANTUM > 0
  otp->p_preempt = CH_TIME_QUANTUM;
#endif
  chSchReadyI(otp);
  chSysSwitch(currp, otp);
}
#endif /* !defined(PORT_OPTIMIZED_DORESCHEDULEBEHIND) */

/**
 * @brief   Switches to the first thread on the runnable queue.
 * @details The current thread is positioned in the ready list ahead of all
 *          threads having the same priority.
 * @note    Not a user function, it is meant to be invoked by the scheduler
 *          itself or from within the port layer.
 *
 * @special
 */
#if !defined(PORT_OPTIMIZED_DORESCHEDULEAHEAD) || defined(__DOXYGEN__)
void chSchDoRescheduleAhead(void) {
  Thread *otp, *cp;

  otp = currp;
  /* Picks the first thread from the ready queue and makes it current.*/
  setcurrp(fifo_remove(&rlist.r_queue));
  currp->p_state = THD_STATE_CURRENT;

  otp->p_state = THD_STATE_READY;
  cp = (Thread *)&rlist.r_queue;
  do {
    cp = cp->p_next;
  } while (cp->p_prio > otp->p_prio);
  /* Insertion on p_prev.*/
  otp->p_next = cp;
  otp->p_prev = cp->p_prev;
  otp->p_prev->p_next = cp->p_prev = otp;

  chSysSwitch(currp, otp);
}
#endif /* !defined(PORT_OPTIMIZED_DORESCHEDULEAHEAD) */

/**
 * @brief   Switches to the first thread on the runnable queue.
 * @details The current thread is positioned in the ready list behind or
 *          ahead of all threads having the same priority depending on
 *          if it used its whole time slice.
 * @note    Not a user function, it is meant to be invoked by the scheduler
 *          itself or from within the port layer.
 *
 * @special
 */
#if !defined(PORT_OPTIMIZED_DORESCHEDULE) || defined(__DOXYGEN__)
void chSchDoReschedule(void) {

#if CH_TIME_QUANTUM > 0
  /* If CH_TIME_QUANTUM is enabled then there are two different scenarios to
     handle on preemption: time quantum elapsed or not.*/
  if (currp->p_preempt == 0) {
    /* The thread consumed its time quantum so it is enqueued behind threads
       with same priority level, however, it acquires a new time quantum.*/
    chSchDoRescheduleBehind();
  }
  else {
    /* The thread didn't consume all its time quantum so it is put ahead of
       threads with equal priority and does not acquire a new time quantum.*/
    chSchDoRescheduleAhead();
  }
#else /* !(CH_TIME_QUANTUM > 0) */
  /* If the round-robin mechanism is disabled then the thread goes always
     ahead of its peers.*/
  chSchDoRescheduleAhead();
#endif /* !(CH_TIME_QUANTUM > 0) */
}
#endif /* !defined(PORT_OPTIMIZED_DORESCHEDULE) */

/** @} */
