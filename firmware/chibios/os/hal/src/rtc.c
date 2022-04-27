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
 * @file    rtc.c
 * @brief   RTC Driver code.
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

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   RTC Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void rtcInit(void) {

  rtc_lld_init();
}

/**
 * @brief   Set current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @api
 */
void rtcSetTime(RTCDriver *rtcp, const RTCTime *timespec) {

  chDbgCheck((rtcp != NULL) && (timespec != NULL), "rtcSetTime");

  chSysLock();
  rtcSetTimeI(rtcp, timespec);
  chSysUnlock();
}

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCTime structure
 *
 * @api
 */
void rtcGetTime(RTCDriver *rtcp, RTCTime *timespec) {

  chDbgCheck((rtcp != NULL) && (timespec != NULL), "rtcGetTime");

  chSysLock();
  rtcGetTimeI(rtcp, timespec);
  chSysUnlock();
}

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Set alarm time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[in] alarmspec pointer to a @p RTCAlarm structure or @p NULL
 *
 * @api
 */
void rtcSetAlarm(RTCDriver *rtcp,
                 rtcalarm_t alarm,
                 const RTCAlarm *alarmspec) {

  chDbgCheck((rtcp != NULL) && (alarm < RTC_ALARMS), "rtcSetAlarm");

  chSysLock();
  rtcSetAlarmI(rtcp, alarm, alarmspec);
  chSysUnlock();
}

/**
 * @brief   Get current alarm.
 * @note    If an alarm has not been set then the returned alarm specification
 *          is not meaningful.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @api
 */
void rtcGetAlarm(RTCDriver *rtcp,
                 rtcalarm_t alarm,
                 RTCAlarm *alarmspec) {

  chDbgCheck((rtcp != NULL) && (alarm < RTC_ALARMS) && (alarmspec != NULL),
             "rtcGetAlarm");

  chSysLock();
  rtcGetAlarmI(rtcp, alarm, alarmspec);
  chSysUnlock();
}
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
 * @api
 */
void rtcSetCallback(RTCDriver *rtcp, rtccb_t callback) {

  chDbgCheck((rtcp != NULL), "rtcSetCallback");

  chSysLock();
  rtcSetCallbackI(rtcp, callback);
  chSysUnlock();
}
#endif /* RTC_SUPPORTS_CALLBACKS */

/**
 * @brief   Get current time in format suitable for usage in FatFS.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @return              FAT time value.
 *
 * @api
 */
uint32_t rtcGetTimeFat(RTCDriver *rtcp) {

  chDbgCheck((rtcp != NULL), "rtcSetTime");
  return rtc_lld_get_time_fat(rtcp);
}

#endif /* HAL_USE_RTC */

/** @} */
