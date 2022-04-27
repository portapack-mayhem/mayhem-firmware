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
 * @file    ch.cpp
 * @brief   C++ wrapper code.
 *
 * @addtogroup cpp_library
 * @{
 */

#include "ch.hpp"

namespace chibios_rt {

  /*------------------------------------------------------------------------*
   * chibios_rt::System                                                     *
   *------------------------------------------------------------------------*/
  void System::init(void) {

    chSysInit();
  }

  void System::lock(void) {

    chSysLock();
  }

  void System::unlock(void) {

    chSysUnlock();
  }

  void System::lockFromIsr(void) {

    chSysLockFromIsr();
  }

  void System::unlockFromIsr(void) {

    chSysUnlockFromIsr();
  }

  systime_t System::getTime(void) {

    return chTimeNow();
  }

  bool System::isTimeWithin(systime_t start, systime_t end) {

    return (bool)chTimeIsWithin(start, end);
  }

  /*------------------------------------------------------------------------*
   * chibios_rt::Core                                                       *
   *------------------------------------------------------------------------*/
  void *Core::alloc(size_t size) {

    return chCoreAlloc(size);
  }

  void *Core::allocI(size_t size) {

    return chCoreAllocI(size);
  }

  size_t Core::getStatus(void) {

    return chCoreStatus();
  }

  /*------------------------------------------------------------------------*
   * chibios_rt::Timer                                                      *
   *------------------------------------------------------------------------*/
  void Timer::setI(systime_t time, vtfunc_t vtfunc, void *par) {

    chVTSetI(&timer_ref, time, vtfunc, par);
  }

  void Timer::resetI() {

    if (chVTIsArmedI(&timer_ref))
      chVTResetI(&timer_ref);
  }

  bool Timer::isArmedI(void) {

    return (bool)chVTIsArmedI(&timer_ref);
  }

  /*------------------------------------------------------------------------*
   * chibios_rt::ThreadReference                                            *
   *------------------------------------------------------------------------*/

  void ThreadReference::stop(void) {

    chDbgPanic("invoked unimplemented method stop()");
  }

  msg_t ThreadReference::suspend(void) {
    msg_t msg;

    chSysLock();

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #1",
                "already referenced");

    thread_ref = chThdSelf();
    chSchGoSleepS(THD_STATE_SUSPENDED);
    msg = thread_ref->p_u.rdymsg;

    chSysUnlock();
    return msg;
  }

  msg_t ThreadReference::suspendS(void) {

    chDbgAssert(thread_ref == NULL,
                "ThreadReference, #2",
                "already referenced");

    thread_ref = chThdSelf();
    chSchGoSleepS(THD_STATE_SUSPENDED);
    return thread_ref->p_u.rdymsg;
  }

  void ThreadReference::resume(msg_t msg) {

    chSysLock()

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #3",
                "not referenced");

    if (thread_ref) {
      Thread *tp = thread_ref;
      thread_ref = NULL;
      chSchWakeupS(tp, msg);
    }

    chSysUnlock();
  }

  void ThreadReference::resumeI(msg_t msg) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #4",
                "not referenced");

    if (thread_ref) {
      Thread *tp = thread_ref;
      thread_ref = NULL;
      tp->p_msg = msg;
      chSchReadyI(tp);
    }
  }

  void ThreadReference::requestTerminate(void) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #5",
                "not referenced");

    chThdTerminate(thread_ref);
  }

#if CH_USE_WAITEXIT
    msg_t ThreadReference::wait(void) {

      chDbgAssert(thread_ref != NULL,
                  "ThreadReference, #6",
                  "not referenced");

      msg_t msg = chThdWait(thread_ref);
      thread_ref = NULL;
      return msg;
    }
#endif /* CH_USE_WAITEXIT */

