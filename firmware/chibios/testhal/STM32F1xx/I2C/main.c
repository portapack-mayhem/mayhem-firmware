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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "i2c_pns.h"
#include "lis3.h"
#include "tmp75.h"
#include "fake.h"


/*
 * Red LEDs blinker thread, times are in milliseconds.
 */
static WORKING_AREA(BlinkWA, 64);
static msg_t Blink(void *arg) {
  chRegSetThreadName("Blink");
  (void)arg;
  while (TRUE) {
    palClearPad(IOPORT3, GPIOC_LED);
    chThdSleepMilliseconds(500);
    palSetPad(IOPORT3, GPIOC_LED);
    chThdSleepMilliseconds(500);
  }
  return 0;
}

/*
 * Accelerometer thread
 */
static WORKING_AREA(PollAccelThreadWA, 256);
static msg_t PollAccelThread(void *arg) {
  chRegSetThreadName("PollAccel");
  (void)arg;
  while (TRUE) {
    /*chThdSleepMilliseconds(rand() & 31);*/
    chThdSleepMilliseconds(32);
    request_acceleration_data();
  }
  return 0;
}


/* Temperature polling thread */
static WORKING_AREA(PollTmp75ThreadWA, 256);
static msg_t PollTmp75Thread(void *arg) {
  chRegSetThreadName("PollTmp75");
  (void)arg;
  while (TRUE) {
    /*chThdSleepMilliseconds(rand() & 31);*/
    chThdSleepMilliseconds(15);
    /* Call reading function */
    request_temperature();
  }
  return 0;
}


/* Temperature polling thread */
static WORKING_AREA(PollFakeThreadWA, 256);
static msg_t PollFakeThread(void *arg) {
  chRegSetThreadName("PollFake");
  (void)arg;
  while (TRUE) {
    chThdSleepMilliseconds(16);
    /* Call reading function */
    request_fake();
  }
  return 0;
}


/*
 * Entry point, note, the main() function is already a thread in the system
 * on entry.
 */
int main(void) {

  halInit();
  chSysInit();

  chThdSleepMilliseconds(200);
  I2CInit_pns();

  /* Create accelerometer thread */
  chThdCreateStatic(PollAccelThreadWA,
          sizeof(PollAccelThreadWA),
          NORMALPRIO,
          PollAccelThread,
          NULL);

  /* Create temperature thread */
  chThdCreateStatic(PollTmp75ThreadWA,
          sizeof(PollTmp75ThreadWA),
          NORMALPRIO,
          PollTmp75Thread,
          NULL);

  /* Create not responding thread */
  chThdCreateStatic(PollFakeThreadWA,
          sizeof(PollFakeThreadWA),
          NORMALPRIO,
          PollFakeThread,
          NULL);

  /* Creates the blinker thread. */
  chThdCreateStatic(BlinkWA, sizeof(BlinkWA), HIGHPRIO, Blink, NULL);

  /* main loop that do nothing */
  while (TRUE) {
    chThdSleepMilliseconds(500);
  }

  return 0;
}
