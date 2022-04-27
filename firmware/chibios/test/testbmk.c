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
 * @page test_benchmarks Kernel Benchmarks
 *
 * File: @ref testbmk.c
 *
 * <h2>Description</h2>
 * This module implements a series of system benchmarks. The benchmarks are
 * useful as a stress test and as a reference when comparing ChibiOS/RT
 * with similar systems.
 *
 * <h2>Objective</h2>
 * Objective of the test module is to provide a performance index for the
 * most critical system subsystems. The performance numbers allow to
 * discover performance regressions between successive ChibiOS/RT releases.
 *
 * <h2>Preconditions</h2>
 * None.
 *
 * <h2>Test Cases</h2>
 * - @subpage test_benchmarks_001
 * - @subpage test_benchmarks_002
 * - @subpage test_benchmarks_003
 * - @subpage test_benchmarks_004
 * - @subpage test_benchmarks_005
 * - @subpage test_benchmarks_006
 * - @subpage test_benchmarks_007
 * - @subpage test_benchmarks_008
 * - @subpage test_benchmarks_009
 * - @subpage test_benchmarks_010
 * - @subpage test_benchmarks_011
 * - @subpage test_benchmarks_012
 * - @subpage test_benchmarks_013
 * .
 * @file testbmk.c Kernel Benchmarks
 * @brief Kernel Benchmarks source file
 * @file testbmk.h
 * @brief Kernel Benchmarks header file
 */

static Semaphore sem1;
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
static Mutex mtx1;
#endif

static msg_t thread1(void *p) {
  Thread *tp;
  msg_t msg;

  (void)p;
  do {
    tp = chMsgWait();
    msg = chMsgGet(tp);
    chMsgRelease(tp, msg);
  } while (msg);
  return 0;
}

#ifdef __GNUC__
__attribute__((noinline))
#endif
static unsigned int msg_loop_test(Thread *tp) {

  uint32_t n = 0;
  test_wait_tick();
  test_start_timer(1000);
  do {
    (void)chMsgSend(tp, 1);
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  (void)chMsgSend(tp, 0);
  return n;
}

/**
 * @page test_benchmarks_001 Messages performance #1
 *
 * <h2>Description</h2>
 * A message server thread is created with a lower priority than the client
 * thread, the messages throughput per second is measured and the result
 * printed in the output log.
 */

static void bmk1_execute(void) {
  uint32_t n;

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()-1, thread1, NULL);
  n = msg_loop_test(threads[0]);
  test_wait_threads();
  test_print("--- Score : ");
  test_printn(n);
  test_print(" msgs/S, ");
  test_printn(n << 1);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk1 = {
  "Benchmark, messages #1",
  NULL,
  NULL,
  bmk1_execute
};

/**
 * @page test_benchmarks_002 Messages performance #2
 *
 * <h2>Description</h2>
 * A message server thread is created with an higher priority than the client
 * thread, the messages throughput per second is measured and the result
 * printed in the output log.
 */

static void bmk2_execute(void) {
  uint32_t n;

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread1, NULL);
  n = msg_loop_test(threads[0]);
  test_wait_threads();
  test_print("--- Score : ");
  test_printn(n);
  test_print(" msgs/S, ");
  test_printn(n << 1);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk2 = {
  "Benchmark, messages #2",
  NULL,
  NULL,
  bmk2_execute
};

static msg_t thread2(void *p) {

  return (msg_t)p;
}

/**
 * @page test_benchmarks_003 Messages performance #3
 *
 * <h2>Description</h2>
 * A message server thread is created with an higher priority than the client
 * thread, four lower priority threads crowd the ready list, the messages
 * throughput per second is measured while the ready list and the result
 * printed in the output log.
 */

static void bmk3_execute(void) {
  uint32_t n;

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread1, NULL);
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()-2, thread2, NULL);
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()-3, thread2, NULL);
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriority()-4, thread2, NULL);
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriority()-5, thread2, NULL);
  n = msg_loop_test(threads[0]);
  test_wait_threads();
  test_print("--- Score : ");
  test_printn(n);
  test_print(" msgs/S, ");
  test_printn(n << 1);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk3 = {
  "Benchmark, messages #3",
  NULL,
  NULL,
  bmk3_execute
};