#if CH_USE_MESSAGES
  msg_t ThreadReference::sendMessage(msg_t msg) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #7",
                "not referenced");

    return chMsgSend(thread_ref, msg);
  }

  bool ThreadReference::isPendingMessage(void) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #7",
                "not referenced");

    return (bool)chMsgIsPendingI(thread_ref);
  }

  msg_t ThreadReference::getMessage(void) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #8",
                "not referenced");

    return chMsgGet(thread_ref);
  }

  void ThreadReference::releaseMessage(msg_t msg) {

    chDbgAssert(thread_ref != NULL,
                "ThreadReference, #9",
                "not referenced");

    chMsgRelease(thread_ref, msg);
  }
#endif /* CH_USE_MESSAGES */

#if CH_USE_EVENTS
    void ThreadReference::signalEvents(eventmask_t mask) {

      chDbgAssert(thread_ref != NULL,
                  "ThreadReference, #10",
                  "not referenced");

      chEvtSignal(thread_ref, mask);
    }

    void ThreadReference::signalEventsI(eventmask_t mask) {

      chDbgAssert(thread_ref != NULL,
                  "ThreadReference, #11",
                  "not referenced");

      chEvtSignalI(thread_ref, mask);
    }
#endif /* CH_USE_EVENTS */

#if CH_USE_DYNAMIC
#endif /* CH_USE_DYNAMIC */

  /*------------------------------------------------------------------------*
   * chibios_rt::BaseThread                                                 *
   *------------------------------------------------------------------------*/
  BaseThread::BaseThread() : ThreadReference(NULL) {

  }

  msg_t BaseThread::main(void) {

    return 0;
  }

  ThreadReference BaseThread::start(tprio_t prio) {

    (void)prio;

    return *this;
  };

  msg_t _thd_start(void *arg) {

    return ((BaseThread *)arg)->main();
  }

  void BaseThread::setName(const char *tname) {

    chRegSetThreadName(tname);
  }

  tprio_t BaseThread::setPriority(tprio_t newprio) {

    return chThdSetPriority(newprio);
  }

  void BaseThread::exit(msg_t msg) {

    chThdExit(msg);
  }

  void BaseThread::exitS(msg_t msg) {

    chThdExitS(msg);
  }

  bool BaseThread::shouldTerminate(void) {

    return (bool)chThdShouldTerminate();
  }

  void BaseThread::sleep(systime_t interval){

    chThdSleep(interval);
  }

  void BaseThread::sleepUntil(systime_t time) {

    chThdSleepUntil(time);
  }

  void BaseThread::yield(void) {

    chThdYield();
  }

#if CH_USE_MESSAGES
  ThreadReference BaseThread::waitMessage(void) {

    ThreadReference tr(chMsgWait());
    return tr;
  }
#endif /* CH_USE_MESSAGES */

#if CH_USE_EVENTS
  eventmask_t BaseThread::getAndClearEvents(eventmask_t mask) {

    return chEvtGetAndClearEvents(mask);
  }

  eventmask_t BaseThread::addEvents(eventmask_t mask) {

    return chEvtAddEvents(mask);
  }

  eventmask_t BaseThread::waitOneEvent(eventmask_t ewmask) {

    return chEvtWaitOne(ewmask);
  }

  eventmask_t BaseThread::waitAnyEvent(eventmask_t ewmask) {

    return chEvtWaitAny(ewmask);
  }

  eventmask_t BaseThread::waitAllEvents(eventmask_t ewmask) {

    return chEvtWaitAll(ewmask);
  }

#if CH_USE_EVENTS_TIMEOUT
  eventmask_t BaseThread::waitOneEventTimeout(eventmask_t ewmask,
                                              systime_t time) {

    return chEvtWaitOneTimeout(ewmask, time);
  }

  eventmask_t BaseThread::waitAnyEventTimeout(eventmask_t ewmask,
                                              systime_t time) {

    return chEvtWaitAnyTimeout(ewmask, time);
  }

  eventmask_t BaseThread::waitAllEventsTimeout(eventmask_t ewmask,
                                               systime_t time) {

    return chEvtWaitAllTimeout(ewmask, time);
  }
#endif /* CH_USE_EVENTS_TIMEOUT */

  void BaseThread::dispatchEvents(const evhandler_t handlers[],
                                  eventmask_t mask) {

    chEvtDispatch(handlers, mask);
  }
