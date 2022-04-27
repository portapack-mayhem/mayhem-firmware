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
 * @file    evtimer.h
 * @brief   Events Generator Timer structures and macros.
 *
 * @addtogroup event_timer
 * @{
 */

#ifndef _EVTIMER_H_
#define _EVTIMER_H_


/*
 * Module dependencies check.
 */
#if !CH_USE_EVENTS
#error "Event Timers require CH_USE_EVENTS"
#endif

/**
 * @brief Event timer structure.
 */
typedef struct {
  VirtualTimer  et_vt;
  EventSource   et_es;
  systime_t     et_interval;
} EvTimer;

#ifdef __cplusplus
extern "C" {
#endif
  void evtStart(EvTimer *etp);
  void evtStop(EvTimer *etp);
#ifdef __cplusplus
}
#endif

/**
 * @brief Initializes an @p EvTimer structure.
 *
 * @param etp the EvTimer structure to be initialized
 * @param time the interval in system ticks
 */
#define evtInit(etp, time) {                                            \
  chEvtInit(&(etp)->et_es);                                             \
  (etp)->et_vt.vt_func = NULL;                                          \
  (etp)->et_interval = (time);                                          \
}

#endif /* _EVTIMER_H_ */

/** @} */