/**
 * @page test_benchmarks_004 Context Switch performance
 *
 * <h2>Description</h2>
 * A thread is created that just performs a @p chSchGoSleepS() into a loop,
 * the thread is awakened as fast is possible by the tester thread.<br>
 * The Context Switch performance is calculated by measuring the number of
 * iterations after a second of continuous operations.
 */

msg_t thread4(void *p) {
  msg_t msg;
  Thread *self = chThdSelf();

  (void)p;
  chSysLock();
  do {
    chSchGoSleepS(THD_STATE_SUSPENDED);
    msg = self->p_u.rdymsg;
  } while (msg == RDY_OK);
  chSysUnlock();
  return 0;
}

static void bmk4_execute(void) {
  Thread *tp;
  uint32_t n;

  tp = threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+1, thread4, NULL);
  n = 0;
  test_wait_tick();
  test_start_timer(1000);
  do {
    chSysLock();
    chSchWakeupS(tp, RDY_OK);
    chSchWakeupS(tp, RDY_OK);
    chSchWakeupS(tp, RDY_OK);
    chSchWakeupS(tp, RDY_OK);
    chSysUnlock();
    n += 4;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  chSysLock();
  chSchWakeupS(tp, RDY_TIMEOUT);
  chSysUnlock();

  test_wait_threads();
  test_print("--- Score : ");
  test_printn(n * 2);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk4 = {
  "Benchmark, context switch",
  NULL,
  NULL,
  bmk4_execute
};

/**
 * @page test_benchmarks_005 Threads performance, full cycle
 *
 * <h2>Description</h2>
 * Threads are continuously created and terminated into a loop. A full
 * @p chThdCreateStatic() / @p chThdExit() / @p chThdWait() cycle is performed
 * in each iteration.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void bmk5_execute(void) {

  uint32_t n = 0;
  void *wap = wa[0];
  tprio_t prio = chThdGetPriority() - 1;
  test_wait_tick();
  test_start_timer(1000);
  do {
    chThdWait(chThdCreateStatic(wap, WA_SIZE, prio, thread2, NULL));
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n);
  test_println(" threads/S");
}

ROMCONST struct testcase testbmk5 = {
  "Benchmark, threads, full cycle",
  NULL,
  NULL,
  bmk5_execute
};

/**
 * @page test_benchmarks_006 Threads performance, create/exit only
 *
 * <h2>Description</h2>
 * Threads are continuously created and terminated into a loop. A partial
 * @p chThdCreateStatic() / @p chThdExit() cycle is performed in each
 * iteration, the @p chThdWait() is not necessary because the thread is
 * created at an higher priority so there is no need to wait for it to
 * terminate.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void bmk6_execute(void) {

  uint32_t n = 0;
  void *wap = wa[0];
  tprio_t prio = chThdGetPriority() + 1;
  test_wait_tick();
  test_start_timer(1000);
  do {
    chThdCreateStatic(wap, WA_SIZE, prio, thread2, NULL);
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n);
  test_println(" threads/S");
}

ROMCONST struct testcase testbmk6 = {
  "Benchmark, threads, create only",
  NULL,
  NULL,
  bmk6_execute
};

/**
 * @page test_benchmarks_007 Mass reschedule performance
 *
 * <h2>Description</h2>
 * Five threads are created and atomically rescheduled by resetting the
 * semaphore where they are waiting on. The operation is performed into a
 * continuous loop.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static msg_t thread3(void *p) {

  (void)p;
  while (!chThdShouldTerminate())
    chSemWait(&sem1);
  return 0;
}

static void bmk7_setup(void) {

  chSemInit(&sem1, 0);
}

static void bmk7_execute(void) {
  uint32_t n;

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()+5, thread3, NULL);
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()+4, thread3, NULL);
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()+3, thread3, NULL);
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriority()+2, thread3, NULL);
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriority()+1, thread3, NULL);

  n = 0;
  test_wait_tick();
  test_start_timer(1000);
  do {
    chSemReset(&sem1, 0);
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_terminate_threads();
  chSemReset(&sem1, 0);
  test_wait_threads();

  test_print("--- Score : ");
  test_printn(n);
  test_print(" reschedules/S, ");
  test_printn(n * 6);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk7 = {
  "Benchmark, mass reschedule, 5 threads",
  bmk7_setup,
  NULL,
  bmk7_execute
};

/**
 * @page test_benchmarks_008 I/O Round-Robin voluntary reschedule.
 *
 * <h2>Description</h2>
 * Five threads are created at equal priority, each thread just increases a
 * variable and yields.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static msg_t thread8(void *p) {

  do {
    chThdYield();
    chThdYield();
    chThdYield();
    chThdYield();
    (*(uint32_t *)p) += 4;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while(!chThdShouldTerminate());
  return 0;
}

static void bmk8_execute(void) {
  uint32_t n;

  n = 0;
  test_wait_tick();

  threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriority()-1, thread8, (void *)&n);
  threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriority()-1, thread8, (void *)&n);
  threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriority()-1, thread8, (void *)&n);
  threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriority()-1, thread8, (void *)&n);
  threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriority()-1, thread8, (void *)&n);

  chThdSleepSeconds(1);
  test_terminate_threads();
  test_wait_threads();

  test_print("--- Score : ");
  test_printn(n);
  test_println(" ctxswc/S");
}

ROMCONST struct testcase testbmk8 = {
  "Benchmark, round robin context switching",
  NULL,
  NULL,
  bmk8_execute
};

#if CH_USE_QUEUES || defined(__DOXYGEN__)
/**
 * @page test_benchmarks_009 I/O Queues throughput
 *
 * <h2>Description</h2>
 * Four bytes are written and then read from an @p InputQueue into a continuous
 * loop.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void bmk9_execute(void) {
  uint32_t n;
  static uint8_t ib[16];
  static InputQueue iq;

  chIQInit(&iq, ib, sizeof(ib), NULL, NULL);
  n = 0;
  test_wait_tick();
  test_start_timer(1000);
  do {
    chSysLock();
    chIQPutI(&iq, 0);
    chIQPutI(&iq, 1);
    chIQPutI(&iq, 2);
    chIQPutI(&iq, 3);
    chSysUnlock();
    (void)chIQGet(&iq);
    (void)chIQGet(&iq);
    (void)chIQGet(&iq);
    (void)chIQGet(&iq);
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n * 4);
  test_println(" bytes/S");
}

ROMCONST struct testcase testbmk9 = {
  "Benchmark, I/O Queues throughput",
  NULL,
  NULL,
  bmk9_execute
};
#endif /* CH_USE_QUEUES */

/**
 * @page test_benchmarks_010 Virtual Timers set/reset performance
 *
 * <h2>Description</h2>
 * A virtual timer is set and immediately reset into a continuous loop.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void tmo(void *param) {(void)param;}

static void bmk10_execute(void) {
  static VirtualTimer vt1, vt2;
  uint32_t n = 0;

  test_wait_tick();
  test_start_timer(1000);
  do {
    chSysLock();
    chVTSetI(&vt1, 1, tmo, NULL);
    chVTSetI(&vt2, 10000, tmo, NULL);
    chVTResetI(&vt1);
    chVTResetI(&vt2);
    chSysUnlock();
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n * 2);
  test_println(" timers/S");
}

ROMCONST struct testcase testbmk10 = {
  "Benchmark, virtual timers set/reset",
  NULL,
  NULL,
  bmk10_execute
};

#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)
/**
 * @page test_benchmarks_011 Semaphores wait/signal performance
 *
 * <h2>Description</h2>
 * A counting semaphore is taken/released into a continuous loop, no Context
 * Switch happens because the counter is always non negative.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void bmk11_setup(void) {

  chSemInit(&sem1, 1);
}

static void bmk11_execute(void) {
  uint32_t n = 0;

  test_wait_tick();
  test_start_timer(1000);
  do {
    chSemWait(&sem1);
    chSemSignal(&sem1);
    chSemWait(&sem1);
    chSemSignal(&sem1);
    chSemWait(&sem1);
    chSemSignal(&sem1);
    chSemWait(&sem1);
    chSemSignal(&sem1);
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n * 4);
  test_println(" wait+signal/S");
}

ROMCONST struct testcase testbmk11 = {
  "Benchmark, semaphores wait/signal",
  bmk11_setup,
  NULL,
  bmk11_execute
};
#endif /* CH_USE_SEMAPHORES */

#if CH_USE_MUTEXES || defined(__DOXYGEN__)
/**
 * @page test_benchmarks_012 Mutexes lock/unlock performance
 *
 * <h2>Description</h2>
 * A mutex is locked/unlocked into a continuous loop, no Context Switch happens
 * because there are no other threads asking for the mutex.<br>
 * The performance is calculated by measuring the number of iterations after
 * a second of continuous operations.
 */

static void bmk12_setup(void) {

  chMtxInit(&mtx1);
}

static void bmk12_execute(void) {
  uint32_t n = 0;

  test_wait_tick();
  test_start_timer(1000);
  do {
    chMtxLock(&mtx1);
    chMtxUnlock();
    chMtxLock(&mtx1);
    chMtxUnlock();
    chMtxLock(&mtx1);
    chMtxUnlock();
    chMtxLock(&mtx1);
    chMtxUnlock();
    n++;
#if defined(SIMULATOR)
    ChkIntSources();
#endif
  } while (!test_timer_done);
  test_print("--- Score : ");
  test_printn(n * 4);
  test_println(" lock+unlock/S");
}

ROMCONST struct testcase testbmk12 = {
  "Benchmark, mutexes lock/unlock",
  bmk12_setup,
  NULL,
  bmk12_execute
};
#endif

/**
 * @page test_benchmarks_013 RAM Footprint
 *
 * <h2>Description</h2>
 * The memory size of the various kernel objects is printed.
 */

static void bmk13_execute(void) {

  test_print("--- System: ");
  test_printn(sizeof(ReadyList) + sizeof(VTList) +
              PORT_IDLE_THREAD_STACK_SIZE +
              (sizeof(Thread) + sizeof(struct intctx) +
               sizeof(struct extctx) +
               PORT_INT_REQUIRED_STACK) * 2);
  test_println(" bytes");
  test_print("--- Thread: ");
  test_printn(sizeof(Thread));
  test_println(" bytes");
  test_print("--- Timer : ");
  test_printn(sizeof(VirtualTimer));
  test_println(" bytes");
#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)
  test_print("--- Semaph: ");
  test_printn(sizeof(Semaphore));
  test_println(" bytes");
#endif
#if CH_USE_EVENTS || defined(__DOXYGEN__)
  test_print("--- EventS: ");
  test_printn(sizeof(EventSource));
  test_println(" bytes");
  test_print("--- EventL: ");
  test_printn(sizeof(EventListener));
  test_println(" bytes");
#endif
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  test_print("--- Mutex : ");
  test_printn(sizeof(Mutex));
  test_println(" bytes");
#endif
#if CH_USE_CONDVARS || defined(__DOXYGEN__)
  test_print("--- CondV.: ");
  test_printn(sizeof(CondVar));
  test_println(" bytes");
#endif
#if CH_USE_QUEUES || defined(__DOXYGEN__)
  test_print("--- Queue : ");
  test_printn(sizeof(GenericQueue));
  test_println(" bytes");
#endif
#if CH_USE_MAILBOXES || defined(__DOXYGEN__)
  test_print("--- MailB.: ");
  test_printn(sizeof(Mailbox));
  test_println(" bytes");
#endif
}

ROMCONST struct testcase testbmk13 = {
  "Benchmark, RAM footprint",
  NULL,
  NULL,
  bmk13_execute
};

/**
 * @brief   Test sequence for benchmarks.
 */
ROMCONST struct testcase * ROMCONST patternbmk[] = {
#if !TEST_NO_BENCHMARKS
  &testbmk1,
  &testbmk2,
  &testbmk3,
  &testbmk4,
  &testbmk5,
  &testbmk6,
  &testbmk7,
  &testbmk8,
#if CH_USE_QUEUES || defined(__DOXYGEN__)
  &testbmk9,
#endif
  &testbmk10,
#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)
  &testbmk11,
#endif
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  &testbmk12,
#endif
  &testbmk13,
#endif
  NULL
};
