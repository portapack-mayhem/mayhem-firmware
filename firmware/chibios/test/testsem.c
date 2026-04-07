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
 * @page test_sem Semaphores test
 *
 * File: @ref testsem.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref semaphores subsystem.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the @ref semaphores code.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_SEMAPHORES
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_sem_001
 * - @subpage test_sem_002
 * - @subpage test_sem_003
 * - @subpage test_sem_004
 * .
 * @file testsem.c
 * @brief Semaphores test source file
 * @file testsem.h
 * @brief Semaphores test header file
 */

#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)

#define ALLOWED_DELAY MS2ST(5)

/*
 * Note, the static initializers are not really required because the
 * variables are explicitly initialized in each test case. It is done in order
 * to test the macros.
 */
static SEMAPHORE_DECL(sem1, 0);

/**
 * @page test_sem_001 Enqueuing test
 *
 * <h2>Description</h2>
 * Five threads with randomized priorities are enqueued to a semaphore then
 * awakened one at time.<br>
 * The test expects that the threads reach their goal in FIFO order or
 * priority order depending on the CH_USE_SEMAPHORES_PRIORITY configuration
 * setting.
 */

static void sem1_setup(void) {

  chSemInit(&sem1, 0);
}

static msg_t thread1(void *p) {

  chSemWait(&sem1);
  test_emit_token(*(char *)p);
  return 0;
}

static void sem1_execute(void) {

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+5, thread1, "A");
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()+1, thread1, "B");
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()+3, thread1, "C");
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriority()+4, thread1, "D");
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriority()+2, thread1, "E");
  chSemSignal(&sem1);
  chSemSignal(&sem1);
  chSemSignal(&sem1);
  chSemSignal(&sem1);
  chSemSignal(&sem1);
  test_wait_threads();
#if CH_USE_SEMAPHORES_PRIORITY
  test_assert_sequence(1, "ADCEB");
#else
  test_assert_sequence(1, "ABCDE");
#endif
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+5, thread1, "A");
  chSysLock();
  chSemAddCounterI(&sem1, 2);
  chSchRescheduleS();
  chSysUnlock();
  test_wait_threads();
  test_assert(2, chSemGetCounterI(&sem1) == 1, "invalid counter");
}

ROMCONST struct testcase testsem1 = {
  "Semaphores, enqueuing",
  sem1_setup,
  NULL,
  sem1_execute
};

/**
 * @page test_sem_002 Timeout test
 *
 * <h2>Description</h2>
 * The three possible semaphore waiting modes (do not wait, wait with timeout,
 * wait without timeout) are explored.<br>
 * The test expects that the semaphore wait function returns the correct value
 * in each of the above scenario and that the semaphore structure status is
 * correct after each operation.
 */

static void sem2_setup(void) {

  chSemInit(&sem1, 0);
}

static msg_t thread2(void *p) {

  (void)p;
  chThdSleepMilliseconds(50);
  chSysLock();
  chSemSignalI(&sem1); /* For coverage reasons */
  chSchRescheduleS();
  chSysUnlock();
  return 0;
}

static void sem2_execute(void) {
  int i;
  systime_t target_time;
  msg_t msg;

  /*
   * Testing special case TIME_IMMEDIATE.
   */
  msg = chSemWaitTimeout(&sem1, TIME_IMMEDIATE);
  test_assert(1, msg == RDY_TIMEOUT, "wrong wake-up message");
  test_assert(2, isempty(&sem1.s_queue), "queue not empty");
  test_assert(3, sem1.s_cnt == 0, "counter not zero");

  /*
   * Testing not timeout condition.
   */
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority() - 1,
                                 thread2, 0);
  msg = chSemWaitTimeout(&sem1, MS2ST(500));
  test_wait_threads();
  test_assert(4, msg == RDY_OK, "wrong wake-up message");
  test_assert(5, isempty(&sem1.s_queue), "queue not empty");
  test_assert(6, sem1.s_cnt == 0, "counter not zero");

  /*
   * Testing timeout condition.
   */
  test_wait_tick();
  target_time = chTimeNow() + MS2ST(5 * 500);
  for (i = 0; i < 5; i++) {
    test_emit_token('A' + i);
    msg = chSemWaitTimeout(&sem1, MS2ST(500));
    test_assert(7, msg == RDY_TIMEOUT, "wrong wake-up message");
    test_assert(8, isempty(&sem1.s_queue), "queue not empty");
    test_assert(9, sem1.s_cnt == 0, "counter not zero");
  }
  test_assert_sequence(10, "ABCDE");
  test_assert_time_window(11, target_time, target_time + ALLOWED_DELAY);
}

