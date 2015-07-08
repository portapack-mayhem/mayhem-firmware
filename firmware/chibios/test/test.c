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
 * @file    test.c
 * @brief   Tests support code.
 *
 * @addtogroup test
 * @{
 */

#include "ch.h"
#include "hal.h"

#include "test.h"
#include "testthd.h"
#include "testsem.h"
#include "testmtx.h"
#include "testmsg.h"
#include "testmbox.h"
#include "testevt.h"
#include "testheap.h"
#include "testpools.h"
#include "testdyn.h"
#include "testqueues.h"
#include "testbmk.h"

/*
 * Array of all the test patterns.
 */
static ROMCONST struct testcase * ROMCONST *patterns[] = {
  patternthd,
  patternsem,
  patternmtx,
  patternmsg,
  patternmbox,
  patternevt,
  patternheap,
  patternpools,
  patterndyn,
  patternqueues,
  patternbmk,
  NULL
};

static bool_t local_fail, global_fail;
static unsigned failpoint;
static char tokens_buffer[MAX_TOKENS];
static char *tokp;

/*
 * Static working areas, the following areas can be used for threads or
 * used as temporary buffers.
 */
union test_buffers test;

/*
 * Pointers to the spawned threads.
 */
Thread *threads[MAX_THREADS];

/*
 * Pointers to the working areas.
 */
void * ROMCONST wa[5] = {test.wa.T0, test.wa.T1, test.wa.T2,
                         test.wa.T3, test.wa.T4};

/*
 * Console output.
 */
static BaseSequentialStream *chp;

/**
 * @brief   Prints a decimal unsigned number.
 *
 * @param[in] n         the number to be printed
 */
void test_printn(uint32_t n) {
  char buf[16], *p;

  if (!n)
    chSequentialStreamPut(chp, '0');
  else {
    p = buf;
    while (n)
      *p++ = (n % 10) + '0', n /= 10;
    while (p > buf)
      chSequentialStreamPut(chp, *--p);
  }
}

/**
 * @brief   Prints a line without final end-of-line.
 *
 * @param[in] msgp      the message
 */
void test_print(const char *msgp) {

  while (*msgp)
    chSequentialStreamPut(chp, *msgp++);
}

/**
 * @brief   Prints a line.
 *
 * @param[in] msgp      the message
 */
void test_println(const char *msgp) {

  test_print(msgp);
  chSequentialStreamWrite(chp, (const uint8_t *)"\r\n", 2);
}

/*
 * Tokens.
 */
static void clear_tokens(void) {

  tokp = tokens_buffer;
}

static void print_tokens(void) {
  char *cp = tokens_buffer;

  while (cp < tokp)
    chSequentialStreamPut(chp, *cp++);
}

/**
 * @brief   Emits a token into the tokens buffer.
 *
 * @param[in] token     the token as a char
 */
void test_emit_token(char token) {

  chSysLock();
  *tokp++ = token;
  chSysUnlock();
}

/*
 * Assertions.
 */
bool_t _test_fail(unsigned point) {

  local_fail = TRUE;
  global_fail = TRUE;
  failpoint = point;
  return TRUE;
}

bool_t _test_assert(unsigned point, bool_t condition) {

  if (!condition)
    return _test_fail(point);
  return FALSE;
}

bool_t _test_assert_sequence(unsigned point, char *expected) {
  char *cp = tokens_buffer;
  while (cp < tokp) {
    if (*cp++ != *expected++)
     return _test_fail(point);
  }
  if (*expected)
    return _test_fail(point);
  clear_tokens();
  return FALSE;
}

bool_t _test_assert_time_window(unsigned point, systime_t start, systime_t end) {

  return _test_assert(point, chTimeIsWithin(start, end));
}

/*
 * Threads utils.
 */

/**
 * @brief   Sets a termination request in all the test-spawned threads.
 */
void test_terminate_threads(void) {
  int i;

  for (i = 0; i < MAX_THREADS; i++)
    if (threads[i])
      chThdTerminate(threads[i]);
}

/**
 * @brief   Waits for the completion of all the test-spawned threads.
 */
void test_wait_threads(void) {
  int i;

  for (i = 0; i < MAX_THREADS; i++)
    if (threads[i] != NULL) {
      chThdWait(threads[i]);
      threads[i] = NULL;
    }
}

#if CH_DBG_THREADS_PROFILING
/**
 * @brief   CPU pulse.
 * @note    The current implementation is not totally reliable.
 *
 * @param[in] duration      CPU pulse duration in milliseconds
 */
void test_cpu_pulse(unsigned duration) {
  systime_t start, end, now;

  start = chThdSelf()->p_time;
  end = start + MS2ST(duration);
  do {
    now = chThdSelf()->p_time;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  }
  while (end > start ? (now >= start) && (now < end) :
                       (now >= start) || (now < end));
}
#endif

/**
 * @brief   Delays execution until next system time tick.
 *
 * @return              The system time.
 */
systime_t test_wait_tick(void) {

  chThdSleep(1);
  return chTimeNow();
}

/*
 * Timer utils.
 */

/**
 * @brief   Set to @p TRUE when the test timer reaches its deadline.
 */
bool_t test_timer_done;

static VirtualTimer vt;
static void tmr(void *p) {
  (void)p;

  test_timer_done = TRUE;
}

/**
 * @brief   Starts the test timer.
 *
 * @param[in] ms        time in milliseconds
 */
void test_start_timer(unsigned ms) {

  systime_t duration = MS2ST(ms);
  test_timer_done = FALSE;
  chVTSet(&vt, duration, tmr, NULL);
}

/*
 * Test suite execution.
 */
static void execute_test(const struct testcase *tcp) {
  int i;

  /* Initialization */
  clear_tokens();
  local_fail = FALSE;
  for (i = 0; i < MAX_THREADS; i++)
    threads[i] = NULL;

  if (tcp->setup != NULL)
    tcp->setup();
  tcp->execute();
  if (tcp->teardown != NULL)
    tcp->teardown();

  test_wait_threads();
}

static void print_line(void) {
  unsigned i;

  for (i = 0; i < 76; i++)
    chSequentialStreamPut(chp, '-');
  chSequentialStreamWrite(chp, (const uint8_t *)"\r\n", 2);
}

/**
 * @brief   Test execution thread function.
 *
 * @param[in] p         pointer to a @p BaseChannel object for test output
 * @return              A failure boolean value.
 */
msg_t TestThread(void *p) {
  int i, j;

  chp = p;
  test_println("");
  test_println("*** ChibiOS/RT test suite");
  test_println("***");
  test_print("*** Kernel:       ");
  test_println(CH_KERNEL_VERSION);
  test_print("*** Compiled:     ");
  test_println(__DATE__ " - " __TIME__);
#ifdef CH_COMPILER_NAME
  test_print("*** Compiler:     ");
  test_println(CH_COMPILER_NAME);
#endif
  test_print("*** Architecture: ");
  test_println(CH_ARCHITECTURE_NAME);
#ifdef CH_CORE_VARIANT_NAME
  test_print("*** Core Variant: ");
  test_println(CH_CORE_VARIANT_NAME);
#endif
#ifdef CH_PORT_INFO
  test_print("*** Port Info:    ");
  test_println(CH_PORT_INFO);
#endif
#ifdef PLATFORM_NAME
  test_print("*** Platform:     ");
  test_println(PLATFORM_NAME);
#endif
#ifdef BOARD_NAME
  test_print("*** Test Board:   ");
  test_println(BOARD_NAME);
#endif
  test_println("");

  global_fail = FALSE;
  i = 0;
  while (patterns[i]) {
    j = 0;
    while (patterns[i][j]) {
      print_line();
      test_print("--- Test Case ");
      test_printn(i + 1);
      test_print(".");
      test_printn(j + 1);
      test_print(" (");
      test_print(patterns[i][j]->name);
      test_println(")");
#if DELAY_BETWEEN_TESTS > 0
      chThdSleepMilliseconds(DELAY_BETWEEN_TESTS);
#endif
      execute_test(patterns[i][j]);
      if (local_fail) {
        test_print("--- Result: FAILURE (#");
        test_printn(failpoint);
        test_print(" [");
        print_tokens();
        test_println("])");
      }
      else
        test_println("--- Result: SUCCESS");
      j++;
    }
    i++;
  }
  print_line();
  test_println("");
  test_print("Final result: ");
  if (global_fail)
    test_println("FAILURE");
  else
    test_println("SUCCESS");

  return (msg_t)global_fail;
}

/** @} */
