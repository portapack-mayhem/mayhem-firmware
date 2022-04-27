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
 * @page test_mtx Mutexes test
 *
 * File: @ref testmtx.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref mutexes and
 * @ref condvars subsystems.<br>
 * Tests on those subsystems are particularly critical because the system-wide
 * implications of the Priority Inheritance mechanism.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the subsystems code.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_MUTEXES
 * - @p CH_USE_CONDVARS
 * - @p CH_DBG_THREADS_PROFILING
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_mtx_001
 * - @subpage test_mtx_002
 * - @subpage test_mtx_003
 * - @subpage test_mtx_004
 * - @subpage test_mtx_005
 * - @subpage test_mtx_006
 * - @subpage test_mtx_007
 * - @subpage test_mtx_008
 * .
 * @file testmtx.c
 * @brief Mutexes and CondVars test source file
 * @file testmtx.h
 * @brief Mutexes and CondVars test header file
 */

#if CH_USE_MUTEXES || defined(__DOXYGEN__)

#define ALLOWED_DELAY 5

/*
 * Note, the static initializers are not really required because the
 * variables are explicitly initialized in each test case. It is done in order
 * to test the macros.
 */
static MUTEX_DECL(m1);
static MUTEX_DECL(m2);
#if CH_USE_CONDVARS || defined(__DOXYGEN__)
static CONDVAR_DECL(c1);
#endif

/**
 * @page test_mtx_001 Priority enqueuing test
 *
 * <h2>Description</h2>
 * Five threads, with increasing priority, are enqueued on a locked mutex then
 * the mutex is unlocked.<br>
 * The test expects the threads to perform their operations in increasing
 * priority order regardless of the initial order.
 */

static void mtx1_setup(void) {

  chMtxInit(&m1);
}

static msg_t thread1(void *p) {

  chMtxLock(&m1);
  test_emit_token(*(char *)p);
  chMtxUnlock();
  return 0;
}

static void mtx1_execute(void) {

  tprio_t prio = chThdGetPriority(); /* Because priority inheritance.*/
  chMtxLock(&m1);
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, prio+1, thread1, "E");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, prio+2, thread1, "D");
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, prio+3, thread1, "C");
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, prio+4, thread1, "B");
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, prio+5, thread1, "A");
  chMtxUnlock();
  test_wait_threads();
  test_assert(1, prio == chThdGetPriority(), "wrong priority level");
  test_assert_sequence(2, "ABCDE");
}

ROMCONST struct testcase testmtx1 = {
  "Mutexes, priority enqueuing test",
  mtx1_setup,
  NULL,
  mtx1_execute
};

#if CH_DBG_THREADS_PROFILING || defined(__DOXYGEN__)
/**
 * @page test_mtx_002 Priority inheritance, simple case
 *
 * <h2>Description</h2>
 * Three threads are involved in the classic priority inversion scenario, a
 * medium priority thread tries to starve an high priority thread by
 * blocking a low priority thread into a mutex lock zone.<br>
 * The test expects the threads to reach their goal in increasing priority
 * order by rearranging their priorities in order to avoid the priority
 * inversion trap.
 *
 * <h2>Scenario</h2>
 * This weird looking diagram should explain what happens in the test case:
 * @code
 * Time ----> 0     10    20    30    40    50    60    70    80    90    100
 *    0 ......AL++++++++++............2+++++++++++AU0---------------++++++G...
 *    1 ..................++++++++++++------------------++++++++++++G.........
 *    2  .............................AL..........++++++AUG...................
 *                                    ^           ^
 * Legend:
 *   0..2 - Priority levels
 *   +++  - Running
 *   ---  - Ready
 *   ...  - Waiting or Terminated
 *   xL   - Lock operation on mutex 'x'
 *   xUn  - Unlock operation on mutex 'x' with priority returning to level 'n'
 *   G    - Goal
 *   ^    - Priority transition (boost or return).
 * @endcode
 */

static void mtx2_setup(void) {

  chMtxInit(&m1);
}

/* Low priority thread */
static msg_t thread2L(void *p) {

  (void)p;
  chMtxLock(&m1);
  test_cpu_pulse(40);
  chMtxUnlock();
  test_cpu_pulse(10);
  test_emit_token('C');
  return 0;
}

