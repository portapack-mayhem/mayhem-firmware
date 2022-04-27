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
 * SDIO configuration.
 */
static const SDCConfig sdccfg = {
  0
};

static uint8_t blkbuf[MMCSD_BLOCK_SIZE * 4 + 1];

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
   * Initializes the SDIO drivers.
   */
  sdcStart(&SDCD1, &sdccfg);
  if (!sdcConnect(&SDCD1)) {
    int i;

    /* Single aligned read.*/
    if (sdcRead(&SDCD1, 0, blkbuf, 1))
      chSysHalt();

    /* Single unaligned read.*/
    if (sdcRead(&SDCD1, 0, blkbuf + 1, 1))
      chSysHalt();

    /* Multiple aligned read.*/
    if (sdcRead(&SDCD1, 0, blkbuf, 4))
      chSysHalt();

    /* Multiple unaligned read.*/
    if (sdcRead(&SDCD1, 0, blkbuf + 1, 4))
      chSysHalt();

    /* Repeated multiple aligned reads.*/
    for (i = 0; i < 1000; i++) {
      if (sdcRead(&SDCD1, 0, blkbuf, 4))
        chSysHalt();
    }

    /* Repeated multiple unaligned reads.*/
    for (i = 0; i < 1000; i++) {
      if (sdcRead(&SDCD1, 0, blkbuf + 1, 4))
        chSysHalt();
    }

    /* Repeated multiple aligned writes.*/
    for (i = 0; i < 100; i++) {
      if (sdcRead(&SDCD1, 0x10000, blkbuf, 4))
        chSysHalt();
      if (sdcWrite(&SDCD1, 0x10000, blkbuf, 4))
        chSysHalt();
      if (sdcWrite(&SDCD1, 0x10000, blkbuf, 4))
        chSysHalt();
    }

    /* Repeated multiple unaligned writes.*/
    for (i = 0; i < 100; i++) {
      if (sdcRead(&SDCD1, 0x10000, blkbuf + 1, 4))
        chSysHalt();
      if (sdcWrite(&SDCD1, 0x10000, blkbuf + 1, 4))
        chSysHalt();
      if (sdcWrite(&SDCD1, 0x10000, blkbuf + 1, 4))
        chSysHalt();
    }

    if (sdcDisconnect(&SDCD1))
      chSysHalt();
  }

  /*
   * Normal main() thread activity.
   */
  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
