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
 * @page test_msg Messages test
 *
 * File: @ref testmsg.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the @ref messages subsystem.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to cover 100% of the @ref messages
 * subsystem code.
 *
 * <h2>Preconditions</h2>
 * The module requires the following kernel options:
 * - @p CH_USE_MESSAGES
 * .
 * In case some of the required options are not enabled then some or all tests
 * may be skipped.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_msg_001
 * .
 * @file testmsg.c
 * @brief Messages test source file
 * @file testmsg.h
 * @brief Messages header file
 */

#if CH_USE_MESSAGES || defined(__DOXYGEN__)

/**
 * @page test_msg_001 Messages Server loop
 *
 * <h2>Description</h2>
 * A thread is spawned that sends four messages back to the tester thread.<br>
 * The test expect to receive the messages in the correct sequence and to
 * not find a fifth message waiting.
 */

static msg_t thread(void *p) {

  chMsgSend(p, 'A');
  chMsgSend(p, 'B');
  chMsgSend(p, 'C');
  return 0;
}

static void msg1_execute(void) {
  Thread *tp;
  msg_t msg;

  /*
   * Testing the whole messages loop.
   */
  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority() + 1,
                                 thread, chThdSelf());
  tp = chMsgWait();
  msg = chMsgGet(tp);
  chMsgRelease(tp, msg);
  test_emit_token(msg);
  tp = chMsgWait();
  msg = chMsgGet(tp);
  chMsgRelease(tp, msg);
  test_emit_token(msg);
  tp = chMsgWait();
  msg = chMsgGet(tp);
  chMsgRelease(tp, msg);
  test_emit_token(msg);
  test_assert_sequence(1, "ABC");
}

ROMCONST struct testcase testmsg1 = {
  "Messages, loop",
  NULL,
  NULL,
  msg1_execute
};

#endif /* CH_USE_MESSAGES */

/**
 * @brief   Test sequence for messages.
 */
ROMCONST struct testcase * ROMCONST patternmsg[] = {
#if CH_USE_MESSAGES || defined(__DOXYGEN__)
  &testmsg1,
#endif
  NULL
};