#endif /* CH_USE_EVENTS */

#if CH_USE_MUTEXES
  void BaseThread::unlockMutex(void) {

    chMtxUnlock();
  }

  void BaseThread::unlockMutexS(void) {

    chMtxUnlockS();
  }

  void BaseThread::unlockAllMutexes(void) {

    chMtxUnlockAll();
  }
#endif /* CH_USE_MUTEXES */

#if CH_USE_SEMAPHORES
  /*------------------------------------------------------------------------*
   * chibios_rt::CounterSemaphore                                           *
   *------------------------------------------------------------------------*/
  CounterSemaphore::CounterSemaphore(cnt_t n) {

    chSemInit(&sem, n);
  }

  void CounterSemaphore::reset(cnt_t n) {

    chSemReset(&sem, n);
  }

  void CounterSemaphore::resetI(cnt_t n) {

    chSemResetI(&sem, n);
  }

  msg_t CounterSemaphore::wait(void) {

    return chSemWait(&sem);
  }

  msg_t CounterSemaphore::waitS(void) {

    return chSemWaitS(&sem);
  }

  msg_t CounterSemaphore::waitTimeout(systime_t time) {

    return chSemWaitTimeout(&sem, time);
  }

  msg_t CounterSemaphore::waitTimeoutS(systime_t time) {

    return chSemWaitTimeoutS(&sem, time);
  }

  void CounterSemaphore::signal(void) {

    chSemSignal(&sem);
  }

  void CounterSemaphore::signalI(void) {

    chSemSignalI(&sem);
  }

  void CounterSemaphore::addCounterI(cnt_t n) {

    chSemAddCounterI(&sem, n);
  }

  cnt_t CounterSemaphore::getCounterI(void) {

    return chSemGetCounterI(&sem);
  }

#if CH_USE_SEMSW
  msg_t CounterSemaphore::signalWait(CounterSemaphore *ssem,
                                     CounterSemaphore *wsem) {

    return chSemSignalWait(&ssem->sem, &wsem->sem);
  }
#endif /* CH_USE_SEMSW */

  /*------------------------------------------------------------------------*
   * chibios_rt::BinarySemaphore                                            *
   *------------------------------------------------------------------------*/
  BinarySemaphore::BinarySemaphore(bool taken) {

    chBSemInit(&bsem, (bool_t)taken);
  }

  msg_t BinarySemaphore::wait(void) {

    return chBSemWait(&bsem);
  }

  msg_t BinarySemaphore::waitS(void) {

    return chBSemWaitS(&bsem);
  }

  msg_t BinarySemaphore::waitTimeout(systime_t time) {

    return chBSemWaitTimeout(&bsem, time);
  }

  msg_t BinarySemaphore::waitTimeoutS(systime_t time) {

    return chBSemWaitTimeoutS(&bsem, time);
  }

  void BinarySemaphore::reset(bool taken) {

    chBSemReset(&bsem, (bool_t)taken);
  }

  void BinarySemaphore::resetI(bool taken) {

    chBSemResetI(&bsem, (bool_t)taken);
  }

  void BinarySemaphore::signal(void) {

    chBSemSignal(&bsem);
  }

  void BinarySemaphore::signalI(void) {

    chBSemSignalI(&bsem);
  }

  bool BinarySemaphore::getStateI(void) {

    return (bool)chBSemGetStateI(&bsem);
  }
#endif /* CH_USE_SEMAPHORES */

#if CH_USE_MUTEXES
  /*------------------------------------------------------------------------*
   * chibios_rt::Mutex                                                      *
   *------------------------------------------------------------------------*/
  Mutex::Mutex(void) {

    chMtxInit(&mutex);
  }

  bool Mutex::tryLock(void) {

    return chMtxTryLock(&mutex);
  }

  bool Mutex::tryLockS(void) {

    return chMtxTryLockS(&mutex);
  }

  void Mutex::lock(void) {

    chMtxLock(&mutex);
  }

  void Mutex::lockS(void) {

    chMtxLockS(&mutex);
  }

