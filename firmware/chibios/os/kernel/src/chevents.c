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
   Concepts and parts of this file have been contributed by Scott (skute).
 */

/**
 * @file    chevents.c
 * @brief   Events code.
 *
 * @addtogroup events
 * @details Event Flags, Event Sources and Event Listeners.
 *          <h2>Operation mode</h2>
 *          Each thread has a mask of pending event flags inside its @p Thread
 *          structure.
 *          Operations defined for event flags:
 *          - <b>Wait</b>, the invoking thread goes to sleep until a certain
 *            AND/OR combination of event flags becomes pending.
 *          - <b>Clear</b>, a mask of event flags is cleared from the pending
 *            events mask, the cleared event flags mask is returned (only the
 *            flags that were actually pending and then cleared).
 *          - <b>Signal</b>, an event mask is directly ORed to the mask of the
 *            signaled thread.
 *          - <b>Broadcast</b>, each thread registered on an Event Source is
 *            signaled with the event flags specified in its Event Listener.
 *          - <b>Dispatch</b>, an events mask is scanned and for each bit set
 *            to one an associated handler function is invoked. Bit masks are
 *            scanned from bit zero upward.
 *          .
 *          An Event Source is a special object that can be "broadcasted" by
 *          a thread or an interrupt service routine. Broadcasting an Event
 *          Source has the effect that all the threads registered on the
 *          Event Source will be signaled with an events mask.<br>
 *          An unlimited number of Event Sources can exists in a system and
 *          each thread can be listening on an unlimited number of
 *          them.
 * @pre     In order to use the Events APIs the @p CH_USE_EVENTS option must be
 *          enabled in @p chconf.h.
 * @post    Enabling events requires 1-4 (depending on the architecture)
 *          extra bytes in the @p Thread structure.
 * @{
 */

#include "ch.h"

#if CH_USE_EVENTS || defined(__DOXYGEN__)
/**
 * @brief   Registers an Event Listener on an Event Source.
 * @details Once a thread has registered as listener on an event source it
 *          will be notified of all events broadcasted there.
 * @note    Multiple Event Listeners can specify the same bits to be ORed to
 *          different threads.
 *
 * @param[in] esp       pointer to the  @p EventSource structure
 * @param[out] elp      pointer to the @p EventListener structure
 * @param[in] mask      the mask of event flags to be ORed to the thread when
 *                      the event source is broadcasted
 *
 * @api
 */
void chEvtRegisterMask(EventSource *esp, EventListener *elp, eventmask_t mask) {

  chDbgCheck((esp != NULL) && (elp != NULL), "chEvtRegisterMask");

  chSysLock();
  elp->el_next     = esp->es_next;
  esp->es_next     = elp;
  elp->el_listener = currp;
  elp->el_mask     = mask;
  elp->el_flags    = 0;
  chSysUnlock();
}

/**
 * @brief   Unregisters an Event Listener from its Event Source.
 * @note    If the event listener is not registered on the specified event
 *          source then the function does nothing.
 * @note    For optimal performance it is better to perform the unregister
 *          operations in inverse order of the register operations (elements
 *          are found on top of the list).
 *
 * @param[in] esp       pointer to the  @p EventSource structure
 * @param[in] elp       pointer to the @p EventListener structure
 *
 * @api
 */
void chEvtUnregister(EventSource *esp, EventListener *elp) {
  EventListener *p;

  chDbgCheck((esp != NULL) && (elp != NULL), "chEvtUnregister");

  p = (EventListener *)esp;
  chSysLock();
  while (p->el_next != (EventListener *)esp) {
    if (p->el_next == elp) {
      p->el_next = elp->el_next;
      break;
    }
    p = p->el_next;
  }
  chSysUnlock();
}

/**
 * @brief   Clears the pending events specified in the mask.
 *
 * @param[in] mask      the events to be cleared
 * @return              The pending events that were cleared.
 *
 * @api
 */
eventmask_t chEvtGetAndClearEvents(eventmask_t mask) {
  eventmask_t m;

  chSysLock();

  m = currp->p_epending & mask;
  currp->p_epending &= ~mask;

  chSysUnlock();
  return m;
}

/**
 * @brief   Adds (OR) a set of event flags on the current thread, this is
 *          @b much faster than using @p chEvtBroadcast() or @p chEvtSignal().
 *
 * @param[in] mask      the event flags to be added
 * @return              The current pending events mask.
 *
 * @api
 */
eventmask_t chEvtAddEvents(eventmask_t mask) {

  chSysLock();

  mask = (currp->p_epending |= mask);

  chSysUnlock();
  return mask;
}

/**
 * @brief   Signals all the Event Listeners registered on the specified Event
 *          Source.
 * @details This function variants ORs the specified event flags to all the
 *          threads registered on the @p EventSource in addition to the event
 *          flags specified by the threads themselves in the
 *          @p EventListener objects.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] esp       pointer to the @p EventSource structure
 * @param[in] flags     the flags set to be added to the listener flags mask
 *
 * @iclass
 */