/* Medium priority thread */
static msg_t thread2M(void *p) {

  (void)p;
  chThdSleepMilliseconds(20);
  test_cpu_pulse(40);
  test_emit_token('B');
  return 0;
}

/* High priority thread */
static msg_t thread2H(void *p) {

  (void)p;
  chThdSleepMilliseconds(40);
  chMtxLock(&m1);
  test_cpu_pulse(10);
  chMtxUnlock();
  test_emit_token('A');
  return 0;
}

static void mtx2_execute(void) {
  systime_t time;

  test_wait_tick();
  time = chTimeNow();
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()-1, thread2H, 0);
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()-2, thread2M, 0);
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()-3, thread2L, 0);
  test_wait_threads();
  test_assert_sequence(1, "ABC");
  test_assert_time_window(2, time + MS2ST(100), time + MS2ST(100) + ALLOWED_DELAY);
}

ROMCONST struct testcase testmtx2 = {
  "Mutexes, priority inheritance, simple case",
  mtx2_setup,
  NULL,
  mtx2_execute
};

/**
 * @page test_mtx_003 Priority inheritance, complex case
 *
 * <h2>Description</h2>
 * Five threads are involved in the complex priority inversion scenario,
 * please refer to the diagram below for the complete scenario.<br>
 * The test expects the threads to perform their operations in increasing
 * priority order by rearranging their priorities in order to avoid the
 * priority inversion trap.
 *
 * <h2>Scenario</h2>
 * This weird looking diagram should explain what happens in the test case:
 * @code
 * Time ----> 0     10    20    30    40    50    60    70    80    90    100   110
 *    0 ......BL++++------------2+++++------4+++++BU0---------------------------G.....
 *    1 ............AL++++2+++++BL----------4-----++++++BU4+++AU1---------------G.....
 *    2 ..................AL----------------------------------------------++++++AUG...
 *    3 ..............................+++++++-----------------------++++++G...........
 *    4 ....................................AL................++++++AUG...............
 *                        ^     ^           ^     ^     ^     ^
 * Legend:
 *   0..4 - Priority levels
 *   +++  - Running
 *   ---  - Ready
 *   ...  - Waiting or Terminated
 *   xL   - Lock operation on mutex 'x'
 *   xUn  - Unlock operation on mutex 'x' with priority returning to level 'n'
 *   ^    - Priority transition (boost or return).
 * @endcode
 */

static void mtx3_setup(void) {

  chMtxInit(&m1); /* Mutex B.*/
  chMtxInit(&m2); /* Mutex A.*/
}

/* Lowest priority thread */
static msg_t thread3LL(void *p) {

  (void)p;
  chMtxLock(&m1);
  test_cpu_pulse(30);
  chMtxUnlock();
  test_emit_token('E');
  return 0;
}

/* Low priority thread */
static msg_t thread3L(void *p) {

  (void)p;
  chThdSleepMilliseconds(10);
  chMtxLock(&m2);
  test_cpu_pulse(20);
  chMtxLock(&m1);
  test_cpu_pulse(10);
  chMtxUnlock();
  test_cpu_pulse(10);
  chMtxUnlock();
  test_emit_token('D');
  return 0;
}

/* Medium priority thread */
static msg_t thread3M(void *p) {

  (void)p;
  chThdSleepMilliseconds(20);
  chMtxLock(&m2);
  test_cpu_pulse(10);
  chMtxUnlock();
  test_emit_token('C');
  return 0;
}

/* High priority thread */
static msg_t thread3H(void *p) {

  (void)p;
  chThdSleepMilliseconds(40);
  test_cpu_pulse(20);
  test_emit_token('B');
  return 0;
}

/* Highest priority thread */
static msg_t thread3HH(void *p) {

  (void)p;
  chThdSleepMilliseconds(50);
  chMtxLock(&m2);
  test_cpu_pulse(10);
  chMtxUnlock();
  test_emit_token('A');
  return 0;
}

static void mtx3_execute(void) {
  systime_t time;

  test_wait_tick();
  time = chTimeNow();
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()-5, thread3LL, 0);
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()-4, thread3L, 0);
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()-3, thread3M, 0);
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriority()-2, thread3H, 0);
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriority()-1, thread3HH, 0);
  test_wait_threads();
  test_assert_sequence(1, "ABCDE");
  test_assert_time_window(2, time + MS2ST(110), time + MS2ST(110) + ALLOWED_DELAY);
}