#if CH_USE_CONDVARS
  /*------------------------------------------------------------------------*
   * chibios_rt::CondVar                                                    *
   *------------------------------------------------------------------------*/
  CondVar::CondVar(void) {

    chCondInit(&condvar);
  }

  void CondVar::signal(void) {

    chCondSignal(&condvar);
  }

  void CondVar::signalI(void) {

    chCondSignalI(&condvar);
  }

  void CondVar::broadcast(void) {

    chCondBroadcast(&condvar);
  }

  void CondVar::broadcastI(void) {

    chCondBroadcastI(&condvar);
  }

  msg_t CondVar::wait(void) {

    return chCondWait(&condvar);
  }

  msg_t CondVar::waitS(void) {

    return chCondWaitS(&condvar);
  }

#if CH_USE_CONDVARS_TIMEOUT
  msg_t CondVar::waitTimeout(systime_t time) {

    return chCondWaitTimeout(&condvar, time);
  }
#endif /* CH_USE_CONDVARS_TIMEOUT */
#endif /* CH_USE_CONDVARS */
#endif /* CH_USE_MUTEXES */

#if CH_USE_EVENTS
  /*------------------------------------------------------------------------*
   * chibios_rt::EvtListener                                              *
   *------------------------------------------------------------------------*/
  flagsmask_t EvtListener::getAndClearFlags(void) {

    return chEvtGetAndClearFlags(&ev_listener);
  }

  flagsmask_t EvtListener::getAndClearFlagsI(void) {

    return chEvtGetAndClearFlagsI(&ev_listener);
  }

  /*------------------------------------------------------------------------*
   * chibios_rt::EvtSource                                                *
   *------------------------------------------------------------------------*/
  EvtSource::EvtSource(void) {

    chEvtInit(&ev_source);
  }

  void EvtSource::registerOne(chibios_rt::EvtListener *elp,
                                eventid_t eid) {

    chEvtRegister(&ev_source, &elp->ev_listener, eid);
  }

  void EvtSource::registerMask(chibios_rt::EvtListener *elp,
                                 eventmask_t emask) {

    chEvtRegisterMask(&ev_source, &elp->ev_listener, emask);
  }

  void EvtSource::unregister(chibios_rt::EvtListener *elp) {

    chEvtUnregister(&ev_source, &elp->ev_listener);
  }

  void EvtSource::broadcastFlags(flagsmask_t flags) {

    chEvtBroadcastFlags(&ev_source, flags);
  }

  void EvtSource::broadcastFlagsI(flagsmask_t flags) {

    chEvtBroadcastFlagsI(&ev_source, flags);
  }
#endif /* CH_USE_EVENTS */

#if CH_USE_QUEUES
  /*------------------------------------------------------------------------*
   * chibios_rt::InQueue                                                    *
   *------------------------------------------------------------------------*/
  InQueue::InQueue(uint8_t *bp, size_t size, qnotify_t infy, void *link) {

    chIQInit(&iq, bp, size, infy, link);
  }

  size_t InQueue::getFullI(void) {

    return chIQGetFullI(&iq);
  }

  size_t InQueue::getEmptyI(void) {

    return chIQGetEmptyI(&iq);
  }

  bool InQueue::isEmptyI(void) {

    return (bool)chIQIsEmptyI(&iq);
  }

  bool InQueue::isFullI(void) {

    return (bool)chIQIsFullI(&iq);
  }

  void InQueue::resetI(void) {

    chIQResetI(&iq);
  }

  msg_t InQueue::putI(uint8_t b) {

    return chIQPutI(&iq, b);
  }

  msg_t InQueue::get() {

    return chIQGet(&iq);
  }

  msg_t InQueue::getTimeout(systime_t time) {

    return chIQGetTimeout(&iq, time);
  }

  size_t InQueue::readTimeout(uint8_t *bp, size_t n, systime_t time) {

    return chIQReadTimeout(&iq, bp, n, time);
  }

  /*------------------------------------------------------------------------*
   * chibios_rt::OutQueue                                                   *
   *------------------------------------------------------------------------*/
  OutQueue::OutQueue(uint8_t *bp, size_t size, qnotify_t onfy, void *link) {

    chOQInit(&oq, bp, size, onfy, link);
  }

  size_t OutQueue::getFullI(void) {

    return chOQGetFullI(&oq);
  }

  size_t OutQueue::getEmptyI(void) {

    return chOQGetEmptyI(&oq);
  }

  bool OutQueue::isEmptyI(void) {

    return (bool)chOQIsEmptyI(&oq);
  }

  bool OutQueue::isFullI(void) {

    return (bool)chOQIsFullI(&oq);
  }

  void OutQueue::resetI(void) {

    chOQResetI(&oq);
  }

  msg_t OutQueue::put(uint8_t b) {

    return chOQPut(&oq, b);
  }

  msg_t OutQueue::putTimeout(uint8_t b, systime_t time) {

    return chOQPutTimeout(&oq, b, time);
  }

  msg_t OutQueue::getI(void) {

    return chOQGetI(&oq);
  }

  size_t OutQueue::writeTimeout(const uint8_t *bp, size_t n,
                                systime_t time) {

    return chOQWriteTimeout(&oq, bp, n, time);
  }
