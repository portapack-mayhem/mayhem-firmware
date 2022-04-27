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

#include "chrtclib.h"

/*
 * Set this flag to TRUE for testing waking up from deep sleep
 */
#define TEST_ALARM_WAKEUP     FALSE

/*
 * Local working variables
 */
static RTCTime timespec;
static RTCAlarm alarmspec;

#if TEST_ALARM_WAKEUP

/*
 * awake indication thread
 */
static WORKING_AREA(blinkWA, 128);
static msg_t blink_thd(void *arg){
  (void)arg;
  while (TRUE) {
    chThdSleepMilliseconds(100);
    palTogglePad(GPIOC, GPIOC_LED);
  }
  return 0;
}

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /* start awake indicator thread */
  chThdCreateStatic(blinkWA, sizeof(blinkWA), NORMALPRIO, blink_thd, NULL);

  /* set alarm in near future */
  rtcGetTime(&RTCD1, &timespec);
  alarmspec.tv_sec = timespec.tv_sec + 30;
  rtcSetAlarm(&RTCD1, 0, &alarmspec);

  while (TRUE) {
    chThdSleepSeconds(10);
    chSysLock();

    /* go sleep */
    PWR->CR |= (PWR_CR_PDDS | PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF);
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
  }
  return 0;
}

#else /* TEST_ALARM_WAKEUP */

/*
 * Manually reloaded test alarm period.
 */
#define RTC_ALARMPERIOD   5

/*
 * Semaphore for signaling from ISR to "main" thread
 */
static BinarySemaphore alarm_sem;

/*
 * Global RTC callback function
 */
static void rtc_cb(RTCDriver *rtcp, rtcevent_t event) {

  (void)rtcp;

  switch (event) {
  case RTC_EVENT_OVERFLOW:
    palTogglePad(GPIOC, GPIOC_LED);
    break;
  case RTC_EVENT_SECOND:
    /* palTogglePad(GPIOC, GPIOC_LED); */
    break;
  case RTC_EVENT_ALARM:
    palTogglePad(GPIOC, GPIOC_LED);
    chSysLockFromIsr();
    chBSemSignalI(&alarm_sem);
    chSysUnlockFromIsr();
    break;
  }
}

/*
 * Application entry point.
 */
int main(void) {
  msg_t status = RDY_TIMEOUT;

  halInit();
  chSysInit();
  chBSemInit(&alarm_sem, TRUE);

  rtcGetTime(&RTCD1, &timespec);
  alarmspec.tv_sec = timespec.tv_sec + RTC_ALARMPERIOD;
  rtcSetAlarm(&RTCD1, 0, &alarmspec);

  rtcSetCallback(&RTCD1, rtc_cb);
  while (TRUE) {

    /* Wait until alarm callback signaled semaphore.*/
    status = chBSemWaitTimeout(&alarm_sem, S2ST(RTC_ALARMPERIOD + 10));

    if (status == RDY_TIMEOUT) {
      chSysHalt();
    }
    else {
      rtcGetTime(&RTCD1, &timespec);
      alarmspec.tv_sec = timespec.tv_sec + RTC_ALARMPERIOD;
      rtcSetAlarm(&RTCD1, 0, &alarmspec);
    }
  }
  return 0;
}
#endif /* TEST_ALARM_WAKEUP */
