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
#include "test.h"

#include "web/webthread.h"

static WORKING_AREA(waWebThread, 1024);

static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {

  (void)p;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(IOPORT2, PIOB_LCD_BL);
    chThdSleepMilliseconds(100);
    palClearPad(IOPORT2, PIOB_LCD_BL);
    chThdSleepMilliseconds(900);
  }
  return 0;
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
   * Activates the serial driver 1 using the driver default configuration.
   */
  sdStart(&SD1, NULL);

  /*
   * Creates the blinker and web server threads.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waWebThread, sizeof(waWebThread), LOWPRIO, WebThread, NULL);

  /*
   * Normal main() thread activity.
   */
  while (TRUE) {
    chThdSleepMilliseconds(500);
    if (!palReadPad(IOPORT2, PIOB_SW1))
      sdWrite(&SD1, (uint8_t *)"Hello World!\r\n", 14);
    if (!palReadPad(IOPORT2, PIOB_SW2))
      TestThread(&SD1);
  }

  return 0;
}
