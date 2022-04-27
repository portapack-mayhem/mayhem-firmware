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
 * @file    STM32/RTCv2/rtc_lld.c
 * @brief   RTC low level driver.
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
 * @details This function must be invoked before trying to read RTC registers.
 *
 * @notapi
 */
#define rtc_lld_apb1_sync() {while ((RTCD1.id_rtc->ISR & RTC_ISR_RSF) == 0);}

/**
 * @brief   Beginning of configuration procedure.
 *
 * @notapi
 */
#define rtc_lld_enter_init() {                                              \
  RTCD1.id_rtc->ISR |= RTC_ISR_INIT;                                        \
  while ((RTCD1.id_rtc->ISR & RTC_ISR_INITF) == 0)                          \
    ;                                                                       \
}

/**
 * @brief   Finalizing of configuration procedure.
 *
 * @notapi
 */
#define rtc_lld_exit_init() {RTCD1.id_rtc->ISR &= ~RTC_ISR_INIT;}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enable access to registers.
 *
 * @api
 */
void rtc_lld_init(void){
  RTCD1.id_rtc = RTC;

  /* Asynchronous part of preloader. Set it to maximum value. */
  uint32_t prediv_a = 0x7F;

  /* Disable write protection. */
  RTCD1.id_rtc->WPR = 0xCA;
  RTCD1.id_rtc->WPR = 0x53;

  /* If calendar not init yet. */
  if (!(RTC->ISR & RTC_ISR_INITS)){
    rtc_lld_enter_init();

    /* Prescaler register must be written in two SEPARATE writes. */
    prediv_a = (prediv_a << 16) |
                (((STM32_RTCCLK / (prediv_a + 1)) - 1) & 0x7FFF);
    RTCD1.id_rtc->PRER = prediv_a;
    RTCD1.id_rtc->PRER = prediv_a;
    rtc_lld_exit_init();
  }
}

/**
 * @brief   Set current time.
 * @note    Fractional part will be silently ignored. There is no possibility
 *          to set it on STM32 platform.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @api
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec) {
  (void)rtcp;

  rtc_lld_enter_init();
  if (timespec->h12)
    RTCD1.id_rtc->CR |= RTC_CR_FMT;
  else
    RTCD1.id_rtc->CR &= ~RTC_CR_FMT;
  RTCD1.id_rtc->TR = timespec->tv_time;
  RTCD1.id_rtc->DR = timespec->tv_date;
  rtc_lld_exit_init();
}

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCTime structure
 *
 * @api
 */
void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec) {
  (void)rtcp;

  rtc_lld_apb1_sync();

#if STM32_RTC_HAS_SUBSECONDS
  {
    uint32_t prer = RTCD1.id_rtc->PRER & 0x7FFF;
    uint32_t ssr  = RTCD1.id_rtc->SSR;
    timespec->tv_msec = (1000 * (prer - ssr)) / (prer + 1);
  }
#endif /* STM32_RTC_HAS_SUBSECONDS */
  timespec->tv_time = RTCD1.id_rtc->TR;
  timespec->tv_date = RTCD1.id_rtc->DR;
}

/**
 * @brief     Set alarm time.
 *
 * @note      Default value after BKP domain reset for both comparators is 0.
 * @note      Function does not performs any checks of alarm time validity.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier starting from zero
 * @param[in] alarmspec pointer to a @p RTCAlarm structure
 *
 * @api
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm *alarmspec) {

  if (alarm == 0) {
    if (alarmspec != NULL){
      rtcp->id_rtc->CR &= ~RTC_CR_ALRAE;
      while(!(rtcp->id_rtc->ISR & RTC_ISR_ALRAWF))
        ;
      rtcp->id_rtc->ALRMAR = alarmspec->tv_datetime;
      rtcp->id_rtc->CR |= RTC_CR_ALRAE;
      rtcp->id_rtc->CR |= RTC_CR_ALRAIE;
    }
    else {
      rtcp->id_rtc->CR &= ~RTC_CR_ALRAIE;
      rtcp->id_rtc->CR &= ~RTC_CR_ALRAE;
    }
  }
#if RTC_ALARMS == 2
  else{
    if (alarmspec != NULL){
      rtcp->id_rtc->CR &= ~RTC_CR_ALRBE;
      while(!(rtcp->id_rtc->ISR & RTC_ISR_ALRBWF))
        ;
      rtcp->id_rtc->ALRMBR = alarmspec->tv_datetime;
      rtcp->id_rtc->CR |= RTC_CR_ALRBE;
      rtcp->id_rtc->CR |= RTC_CR_ALRBIE;
    }
    else {
      rtcp->id_rtc->CR &= ~RTC_CR_ALRBIE;
      rtcp->id_rtc->CR &= ~RTC_CR_ALRBE;
    }
  }
#endif /* RTC_ALARMS == 2 */
}

