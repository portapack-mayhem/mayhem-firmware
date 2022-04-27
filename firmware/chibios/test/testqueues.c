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
 * @page test_queues I/O Queues test
 *
 * File: @ref testqueues.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref io_queues subsystem.
 * The tests are performed by inserting and removing data from queues and by
 * checking both the queues status and the correct sequence of the extracted
 * data.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the @ref io_queues code.<br>
 * Note that the @ref io_queues subsystem depends on the @ref semaphores
 * subsystem that has to met its testing objectives as well.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_QUEUES (and dependent options)
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_queues_001
 * - @subpage test_queues_002
 * .
 * @file testqueues.c
 * @brief I/O Queues test source file
 * @file testqueues.h
 * @brief I/O Queues test header file
 */

#if CH_USE_QUEUES || defined(__DOXYGEN__)

#define TEST_QUEUES_SIZE 4

static void notify(GenericQueue *qp) {
  (void)qp;
}

/*
 * Note, the static initializers are not really required because the
 * variables are explicitly initialized in each test case. It is done in order
 * to test the macros.
 */
static INPUTQUEUE_DECL(iq, test.wa.T0, TEST_QUEUES_SIZE, notify, NULL);
static OUTPUTQUEUE_DECL(oq, test.wa.T1, TEST_QUEUES_SIZE, notify, NULL);

/**
 * @page test_queues_001 Input Queues functionality and APIs
 *
 * <h2>Description</h2>
 * This test case tests sysnchronos and asynchronous operations on an
 * @p InputQueue object including timeouts. The queue state must remain
 * consistent through the whole test.
 */

static void queues1_setup(void) {

  chIQInit(&iq, wa[0], TEST_QUEUES_SIZE, notify, NULL);
}

static msg_t thread1(void *p) {

  (void)p;
  chIQGetTimeout(&iq, MS2ST(200));
  return 0;
}

static void queues1_execute(void) {
  unsigned i;
  size_t n;

  /* Initial empty state */
  test_assert_lock(1, chIQIsEmptyI(&iq), "not empty");

  /* Queue filling */
  chSysLock();
  for (i = 0; i < TEST_QUEUES_SIZE; i++)
    chIQPutI(&iq, 'A' + i);
  chSysUnlock();
  test_assert_lock(2, chIQIsFullI(&iq), "still has space");
  test_assert_lock(3, chIQPutI(&iq, 0) == Q_FULL, "failed to report Q_FULL");

  /* Queue emptying */
  for (i = 0; i < TEST_QUEUES_SIZE; i++)
    test_emit_token(chIQGet(&iq));
  test_assert_lock(4, chIQIsEmptyI(&iq), "still full");
  test_assert_sequence(5, "ABCD");

  /* Queue filling again */
  chSysLock();
  for (i = 0; i < TEST_QUEUES_SIZE; i++)
    chIQPutI(&iq, 'A' + i);
  chSysUnlock();

  /* Reading the whole thing */
  n = chIQReadTimeout(&iq, wa[1], TEST_QUEUES_SIZE * 2, TIME_IMMEDIATE);
  test_assert(6, n == TEST_QUEUES_SIZE, "wrong returned size");
  test_assert_lock(7, chIQIsEmptyI(&iq), "still full");

  /* Queue filling again */
  chSysLock();
  for (i = 0; i < TEST_QUEUES_SIZE; i++)
    chIQPutI(&iq, 'A' + i);
  chSysUnlock();

  /* Partial reads */
  n = chIQReadTimeout(&iq, wa[1], TEST_QUEUES_SIZE / 2, TIME_IMMEDIATE);
  test_assert(8, n == TEST_QUEUES_SIZE / 2, "wrong returned size");
  n = chIQReadTimeout(&iq, wa[1], TEST_QUEUES_SIZE / 2, TIME_IMMEDIATE);
  test_assert(9, n == TEST_QUEUES_SIZE / 2, "wrong returned size");
  test_assert_lock(10, chIQIsEmptyI(&iq), "still full");

  /* Testing reset */
  chSysLock();
  chIQPutI(&iq, 0);
  chIQResetI(&iq);
  chSysUnlock();
  test_assert_lock(11, chIQGetFullI(&iq) == 0, "still full");
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread1, NULL);
  test_assert_lock(12, chIQGetFullI(&iq) == 0, "not empty");
  test_wait_threads();

  /* Timeout */
  test_assert(13, chIQGetTimeout(&iq, 10) == Q_TIMEOUT, "wrong timeout return");
}

