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
 * @file    chrtclib.h
 * @brief   RTC time conversion utilities header.
 *
 * @addtogroup chrtclib
 * @{
 */

#ifndef CHRTCLIB_H_
#define CHRTCLIB_H_

#include <time.h>

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#if !STM32_RTC_IS_CALENDAR
  uint32_t rtcGetTimeFat(RTCDriver *rtcp);
#endif
  void rtcGetTimeTm(RTCDriver *rtcp, struct tm *timp);
  void rtcSetTimeTm(RTCDriver *rtcp, struct tm *timp);
  time_t rtcGetTimeUnixSec(RTCDriver *rtcp);
  uint64_t rtcGetTimeUnixUsec(RTCDriver *rtcp);
  void rtcSetTimeUnixSec(RTCDriver *rtcp, time_t tv_sec);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* CHRTCLIB_H_ */

/** @} */
