/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    ch.hpp
 * @brief   C++ wrapper classes and definitions.
 *
 * @addtogroup cpp_library
 * @{
 */

#include <ch.h>

#ifndef _CH_HPP_
#define _CH_HPP_

/**
 * @brief   ChibiOS kernel-related classes and interfaces.
 */
namespace chibios_rt {

  /*------------------------------------------------------------------------*
   * chibios_rt::System                                                     *
   *------------------------------------------------------------------------*/
  /**
   * @brief Class encapsulating the base system functionalities.
   */
  class System {
  public:
    /**
     * @brief   ChibiOS/RT initialization.
     * @details After executing this function the current instructions stream
     *          becomes the main thread.
     * @pre     Interrupts must be still disabled when @p chSysInit() is invoked
     *          and are internally enabled.
     * @post    The main thread is created with priority @p NORMALPRIO.
     * @note    This function has special, architecture-dependent, requirements,
     *          see the notes into the various port reference manuals.
     *
     * @special
     */
    static void init(void);

    /**
     * @brief   Enters the kernel lock mode.
     *
     * @special
     */
    static void lock(void);

    /**
     * @brief   Leaves the kernel lock mode.
     *
     * @special
     */
    static void unlock(void);

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
    static void lockFromIsr(void);

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
    static void unlockFromIsr(void);


    /**
     * @brief   Returns the system time as system ticks.
     * @note    The system tick time interval is implementation dependent.
     *
     * @return          The system time.
     *
     * @api
     */
    static systime_t getTime(void);

    /**
     * @brief   Checks if the current system time is within the specified time
     *          window.
     * @note    When start==end then the function returns always true because the
     *          whole time range is specified.
     *
     * @param[in] start     the start of the time window (inclusive)
     * @param[in] end       the end of the time window (non inclusive)
     * @retval true         current time within the specified time window.
     * @retval false        current time not within the specified time window.
     *
     * @api
     */
    static bool isTimeWithin(systime_t start, systime_t end);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::System                                                     *
   *------------------------------------------------------------------------*/
  /**
   * @brief Class encapsulating the base system functionalities.
   */
  class Core {
  public:

    /**
     * @brief   Allocates a memory block.
     * @details The size of the returned block is aligned to the alignment
     *          type so it is not possible to allocate less
     *          than <code>MEM_ALIGN_SIZE</code>.
     *
     * @param[in] size      the size of the block to be allocated
     * @return              A pointer to the allocated memory block.
     * @retval NULL         allocation failed, core memory exhausted.
     *
     * @api
     */
    static void *alloc(size_t size);

    /**
     * @brief   Allocates a memory block.
     * @details The size of the returned block is aligned to the alignment
     *          type so it is not possible to allocate less than
     *          <code>MEM_ALIGN_SIZE</code>.
     *
     * @param[in] size      the size of the block to be allocated.
     * @return              A pointer to the allocated memory block.
     * @retval NULL         allocation failed, core memory exhausted.
     *
     * @iclass
     */
    static void *allocI(size_t size);

    /**
     * @brief   Core memory status.
     *
     * @return              The size, in bytes, of the free core memory.
     *
     * @api
     */
    static size_t getStatus(void);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::Timer                                                      *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Timer class.
   */
  class Timer {
  public:
    /**
     * @brief   Embedded @p VirtualTimer structure.
     */
    ::VirtualTimer timer_ref;

    /**
     * @brief   Enables a virtual timer.
     * @note    The associated function is invoked from interrupt context.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the special values are handled as follow:
     *                      - @a TIME_INFINITE is allowed but interpreted as a
     *                        normal time specification.
     *                      - @a TIME_IMMEDIATE this value is not allowed.
     *                      .
     * @param[in] vtfunc    the timer callback function. After invoking the
     *                      callback the timer is disabled and the structure
     *                      can be disposed or reused.
     * @param[in] par       a parameter that will be passed to the callback
     *                      function
     *
     * @iclass
     */
    void setI(systime_t time, vtfunc_t vtfunc, void *par);

    /**
     * @brief   Resets the timer, if armed.
     *
     * @iclass
     */
    void resetI();

    /**
     * @brief   Returns the timer status.
     *
     * @retval TRUE         The timer is armed.
     * @retval FALSE        The timer already fired its callback.
     *
     * @iclass
     */
    bool isArmedI(void);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::ThreadReference                                            *
   *------------------------------------------------------------------------*/
  /**
   * @brief     Thread reference class.
   * @details   This class encapsulates a reference to a system thread. All
   *            operations involving another thread are performed through
   *            an object of this type.
   */
  class ThreadReference {
  public:
    /**
     * @brief   Pointer to the system thread.
     */
    ::Thread *thread_ref;

    /**
     * @brief   Thread reference constructor.
     *
     * @param[in] tp            the target thread. This parameter can be
     *                          @p NULL if the thread is not known at
     *                          creation time.
     *
     * @init
     */
    ThreadReference(Thread *tp) : thread_ref(tp) {

    };

    /**
     * @brief   Stops the thread.
     * @note    The implementation is left to descendant classes and is
     *          optional.
     */
    virtual void stop(void);

    /**
     * @brief   Suspends the current thread on the reference.
     * @details The suspended thread becomes the referenced thread. It is
     *          possible to use this method only if the thread reference
     *          was set to @p NULL.
     *
     * @return                  The incoming message.
     *
     * @api
     */
    msg_t suspend(void);

    /**
     * @brief   Suspends the current thread on the reference.
     * @details The suspended thread becomes the referenced thread. It is
     *          possible to use this method only if the thread reference
     *          was set to @p NULL.
     *
     * @return                  The incoming message.
     *
     * @sclass
     */
    msg_t suspendS(void);

    /**
     * @brief   Resumes the currently referenced thread, if any.
     *
     * @param[in] msg       the wakeup message
     *
     * @api
     */
    void resume(msg_t msg);

    /**
     * @brief   Resumes the currently referenced thread, if any.
     *
     * @param[in] msg       the wakeup message
     *
     * @iclass
     */
    void resumeI(msg_t msg);

    /**
     * @brief   Requests a thread termination.
     * @pre     The target thread must be written to invoke periodically
     *          @p chThdShouldTerminate() and terminate cleanly if it returns
     *          @p TRUE.
     * @post    The specified thread will terminate after detecting the
     *          termination condition.
     *
     * @api
     */
    void requestTerminate(void);

#if CH_USE_WAITEXIT || defined(__DOXYGEN__)
    /**
     * @brief   Blocks the execution of the invoking thread until the specified
     *          thread terminates then the exit code is returned.
     * @details This function waits for the specified thread to terminate then
     *          decrements its reference counter, if the counter reaches zero
     *          then the thread working area is returned to the proper
     *          allocator.<br>
     *          The memory used by the exited thread is handled in different
     *          ways depending on the API that spawned the thread:
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
     * @post    After invoking @p chThdWait() the thread pointer becomes
     *          invalid and must not be used as parameter for further system
     *          calls.
     * @note    If @p CH_USE_DYNAMIC is not specified this function just waits
     *          for the thread termination, no memory allocators are involved.
     *
     * @return              The exit code from the terminated thread.
     *
     * @api
     */
    msg_t wait(void);
#endif /* CH_USE_WAITEXIT */

#if CH_USE_MESSAGES || defined(__DOXYGEN__)
    /**
     * @brief   Sends a message to the thread and returns the answer.
     *
     * @param[in] msg           the sent message
     * @return                  The returned message.
     *
     * @api
     */
    msg_t sendMessage(msg_t msg);

    /**
     * @brief   Returns true if there is at least one message in queue.
     *
     * @retval true             A message is waiting in queue.
     * @retval false            A message is not waiting in queue.
     *
     * @api
     */
    bool isPendingMessage(void);

    /**
     * @brief   Returns an enqueued message or @p NULL.
     *
     * @return                  The incoming message.
     *
     * @api
     */
    msg_t getMessage(void);

    /**
     * @brief   Releases the next message in queue with a reply.
     *
     * @param[in] msg           the answer message
     *
     * @api
     */
    void releaseMessage(msg_t msg);
#endif /* CH_USE_MESSAGES */

#if CH_USE_EVENTS || defined(__DOXYGEN__)
    /**
     * @brief   Adds a set of event flags directly to specified @p Thread.
     *
     * @param[in] mask      the event flags set to be ORed
     *
     * @api
     */
    void signalEvents(eventmask_t mask);

    /**
     * @brief   Adds a set of event flags directly to specified @p Thread.
     *
     * @param[in] mask      the event flags set to be ORed
     *
     * @iclass
     */
    void signalEventsI(eventmask_t mask);
#endif /* CH_USE_EVENTS */

#if CH_USE_DYNAMIC || defined(__DOXYGEN__)
#endif /* CH_USE_DYNAMIC */
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::BaseThread                                                 *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Abstract base class for a ChibiOS/RT thread.
   * @details The thread body is the virtual function @p Main().
   */
  class BaseThread : public ThreadReference {
  public:
    /**
     * @brief   BaseThread constructor.
     *
     * @init
     */
    BaseThread(void);

    /**
     * @brief   Thread body function.
     *
     * @return                  The exit message.
     *
     * @api
     */
    virtual msg_t main(void);

    /**
     * @brief   Creates and starts a system thread.
     *
     * @param[in] prio          thread priority
     * @return                  A reference to the created thread with
     *                          reference counter set to one.
     *
     * @api
     */
    virtual ThreadReference start(tprio_t prio);

    /**
     * @brief   Sets the current thread name.
     * @pre     This function only stores the pointer to the name if the option
     *          @p CH_USE_REGISTRY is enabled else no action is performed.
     *
     * @param[in] tname         thread name as a zero terminated string
     *
     * @api
     */
    static void setName(const char *tname);

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
    static tprio_t setPriority(tprio_t newprio);

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
    static void exit(msg_t msg);

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
    static void exitS(msg_t msg);

    /**
     * @brief   Verifies if the current thread has a termination request
     *          pending.
     * @note    Can be invoked in any context.
     *
     * @retval TRUE         termination request pending.
     * @retval FALSE        termination request not pending.
     *
     * @special
     */
    static bool shouldTerminate(void);

    /**
     * @brief   Suspends the invoking thread for the specified time.
     *
     * @param[in] interval  the delay in system ticks, the special values are
     *                      handled as follow:
     *                      - @a TIME_INFINITE the thread enters an infinite
     *                        sleep state.
     *                      - @a TIME_IMMEDIATE this value is not allowed.
     *                      .
     *
     * @api
     */
    static void sleep(systime_t interval);

    /**
     * @brief   Suspends the invoking thread until the system time arrives to
     *          the specified value.
     *
     * @param[in] time      absolute system time
     *
     * @api
     */
    static void sleepUntil(systime_t time);

    /**
     * @brief   Yields the time slot.
     * @details Yields the CPU control to the next thread in the ready list
     *          with equal priority, if any.
     *
     * @api
     */
    static void yield(void);

#if CH_USE_MESSAGES || defined(__DOXYGEN__)
    /**
     * @brief   Waits for a message.
     *
     * @return                  The sender thread.
     *
     * @api
     */
    static ThreadReference waitMessage(void);
#endif /* CH_USE_MESSAGES */

#if CH_USE_EVENTS || defined(__DOXYGEN__)
    /**
     * @brief   Clears the pending events specified in the mask.
     *
     * @param[in] mask      the events to be cleared
     * @return              The pending events that were cleared.
     *
     * @api
     */
    static eventmask_t getAndClearEvents(eventmask_t mask);

    /**
     * @brief   Adds (OR) a set of event flags on the current thread, this is
     *          @b much faster than using @p chEvtBroadcast() or
     *          @p chEvtSignal().
     *
     * @param[in] mask      the event flags to be added
     * @return              The current pending events mask.
     *
     * @api
     */
    static eventmask_t addEvents(eventmask_t mask);

    /**
     * @brief   Waits for a single event.
     * @details A pending event among those specified in @p ewmask is selected,
     *          cleared and its mask returned.
     * @note    One and only one event is served in the function, the one with
     *          the lowest event id. The function is meant to be invoked into
     *          a loop in order to serve all the pending events.<br>
     *          This means that Event Listeners with a lower event identifier
     *          have an higher priority.
     *
     * @param[in] ewmask        mask of the events that the function should
     *                          wait for, @p ALL_EVENTS enables all the events
     * @return                  The mask of the lowest id served and cleared
     *                          event.
     *
     * @api
     */
    static eventmask_t waitOneEvent(eventmask_t ewmask);

    /**
     * @brief   Waits for any of the specified events.
     * @details The function waits for any event among those specified in
     *          @p ewmask to become pending then the events are cleared and
     *          returned.
     *
     * @param[in] ewmask        mask of the events that the function should
     *                          wait for, @p ALL_EVENTS enables all the events
     * @return                  The mask of the served and cleared events.
     *
     * @api
     */
    static eventmask_t waitAnyEvent(eventmask_t ewmask);

    /**
     * @brief   Waits for all the specified event flags then clears them.
     * @details The function waits for all the events specified in @p ewmask
     *          to become pending then the events are cleared and returned.
     *
     * @param[in] ewmask        mask of the event ids that the function should
     *                          wait for
     * @return                  The mask of the served and cleared events.
     *
     * @api
     */
    static eventmask_t waitAllEvents(eventmask_t ewmask);

#if CH_USE_EVENTS_TIMEOUT || defined(__DOXYGEN__)
    /**
     * @brief   Waits for a single event.
     * @details A pending event among those specified in @p ewmask is selected,
     *          cleared and its mask returned.
     * @note    One and only one event is served in the function, the one with
     *          the lowest event id. The function is meant to be invoked into
     *          a loop in order to serve all the pending events.<br>
     *          This means that Event Listeners with a lower event identifier
     *          have an higher priority.
     *
     * @param[in] ewmask        mask of the events that the function should
     *                          wait for, @p ALL_EVENTS enables all the events
     *
     * @param[in] time          the number of ticks before the operation
     *                          timouts
     * @return                  The mask of the lowest id served and cleared
     *                          event.
     * @retval 0                if the specified timeout expired.
     *
     * @api
     */
    static eventmask_t waitOneEventTimeout(eventmask_t ewmask,
                                           systime_t time);

    /**
     * @brief   Waits for any of the specified events.
     * @details The function waits for any event among those specified in
     *          @p ewmask to become pending then the events are cleared and
     *          returned.
     *
     * @param[in] ewmask        mask of the events that the function should
     *                          wait for, @p ALL_EVENTS enables all the events
     * @param[in] time          the number of ticks before the operation
     *                          timouts
     * @return                  The mask of the served and cleared events.
     * @retval 0                if the specified timeout expired.
     *
     * @api
     */
    static eventmask_t waitAnyEventTimeout(eventmask_t ewmask,
                                           systime_t time);

    /**
     * @brief   Waits for all the specified event flags then clears them.
     * @details The function waits for all the events specified in @p ewmask
     *          to become pending then the events are cleared and returned.
     *
     * @param[in] ewmask        mask of the event ids that the function should
     *                          wait for
     * @param[in] time          the number of ticks before the operation
     *                          timouts
     * @return                  The mask of the served and cleared events.
     * @retval 0                if the specified timeout expired.
     *
     * @api
     */
    static eventmask_t waitAllEventsTimeout(eventmask_t ewmask,
                                            systime_t time);
#endif /* CH_USE_EVENTS_TIMEOUT */

    /**
     * @brief   Invokes the event handlers associated to an event flags mask.
     *
     * @param[in] mask      mask of the event flags to be dispatched
     * @param[in] handlers  an array of @p evhandler_t. The array must have
     *                      size equal to the number of bits in eventmask_t.
     *
     * @api
     */
    static void dispatchEvents(const evhandler_t handlers[],
                               eventmask_t mask);
#endif /* CH_USE_EVENTS */

#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief   Unlocks the next owned mutex in reverse lock order.
     * @pre     The invoking thread <b>must</b> have at least one owned mutex.
     * @post    The mutex is unlocked and removed from the per-thread stack of
     *          owned mutexes.
     *
     * @return              A pointer to the unlocked mutex.
     *
     * @api
     */
    static void unlockMutex(void);

    /**
     * @brief   Unlocks the next owned mutex in reverse lock order.
     * @pre     The invoking thread <b>must</b> have at least one owned mutex.
     * @post    The mutex is unlocked and removed from the per-thread stack of
     *          owned mutexes.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel.
     *
     * @return              A pointer to the unlocked mutex.
     *
     * @sclass
     */
    static void unlockMutexS(void);

    /**
     * @brief   Unlocks all the mutexes owned by the invoking thread.
     * @post    The stack of owned mutexes is emptied and all the found
     *          mutexes are unlocked.
     * @note    This function is <b>MUCH MORE</b> efficient than releasing the
     *          mutexes one by one and not just because the call overhead,
     *          this function does not have any overhead related to the
     *          priority inheritance mechanism.
     *
     * @api
     */
    static void unlockAllMutexes(void);
#endif /* CH_USE_MUTEXES */
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::BaseStaticThread                                           *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Static threads template class.
   * @details This class introduces static working area allocation.
   *
   * @param N               the working area size for the thread class
   */
  template <int N>
  class BaseStaticThread : public BaseThread {
  protected:
    WORKING_AREA(wa, N);

  public:
    /**
     * @brief   Thread constructor.
     * @details The thread object is initialized but the thread is not
     *          started here.
     *
     * @init
     */
    BaseStaticThread(void) : BaseThread() {

    }

    /**
     * @brief   Creates and starts a system thread.
     *
     * @param[in] prio          thread priority
     * @return                  A reference to the created thread with
     *                          reference counter set to one.
     *
     * @api
     */
    virtual ThreadReference start(tprio_t prio) {
      msg_t _thd_start(void *arg);

      thread_ref = chThdCreateStatic(wa, sizeof(wa), prio, _thd_start, this);
      return *this;
    }
  };

#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::CounterSemaphore                                           *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a semaphore.
   */
  class CounterSemaphore {
  public:
    /**
     * @brief   Embedded @p ::Semaphore structure.
     */
    ::Semaphore sem;

    /**
     * @brief   CounterSemaphore constructor.
     * @details The embedded @p ::Semaphore structure is initialized.
     *
     * @param[in] n             the semaphore counter value, must be greater
     *                          or equal to zero
     *
     * @init
     */
    CounterSemaphore(cnt_t n);

    /**
     * @brief   Performs a reset operation on the semaphore.
     * @post    After invoking this function all the threads waiting on the
     *          semaphore, if any, are released and the semaphore counter is
     *          set to the specified, non negative, value.
     * @note    The released threads can recognize they were waked up by a
     *          reset rather than a signal because the @p chSemWait() will
     *          return @p RDY_RESET instead of @p RDY_OK.
     *
     * @param[in] n         the new value of the semaphore counter. The value
     *                      must be non-negative.
     *
     * @api
     */
    void reset(cnt_t n);

    /**
     * @brief   Performs a reset operation on the semaphore.
     * @post    After invoking this function all the threads waiting on the
     *          semaphore, if any, are released and the semaphore counter is
     *          set to the specified, non negative, value.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel. Note
     *          that interrupt handlers always reschedule on exit so an
     *          explicit reschedule must not be performed in ISRs.
     * @note    The released threads can recognize they were waked up by a
     *          reset rather than a signal because the @p chSemWait() will
     *          return @p RDY_RESET instead of @p RDY_OK.
     *
     * @param[in] n         the new value of the semaphore counter. The value
     *                      must be non-negative.
     *
     * @iclass
     */
    void resetI(cnt_t n);

    /**
     * @brief   Performs a wait operation on a semaphore.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the thread has not stopped on the semaphore or
     *                      the semaphore has been signaled.
     * @retval RDY_RESET    if the semaphore has been reset using
     *                      @p chSemReset().
     *
     * @api
     */
    msg_t wait(void);

    /**
     * @brief   Performs a wait operation on a semaphore.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the thread has not stopped on the semaphore or
     *                      the semaphore has been signaled.
     * @retval RDY_RESET    if the semaphore has been reset using
     *                      @p chSemReset().
     *
     * @sclass
     */
    msg_t waitS(void);

    /**
     * @brief   Performs a wait operation on a semaphore with timeout
     *          specification.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the thread has not stopped on the semaphore or
     *                      the semaphore has been signaled.
     * @retval RDY_RESET    if the semaphore has been reset using
     *                      @p chSemReset().
     * @retval RDY_TIMEOUT  if the semaphore has not been signaled or reset
     *                      within the specified timeout.
     *
     * @api
     */
    msg_t waitTimeout(systime_t time);

    /**
     * @brief   Performs a wait operation on a semaphore with timeout
     *          specification.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the thread has not stopped on the semaphore or
     *                      the semaphore has been signaled.
     * @retval RDY_RESET    if the semaphore has been reset using
     *                      @p chSemReset().
     * @retval RDY_TIMEOUT  if the semaphore has not been signaled or reset
     *                      within the specified timeout.
     *
     * @sclass
     */
    msg_t waitTimeoutS(systime_t time);

    /**
     * @brief   Performs a signal operation on a semaphore.
     *
     * @api
     */
    void signal(void);

    /**
     * @brief   Performs a signal operation on a semaphore.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel. Note
     *          that interrupt handlers always reschedule on exit so an
     *          explicit reschedule must not be performed in ISRs.
     *
     * @iclass
     */
    void signalI(void);

    /**
     * @brief   Adds the specified value to the semaphore counter.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel. Note
     *          that interrupt handlers always reschedule on exit so an explicit
     *          reschedule must not be performed in ISRs.
     *
     * @param[in] n         value to be added to the semaphore counter. The
     *                      value must be positive.
     *
     * @iclass
     */
    void addCounterI(cnt_t n);

    /**
     * @brief   Returns the semaphore counter value.
     *
     * @return                  The semaphore counter value.
     *
     * @iclass
     */
    cnt_t getCounterI(void);

#if CH_USE_SEMSW || defined(__DOXYGEN__)
    /**
     * @brief   Atomic signal and wait operations.
     *
     * @param[in] ssem          @p Semaphore object to be signaled
     * @param[in] wsem          @p Semaphore object to wait on
     * @return                  A message specifying how the invoking thread
     *                          has been released from the semaphore.
     * @retval RDY_OK           if the thread has not stopped on the semaphore
     *                          or the semaphore has been signaled.
     * @retval RDY_RESET        if the semaphore has been reset using
     *                          @p chSemReset().
     *
     * @api
     */
    static msg_t signalWait(CounterSemaphore *ssem,
                            CounterSemaphore *wsem);
#endif /* CH_USE_SEMSW */
  };
  /*------------------------------------------------------------------------*
   * chibios_rt::BinarySemaphore                                            *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a binary semaphore.
   */
  class BinarySemaphore {
  public:
    /**
     * @brief   Embedded @p ::Semaphore structure.
     */
    ::BinarySemaphore bsem;

    /**
     * @brief   BinarySemaphore constructor.
     * @details The embedded @p ::BinarySemaphore structure is initialized.
     *
     * @param[in] taken     initial state of the binary semaphore:
     *                      - @a false, the initial state is not taken.
     *                      - @a true, the initial state is taken.
     *                      .
     *
     * @init
     */
    BinarySemaphore(bool taken);

    /**
     * @brief   Wait operation on the binary semaphore.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the binary semaphore has been successfully
     *                      taken.
     * @retval RDY_RESET    if the binary semaphore has been reset using
     *                      @p bsemReset().
     *
     * @api
     */
    msg_t wait(void);

    /**
     * @brief   Wait operation on the binary semaphore.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the binary semaphore has been successfully
     *                      taken.
     * @retval RDY_RESET    if the binary semaphore has been reset using
     *                      @p bsemReset().
     *
     * @sclass
     */
    msg_t waitS(void);

    /**
     * @brief   Wait operation on the binary semaphore.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the binary semaphore has been successfully
     *                      taken.
     * @retval RDY_RESET    if the binary semaphore has been reset using
     *                      @p bsemReset().
     * @retval RDY_TIMEOUT  if the binary semaphore has not been signaled
     *                      or reset within the specified timeout.
     *
     * @api
     */
    msg_t waitTimeout(systime_t time);

    /**
     * @brief   Wait operation on the binary semaphore.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              A message specifying how the invoking thread has
     *                      been released from the semaphore.
     * @retval RDY_OK       if the binary semaphore has been successfully
     *                      taken.
     * @retval RDY_RESET    if the binary semaphore has been reset using
     *                      @p bsemReset().
     * @retval RDY_TIMEOUT  if the binary semaphore has not been signaled
     *                      or reset within the specified timeout.
     *
     * @sclass
     */
    msg_t waitTimeoutS(systime_t time);

    /**
     * @brief   Reset operation on the binary semaphore.
     * @note    The released threads can recognize they were waked up by a
     *          reset rather than a signal because the @p bsemWait() will
     *          return @p RDY_RESET instead of @p RDY_OK.
     *
     * @param[in] taken     new state of the binary semaphore
     *                      - @a FALSE, the new state is not taken.
     *                      - @a TRUE, the new state is taken.
     *                      .
     *
     * @api
     */
    void reset(bool taken);

    /**
     * @brief   Reset operation on the binary semaphore.
     * @note    The released threads can recognize they were waked up by a
     *          reset rather than a signal because the @p bsemWait() will
     *          return @p RDY_RESET instead of @p RDY_OK.
     * @note    This function does not reschedule.
     *
     * @param[in] taken     new state of the binary semaphore
     *                      - @a FALSE, the new state is not taken.
     *                      - @a TRUE, the new state is taken.
     *                      .
     *
     * @iclass
     */
    void resetI(bool taken);

    /**
     * @brief   Performs a signal operation on a binary semaphore.
     *
     * @api
     */
    void signal(void);

    /**
     * @brief   Performs a signal operation on a binary semaphore.
     * @note    This function does not reschedule.
     *
     * @iclass
     */
    void signalI(void);

    /**
     * @brief   Returns the binary semaphore current state.
     *
     * @return              The binary semaphore current state.
     * @retval false        if the binary semaphore is not taken.
     * @retval true         if the binary semaphore is taken.
     *
     * @iclass
     */
    bool getStateI(void);
};
#endif /* CH_USE_SEMAPHORES */

#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::Mutex                                                      *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a mutex.
   */
  class Mutex {
  public:
    /**
     * @brief   Embedded @p ::Mutex structure.
     */
    ::Mutex mutex;

    /**
     * @brief   Mutex object constructor.
     * @details The embedded @p ::Mutex structure is initialized.
     *
     * @init
     */
    Mutex(void);

    /**
     * @brief   Tries to lock a mutex.
     * @details This function attempts to lock a mutex, if the mutex is already
     *          locked by another thread then the function exits without
     *          waiting.
     * @post    The mutex is locked and inserted in the per-thread stack of
     *          owned mutexes.
     * @note    This function does not have any overhead related to the
     *          priority inheritance mechanism because it does not try to
     *          enter a sleep state.
     *
     * @return              The operation status.
     * @retval TRUE         if the mutex has been successfully acquired
     * @retval FALSE        if the lock attempt failed.
     *
     * @api
     */
    bool tryLock(void);

    /**
     * @brief   Tries to lock a mutex.
     * @details This function attempts to lock a mutex, if the mutex is already
     *          taken by another thread then the function exits without
     *          waiting.
     * @post    The mutex is locked and inserted in the per-thread stack of
     *          owned mutexes.
     * @note    This function does not have any overhead related to the
     *          priority inheritance mechanism because it does not try to
     *          enter a sleep state.
     *
     * @return              The operation status.
     * @retval TRUE         if the mutex has been successfully acquired
     * @retval FALSE        if the lock attempt failed.
     *
     * @sclass
     */
    bool tryLockS(void);

    /**
     * @brief   Locks the specified mutex.
     * @post    The mutex is locked and inserted in the per-thread stack of
     *          owned mutexes.
     *
     * @api
     */
    void lock(void);

    /**
     * @brief   Locks the specified mutex.
     * @post    The mutex is locked and inserted in the per-thread stack of
     *          owned mutexes.
     *
     * @sclass
     */
    void lockS(void);
  };

#if CH_USE_CONDVARS || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::CondVar                                                    *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a conditional variable.
   */
  class CondVar {
  public:
    /**
     * @brief   Embedded @p ::CondVar structure.
     */
    ::CondVar condvar;

    /**
     * @brief   CondVar object constructor.
     * @details The embedded @p ::CondVar structure is initialized.
     *
     * @init
     */
    CondVar(void);

    /**
     * @brief   Signals one thread that is waiting on the condition variable.
     *
     * @api
     */
    void signal(void);

    /**
     * @brief   Signals one thread that is waiting on the condition variable.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel. Note
     *          that interrupt handlers always reschedule on exit so an
     *          explicit reschedule must not be performed in ISRs.
     *
     * @iclass
     */
    void signalI(void);

    /**
     * @brief   Signals all threads that are waiting on the condition variable.
     *
     * @api
     */
    void broadcast(void);

    /**
     * @brief   Signals all threads that are waiting on the condition variable.
     * @post    This function does not reschedule so a call to a rescheduling
     *          function must be performed before unlocking the kernel. Note
     *          that interrupt handlers always reschedule on exit so an
     *          explicit reschedule must not be performed in ISRs.
     *
     * @iclass
     */
    void broadcastI(void);

    /**
     * @brief   Waits on the condition variable releasing the mutex lock.
     * @details Releases the currently owned mutex, waits on the condition
     *          variable, and finally acquires the mutex again. All the
     *          sequence is performed atomically.
     * @pre     The invoking thread <b>must</b> have at least one owned mutex.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the condition variable.
     * @retval RDY_OK       if the condvar has been signaled using
     *                      @p chCondSignal().
     * @retval RDY_RESET    if the condvar has been signaled using
     *                      @p chCondBroadcast().
     *
     * @api
     */
    msg_t wait(void);

    /**
     * @brief   Waits on the condition variable releasing the mutex lock.
     * @details Releases the currently owned mutex, waits on the condition
     *          variable, and finally acquires the mutex again. All the
     *          sequence is performed atomically.
     * @pre     The invoking thread <b>must</b> have at least one owned mutex.
     *
     * @return              A message specifying how the invoking thread has
     *                      been released from the condition variable.
     * @retval RDY_OK       if the condvar has been signaled using
     *                      @p chCondSignal().
     * @retval RDY_RESET    if the condvar has been signaled using
     *                      @p chCondBroadcast().
     *
     * @sclass
     */
    msg_t waitS(void);

#if CH_USE_CONDVARS_TIMEOUT || defined(__DOXYGEN__)
    /**
     * @brief   Waits on the CondVar while releasing the controlling mutex.
     *
     * @param[in] time          the number of ticks before the operation fails
     * @return                  The wakep mode.
     * @retval RDY_OK if        the condvar was signaled using
     *                          @p chCondSignal().
     * @retval RDY_RESET        if the condvar was signaled using
     *                          @p chCondBroadcast().
     * @retval RDY_TIMEOUT      if the condvar was not signaled within the
     *                          specified timeout.
     *
     * @api
     */
    msg_t waitTimeout(systime_t time);
#endif /* CH_USE_CONDVARS_TIMEOUT */
  };
#endif /* CH_USE_CONDVARS */
#endif /* CH_USE_MUTEXES */

#if CH_USE_EVENTS || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::EvtListener                                                *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating an event listener.
   */
  class EvtListener {
  public:
    /**
     * @brief   Embedded @p ::EventListener structure.
     */
    struct ::EventListener ev_listener;

    /**
     * @brief   Returns the pending flags from the listener and clears them.
     *
     * @return                  The flags added to the listener by the
     *                          associated event source.
     *
     * @api
     */
    flagsmask_t getAndClearFlags(void);

    /**
     * @brief   Returns the flags associated to an @p EventListener.
     * @details The flags are returned and the @p EventListener flags mask is
     *          cleared.
     *
     * @return              The flags added to the listener by the associated
     *                      event source.
     *
     * @iclass
     */
    flagsmask_t getAndClearFlagsI(void);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::EvtSource                                                  *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating an event source.
   */
  class EvtSource {
  public:
    /**
     * @brief   Embedded @p ::EventSource structure.
     */
    struct ::EventSource ev_source;

    /**
     * @brief   EvtSource object constructor.
     * @details The embedded @p ::EventSource structure is initialized.
     *
     * @init
     */
    EvtSource(void);

    /**
     * @brief   Registers a listener on the event source.
     *
     * @param[in] elp           pointer to the @p EvtListener object
     * @param[in] eid           numeric identifier assigned to the Event
     *                          Listener
     *
     * @api
     */
    void registerOne(chibios_rt::EvtListener *elp, eventid_t eid);

    /**
     * @brief   Registers an Event Listener on an Event Source.
     * @note    Multiple Event Listeners can specify the same bits to be added.
     *
     * @param[in] elp           pointer to the @p EvtListener object
     * @param[in] emask         the mask of event flags to be pended to the
     *                          thread when the event source is broadcasted
     *
     * @api
     */
    void registerMask(chibios_rt::EvtListener *elp, eventmask_t emask);

    /**
     * @brief   Unregisters a listener.
     * @details The specified listeners is no more signaled by the event
     *          source.
     *
     * @param[in] elp           the listener to be unregistered
     *
     * @api
     */
    void unregister(chibios_rt::EvtListener *elp);

    /**
     * @brief   Broadcasts on an event source.
     * @details All the listeners registered on the event source are signaled
     *          and the flags are added to the listener's flags mask.
     *
     * @param[in] flags         the flags set to be added to the listener
     *                          flags mask
     *
     * @api
     */
    void broadcastFlags(flagsmask_t flags);

    /**
     * @brief   Broadcasts on an event source.
     * @details All the listeners registered on the event source are signaled
     *          and the flags are added to the listener's flags mask.
     *
     * @param[in] flags         the flags set to be added to the listener
     *                          flags mask
     *
     * @iclass
     */
    void broadcastFlagsI(flagsmask_t flags);
  };
#endif /* CH_USE_EVENTS */

#if CH_USE_QUEUES || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::InQueue                                                    *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating an input queue.
   */
  class InQueue {
  private:
    /**
     * @brief   Embedded @p ::InputQueue structure.
     */
    ::InputQueue iq;

  public:
    /**
     * @brief   InQueue constructor.
     *
     * @param[in] bp        pointer to a memory area allocated as queue buffer
     * @param[in] size      size of the queue buffer
     * @param[in] infy      pointer to a callback function that is invoked when
     *                      data is read from the queue. The value can be
     *                      @p NULL.
     * @param[in] link      application defined pointer
     *
     * @init
     */
    InQueue(uint8_t *bp, size_t size, qnotify_t infy, void *link);

    /**
     * @brief   Returns the filled space into an input queue.
     *
     * @return              The number of full bytes in the queue.
     * @retval 0            if the queue is empty.
     *
     * @iclass
     */
    size_t getFullI(void);

    /**
     * @brief   Returns the empty space into an input queue.
     *
     * @return              The number of empty bytes in the queue.
     * @retval 0            if the queue is full.
     *
     * @iclass
     */
    size_t getEmptyI(void);

    /**
     * @brief   Evaluates to @p TRUE if the specified input queue is empty.
     *
     * @return              The queue status.
     * @retval false        if the queue is not empty.
     * @retval true         if the queue is empty.
     *
     * @iclass
     */
    bool isEmptyI(void);

    /**
     * @brief   Evaluates to @p TRUE if the specified input queue is full.
     *
     * @return              The queue status.
     * @retval FALSE        if the queue is not full.
     * @retval TRUE         if the queue is full.
     *
     * @iclass
     */
    bool isFullI(void);

    /**
     * @brief   Resets an input queue.
     * @details All the data in the input queue is erased and lost, any waiting
     *          thread is resumed with status @p Q_RESET.
     * @note    A reset operation can be used by a low level driver in order to
     *          obtain immediate attention from the high level layers.
     * @iclass
     */
    void resetI(void);

    /**
     * @brief   Input queue write.
     * @details A byte value is written into the low end of an input queue.
     *
     * @param[in] b         the byte value to be written in the queue
     * @return              The operation status.
     * @retval Q_OK         if the operation has been completed with success.
     * @retval Q_FULL       if the queue is full and the operation cannot be
     *                      completed.
     *
     * @iclass
     */
    msg_t putI(uint8_t b);

    /**
     * @brief   Input queue read.
     * @details This function reads a byte value from an input queue. If the
     *          queue is empty then the calling thread is suspended until a
     *          byte arrives in the queue.
     *
     * @return              A byte value from the queue.
     * @retval Q_RESET      if the queue has been reset.
     *
     * @api
     */
    msg_t get();

    /**
     * @brief   Input queue read with timeout.
     * @details This function reads a byte value from an input queue. If the
     *          queue is empty then the calling thread is suspended until a
     *          byte arrives in the queue or a timeout occurs.
     * @note    The callback is invoked before reading the character from the
     *          buffer or before entering the state @p THD_STATE_WTQUEUE.
     *
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              A byte value from the queue.
     * @retval Q_TIMEOUT    if the specified time expired.
     * @retval Q_RESET      if the queue has been reset.
     *
     * @api
     */
    msg_t getTimeout(systime_t time);

    /**
     * @brief   Input queue read with timeout.
     * @details The function reads data from an input queue into a buffer. The
     *          operation completes when the specified amount of data has been
     *          transferred or after the specified timeout or if the queue has
     *          been reset.
     * @note    The function is not atomic, if you need atomicity it is
     *          suggested to use a semaphore or a mutex for mutual exclusion.
     * @note    The callback is invoked before reading each character from the
     *          buffer or before entering the state @p THD_STATE_WTQUEUE.
     *
     * @param[out] bp       pointer to the data buffer
     * @param[in] n         the maximum amount of data to be transferred, the
     *                      value 0 is reserved
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The number of bytes effectively transferred.
     *
     * @api
     */
    size_t readTimeout(uint8_t *bp, size_t n, systime_t time);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::InQueueBuffer                                              *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Template class encapsulating an input queue and its buffer.
   *
   * @param N                   size of the input queue
   */
  template <int N>
  class InQueueBuffer : public InQueue {
  private:
    uint8_t iq_buf[N];

  public:
    /**
     * @brief   InQueueBuffer constructor.
     *
     * @param[in] infy      input notify callback function
     * @param[in] link      parameter to be passed to the callback
     *
     * @init
     */
    InQueueBuffer(qnotify_t infy, void *link) : InQueue(iq_buf, N,
                                                        infy, link) {
    }
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::OutQueue                                                   *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating an output queue.
   */
  class OutQueue {
  private:
    /**
     * @brief   Embedded @p ::OutputQueue structure.
     */
    ::OutputQueue oq;

  public:
    /**
     * @brief   OutQueue constructor.
     *
     * @param[in] bp        pointer to a memory area allocated as queue buffer
     * @param[in] size      size of the queue buffer
     * @param[in] onfy      pointer to a callback function that is invoked when
     *                      data is written to the queue. The value can be
     *                      @p NULL.
     * @param[in] link      application defined pointer
     *
     * @init
     */
    OutQueue(uint8_t *bp, size_t size, qnotify_t onfy, void *link);

    /**
     * @brief   Returns the filled space into an output queue.
     *
     * @return              The number of full bytes in the queue.
     * @retval 0            if the queue is empty.
     *
     * @iclass
     */
    size_t getFullI(void);

    /**
     * @brief   Returns the empty space into an output queue.
     *
     * @return              The number of empty bytes in the queue.
     * @retval 0            if the queue is full.
     *
     * @iclass
     */
    size_t getEmptyI(void);

    /**
     * @brief   Evaluates to @p TRUE if the specified output queue is empty.
     *
     * @return              The queue status.
     * @retval false        if the queue is not empty.
     * @retval true         if the queue is empty.
     *
     * @iclass
     */
    bool isEmptyI(void);

    /**
     * @brief   Evaluates to @p TRUE if the specified output queue is full.
     *
     * @return              The queue status.
     * @retval FALSE        if the queue is not full.
     * @retval TRUE         if the queue is full.
     *
     * @iclass
     */
    bool isFullI(void);

    /**
     * @brief   Resets an output queue.
     * @details All the data in the output queue is erased and lost, any
     *          waiting thread is resumed with status @p Q_RESET.
     * @note    A reset operation can be used by a low level driver in order
     *          to obtain immediate attention from the high level layers.
     *
     * @iclass
     */
    void resetI(void);

    /**
     * @brief   Output queue write.
     * @details This function writes a byte value to an output queue. If the
     *          queue is full then the calling thread is suspended until there
     *          is space in the queue.
     *
     * @param[in] b         the byte value to be written in the queue
     * @return              The operation status.
     * @retval Q_OK         if the operation succeeded.
     * @retval Q_RESET      if the queue has been reset.
     *
     * @api
     */
    msg_t put(uint8_t b);

    /**
     * @brief   Output queue write with timeout.
     * @details This function writes a byte value to an output queue. If the
     *          queue is full then the calling thread is suspended until there
     *          is space in the queue or a timeout occurs.
     * @note    The callback is invoked after writing the character into the
     *          buffer.
     *
     * @param[in] b         the byte value to be written in the queue
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval Q_OK         if the operation succeeded.
     * @retval Q_TIMEOUT    if the specified time expired.
     * @retval Q_RESET      if the queue has been reset.
     *
     * @api
     */
    msg_t putTimeout(uint8_t b, systime_t time);

    /**
     * @brief   Output queue read.
     * @details A byte value is read from the low end of an output queue.
     *
     * @return              The byte value from the queue.
     * @retval Q_EMPTY      if the queue is empty.
     *
     * @iclass
     */
    msg_t getI(void);

    /**
     * @brief   Output queue write with timeout.
     * @details The function writes data from a buffer to an output queue. The
     *          operation completes when the specified amount of data has been
     *          transferred or after the specified timeout or if the queue has
     *          been reset.
     * @note    The function is not atomic, if you need atomicity it is
     *          suggested to use a semaphore or a mutex for mutual exclusion.
     * @note    The callback is invoked after writing each character into the
     *          buffer.
     *
     * @param[out] bp       pointer to the data buffer
     * @param[in] n         the maximum amount of data to be transferred, the
     *                      value 0 is reserved
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The number of bytes effectively transferred.
     *
     * @api
     */
    size_t writeTimeout(const uint8_t *bp, size_t n, systime_t time);
};

  /*------------------------------------------------------------------------*
   * chibios_rt::OutQueueBuffer                                             *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Template class encapsulating an output queue and its buffer.
   *
   * @param N                   size of the output queue
   */
  template <int N>
  class OutQueueBuffer : public OutQueue {
  private:
    uint8_t oq_buf[N];

  public:
    /**
     * @brief   OutQueueBuffer constructor.
     *
     * @param[in] onfy      output notify callback function
     * @param[in] link      parameter to be passed to the callback
     *
     * @init
     */
    OutQueueBuffer(qnotify_t onfy, void *link) : OutQueue(oq_buf, N,
                                                          onfy, link) {
    }
  };
#endif /* CH_USE_QUEUES */

#if CH_USE_MAILBOXES || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::Mailbox                                                    *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a mailbox.
   */
  class Mailbox {
  public:
    /**
     * @brief   Embedded @p ::Mailbox structure.
     */
    ::Mailbox mb;

    /**
     * @brief   Mailbox constructor.
     * @details The embedded @p ::Mailbox structure is initialized.
     *
     * @param[in] buf           pointer to the messages buffer as an array of
     *                          @p msg_t
     * @param[in] n             number of elements in the buffer array
     *
     * @init
     */
    Mailbox(msg_t *buf, cnt_t n);

    /**
     * @brief   Resets a Mailbox object.
     * @details All the waiting threads are resumed with status @p RDY_RESET
     *          and the queued messages are lost.
     *
     * @api
     */
    void reset(void);

    /**
     * @brief   Posts a message into a mailbox.
     * @details The invoking thread waits until a empty slot in the mailbox
     *          becomes available or the specified time runs out.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @api
     */
    msg_t post(msg_t msg, systime_t time);

    /**
     * @brief   Posts a message into a mailbox.
     * @details The invoking thread waits until a empty slot in the mailbox
     *          becomes available or the specified time runs out.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @sclass
     */
    msg_t postS(msg_t msg, systime_t time);

    /**
     * @brief   Posts a message into a mailbox.
     * @details This variant is non-blocking, the function returns a timeout
     *          condition if the queue is full.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_TIMEOUT  if the mailbox is full and the message cannot be
     *                      posted.
     *
     * @iclass
     */
    msg_t postI(msg_t msg);

    /**
     * @brief   Posts an high priority message into a mailbox.
     * @details The invoking thread waits until a empty slot in the mailbox
     *          becomes available or the specified time runs out.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @api
     */
    msg_t postAhead(msg_t msg, systime_t time);

    /**
     * @brief   Posts an high priority message into a mailbox.
     * @details The invoking thread waits until a empty slot in the mailbox
     *          becomes available or the specified time runs out.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @param[in] time      the number of ticks before the operation timeouts,
     *                      the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @sclass
     */
    msg_t postAheadS(msg_t msg, systime_t time);

    /**
     * @brief   Posts an high priority message into a mailbox.
     * @details This variant is non-blocking, the function returns a timeout
     *          condition if the queue is full.
     *
     * @param[in] msg       the message to be posted on the mailbox
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly posted.
     * @retval RDY_TIMEOUT  if the mailbox is full and the message cannot be
     *                      posted.
     *
     * @iclass
     */
    msg_t postAheadI(msg_t msg);

    /**
     * @brief   Retrieves a message from a mailbox.
     * @details The invoking thread waits until a message is posted in the
     *          mailbox or the specified time runs out.
     *
     * @param[out] msgp     pointer to a message variable for the received
     * @param[in] time      message the number of ticks before the operation
     *                      timeouts, the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly fetched.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @api
     */
    msg_t fetch(msg_t *msgp, systime_t time);

    /**
     * @brief   Retrieves a message from a mailbox.
     * @details The invoking thread waits until a message is posted in the
     *          mailbox or the specified time runs out.
     *
     * @param[out] msgp     pointer to a message variable for the received
     * @param[in] time      message the number of ticks before the operation
     *                      timeouts, the following special values are allowed:
     *                      - @a TIME_IMMEDIATE immediate timeout.
     *                      - @a TIME_INFINITE no timeout.
     *                      .
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly fetched.
     * @retval RDY_RESET    if the mailbox has been reset while waiting.
     * @retval RDY_TIMEOUT  if the operation has timed out.
     *
     * @sclass
     */
    msg_t fetchS(msg_t *msgp, systime_t time);

    /**
     * @brief   Retrieves a message from a mailbox.
     * @details This variant is non-blocking, the function returns a timeout
     *          condition if the queue is empty.
     *
     * @param[out] msgp     pointer to a message variable for the received
     *                      message
     * @return              The operation status.
     * @retval RDY_OK       if a message has been correctly fetched.
     * @retval RDY_TIMEOUT  if the mailbox is empty and a message cannot be
     *                      fetched.
     *
     * @iclass
     */
    msg_t fetchI(msg_t *msgp);

    /**
     * @brief   Returns the number of free message slots into a mailbox.
     * @note    Can be invoked in any system state but if invoked out of a
     *          locked state then the returned value may change after reading.
     * @note    The returned value can be less than zero when there are waiting
     *          threads on the internal semaphore.
     *
     * @return              The number of empty message slots.
     *
     * @iclass
     */
    cnt_t getFreeCountI(void);

    /**
     * @brief   Returns the number of used message slots into a mailbox.
     * @note    Can be invoked in any system state but if invoked out of a
     *          locked state then the returned value may change after reading.
     * @note    The returned value can be less than zero when there are waiting
     *          threads on the internal semaphore.
     *
     * @return              The number of queued messages.
     *
     * @iclass
     */
    cnt_t getUsedCountI(void);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::MailboxBuffer                                              *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Template class encapsulating a mailbox and its messages buffer.
   *
   * @param N                   size of the mailbox
   */
  template <int N>
  class MailboxBuffer : public Mailbox {
  private:
    msg_t   mb_buf[N];

  public:
    /**
     * @brief   BufferMailbox constructor.
     *
     * @init
     */
    MailboxBuffer(void) : Mailbox(mb_buf,
                                  (cnt_t)(sizeof mb_buf / sizeof (msg_t))) {
    }
  };
#endif /* CH_USE_MAILBOXES */

#if CH_USE_MEMPOOLS || defined(__DOXYGEN__)
  /*------------------------------------------------------------------------*
   * chibios_rt::MemoryPool                                                 *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Class encapsulating a mailbox.
   */
  class MemoryPool {
  public:
    /**
     * @brief   Embedded @p ::MemoryPool structure.
     */
    ::MemoryPool pool;

    /**
     * @brief   MemoryPool constructor.
     *
     * @param[in] size      the size of the objects contained in this memory
     *                      pool, the minimum accepted size is the size of
     *                      a pointer to void.
     * @param[in] provider  memory provider function for the memory pool or
     *                      @p NULL if the pool is not allowed to grow
     *                      automatically
     *
     * @init
     */
    MemoryPool(size_t size, memgetfunc_t provider);

    /**
     * @brief   MemoryPool constructor.
     *
     * @param[in] size      the size of the objects contained in this memory
     *                      pool, the minimum accepted size is the size of
     *                      a pointer to void.
     * @param[in] provider  memory provider function for the memory pool or
     *                      @p NULL if the pool is not allowed to grow
     *                      automatically
     * @param[in] p         pointer to the array first element
     * @param[in] n         number of elements in the array
     *
     * @init
     */
    MemoryPool(size_t size, memgetfunc_t provider, void* p, size_t n);

    /**
     * @brief   Loads a memory pool with an array of static objects.
     * @pre     The memory pool must be already been initialized.
     * @pre     The array elements must be of the right size for the specified
     *          memory pool.
     * @post    The memory pool contains the elements of the input array.
     *
     * @param[in] p         pointer to the array first element
     * @param[in] n         number of elements in the array
     *
     * @api
     */
    void loadArray(void *p, size_t n);

    /**
     * @brief   Allocates an object from a memory pool.
     * @pre     The memory pool must be already been initialized.
     *
     * @return              The pointer to the allocated object.
     * @retval NULL         if pool is empty.
     *
     * @iclass
     */
    void *allocI(void);

    /**
     * @brief   Allocates an object from a memory pool.
     * @pre     The memory pool must be already been initialized.
     *
     * @return              The pointer to the allocated object.
     * @retval NULL         if pool is empty.
     *
     * @api
     */
    void *alloc(void);

    /**
     * @brief   Releases an object into a memory pool.
     * @pre     The memory pool must be already been initialized.
     * @pre     The freed object must be of the right size for the specified
     *          memory pool.
     * @pre     The object must be properly aligned to contain a pointer to
     *          void.
     *
     * @param[in] objp      the pointer to the object to be released
     *
     * @iclass
     */
    void free(void *objp);

    /**
     * @brief   Adds an object to a memory pool.
     * @pre     The memory pool must be already been initialized.
     * @pre     The added object must be of the right size for the specified
     *          memory pool.
     * @pre     The added object must be memory aligned to the size of
     *          @p stkalign_t type.
     * @note    This function is just an alias for @p chPoolFree() and has been
     *          added for clarity.
     *
     * @param[in] objp      the pointer to the object to be added
     *
     * @iclass
     */
    void freeI(void *objp);
  };

  /*------------------------------------------------------------------------*
   * chibios_rt::ObjectsPool                                                *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Template class encapsulating a memory pool and its elements.
   */
  template<class T, size_t N>
  class ObjectsPool : public MemoryPool {
  private:
    /* The buffer is declared as an array of pointers to void for two
       reasons:
       1) The objects must be properly aligned to hold a pointer as
          first field.
       2) There is no need to invoke constructors for object that are
          into the pool.*/
    void *pool_buf[(N * sizeof (T)) / sizeof (void *)];

  public:
    /**
     * @brief   ObjectsPool constructor.
     *
     * @init
     */
    ObjectsPool(void) : MemoryPool(sizeof (T), NULL) {

      loadArray(pool_buf, N);
    }
  };
#endif /* CH_USE_MEMPOOLS */

  /*------------------------------------------------------------------------*
   * chibios_rt::BaseSequentialStreamInterface                              *
   *------------------------------------------------------------------------*/
  /**
   * @brief   Interface of a ::BaseSequentialStream.
   * @note    You can cast a ::BaseSequentialStream to this interface and use
   *          it, the memory layout is the same.
   */
  class BaseSequentialStreamInterface {
  public:
    /**
     * @brief   Sequential Stream write.
     * @details The function writes data from a buffer to a stream.
     *
     * @param[in] bp        pointer to the data buffer
     * @param[in] n         the maximum amount of data to be transferred
     * @return              The number of bytes transferred. The return value
     *                      can be less than the specified number of bytes if
     *                      an end-of-file condition has been met.
     *
     * @api
     */
    virtual size_t write(const uint8_t *bp, size_t n) = 0;

    /**
     * @brief   Sequential Stream read.
     * @details The function reads data from a stream into a buffer.
     *
     * @param[out] bp       pointer to the data buffer
     * @param[in] n         the maximum amount of data to be transferred
     * @return              The number of bytes transferred. The return value
     *                      can be less than the specified number of bytes if
     *                      an end-of-file condition has been met.
     *
     * @api
     */
    virtual size_t read(uint8_t *bp, size_t n) = 0;

    /**
     * @brief   Sequential Stream blocking byte write.
     * @details This function writes a byte value to a channel. If the channel
     *          is not ready to accept data then the calling thread is
     *          suspended.
     *
     * @param[in] b         the byte value to be written to the channel
     *
     * @return              The operation status.
     * @retval Q_OK         if the operation succeeded.
     * @retval Q_RESET      if an end-of-file condition has been met.
     *
     * @api
     */
    virtual msg_t put(uint8_t b) = 0;

    /**
     * @brief   Sequential Stream blocking byte read.
     * @details This function reads a byte value from a channel. If the data
     *          is not available then the calling thread is suspended.
     *
     * @return              A byte value from the queue.
     * @retval Q_RESET      if an end-of-file condition has been met.
     *
     * @api
     */
    virtual msg_t get(void) = 0;
  };
}

#endif /* _CH_HPP_ */

/** @} */