ROMCONST struct testcase testqueues1 = {
  "Queues, input queues",
  queues1_setup,
  NULL,
  queues1_execute
};

/**
 * @page test_queues_002 Output Queues functionality and APIs
 *
 * <h2>Description</h2>
 * This test case tests sysnchronos and asynchronous operations on an
 * @p OutputQueue object including timeouts. The queue state must remain
 * consistent through the whole test.
 */

static void queues2_setup(void) {

  chOQInit(&oq, wa[0], TEST_QUEUES_SIZE, notify, NULL);
}

static msg_t thread2(void *p) {

  (void)p;
  chOQPutTimeout(&oq, 0, MS2ST(200));
  return 0;
}

static void queues2_execute(void) {
  unsigned i;
  size_t n;

  /* Initial empty state */
  test_assert_lock(1, chOQIsEmptyI(&oq), "not empty");

  /* Queue filling */
  for (i = 0; i < TEST_QUEUES_SIZE; i++)
    chOQPut(&oq, 'A' + i);
  test_assert_lock(2, chOQIsFullI(&oq), "still has space");

  /* Queue emptying */
  for (i = 0; i < TEST_QUEUES_SIZE; i++) {
    char c;

    chSysLock();
    c = chOQGetI(&oq);
    chSysUnlock();
    test_emit_token(c);
  }
  test_assert_lock(3, chOQIsEmptyI(&oq), "still full");
  test_assert_sequence(4, "ABCD");
  test_assert_lock(5, chOQGetI(&oq) == Q_EMPTY, "failed to report Q_EMPTY");

  /* Writing the whole thing */
  n = chOQWriteTimeout(&oq, wa[1], TEST_QUEUES_SIZE * 2, TIME_IMMEDIATE);
  test_assert(6, n == TEST_QUEUES_SIZE, "wrong returned size");
  test_assert_lock(7, chOQIsFullI(&oq), "not full");
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread2, NULL);
  test_assert_lock(8, chOQGetFullI(&oq) == TEST_QUEUES_SIZE, "not empty");
  test_wait_threads();

  /* Testing reset */
  chSysLock();
  chOQResetI(&oq);
  chSysUnlock();
  test_assert_lock(9, chOQGetFullI(&oq) == 0, "still full");

  /* Partial writes */
  n = chOQWriteTimeout(&oq, wa[1], TEST_QUEUES_SIZE / 2, TIME_IMMEDIATE);
  test_assert(10, n == TEST_QUEUES_SIZE / 2, "wrong returned size");
  n = chOQWriteTimeout(&oq, wa[1], TEST_QUEUES_SIZE / 2, TIME_IMMEDIATE);
  test_assert(11, n == TEST_QUEUES_SIZE / 2, "wrong returned size");
  test_assert_lock(12, chOQIsFullI(&oq), "not full");

  /* Timeout */
  test_assert(13, chOQPutTimeout(&oq, 0, 10) == Q_TIMEOUT, "wrong timeout return");
}

ROMCONST struct testcase testqueues2 = {
  "Queues, output queues",
  queues2_setup,
  NULL,
  queues2_execute
};
#endif /* CH_USE_QUEUES */

/**
 * @brief   Test sequence for queues.
 */
ROMCONST struct testcase * ROMCONST patternqueues[] = {
#if CH_USE_QUEUES || defined(__DOXYGEN__)
  &testqueues1,
  &testqueues2,
#endif
  NULL
};
