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

/**
 * @file    STM32/RTCv1/rtc_lld.c
 * @brief   STM32 RTC subsystem low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
RTCDriver RTCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Wait for synchronization of RTC registers with APB1 bus.
 * @details This function must be invoked before trying to read RTC registers
 *          in the backup domain: DIV, CNT, ALR. CR registers can always
 *          be read.
 *
 * @notapi
 */
#define rtc_lld_apb1_sync() {while ((RTC->CRL & RTC_CRL_RSF) == 0);}

/**
 * @brief   Wait for for previous write operation complete.
 * @details This function must be invoked before writing to any RTC registers
 *
 * @notapi
 */
#define rtc_lld_wait_write() {while ((RTC->CRL & RTC_CRL_RTOFF) == 0);}

/**
 * @brief   Acquires write access to RTC registers.
 * @details Before writing to the backup domain RTC registers the previous
 *          write operation must be completed. Use this function before
 *          writing to PRL, CNT, ALR registers.
 *
 * @notapi
 */
#define rtc_lld_acquire() {rtc_lld_wait_write(); RTC->CRL |= RTC_CRL_CNF;}

/**
 * @brief   Releases write access to RTC registers.
 *
 * @notapi
 */
#define rtc_lld_release() {RTC->CRL &= ~RTC_CRL_CNF;}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   RTC interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RTC_IRQHandler) {
  uint16_t flags;

  CH_IRQ_PROLOGUE();

  /* This wait works only when AHB1 bus was previously powered off by any
     reason (standby, reset, etc). In other cases it does nothing.*/
  rtc_lld_apb1_sync();

  /* Mask of all enabled and pending sources.*/
  flags = RTC->CRH & RTC->CRL;
  RTC->CRL &= ~(RTC_CRL_SECF | RTC_CRL_ALRF | RTC_CRL_OWF);

  if (flags & RTC_CRL_SECF)
    RTCD1.callback(&RTCD1, RTC_EVENT_SECOND);

  if (flags & RTC_CRL_ALRF)
    RTCD1.callback(&RTCD1, RTC_EVENT_ALARM);

  if (flags & RTC_CRL_OWF)
    RTCD1.callback(&RTCD1, RTC_EVENT_OVERFLOW);

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Load value of RTCCLK to prescaler registers.
 * @note    The pre-scaler must not be set on every reset as RTC clock
 *          counts are lost when it is set.
 * @note    This function designed to be called from
 *          hal_lld_backup_domain_init(). Because there is only place
 *          where possible to detect BKP domain reset event reliably.
 *
 * @notapi
 */
void rtc_lld_set_prescaler(void){
  rtc_lld_acquire();
  RTC->PRLH = (uint16_t)((STM32_RTCCLK - 1) >> 16) & 0x000F;
  RTC->PRLL = (uint16_t)(((STM32_RTCCLK - 1))      & 0xFFFF);
  rtc_lld_release();
}

/**
 * @brief   Initialize RTC.
 *
 * @notapi
 */
void rtc_lld_init(void){

  /* RSF bit must be cleared by software after an APB1 reset or an APB1 clock
     stop. Otherwise its value will not be actual. */
  RTC->CRL &= ~RTC_CRL_RSF;

  /* Required because access to PRL.*/
  rtc_lld_apb1_sync();

  /* All interrupts initially disabled.*/
  rtc_lld_wait_write();
  RTC->CRH = 0;

  /* Callback initially disabled.*/
  RTCD1.callback = NULL;

  /* IRQ vector permanently assigned to this driver.*/
  nvicEnableVector(RTC_IRQn, CORTEX_PRIORITY_MASK(STM32_RTC_IRQ_PRIORITY));
}

/**
 * @brief   Set current time.
 * @note    Fractional part will be silently ignored. There is no possibility
 *          to change it on STM32F1xx platform.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @notapi
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec) {

  (void)rtcp;

  rtc_lld_acquire();
  RTC->CNTH = (uint16_t)(timespec->tv_sec >> 16);
  RTC->CNTL = (uint16_t)(timespec->tv_sec & 0xFFFF);
  rtc_lld_release();
}

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @notapi
 */
void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec) {
  (void)rtcp;

  uint32_t time_frac;

  /* Required because access to CNT and DIV.*/
  rtc_lld_apb1_sync();

  /* Loops until two consecutive read returning the same value.*/
  do {
    timespec->tv_sec = ((uint32_t)(RTC->CNTH) << 16) + RTC->CNTL;
    time_frac = (((uint32_t)RTC->DIVH) << 16) + (uint32_t)RTC->DIVL;
  } while ((timespec->tv_sec) != (((uint32_t)(RTC->CNTH) << 16) + RTC->CNTL));

  timespec->tv_msec = (uint16_t)(((STM32_RTCCLK - 1 - time_frac) * 1000) /
                                 STM32_RTCCLK);
}

/**
 * @brief   Set alarm time.
 *
 * @note      Default value after BKP domain reset is 0xFFFFFFFF
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[in] alarmspec pointer to a @p RTCAlarm structure
 *
 * @notapi
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm *alarmspec) {

  (void)rtcp;
  (void)alarm;

  rtc_lld_acquire();
  if (alarmspec != NULL) {
    RTC->ALRH = (uint16_t)(alarmspec->tv_sec >> 16);
    RTC->ALRL = (uint16_t)(alarmspec->tv_sec & 0xFFFF);
  }
  else {
    RTC->ALRH = 0;
    RTC->ALRL = 0;
  }
  rtc_lld_release();
}

/**
 * @brief   Get current alarm.
 * @note    If an alarm has not been set then the returned alarm specification
 *          is not meaningful.
 *
 * @note    Default value after BKP domain reset is 0xFFFFFFFF.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @notapi
 */
void rtc_lld_get_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       RTCAlarm *alarmspec) {

  (void)rtcp;
  (void)alarm;

  /* Required because access to ALR.*/
  rtc_lld_apb1_sync();

  alarmspec->tv_sec = ((RTC->ALRH << 16) + RTC->ALRL);
}

/**
 * @brief   Enables or disables RTC callbacks.
 * @details This function enables or disables callbacks, use a @p NULL pointer
 *          in order to disable a callback.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] callback  callback function pointer or @p NULL
 *
 * @notapi
 */
void rtc_lld_set_callback(RTCDriver *rtcp, rtccb_t callback) {

  if (callback != NULL) {

    /* IRQ sources enabled only after setting up the callback.*/
    rtcp->callback = callback;

    rtc_lld_wait_write();
    RTC->CRL &= ~(RTC_CRL_OWF | RTC_CRL_ALRF | RTC_CRL_SECF);
    RTC->CRH = RTC_CRH_OWIE | RTC_CRH_ALRIE | RTC_CRH_SECIE;
  }
  else {
    rtc_lld_wait_write();
    RTC->CRH = 0;

    /* Callback set to NULL only after disabling the IRQ sources.*/
    rtcp->callback = NULL;
  }
}

#include "chrtclib.h"

/**
 * @brief   Get current time in format suitable for usage in FatFS.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @return              FAT time value.
 *
 * @api
 */
uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp) {
  uint32_t fattime;
  struct tm timp;

  rtcGetTimeTm(rtcp, &timp);

  fattime  = (timp.tm_sec)       >> 1;
  fattime |= (timp.tm_min)       << 5;
  fattime |= (timp.tm_hour)      << 11;
  fattime |= (timp.tm_mday)      << 16;
  fattime |= (timp.tm_mon + 1)   << 21;
  fattime |= (timp.tm_year - 80) << 25;

  return fattime;
}
#endif /* HAL_USE_RTC */

/** @} */
