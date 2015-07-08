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

#include "ch.h"
#include "test.h"

/**
 * @page test_events Events test
 *
 * File: @ref testevt.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref events subsystem.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the @ref events subsystem.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_EVENTS
 * - @p CH_USE_EVENTS_TIMEOUT
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_events_001
 * - @subpage test_events_002
 * - @subpage test_events_003
 * .
 * @file testevt.c
 * @brief Events test source file
 * @file testevt.h
 * @brief Events test header file
 */

#if CH_USE_EVENTS || defined(__DOXYGEN__)

#define ALLOWED_DELAY MS2ST(5)

/*
 * Note, the static initializers are not really required because the
 * variables are explicitly initialized in each test case. It is done in order
 * to test the macros.
 */
static EVENTSOURCE_DECL(es1);
static EVENTSOURCE_DECL(es2);

/**
 * @page test_events_001 Events registration and dispatch
 *
 * <h2>Description</h2>
 * Two event listeners are registered on an event source and then unregistered
 * in the same order.<br>
 * The test expects that the even source has listeners after the registrations
 * and after the first unregistration, then, after the second unegistration,
 * the test expects no more listeners.<br>
 * In the second part the test dispatches three event flags and verifies that
 * the associated event handlers are invoked in LSb-first order.
 */

static void evt1_setup(void) {

  chEvtGetAndClearEvents(ALL_EVENTS);
}

static void h1(eventid_t id) {(void)id;test_emit_token('A');}
static void h2(eventid_t id) {(void)id;test_emit_token('B');}
static void h3(eventid_t id) {(void)id;test_emit_token('C');}
static ROMCONST evhandler_t evhndl[] = {h1, h2, h3};

static void evt1_execute(void) {
  EventListener el1, el2;

  /*
   * Testing chEvtRegisterMask() and chEvtUnregister().
   */
  chEvtInit(&es1);
  chEvtRegisterMask(&es1, &el1, 1);
  chEvtRegisterMask(&es1, &el2, 2);
  test_assert(1, chEvtIsListeningI(&es1), "no listener");
  chEvtUnregister(&es1, &el1);
  test_assert(2, chEvtIsListeningI(&es1), "no listener");
  chEvtUnregister(&es1, &el2);
  test_assert(3, !chEvtIsListeningI(&es1), "stuck listener");

  /*
   * Testing chEvtDispatch().
   */
  chEvtDispatch(evhndl, 7);
  test_assert_sequence(4, "ABC");
}

ROMCONST struct testcase testevt1 = {
  "Events, registration and dispatch",
  evt1_setup,
  NULL,
  evt1_execute
};

/**
 * @page test_events_002 Events wait and broadcast
 *
 * <h2>Description</h2>
 * In this test the following APIs are indipently tested by starting threads
 * that signal/broadcast events after fixed delays:
 * - @p chEvtWaitOne()
 * - @p chEvtWaitAny()
 * - @p chEvtWaitAll()
 * .
 * After each test phase the test verifies that the events have been served at
 * the expected time and that there are no stuck event flags.
 */

static void evt2_setup(void) {

  chEvtGetAndClearEvents(ALL_EVENTS);
}

static msg_t thread1(void *p) {

  chThdSleepMilliseconds(50);
  chEvtSignal((Thread *)p, 1);
  return 0;
}

static msg_t thread2(void *p) {

  (void)p;
  chEvtBroadcast(&es1);
  chThdSleepMilliseconds(50);
  chEvtBroadcast(&es2);
  return 0;
}

