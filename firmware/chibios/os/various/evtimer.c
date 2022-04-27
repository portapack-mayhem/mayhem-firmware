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
 * @file    evtimer.c
 * @brief   Events Generator Timer code.
 *
 * @addtogroup event_timer
 * @{
 */

#include "ch.h"
#include "evtimer.h"

static void tmrcb(void *p) {
  EvTimer *etp = p;

  chSysLockFromIsr();
  chEvtBroadcastI(&etp->et_es);
  chVTSetI(&etp->et_vt, etp->et_interval, tmrcb, etp);
  chSysUnlockFromIsr();
}

/**
 * @brief Starts the timer
 * @details If the timer was already running then the function has no effect.
 *
 * @param etp pointer to an initialized @p EvTimer structure.
 */
void evtStart(EvTimer *etp) {

  chSysLock();

  if (!chVTIsArmedI(&etp->et_vt))
    chVTSetI(&etp->et_vt, etp->et_interval, tmrcb, etp);

  chSysUnlock();
}

/**
 * @brief Stops the timer.
 * @details If the timer was already stopped then the function has no effect.
 *
 * @param etp pointer to an initialized @p EvTimer structure.
 */
void evtStop(EvTimer *etp) {

  chVTReset(&etp->et_vt);
}

/** @} */