ROMCONST struct testcase testmtx3 = {
  "Mutexes, priority inheritance, complex case",
  mtx3_setup,
  NULL,
  mtx3_execute
};
#endif /* CH_DBG_THREADS_PROFILING */

/**
 * @page test_mtx_004 Priority return verification
 *
 * <h2>Description</h2>
 * Two threads are spawned that try to lock the mutexes locked by the tester
 * thread with precise timing.<br>
 * The test expects that the priority changes caused by the priority
 * inheritance algorithm happen at the right moment and with the right values.
 */

static void mtx4_setup(void) {

  chMtxInit(&m1);
  chMtxInit(&m2);
}

static msg_t thread4a(void *p) {

  (void)p;
  chThdSleepMilliseconds(50);
  chMtxLock(&m2);
  chMtxUnlock();
  return 0;
}

static msg_t thread4b(void *p) {

  (void)p;
  chThdSleepMilliseconds(150);
  chMtxLock(&m1);
  chMtxUnlock();
  return 0;
}

static void mtx4_execute(void) {
  tprio_t p, p1, p2;

  p = chThdGetPriority();
  p1 = p + 1;
  p2 = p + 2;
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, p1, thread4a, "B");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, p2, thread4b, "A");
  chMtxLock(&m2);
  test_assert(1, chThdGetPriority() == p, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(2, chThdGetPriority() == p1, "wrong priority level");
  chMtxLock(&m1);
  test_assert(3, chThdGetPriority() == p1, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(4, chThdGetPriority() == p2, "wrong priority level");
  chMtxUnlock();
  test_assert(5, chThdGetPriority() == p1, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(6, chThdGetPriority() == p1, "wrong priority level");
  chMtxUnlockAll();
  test_assert(7, chThdGetPriority() == p, "wrong priority level");
  test_wait_threads();

  /* Test repeated in order to cover chMtxUnlockS().*/
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, p1, thread4a, "D");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, p2, thread4b, "C");
  chMtxLock(&m2);
  test_assert(8, chThdGetPriority() == p, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(9, chThdGetPriority() == p1, "wrong priority level");
  chMtxLock(&m1);
  test_assert(10, chThdGetPriority() == p1, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(11, chThdGetPriority() == p2, "wrong priority level");
  chSysLock();
  chMtxUnlockS();
  chSchRescheduleS();
  chSysUnlock();
  test_assert(12, chThdGetPriority() == p1, "wrong priority level");
  chThdSleepMilliseconds(100);
  test_assert(13, chThdGetPriority() == p1, "wrong priority level");
  chMtxUnlockAll();
  test_assert(14, chThdGetPriority() == p, "wrong priority level");
  test_wait_threads();
}

ROMCONST struct testcase testmtx4 = {
  "Mutexes, priority return",
  mtx4_setup,
  NULL,
  mtx4_execute
};

/**
 * @page test_mtx_005 Mutex status
 *
 * <h2>Description</h2>
 * Various tests on the mutex structure status after performing some lock and
 * unlock operations.<br>
 * The test expects that the internal mutex status is consistent after each
 * operation.
 */

static void mtx5_setup(void) {

  chMtxInit(&m1);
}

static void mtx5_execute(void) {
  bool_t b;
  tprio_t prio;

  prio = chThdGetPriority();

  b = chMtxTryLock(&m1);
  test_assert(1, b, "already locked");

  b = chMtxTryLock(&m1);
  test_assert(2, !b, "not locked");

  chSysLock();
  chMtxUnlockS();
  chSysUnlock();

  test_assert(3, isempty(&m1.m_queue), "queue not empty");
  test_assert(4, m1.m_owner == NULL, "still owned");
  test_assert(5, chThdGetPriority() == prio, "wrong priority level");
  
  chMtxLock(&m1);
  chMtxUnlockAll();
  test_assert(6, isempty(&m1.m_queue), "queue not empty");
  test_assert(7, m1.m_owner == NULL, "still owned");
}

ROMCONST struct testcase testmtx5 = {
  "Mutexes, status",
  mtx5_setup,
  NULL,
  mtx5_execute
};

#if CH_USE_CONDVARS || defined(__DOXYGEN__)
/**
 * @page test_mtx_006 Condition Variable signal test
 *
 * <h2>Description</h2>
 * Five threads take a mutex and then enter a conditional variable queue, the
 * tester thread then proceeds to signal the conditional variable five times
 * atomically.<br>
 * The test expects the threads to reach their goal in increasing priority
 * order regardless of the initial order.
 */

static void mtx6_setup(void) {

  chCondInit(&c1);
  chMtxInit(&m1);
}

static msg_t thread10(void *p) {

  chMtxLock(&m1);
  chCondWait(&c1);
  test_emit_token(*(char *)p);
  chMtxUnlock();
  return 0;
}

static void mtx6_execute(void) {

  tprio_t prio = chThdGetPriority();
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, prio+1, thread10, "E");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, prio+2, thread10, "D");
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, prio+3, thread10, "C");
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, prio+4, thread10, "B");
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, prio+5, thread10, "A");
  chSysLock();
  chCondSignalI(&c1);
  chCondSignalI(&c1);
  chCondSignalI(&c1);
  chCondSignalI(&c1);
  chCondSignalI(&c1);
  chSchRescheduleS();
  chSysUnlock();
  test_wait_threads();
  test_assert_sequence(1, "ABCDE");
}

