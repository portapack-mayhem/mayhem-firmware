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
 * @file    test.h
 * @brief   Tests support header.
 *
 * @addtogroup test
 * @{
 */

#ifndef _TEST_H_
#define _TEST_H_

/**
 * @brief   Delay inserted between test cases.
 */
#if !defined(DELAY_BETWEEN_TESTS) || defined(__DOXYGEN__)
#define DELAY_BETWEEN_TESTS     200
#endif

/**
 * @brief   If @p TRUE then benchmarks are not included.
 */
#if !defined(TEST_NO_BENCHMARKS) || defined(__DOXYGEN__)
#define TEST_NO_BENCHMARKS      FALSE
#endif

#define MAX_THREADS             5
#define MAX_TOKENS              16

#if defined(CH_ARCHITECTURE_AVR) || defined(CH_ARCHITECTURE_MSP430)
#define THREADS_STACK_SIZE      48
#elif defined(CH_ARCHITECTURE_STM8)
#define THREADS_STACK_SIZE      64
#elif defined(CH_ARCHITECTURE_SIMIA32)
#define THREADS_STACK_SIZE      512
#else
#define THREADS_STACK_SIZE      128
#endif
#define WA_SIZE THD_WA_SIZE(THREADS_STACK_SIZE)

/**
 * @brief   Structure representing a test case.
 */
struct testcase {
  const char *name;             /**< @brief Test case name.                 */
  void (*setup)(void);          /**< @brief Test case preparation function. */
  void (*teardown)(void);       /**< @brief Test case clean up function.    */
  void (*execute)(void);        /**< @brief Test case execution function.   */
};

#ifndef __DOXYGEN__
union test_buffers {
  struct {
    WORKING_AREA(T0, THREADS_STACK_SIZE);
    WORKING_AREA(T1, THREADS_STACK_SIZE);
    WORKING_AREA(T2, THREADS_STACK_SIZE);
    WORKING_AREA(T3, THREADS_STACK_SIZE);
    WORKING_AREA(T4, THREADS_STACK_SIZE);
  } wa;
  uint8_t buffer[WA_SIZE * 5];
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
  msg_t TestThread(void *p);
  void test_printn(uint32_t n);
  void test_print(const char *msgp);
  void test_println(const char *msgp);
  void test_emit_token(char token);
  bool_t _test_fail(unsigned point);
  bool_t _test_assert(unsigned point, bool_t condition);
  bool_t _test_assert_sequence(unsigned point, char *expected);
  bool_t _test_assert_time_window(unsigned point, systime_t start, systime_t end);
  void test_terminate_threads(void);
  void test_wait_threads(void);
  systime_t test_wait_tick(void);
  void test_start_timer(unsigned ms);
#if CH_DBG_THREADS_PROFILING
  void test_cpu_pulse(unsigned duration);
#endif
#if defined(WIN32)
  void ChkIntSources(void);
#endif
#ifdef __cplusplus
}
#endif

/**
 * @brief   Test failure enforcement.
 */
#define test_fail(point) {                                                  \
  _test_fail(point);                                                        \
  return;                                                                   \
}

/**
 * @brief   Test assertion.
 *
 * @param[in] point     numeric assertion identifier
 * @param[in] condition a boolean expression that must be verified to be true
 * @param[in] msg       failure message
 */
#define test_assert(point, condition, msg) {                                \
  if (_test_assert(point, condition))                                       \
    return;                                                                 \
}

/**
 * @brief   Test assertion with lock.
 *
 * @param[in] point     numeric assertion identifier
 * @param[in] condition a boolean expression that must be verified to be true
 * @param[in] msg       failure message
 */
#define test_assert_lock(point, condition, msg) {                           \
  chSysLock();                                                              \
  if (_test_assert(point, condition)) {                                     \
    chSysUnlock();                                                          \
    return;                                                                 \
  }                                                                         \
  chSysUnlock();                                                            \
}

/**
 * @brief   Test sequence assertion.
 *
 * @param[in] point     numeric assertion identifier
 * @param[in] expected  string to be matched with the tokens buffer
 */
#define test_assert_sequence(point, expected) {                             \
  if (_test_assert_sequence(point, expected))                               \
    return;                                                                 \
}

/**
 * @brief   Test time window assertion.
 *
 * @param[in] point     numeric assertion identifier
 * @param[in] start     initial time in the window (included)
 * @param[in] end       final time in the window (not included)
 */
#define test_assert_time_window(point, start, end) {                        \
  if (_test_assert_time_window(point, start, end))                          \
    return;                                                                 \
}

#if !defined(__DOXYGEN__)
extern Thread *threads[MAX_THREADS];
extern union test_buffers test;
extern void * ROMCONST wa[];
extern bool_t test_timer_done;
#endif

#endif /* _TEST_H_ */

/** @} */
