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
#include "hal.h"

/*
 * LED blinker1 thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker1");
  while (TRUE) {
    palClearPad(GPIO3, GPIO3_LED1);
    chThdSleepMilliseconds(500);
    palSetPad(GPIO3, GPIO3_LED1);
    chThdSleepMilliseconds(500);
  }
}

/*
 * LED blinker2 thread, times are in milliseconds.
 */
static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker2");
  while (TRUE) {
    palClearPad(GPIO3, GPIO3_LED2);
    chThdSleepMilliseconds(500);
    palSetPad(GPIO3, GPIO3_LED2);
    chThdSleepMilliseconds(480);
  }
}

/*
 * LED scanlight thread, times are in milliseconds.
 */
static WORKING_AREA(waThread3, 128);
static msg_t Thread3(void *arg) {

  (void)arg;
  chRegSetThreadName("scanner1");
  palSetPort(GPIO2, PAL_PORT_BIT(GPIO2_LED5) |
                    PAL_PORT_BIT(GPIO2_LED6) |
                    PAL_PORT_BIT(GPIO2_LED7) |
                    PAL_PORT_BIT(GPIO2_LED8));
  while (TRUE) {
    palClearPort( GPIO2, PAL_PORT_BIT(GPIO2_LED5));
    chThdSleepMilliseconds(50);
    palSetPort(   GPIO2, PAL_PORT_BIT(GPIO2_LED8));
    chThdSleepMilliseconds(150);
    palClearPort( GPIO2, PAL_PORT_BIT(GPIO2_LED6));
    chThdSleepMilliseconds(50);
    palSetPort(   GPIO2, PAL_PORT_BIT(GPIO2_LED5));
    chThdSleepMilliseconds(150);
    palClearPort( GPIO2, PAL_PORT_BIT(GPIO2_LED7));
    chThdSleepMilliseconds(50);
    palSetPort(   GPIO2, PAL_PORT_BIT(GPIO2_LED6));
    chThdSleepMilliseconds(150);
    palClearPort( GPIO2, PAL_PORT_BIT(GPIO2_LED8));
    chThdSleepMilliseconds(50);
    palSetPort(   GPIO2, PAL_PORT_BIT(GPIO2_LED7));
    chThdSleepMilliseconds(150);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Creates the LED threads.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);
  chThdCreateStatic(waThread3, sizeof(waThread3), NORMALPRIO, Thread3, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