ROMCONST struct testcase testmtx6 = {
  "CondVar, signal test",
  mtx6_setup,
  NULL,
  mtx6_execute
};

/**
 * @page test_mtx_007 Condition Variable broadcast test
 *
 * <h2>Description</h2>
 * Five threads take a mutex and then enter a conditional variable queue, the
 * tester thread then proceeds to broadcast the conditional variable.<br>
 * The test expects the threads to reach their goal in increasing priority
 * order regardless of the initial order.
 */

static void mtx7_setup(void) {

  chCondInit(&c1);
  chMtxInit(&m1);
}

static void mtx7_execute(void) {

  tprio_t prio = chThdGetPriority();
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, prio+1, thread10, "E");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, prio+2, thread10, "D");
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, prio+3, thread10, "C");
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, prio+4, thread10, "B");
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, prio+5, thread10, "A");
  chCondBroadcast(&c1);
  test_wait_threads();
  test_assert_sequence(1, "ABCDE");
}

ROMCONST struct testcase testmtx7 = {
  "CondVar, broadcast test",
  mtx7_setup,
  NULL,
  mtx7_execute
};

/**
 * @page test_mtx_008 Condition Variable priority boost test
 *
 * <h2>Description</h2>
 * This test case verifies the priority boost of a thread waiting on a
 * conditional variable queue. It tests this very specific situation in order
 * to complete the code coverage.
 */

static void mtx8_setup(void) {

  chCondInit(&c1);
  chMtxInit(&m1);
  chMtxInit(&m2);
}

static msg_t thread11(void *p) {

  chMtxLock(&m2);
  chMtxLock(&m1);
#if CH_USE_CONDVARS_TIMEOUT || defined(__DOXYGEN__)
  chCondWaitTimeout(&c1, TIME_INFINITE);
#else
  chCondWait(&c1);
#endif
  test_emit_token(*(char *)p);
  chMtxUnlock();
  chMtxUnlock();
  return 0;
}

static msg_t thread12(void *p) {

  chMtxLock(&m2);
  test_emit_token(*(char *)p);
  chMtxUnlock();
  return 0;
}

static void mtx8_execute(void) {

  tprio_t prio = chThdGetPriority();
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, prio+1, thread11, "A");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, prio+2, thread10, "C");
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, prio+3, thread12, "B");
  chCondSignal(&c1);
  chCondSignal(&c1);
  test_wait_threads();
  test_assert_sequence(1, "ABC");
}

ROMCONST struct testcase testmtx8 = {
  "CondVar, boost test",
  mtx8_setup,
  NULL,
  mtx8_execute
};
#endif /* CH_USE_CONDVARS */
#endif /* CH_USE_MUTEXES */

/**
 * @brief   Test sequence for mutexes.
 */
ROMCONST struct testcase * ROMCONST patternmtx[] = {
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  &testmtx1,
#if CH_DBG_THREADS_PROFILING || defined(__DOXYGEN__)
  &testmtx2,
  &testmtx3,
#endif
  &testmtx4,
  &testmtx5,
#if CH_USE_CONDVARS || defined(__DOXYGEN__)
  &testmtx6,
  &testmtx7,
  &testmtx8,
#endif
#endif
  NULL
};
