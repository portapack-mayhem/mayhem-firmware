/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    rtc.h
 * @brief   RTC Driver macros and structures.
 *
 * @addtogroup RTC
 * @{
 */

#ifndef _RTC_H_
#define _RTC_H_

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Date/Time bit masks
 * @{
 */
#define RTC_TIME_SECONDS_MASK   0x0000001F  /* @brief Seconds mask.         */
#define RTC_TIME_MINUTES_MASK   0x000007E0  /* @brief Minutes mask.         */
#define RTC_TIME_HOURS_MASK     0x0000F800  /* @brief Hours mask.           */
#define RTC_DATE_DAYS_MASK      0x001F0000  /* @brief Days mask.            */
#define RTC_DATE_MONTHS_MASK    0x01E00000  /* @brief Months mask.          */
#define RTC_DATE_YEARS_MASK     0xFE000000  /* @brief Years mask.           */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an RTC driver.
 */
typedef struct RTCDriver RTCDriver;

/**
 * @brief   Type of a structure representing an RTC time stamp.
 */
typedef struct RTCTime RTCTime;

#include "rtc_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Set current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @iclass
 */
#define rtcSetTimeI(rtcp, timespec) rtc_lld_set_time(rtcp, timespec)

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCTime structure
 *
 * @iclass
 */
#define rtcGetTimeI(rtcp, timespec) rtc_lld_get_time(rtcp, timespec)

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Set alarm time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[in] alarmspec pointer to a @p RTCAlarm structure or @p NULL
 *
 * @iclass
 */
#define rtcSetAlarmI(rtcp, alarm, alarmspec)                                \
  rtc_lld_set_alarm(rtcp, alarm, alarmspec)

/**
 * @brief   Get current alarm.
 * @note    If an alarm has not been set then the returned alarm specification
 *          is not meaningful.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @iclass
 */
#define rtcGetAlarmI(rtcp, alarm, alarmspec)                                \
  rtc_lld_get_alarm(rtcp, alarm, alarmspec)
#endif /* RTC_ALARMS > 0 */

#if RTC_SUPPORTS_CALLBACKS || defined(__DOXYGEN__)
/**
 * @brief   Enables or disables RTC callbacks.
 * @details This function enables or disables the callback, use a @p NULL
 *          pointer in order to disable it.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] callback  callback function pointer or @p NULL
 *
 * @iclass
 */
#define rtcSetCallbackI(rtcp, callback) rtc_lld_set_callback(rtcp, callback)
#endif /* RTC_SUPPORTS_CALLBACKS */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void rtcInit(void);
  void rtcSetTime(RTCDriver *rtcp, const RTCTime *timespec);
  void rtcGetTime(RTCDriver *rtcp, RTCTime *timespec);
#if RTC_ALARMS > 0
  void rtcSetAlarm(RTCDriver *rtcp,
                   rtcalarm_t alarm,
                   const RTCAlarm *alarmspec);
  void rtcGetAlarm(RTCDriver *rtcp, rtcalarm_t alarm, RTCAlarm *alarmspec);
#endif
  uint32_t rtcGetTimeFat(RTCDriver *rtcp);
#if RTC_SUPPORTS_CALLBACKS
  void rtcSetCallback(RTCDriver *rtcp, rtccb_t callback);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */
#endif /* _RTC_H_ */

/** @} */