ROMCONST struct testcase testsem2 = {
  "Semaphores, timeout",
  sem2_setup,
  NULL,
  sem2_execute
};

#if CH_USE_SEMSW || defined(__DOXYGEN__)
/**
 * @page test_sem_003 Atomic signal-wait test
 *
 * <h2>Description</h2>
 * This test case explicitly addresses the @p chSemWaitSignal() function. A
 * thread is created that performs a wait and a signal operations.
 * The tester thread is awakened from an atomic wait/signal operation.<br>
 * The test expects that the semaphore wait function returns the correct value
 * in each of the above scenario and that the semaphore structure status is
 * correct after each operation.
 */

static void sem3_setup(void) {

  chSemInit(&sem1, 0);
}

static msg_t thread3(void *p) {

  (void)p;
  chSemWait(&sem1);
  chSemSignal(&sem1);
  return 0;
}

static void sem3_execute(void) {

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread3, 0);
  chSemSignalWait(&sem1, &sem1);
  test_assert(1, isempty(&sem1.s_queue), "queue not empty");
  test_assert(2, sem1.s_cnt == 0, "counter not zero");

  chSemSignalWait(&sem1, &sem1);
  test_assert(3, isempty(&sem1.s_queue), "queue not empty");
  test_assert(4, sem1.s_cnt == 0, "counter not zero");
}

ROMCONST struct testcase testsem3 = {
  "Semaphores, atomic signal-wait",
  sem3_setup,
  NULL,
  sem3_execute
};
#endif /* CH_USE_SEMSW */

/**
 * @page test_sem_004 Binary Wait and Signal
 *
 * <h2>Description</h2>
 * This test case tests the binary semaphores functionality. The test both
 * checks the binary semaphore status and the expected status of the underlying
 * counting semaphore.
 */
static msg_t thread4(void *p) {

  chBSemSignal((BinarySemaphore *)p);
  return 0;
}

static void sem4_execute(void) {
  BinarySemaphore bsem;
  
  /* Creates a taken binary semaphore.*/
  chBSemInit(&bsem, TRUE);
  chBSemReset(&bsem, TRUE);
  test_assert(1, chBSemGetStateI(&bsem) == TRUE, "not taken");

  /* Starts a signaler thread at a lower priority.*/
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE,
                                 chThdGetPriority()-1, thread4, &bsem);
                                 
  /* Waits to be signaled.*/
  chBSemWait(&bsem);
  
  /* The binary semaphore is expected to be taken.*/
  test_assert(2, chBSemGetStateI(&bsem) == TRUE, "not taken");
  
  /* Releasing it, check both the binary semaphore state and the underlying
     counter semaphore state..*/
  chBSemSignal(&bsem);
  test_assert(3, chBSemGetStateI(&bsem) == FALSE, "still taken");
  test_assert(4, chSemGetCounterI(&bsem.bs_sem) == 1, "unexpected counter");
  
  /* Checking signaling overflow, the counter must not go beyond 1.*/
  chBSemSignal(&bsem);
  test_assert(3, chBSemGetStateI(&bsem) == FALSE, "taken");
  test_assert(5, chSemGetCounterI(&bsem.bs_sem) == 1, "unexpected counter");
}

ROMCONST struct testcase testsem4 = {
  "Binary Semaphores, functionality",
  NULL,
  NULL,
  sem4_execute
};
#endif /* CH_USE_SEMAPHORES */

/**
 * @brief   Test sequence for semaphores.
 */
ROMCONST struct testcase * ROMCONST patternsem[] = {
#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)
  &testsem1,
  &testsem2,
#if CH_USE_SEMSW || defined(__DOXYGEN__)
  &testsem3,
#endif
  &testsem4,
#endif
  NULL
};
