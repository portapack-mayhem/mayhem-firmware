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
 * @page test_mbox Mailboxes test
 *
 * File: @ref testmbox.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref mailboxes subsystem.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the @ref mailboxes
 * subsystem code.<br>
 * Note that the @ref mailboxes subsystem depends on the @ref semaphores
 * subsystem that has to met its testing objectives as well.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_MAILBOXES
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_mbox_001
 * .
 * @file testmbox.c
 * @brief Mailboxes test source file
 * @file testmbox.h
 * @brief Mailboxes header file
 */

#if CH_USE_MAILBOXES || defined(__DOXYGEN__)

#define ALLOWED_DELAY MS2ST(5)
#define MB_SIZE 5

/*
 * Note, the static initializers are not really required because the
 * variables are explicitly initialized in each test case. It is done in order
 * to test the macros.
 */
static MAILBOX_DECL(mb1, test.wa.T0, MB_SIZE);

/**
 * @page test_mbox_001 Queuing and timeouts
 *
 * <h2>Description</h2>
 * Messages are posted/fetched from a mailbox in carefully designed sequences
 * in order to stimulate all the possible code paths inside the mailbox.<br>
 * The test expects to find a consistent mailbox status after each operation.
 */

static void mbox1_setup(void) {

  chMBInit(&mb1, (msg_t *)test.wa.T0, MB_SIZE);
}

static void mbox1_execute(void) {
  msg_t msg1, msg2;
  unsigned i;

  /*
   * Testing initial space.
   */
  test_assert(1, chMBGetFreeCountI(&mb1) == MB_SIZE, "wrong size");

  /*
   * Testing enqueuing and backward circularity.
   */
  for (i = 0; i < MB_SIZE - 1; i++) {
    msg1 = chMBPost(&mb1, 'B' + i, TIME_INFINITE);
    test_assert(2, msg1 == RDY_OK, "wrong wake-up message");
  }
  msg1 = chMBPostAhead(&mb1, 'A', TIME_INFINITE);
  test_assert(3, msg1 == RDY_OK, "wrong wake-up message");

  /*
   * Testing post timeout.
   */
  msg1 = chMBPost(&mb1, 'X', 1);
  test_assert(4, msg1 == RDY_TIMEOUT, "wrong wake-up message");
  chSysLock();
  msg1 = chMBPostI(&mb1, 'X');
  chSysUnlock();
  test_assert(5, msg1 == RDY_TIMEOUT, "wrong wake-up message");
  msg1 = chMBPostAhead(&mb1, 'X', 1);
  test_assert(6, msg1 == RDY_TIMEOUT, "wrong wake-up message");
  chSysLock();
  msg1 = chMBPostAheadI(&mb1, 'X');
  chSysUnlock();
  test_assert(7, msg1 == RDY_TIMEOUT, "wrong wake-up message");

  /*
   * Testing final conditions.
   */
  test_assert_lock(8, chMBGetFreeCountI(&mb1) == 0, "still empty");
  test_assert_lock(9, chMBGetUsedCountI(&mb1) == MB_SIZE, "not full");
  test_assert_lock(10, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");

  /*
   * Testing dequeuing.
   */
  for (i = 0; i < MB_SIZE; i++) {
    msg1 = chMBFetch(&mb1, &msg2, TIME_INFINITE);
    test_assert(11, msg1 == RDY_OK, "wrong wake-up message");
    test_emit_token(msg2);
  }
  test_assert_sequence(12, "ABCDE");

  /*
   * Testing buffer circularity.
   */
  msg1 = chMBPost(&mb1, 'B' + i, TIME_INFINITE);
  test_assert(13, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBFetch(&mb1, &msg2, TIME_INFINITE);
  test_assert(14, msg1 == RDY_OK, "wrong wake-up message");
  test_assert(15, mb1.mb_buffer == mb1.mb_wrptr, "write pointer not aligned to base");
  test_assert(16, mb1.mb_buffer == mb1.mb_rdptr, "read pointer not aligned to base");

  /*
   * Testing fetch timeout.
   */
  msg1 = chMBFetch(&mb1, &msg2, 1);
  test_assert(17, msg1 == RDY_TIMEOUT, "wrong wake-up message");
  chSysLock();
  msg1 = chMBFetchI(&mb1, &msg2);
  chSysUnlock();
  test_assert(18, msg1 == RDY_TIMEOUT, "wrong wake-up message");

  /*
   * Testing final conditions.
   */
  test_assert_lock(19, chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
  test_assert_lock(20, chMBGetUsedCountI(&mb1) == 0, "still full");
  test_assert_lock(21, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");

  /*
   * Testing I-Class.
   */
  chSysLock();
  msg1 = chMBPostI(&mb1, 'A');
  test_assert(22, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostI(&mb1, 'B');
  test_assert(23, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostI(&mb1, 'C');
  test_assert(24, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostI(&mb1, 'D');
  test_assert(25, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostI(&mb1, 'E');
  chSysUnlock();
  test_assert(26, msg1 == RDY_OK, "wrong wake-up message");
  test_assert(27, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");
  for (i = 0; i < MB_SIZE; i++) {
    chSysLock();
    msg1 = chMBFetchI(&mb1, &msg2);
    chSysUnlock();
    test_assert(28, msg1 == RDY_OK, "wrong wake-up message");
    test_emit_token(msg2);
  }
  test_assert_sequence(29, "ABCDE");
  test_assert_lock(30, chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
  test_assert_lock(31, chMBGetUsedCountI(&mb1) == 0, "still full");
  test_assert(32, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");

  chSysLock();
  msg1 = chMBPostAheadI(&mb1, 'E');
  test_assert(33, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostAheadI(&mb1, 'D');
  test_assert(34, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostAheadI(&mb1, 'C');
  test_assert(35, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostAheadI(&mb1, 'B');
  test_assert(36, msg1 == RDY_OK, "wrong wake-up message");
  msg1 = chMBPostAheadI(&mb1, 'A');
  chSysUnlock();
  test_assert(37, msg1 == RDY_OK, "wrong wake-up message");
  test_assert(38, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");
  for (i = 0; i < MB_SIZE; i++) {
    chSysLock();
    msg1 = chMBFetchI(&mb1, &msg2);
    chSysUnlock();
    test_assert(39, msg1 == RDY_OK, "wrong wake-up message");
    test_emit_token(msg2);
  }
  test_assert_sequence(40, "ABCDE");
  test_assert_lock(41, chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
  test_assert_lock(42, chMBGetUsedCountI(&mb1) == 0, "still full");
  test_assert(43, mb1.mb_rdptr == mb1.mb_wrptr, "pointers not aligned");

  /*
   * Testing reset.
   */
  chMBReset(&mb1);

  /*
   * Re-testing final conditions.
   */
  test_assert_lock(44, chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
  test_assert_lock(45, chMBGetUsedCountI(&mb1) == 0, "still full");
  test_assert_lock(46, mb1.mb_buffer == mb1.mb_wrptr, "write pointer not aligned to base");
  test_assert_lock(47, mb1.mb_buffer == mb1.mb_rdptr, "read pointer not aligned to base");
}

ROMCONST struct testcase testmbox1 = {
  "Mailboxes, queuing and timeouts",
  mbox1_setup,
  NULL,
  mbox1_execute
};

#endif /* CH_USE_MAILBOXES */

/**
 * @brief   Test sequence for mailboxes.
 */
ROMCONST struct testcase * ROMCONST patternmbox[] = {
#if CH_USE_MAILBOXES || defined(__DOXYGEN__)
  &testmbox1,
#endif
  NULL
};
