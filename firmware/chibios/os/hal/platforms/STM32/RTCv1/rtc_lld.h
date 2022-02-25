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
 * @file    STM32/RTCv1/rtc_lld.h
 * @brief   STM32F1xx RTC subsystem low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#ifndef _RTC_LLD_H_
#define _RTC_LLD_H_

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   This RTC implementation supports callbacks.
 */
#define RTC_SUPPORTS_CALLBACKS      TRUE

/**
 * @brief   One alarm comparator available.
 */
#define RTC_ALARMS                  1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/*
 * RTC driver system settings.
 */
#define STM32_RTC_IRQ_PRIORITY      15
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if HAL_USE_RTC && !STM32_HAS_RTC
#error "RTC not present in the selected device"
#endif

#if STM32_RTCCLK == 0
#error "RTC clock not enabled"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an RTC alarm time stamp.
 */
typedef struct RTCAlarm RTCAlarm;

/**
 * @brief   Type of a structure representing an RTC callbacks config.
 */
typedef struct RTCCallbackConfig RTCCallbackConfig;

/**
 * @brief   Type of an RTC alarm.
 * @details Meaningful on platforms with more than 1 alarm comparator.
 */
typedef uint32_t rtcalarm_t;

/**
 * @brief   Type of an RTC event.
 */
typedef enum {
  RTC_EVENT_SECOND = 0,                 /** Triggered every second.         */
  RTC_EVENT_ALARM = 1,                  /** Triggered on alarm.             */
  RTC_EVENT_OVERFLOW = 2                /** Triggered on counter overflow.  */
} rtcevent_t;

/**
 * @brief   Type of a generic RTC callback.
 */
typedef void (*rtccb_t)(RTCDriver *rtcp, rtcevent_t event);

/**
 * @brief   Structure representing an RTC callbacks config.
 */
struct RTCCallbackConfig{
  /**
   * @brief Generic RTC callback pointer.
   */
  rtccb_t           callback;
};

/**
 * @brief   Structure representing an RTC time stamp.
 */
struct RTCTime {
  /**
   * @brief Seconds since UNIX epoch.
   */
  uint32_t tv_sec;
  /**
   * @brief Fractional part.
   */
  uint32_t tv_msec;
};

/**
 * @brief   Structure representing an RTC alarm time stamp.
 */
struct RTCAlarm {
  /**
   * @brief Seconds since UNIX epoch.
   */
  uint32_t          tv_sec;
};

/**
 * @brief   Structure representing an RTC driver.
 */
struct RTCDriver{
  /**
   * @brief Callback pointer.
   */
  rtccb_t           callback;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern RTCDriver RTCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void rtc_lld_set_prescaler(void);
  void rtc_lld_init(void);
  void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec);
  void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec);
  void rtc_lld_set_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         const RTCAlarm *alarmspec);
  void rtc_lld_get_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         RTCAlarm *alarmspec);
  void rtc_lld_set_callback(RTCDriver *rtcp, rtccb_t callback);
  uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* _RTC_LLD_H_ */

/** @} */