static void evt2_execute(void) {
  eventmask_t m;
  EventListener el1, el2;
  systime_t target_time;

  /*
   * Test on chEvtWaitOne() without wait.
   */
  chEvtAddEvents(7);
  m = chEvtWaitOne(ALL_EVENTS);
  test_assert(1, m == 1, "single event error");
  m = chEvtWaitOne(ALL_EVENTS);
  test_assert(2, m == 2, "single event error");
  m = chEvtWaitOne(ALL_EVENTS);
  test_assert(3, m == 4, "single event error");
  m = chEvtGetAndClearEvents(ALL_EVENTS);
  test_assert(4, m == 0, "stuck event");

  /*
   * Test on chEvtWaitOne() with wait.
   */
  test_wait_tick();
  target_time = chTimeNow() + MS2ST(50);
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority() - 1,
                                 thread1, chThdSelf());
  m = chEvtWaitOne(ALL_EVENTS);
  test_assert_time_window(5, target_time, target_time + ALLOWED_DELAY);
  test_assert(6, m == 1, "single event error");
  m = chEvtGetAndClearEvents(ALL_EVENTS);
  test_assert(7, m == 0, "stuck event");
  test_wait_threads();

  /*
   * Test on chEvtWaitAny() without wait.
   */
  chEvtAddEvents(5);
  m = chEvtWaitAny(ALL_EVENTS);
  test_assert(8, m == 5, "unexpected pending bit");
  m = chEvtGetAndClearEvents(ALL_EVENTS);
  test_assert(9, m == 0, "stuck event");

  /*
   * Test on chEvtWaitAny() with wait.
   */
  test_wait_tick();
  target_time = chTimeNow() + MS2ST(50);
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority() - 1,
                                 thread1, chThdSelf());
  m = chEvtWaitAny(ALL_EVENTS);
  test_assert_time_window(10, target_time, target_time + ALLOWED_DELAY);
  test_assert(11, m == 1, "single event error");
  m = chEvtGetAndClearEvents(ALL_EVENTS);
  test_assert(12, m == 0, "stuck event");
  test_wait_threads();

  /*
   * Test on chEvtWaitAll().
   */
  chEvtInit(&es1);
  chEvtInit(&es2);
  chEvtRegisterMask(&es1, &el1, 1);
  chEvtRegisterMask(&es2, &el2, 4);
  test_wait_tick();
  target_time = chTimeNow() + MS2ST(50);
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority() - 1,
                                 thread2, "A");
  m = chEvtWaitAll(5);
  test_assert_time_window(13, target_time, target_time + ALLOWED_DELAY);
  m = chEvtGetAndClearEvents(ALL_EVENTS);
  test_assert(14, m == 0, "stuck event");
  test_wait_threads();
  chEvtUnregister(&es1, &el1);
  chEvtUnregister(&es2, &el2);
  test_assert(15, !chEvtIsListeningI(&es1), "stuck listener");
  test_assert(16, !chEvtIsListeningI(&es2), "stuck listener");
}

ROMCONST struct testcase testevt2 = {
  "Events, wait and broadcast",
  evt2_setup,
  NULL,
  evt2_execute
};

#if CH_USE_EVENTS_TIMEOUT || defined(__DOXYGEN__)
/**
 * @page test_events_003 Events timeout
 *
 * <h2>Description</h2>
 * In this test the following APIs are let to timeout twice: immediatly and
 * after 10ms:
 * In this test the following APIs are indipently tested by starting threads
 * that broadcast events after fixed delays:
 * - @p chEvtWaitOneTimeout()
 * - @p chEvtWaitAnyTimeout()
 * - @p chEvtWaitAllTimeout()
 * .
 * After each test phase the test verifies that there are no stuck event flags.
 */

static void evt3_setup(void) {

  chEvtGetAndClearEvents(ALL_EVENTS);
}

static void evt3_execute(void) {
  eventmask_t m;

  /*
   * Tests various timeout situations.
   */
  m = chEvtWaitOneTimeout(ALL_EVENTS, TIME_IMMEDIATE);
  test_assert(1, m == 0, "spurious event");
  m = chEvtWaitAnyTimeout(ALL_EVENTS, TIME_IMMEDIATE);
  test_assert(2, m == 0, "spurious event");
  m = chEvtWaitAllTimeout(ALL_EVENTS, TIME_IMMEDIATE);
  test_assert(3, m == 0, "spurious event");
  m = chEvtWaitOneTimeout(ALL_EVENTS, 10);
  test_assert(4, m == 0, "spurious event");
  m = chEvtWaitAnyTimeout(ALL_EVENTS, 10);
  test_assert(5, m == 0, "spurious event");
  m = chEvtWaitAllTimeout(ALL_EVENTS, 10);
  test_assert(6, m == 0, "spurious event");
}

ROMCONST struct testcase testevt3 = {
  "Events, timeouts",
  evt3_setup,
  NULL,
  evt3_execute
};
#endif /* CH_USE_EVENTS_TIMEOUT */

/**
 * @brief   Test sequence for events.
 */
ROMCONST struct testcase * ROMCONST patternevt[] = {
#if CH_USE_EVENTS || defined(__DOXYGEN__)
  &testevt1,
  &testevt2,
#if CH_USE_EVENTS_TIMEOUT || defined(__DOXYGEN__)
  &testevt3,
#endif
#endif
  NULL
};

#endif /* CH_USE_EVENTS */