void chEvtBroadcastFlagsI(EventSource *esp, flagsmask_t flags) {
  EventListener *elp;

  chDbgCheckClassI();
  chDbgCheck(esp != NULL, "chEvtBroadcastMaskI");

  elp = esp->es_next;
  while (elp != (EventListener *)esp) {
    elp->el_flags |= flags;
    chEvtSignalI(elp->el_listener, elp->el_mask);
    elp = elp->el_next;
  }
}

/**
 * @brief   Returns the flags associated to an @p EventListener.
 * @details The flags are returned and the @p EventListener flags mask is
 *          cleared.
 *
 * @param[in] elp       pointer to the @p EventListener structure
 * @return              The flags added to the listener by the associated
 *                      event source.
 *
 * @api
 */
flagsmask_t chEvtGetAndClearFlags(EventListener *elp) {
  flagsmask_t flags;

  chSysLock();

  flags = elp->el_flags;
  elp->el_flags = 0;

  chSysUnlock();
  return flags;
}

/**
 * @brief   Adds a set of event flags directly to specified @p Thread.
 *
 * @param[in] tp        the thread to be signaled
 * @param[in] mask      the event flags set to be ORed
 *
 * @api
 */
void chEvtSignal(Thread *tp, eventmask_t mask) {

  chDbgCheck(tp != NULL, "chEvtSignal");

  chSysLock();
  chEvtSignalI(tp, mask);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Adds a set of event flags directly to specified @p Thread.
 * @post    This function does not reschedule so a call to a rescheduling
 *          function must be performed before unlocking the kernel. Note that
 *          interrupt handlers always reschedule on exit so an explicit
 *          reschedule must not be performed in ISRs.
 *
 * @param[in] tp        the thread to be signaled
 * @param[in] mask      the event flags set to be ORed
 *
 * @iclass
 */
void chEvtSignalI(Thread *tp, eventmask_t mask) {

  chDbgCheckClassI();
  chDbgCheck(tp != NULL, "chEvtSignalI");

  tp->p_epending |= mask;
  /* Test on the AND/OR conditions wait states.*/
  if (((tp->p_state == THD_STATE_WTOREVT) &&
       ((tp->p_epending & tp->p_u.ewmask) != 0)) ||
      ((tp->p_state == THD_STATE_WTANDEVT) &&
       ((tp->p_epending & tp->p_u.ewmask) == tp->p_u.ewmask)))
    chSchReadyI(tp)->p_u.rdymsg = RDY_OK;
}

/**
 * @brief   Signals all the Event Listeners registered on the specified Event
 *          Source.
 * @details This function variants ORs the specified event flags to all the
 *          threads registered on the @p EventSource in addition to the event
 *          flags specified by the threads themselves in the
 *          @p EventListener objects.
 *
 * @param[in] esp       pointer to the @p EventSource structure
 * @param[in] flags     the flags set to be added to the listener flags mask
 *
 * @api
 */
void chEvtBroadcastFlags(EventSource *esp, flagsmask_t flags) {

  chSysLock();
  chEvtBroadcastFlagsI(esp, flags);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Returns the flags associated to an @p EventListener.
 * @details The flags are returned and the @p EventListener flags mask is
 *          cleared.
 *
 * @param[in] elp       pointer to the @p EventListener structure
 * @return              The flags added to the listener by the associated
 *                      event source.
 *
 * @iclass
 */
flagsmask_t chEvtGetAndClearFlagsI(EventListener *elp) {
  flagsmask_t flags;

  flags = elp->el_flags;
  elp->el_flags = 0;

  return flags;
}

/**
 * @brief   Invokes the event handlers associated to an event flags mask.
 *
 * @param[in] mask      mask of the event flags to be dispatched
 * @param[in] handlers  an array of @p evhandler_t. The array must have size
 *                      equal to the number of bits in eventmask_t.
 *
 * @api
 */
void chEvtDispatch(const evhandler_t *handlers, eventmask_t mask) {
  eventid_t eid;

  chDbgCheck(handlers != NULL, "chEvtDispatch");

  eid = 0;
  while (mask) {
    if (mask & EVENT_MASK(eid)) {
      chDbgAssert(handlers[eid] != NULL,
                  "chEvtDispatch(), #1",
                  "null handler");
      mask &= ~EVENT_MASK(eid);
      handlers[eid](eid);
    }
    eid++;
  }
}

#if CH_OPTIMIZE_SPEED || !CH_USE_EVENTS_TIMEOUT || defined(__DOXYGEN__)
/**
 * @brief   Waits for exactly one of the specified events.
 * @details The function waits for one event among those specified in
 *          @p mask to become pending then the event is cleared and returned.
 * @note    One and only one event is served in the function, the one with the
 *          lowest event id. The function is meant to be invoked into a loop in
 *          order to serve all the pending events.<br>
 *          This means that Event Listeners with a lower event identifier have
 *          an higher priority.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS enables all the events
 * @return              The mask of the lowest id served and cleared event.
 *
 * @api
 */
eventmask_t chEvtWaitOne(eventmask_t mask) {
  Thread *ctp = currp;
  eventmask_t m;

  chSysLock();

  if ((m = (ctp->p_epending & mask)) == 0) {
    ctp->p_u.ewmask = mask;
    chSchGoSleepS(THD_STATE_WTOREVT);
    m = ctp->p_epending & mask;
  }
  m ^= m & (m - 1);
  ctp->p_epending &= ~m;

  chSysUnlock();
  return m;
}

/**
 * @brief   Waits for any of the specified events.
 * @details The function waits for any event among those specified in
 *          @p mask to become pending then the events are cleared and returned.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS enables all the events
 * @return              The mask of the served and cleared events.
 *
 * @api
 */
eventmask_t chEvtWaitAny(eventmask_t mask) {
  Thread *ctp = currp;
  eventmask_t m;

  chSysLock();

  if ((m = (ctp->p_epending & mask)) == 0) {
    ctp->p_u.ewmask = mask;
    chSchGoSleepS(THD_STATE_WTOREVT);
    m = ctp->p_epending & mask;
  }
  ctp->p_epending &= ~m;

  chSysUnlock();
  return m;
}

/**
 * @brief   Waits for all the specified events.
 * @details The function waits for all the events specified in @p mask to
 *          become pending then the events are cleared and returned.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS requires all the events
 * @return              The mask of the served and cleared events.
 *
 * @api
 */
eventmask_t chEvtWaitAll(eventmask_t mask) {
  Thread *ctp = currp;

  chSysLock();

  if ((ctp->p_epending & mask) != mask) {
    ctp->p_u.ewmask = mask;
    chSchGoSleepS(THD_STATE_WTANDEVT);
  }
  ctp->p_epending &= ~mask;

  chSysUnlock();
  return mask;
}
#endif /* CH_OPTIMIZE_SPEED || !CH_USE_EVENTS_TIMEOUT */

#if CH_USE_EVENTS_TIMEOUT || defined(__DOXYGEN__)
/**
 * @brief   Waits for exactly one of the specified events.
 * @details The function waits for one event among those specified in
 *          @p mask to become pending then the event is cleared and returned.
 * @note    One and only one event is served in the function, the one with the
 *          lowest event id. The function is meant to be invoked into a loop in
 *          order to serve all the pending events.<br>
 *          This means that Event Listeners with a lower event identifier have
 *          an higher priority.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS enables all the events
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The mask of the lowest id served and cleared event.
 * @retval 0            if the operation has timed out.
 *
 * @api
 */
eventmask_t chEvtWaitOneTimeout(eventmask_t mask, systime_t time) {
  Thread *ctp = currp;
  eventmask_t m;

  chSysLock();

  if ((m = (ctp->p_epending & mask)) == 0) {
    if (TIME_IMMEDIATE == time) {
      chSysUnlock();
      return (eventmask_t)0;
    }
    ctp->p_u.ewmask = mask;
    if (chSchGoSleepTimeoutS(THD_STATE_WTOREVT, time) < RDY_OK) {
      chSysUnlock();
      return (eventmask_t)0;
    }
    m = ctp->p_epending & mask;
  }
  m ^= m & (m - 1);
  ctp->p_epending &= ~m;

  chSysUnlock();
  return m;
}

/**
 * @brief   Waits for any of the specified events.
 * @details The function waits for any event among those specified in
 *          @p mask to become pending then the events are cleared and
 *          returned.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS enables all the events
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The mask of the served and cleared events.
 * @retval 0            if the operation has timed out.
 *
 * @api
 */
eventmask_t chEvtWaitAnyTimeout(eventmask_t mask, systime_t time) {
  Thread *ctp = currp;
  eventmask_t m;

  chSysLock();

  if ((m = (ctp->p_epending & mask)) == 0) {
    if (TIME_IMMEDIATE == time) {
      chSysUnlock();
      return (eventmask_t)0;
    }
    ctp->p_u.ewmask = mask;
    if (chSchGoSleepTimeoutS(THD_STATE_WTOREVT, time) < RDY_OK) {
      chSysUnlock();
      return (eventmask_t)0;
    }
    m = ctp->p_epending & mask;
  }
  ctp->p_epending &= ~m;

  chSysUnlock();
  return m;
}

/**
 * @brief   Waits for all the specified events.
 * @details The function waits for all the events specified in @p mask to
 *          become pending then the events are cleared and returned.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS requires all the events
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The mask of the served and cleared events.
 * @retval 0            if the operation has timed out.
 *
 * @api
 */
eventmask_t chEvtWaitAllTimeout(eventmask_t mask, systime_t time) {
  Thread *ctp = currp;

  chSysLock();

  if ((ctp->p_epending & mask) != mask) {
    if (TIME_IMMEDIATE == time) {
      chSysUnlock();
      return (eventmask_t)0;
    }
    ctp->p_u.ewmask = mask;
    if (chSchGoSleepTimeoutS(THD_STATE_WTANDEVT, time) < RDY_OK) {
      chSysUnlock();
      return (eventmask_t)0;
    }
  }
  ctp->p_epending &= ~mask;

  chSysUnlock();
  return mask;
}
#endif /* CH_USE_EVENTS_TIMEOUT */

#endif /* CH_USE_EVENTS */

/** @} */
