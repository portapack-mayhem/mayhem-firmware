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
 * GPT1 callback.
 */
static void gpt1cb(GPTDriver *gptp) {

  (void)gptp;
  palClearPad(IOPORT3, GPIOC_LED);
  chSysLockFromIsr();
  gptStartOneShotI(&GPTD2, 200);   /* 0.02 second pulse.*/
  chSysUnlockFromIsr();
}

/*
 * GPT2 callback.
 */
static void gpt2cb(GPTDriver *gptp) {

  (void)gptp;
  palSetPad(IOPORT3, GPIOC_LED);
}

/*
 * GPT1 configuration.
 */
static const GPTConfig gpt1cfg = {
  10000,    /* 10kHz timer clock.*/
  gpt1cb,   /* Timer callback.*/
  0
};

/*
 * GPT2 configuration.
 */
static const GPTConfig gpt2cfg = {
  10000,    /* 10kHz timer clock.*/
  gpt2cb,   /* Timer callback.*/
  0
};

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
   * Initializes the GPT drivers 1 and 2.
   */
  gptStart(&GPTD1, &gpt1cfg);
  gptPolledDelay(&GPTD1, 10); /* Small delay.*/
  gptStart(&GPTD2, &gpt2cfg);
  gptPolledDelay(&GPTD2, 10); /* Small delay.*/

  /*
   * Normal main() thread activity, it changes the GPT1 period every
   * five seconds.
   */
  while (TRUE) {
    gptStartContinuous(&GPTD1, 5000);
    chThdSleepMilliseconds(5000);
    gptStopTimer(&GPTD1);
    gptStartContinuous(&GPTD1, 2500);
    chThdSleepMilliseconds(5000);
    gptStopTimer(&GPTD1);
  }
  return 0;
}
