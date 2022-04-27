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
 * @file    chthreads.c
 * @brief   Threads code.
 *
 * @addtogroup threads
 * @details Threads related APIs and services.
 *
 *          <h2>Operation mode</h2>
 *          A thread is an abstraction of an independent instructions flow.
 *          In ChibiOS/RT a thread is represented by a "C" function owning
 *          a processor context, state informations and a dedicated stack
 *          area. In this scenario static variables are shared among all
 *          threads while automatic variables are local to the thread.<br>
 *          Operations defined for threads:
 *          - <b>Create</b>, a thread is started on the specified thread
 *            function. This operation is available in multiple variants,
 *            both static and dynamic.
 *          - <b>Exit</b>, a thread terminates by returning from its top
 *            level function or invoking a specific API, the thread can
 *            return a value that can be retrieved by other threads.
 *          - <b>Wait</b>, a thread waits for the termination of another
 *            thread and retrieves its return value.
 *          - <b>Resume</b>, a thread created in suspended state is started.
 *          - <b>Sleep</b>, the execution of a thread is suspended for the
 *            specified amount of time or the specified future absolute time
 *            is reached.
 *          - <b>SetPriority</b>, a thread changes its own priority level.
 *          - <b>Yield</b>, a thread voluntarily renounces to its time slot.
 *          .
 *          The threads subsystem is implicitly included in kernel however
 *          some of its part may be excluded by disabling them in @p chconf.h,
 *          see the @p CH_USE_WAITEXIT and @p CH_USE_DYNAMIC configuration
 *          options.
 * @{
 */

#include "ch.h"

/**
 * @brief   Initializes a thread structure.
 * @note    This is an internal functions, do not use it in application code.
 *
 * @param[in] tp        pointer to the thread
 * @param[in] prio      the priority level for the new thread
 * @return              The same thread pointer passed as parameter.
 *
 * @notapi
 */
Thread *_thread_init(Thread *tp, tprio_t prio) {

  tp->p_prio = prio;
  tp->p_state = THD_STATE_SUSPENDED;
  tp->p_flags = THD_MEM_MODE_STATIC;
#if CH_TIME_QUANTUM > 0
  tp->p_preempt = CH_TIME_QUANTUM;
#endif
#if CH_USE_MUTEXES
  tp->p_realprio = prio;
  tp->p_mtxlist = NULL;
#endif
#if CH_USE_EVENTS
  tp->p_epending = 0;
#endif
#if CH_DBG_THREADS_PROFILING
  tp->p_time = 0;
#endif
#if CH_USE_DYNAMIC
  tp->p_refs = 1;
#endif
#if CH_USE_REGISTRY
  tp->p_name = NULL;
  REG_INSERT(tp);
#endif
#if CH_USE_WAITEXIT
  list_init(&tp->p_waiting);
#endif
#if CH_USE_MESSAGES
  queue_init(&tp->p_msgqueue);
#endif
#if CH_DBG_ENABLE_STACK_CHECK
  tp->p_stklimit = (stkalign_t *)(tp + 1);
#endif
#if defined(THREAD_EXT_INIT_HOOK)
  THREAD_EXT_INIT_HOOK(tp);
#endif
  return tp;
}

#if CH_DBG_FILL_THREADS || defined(__DOXYGEN__)
/**
 * @brief   Memory fill utility.
 *
 * @param[in] startp    first address to fill
 * @param[in] endp      last address to fill +1
 * @param[in] v         filler value
 *
 * @notapi
 */
void _thread_memfill(uint8_t *startp, uint8_t *endp, uint8_t v) {

  while (startp < endp)
    *startp++ = v;
}
#endif /* CH_DBG_FILL_THREADS */

/**
 * @brief   Creates a new thread into a static memory area.
 * @details The new thread is initialized but not inserted in the ready list,
 *          the initial state is @p THD_STATE_SUSPENDED.
 * @post    The initialized thread can be subsequently started by invoking
 *          @p chThdResume(), @p chThdResumeI() or @p chSchWakeupS()
 *          depending on the execution context.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    Threads created using this function do not obey to the
 *          @p CH_DBG_FILL_THREADS debug option because it would keep
 *          the kernel locked for too much time.
 *
 * @param[out] wsp      pointer to a working area dedicated to the thread stack
 * @param[in] size      size of the working area
 * @param[in] prio      the priority level for the new thread
 * @param[in] pf        the thread function
 * @param[in] arg       an argument passed to the thread function. It can be
 *                      @p NULL.
 * @return              The pointer to the @p Thread structure allocated for
 *                      the thread into the working space area.
 *
 * @iclass
 */
Thread *chThdCreateI(void *wsp, size_t size,
                     tprio_t prio, tfunc_t pf, void *arg) {
  /* Thread structure is laid out in the lower part of the thread workspace.*/
  Thread *tp = wsp;

  chDbgCheckClassI();

  chDbgCheck((wsp != NULL) && (size >= THD_WA_SIZE(0)) &&
             (prio <= HIGHPRIO) && (pf != NULL),
             "chThdCreateI");
  SETUP_CONTEXT(wsp, size, pf, arg);
  return _thread_init(tp, prio);
}

/**
 * @brief   Creates a new thread into a static memory area.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 *
 * @param[out] wsp      pointer to a working area dedicated to the thread stack
 * @param[in] size      size of the working area
 * @param[in] prio      the priority level for the new thread
 * @param[in] pf        the thread function
 * @param[in] arg       an argument passed to the thread function. It can be
 *                      @p NULL.
 * @return              The pointer to the @p Thread structure allocated for
 *                      the thread into the working space area.
 *
 * @api
 */
Thread *chThdCreateStatic(void *wsp, size_t size,
                          tprio_t prio, tfunc_t pf, void *arg) {
  Thread *tp;
  
#if CH_DBG_FILL_THREADS
  _thread_memfill((uint8_t *)wsp,
                  (uint8_t *)wsp + sizeof(Thread),
                  CH_THREAD_FILL_VALUE);
  _thread_memfill((uint8_t *)wsp + sizeof(Thread),
                  (uint8_t *)wsp + size,
                  CH_STACK_FILL_VALUE);
#endif
  chSysLock();
  chSchWakeupS(tp = chThdCreateI(wsp, size, prio, pf, arg), RDY_OK);
  chSysUnlock();
  return tp;
}

/**
 * @brief   Changes the running thread priority level then reschedules if
 *          necessary.
 * @note    The function returns the real thread priority regardless of the
 *          current priority that could be higher than the real priority
 *          because the priority inheritance mechanism.
 *
 * @param[in] newprio   the new priority level of the running thread
 * @return              The old priority level.
 *
 * @api
 */
tprio_t chThdSetPriority(tprio_t newprio) {
  tprio_t oldprio;

  chDbgCheck(newprio <= HIGHPRIO, "chThdSetPriority");

  chSysLock();
#if CH_USE_MUTEXES
  oldprio = currp->p_realprio;
  if ((currp->p_prio == currp->p_realprio) || (newprio > currp->p_prio))
    currp->p_prio = newprio;
  currp->p_realprio = newprio;
#else
  oldprio = currp->p_prio;
  currp->p_prio = newprio;
#endif
  chSchRescheduleS();
  chSysUnlock();
  return oldprio;
}

/**
 * @brief   Resumes a suspended thread.
 * @pre     The specified thread pointer must refer to an initialized thread
 *          in the @p THD_STATE_SUSPENDED state.
 * @post    The specified thread is immediately started or put in the ready
 *          list depending on the relative priority levels.
 * @note    Use this function to start threads created with @p chThdCreateI().
 *
 * @param[in] tp        pointer to the thread
 * @return              The pointer to the thread.
 *
 * @api
 */
Thread *chThdResume(Thread *tp) {

  chSysLock();
  chDbgAssert(tp->p_state == THD_STATE_SUSPENDED,
              "chThdResume(), #1",
              "thread not in THD_STATE_SUSPENDED state");
  chSchWakeupS(tp, RDY_OK);
  chSysUnlock();
  return tp;
}

/**
 * @brief   Requests a thread termination.
 * @pre     The target thread must be written to invoke periodically
 *          @p chThdShouldTerminate() and terminate cleanly if it returns
 *          @p TRUE.
 * @post    The specified thread will terminate after detecting the termination
 *          condition.
 *
 * @param[in] tp        pointer to the thread
 *
 * @api
 */
void chThdTerminate(Thread *tp) {

  chSysLock();
  tp->p_flags |= THD_TERMINATE;
  chSysUnlock();
}

/**
 * @brief   Suspends the invoking thread for the specified time.
 *
 * @param[in] time      the delay in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 *
 * @api
 */
void chThdSleep(systime_t time) {

  chDbgCheck(time != TIME_IMMEDIATE, "chThdSleep");

  chSysLock();
  chThdSleepS(time);
  chSysUnlock();
}

/**
 * @brief   Suspends the invoking thread until the system time arrives to the
 *          specified value.
 *
 * @param[in] time      absolute system time
 *
 * @api
 */
void chThdSleepUntil(systime_t time) {

  chSysLock();
  if ((time -= chTimeNow()) > 0)
    chThdSleepS(time);
  chSysUnlock();
}

/**
 * @brief   Yields the time slot.
 * @details Yields the CPU control to the next thread in the ready list with
 *          equal priority, if any.
 *
 * @api
 */
void chThdYield(void) {

  chSysLock();
  chSchDoYieldS();
  chSysUnlock();
}

/**
 * @brief   Terminates the current thread.
 * @details The thread goes in the @p THD_STATE_FINAL state holding the
 *          specified exit status code, other threads can retrieve the
 *          exit status code by invoking the function @p chThdWait().
 * @post    Eventual code after this function will never be executed,
 *          this function never returns. The compiler has no way to
 *          know this so do not assume that the compiler would remove
 *          the dead code.
 *
 * @param[in] msg       thread exit code
 *
 * @api
 */
void chThdExit(msg_t msg) {

  chSysLock();
  chThdExitS(msg);
  /* The thread never returns here.*/
}

/**
 * @brief   Terminates the current thread.
 * @details The thread goes in the @p THD_STATE_FINAL state holding the
 *          specified exit status code, other threads can retrieve the
 *          exit status code by invoking the function @p chThdWait().
 * @post    Eventual code after this function will never be executed,
 *          this function never returns. The compiler has no way to
 *          know this so do not assume that the compiler would remove
 *          the dead code.
 *
 * @param[in] msg       thread exit code
 *
 * @sclass
 */
void chThdExitS(msg_t msg) {
  Thread *tp = currp;

  tp->p_u.exitcode = msg;
#if defined(THREAD_EXT_EXIT_HOOK)
  THREAD_EXT_EXIT_HOOK(tp);
#endif
#if CH_USE_WAITEXIT
  while (notempty(&tp->p_waiting))
    chSchReadyI(list_remove(&tp->p_waiting));
#endif
#if CH_USE_REGISTRY
  /* Static threads are immediately removed from the registry because
     there is no memory to recover.*/
  if ((tp->p_flags & THD_MEM_MODE_MASK) == THD_MEM_MODE_STATIC)
    REG_REMOVE(tp);
#endif
  chSchGoSleepS(THD_STATE_FINAL);
  /* The thread never returns here.*/
  chDbgAssert(FALSE, "chThdExitS(), #1", "zombies apocalypse");
}

#if CH_USE_WAITEXIT || defined(__DOXYGEN__)
/**
 * @brief   Blocks the execution of the invoking thread until the specified
 *          thread terminates then the exit code is returned.
 * @details This function waits for the specified thread to terminate then
 *          decrements its reference counter, if the counter reaches zero then
 *          the thread working area is returned to the proper allocator.<br>
 *          The memory used by the exited thread is handled in different ways
 *          depending on the API that spawned the thread:
 *          - If the thread was spawned by @p chThdCreateStatic() or by
 *            @p chThdCreateI() then nothing happens and the thread working
 *            area is not released or modified in any way. This is the
 *            default, totally static, behavior.
 *          - If the thread was spawned by @p chThdCreateFromHeap() then
 *            the working area is returned to the system heap.
 *          - If the thread was spawned by @p chThdCreateFromMemoryPool()
 *            then the working area is returned to the owning memory pool.
 *          .
 * @pre     The configuration option @p CH_USE_WAITEXIT must be enabled in
 *          order to use this function.
 * @post    Enabling @p chThdWait() requires 2-4 (depending on the
 *          architecture) extra bytes in the @p Thread structure.
 * @post    After invoking @p chThdWait() the thread pointer becomes invalid
 *          and must not be used as parameter for further system calls.
 * @note    If @p CH_USE_DYNAMIC is not specified this function just waits for
 *          the thread termination, no memory allocators are involved.
 *
 * @param[in] tp        pointer to the thread
 * @return              The exit code from the terminated thread.
 *
 * @api
 */
msg_t chThdWait(Thread *tp) {
  msg_t msg;

  chDbgCheck(tp != NULL, "chThdWait");

  chSysLock();
  chDbgAssert(tp != currp, "chThdWait(), #1", "waiting self");
#if CH_USE_DYNAMIC
  chDbgAssert(tp->p_refs > 0, "chThdWait(), #2", "not referenced");
#endif
  if (tp->p_state != THD_STATE_FINAL) {
    list_insert(currp, &tp->p_waiting);
    chSchGoSleepS(THD_STATE_WTEXIT);
  }
  msg = tp->p_u.exitcode;
  chSysUnlock();
#if CH_USE_DYNAMIC
  chThdRelease(tp);
#endif
  return msg;
}
#endif /* CH_USE_WAITEXIT */

/** @} */