/**
 * @brief   Get alarm time.
 *
 * @param[in] rtcp       pointer to RTC driver structure
 * @param[in] alarm      alarm identifier starting from zero
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @api
 */
void rtc_lld_get_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       RTCAlarm *alarmspec) {

  if (alarm == 0)
    alarmspec->tv_datetime = rtcp->id_rtc->ALRMAR;
#if RTC_ALARMS == 2
  else
    alarmspec->tv_datetime = rtcp->id_rtc->ALRMBR;
#endif
}

/**
 * @brief     Sets time of periodic wakeup.
 *
 * @note      Default value after BKP domain reset is 0x0000FFFF
 *
 * @param[in] rtcp       pointer to RTC driver structure
 * @param[in] wakeupspec pointer to a @p RTCWakeup structure
 *
 * @api
 */
#if RTC_HAS_PERIODIC_WAKEUPS
void rtcSetPeriodicWakeup_v2(RTCDriver *rtcp, const RTCWakeup *wakeupspec) {

  if (wakeupspec != NULL){
    chDbgCheck((wakeupspec->wakeup != 0x30000),
               "rtc_lld_set_periodic_wakeup, forbidden combination");

    rtcp->id_rtc->CR &= ~RTC_CR_WUTE;
    while(!(rtcp->id_rtc->ISR & RTC_ISR_WUTWF))
      ;
    rtcp->id_rtc->WUTR = wakeupspec->wakeup & 0xFFFF;
    rtcp->id_rtc->CR   = (wakeupspec->wakeup >> 16) & 0x7;
    rtcp->id_rtc->CR |= RTC_CR_WUTIE;
    rtcp->id_rtc->CR |= RTC_CR_WUTE;
  }
  else {
    rtcp->id_rtc->CR &= ~RTC_CR_WUTIE;
    rtcp->id_rtc->CR &= ~RTC_CR_WUTE;
  }
}
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

/**
 * @brief     Gets time of periodic wakeup.
 *
 * @note      Default value after BKP domain reset is 0x0000FFFF
 *
 * @param[in] rtcp        pointer to RTC driver structure
 * @param[out] wakeupspec pointer to a @p RTCWakeup structure
 *
 * @api
 */
#if RTC_HAS_PERIODIC_WAKEUPS
void rtcGetPeriodicWakeup_v2(RTCDriver *rtcp, RTCWakeup *wakeupspec) {

  wakeupspec->wakeup  = 0;
  wakeupspec->wakeup |= rtcp->id_rtc->WUTR;
  wakeupspec->wakeup |= (((uint32_t)rtcp->id_rtc->CR) & 0x7) << 16;
}
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

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
  RTCTime timespec;
  uint32_t tv_time;
  uint32_t tv_date;
  uint32_t v;

  chSysLock();
  rtcGetTimeI(rtcp, &timespec);
  chSysUnlock();

  tv_time = timespec.tv_time;
  tv_date = timespec.tv_date;

  v =  (tv_time & RTC_TR_SU) >> RTC_TR_SU_OFFSET;
  v += ((tv_time & RTC_TR_ST) >> RTC_TR_ST_OFFSET) * 10;
  fattime  = v >> 1;

  v =  (tv_time & RTC_TR_MNU) >> RTC_TR_MNU_OFFSET;
  v += ((tv_time & RTC_TR_MNT) >> RTC_TR_MNT_OFFSET) * 10;
  fattime |= v << 5;

  v =  (tv_time & RTC_TR_HU) >> RTC_TR_HU_OFFSET;
  v += ((tv_time & RTC_TR_HT) >> RTC_TR_HT_OFFSET) * 10;
  v += 12 * ((tv_time & RTC_TR_PM) >> RTC_TR_PM_OFFSET);
  fattime |= v << 11;

  v =  (tv_date & RTC_DR_DU) >> RTC_DR_DU_OFFSET;
  v += ((tv_date & RTC_DR_DT) >> RTC_DR_DT_OFFSET) * 10;
  fattime |= v << 16;

  v =  (tv_date & RTC_DR_MU) >> RTC_DR_MU_OFFSET;
  v += ((tv_date & RTC_DR_MT) >> RTC_DR_MT_OFFSET) * 10;
  fattime |= v << 21;

  v =  (tv_date & RTC_DR_YU) >> RTC_DR_YU_OFFSET;
  v += ((tv_date & RTC_DR_YT) >> RTC_DR_YT_OFFSET) * 10;
  v += 2000 - 1900 - 80;
  fattime |= v << 25;

  return fattime;
}

#endif /* HAL_USE_RTC */

/** @} */