#endif /* CH_USE_QUEUES */

#if CH_USE_MAILBOXES
  /*------------------------------------------------------------------------*
   * chibios_rt::Mailbox                                                    *
   *------------------------------------------------------------------------*/
  Mailbox::Mailbox(msg_t *buf, cnt_t n) {

    chMBInit(&mb, buf, n);
  }

  void Mailbox::reset(void) {

    chMBReset(&mb);
  }

  msg_t Mailbox::post(msg_t msg, systime_t time) {

    return chMBPost(&mb, msg, time);
  }

  msg_t Mailbox::postS(msg_t msg, systime_t time) {

    return chMBPostS(&mb, msg, time);
  }

  msg_t Mailbox::postI(msg_t msg) {

    return chMBPostI(&mb, msg);
  }

  msg_t Mailbox::postAhead(msg_t msg, systime_t time) {

    return chMBPostAhead(&mb, msg, time);
  }

  msg_t Mailbox::postAheadS(msg_t msg, systime_t time) {

    return chMBPostAheadS(&mb, msg, time);
  }

  msg_t Mailbox::postAheadI(msg_t msg) {

    return chMBPostAheadI(&mb, msg);
  }

  msg_t Mailbox::fetch(msg_t *msgp, systime_t time) {

    return chMBFetch(&mb, msgp, time);
  }

  msg_t Mailbox::fetchS(msg_t *msgp, systime_t time) {

    return chMBFetchS(&mb, msgp, time);
  }

  msg_t Mailbox::fetchI(msg_t *msgp) {

    return chMBFetchI(&mb, msgp);
  }

  cnt_t Mailbox::getFreeCountI(void) {

    return chMBGetFreeCountI(&mb);
  }

  cnt_t Mailbox::getUsedCountI(void) {

    return chMBGetUsedCountI(&mb);
  }
#endif /* CH_USE_MAILBOXES */

#if CH_USE_MEMPOOLS
  /*------------------------------------------------------------------------*
   * chibios_rt::MemoryPool                                                 *
   *------------------------------------------------------------------------*/
  MemoryPool::MemoryPool(size_t size, memgetfunc_t provider) {

    chPoolInit(&pool, size, provider);
  }

  MemoryPool::MemoryPool(size_t size, memgetfunc_t provider, void* p, size_t n) {

    chPoolInit(&pool, size, provider);
    chPoolLoadArray(&pool, p, n);
  }


  void MemoryPool::loadArray(void *p, size_t n) {

    chPoolLoadArray(&pool, p, n);
  }

  void *MemoryPool::allocI(void) {

    return chPoolAllocI(&pool);
  }

  void *MemoryPool::alloc(void) {

    return chPoolAlloc(&pool);
  }

  void MemoryPool::free(void *objp) {

    chPoolFree(&pool, objp);
  }

  void MemoryPool::freeI(void *objp) {

    chPoolFreeI(&pool, objp);
  }
#endif /* CH_USE_MEMPOOLS */
}

/** @} */
