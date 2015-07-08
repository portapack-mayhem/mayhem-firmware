/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    LPC43xx/rtc_lld.c
 * @brief   RTC low level driver.
 *
 * @addtogroup RTC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_RTC || defined(__DOXYGEN__)

#include "stdlib.h"

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

static bool clock_32k768_is_running(void) {
  const uint32_t creg0_masked = LPC_CREG->CREG0 & ((1U << 3) | (1U << 2));
  const uint32_t creg0_expected = 0;
  return (creg0_masked == creg0_expected);
}

static void clock_32k768_init(void) {
  // Reset and enable 32.768kHz oscillator
  LPC_CREG->CREG0 &= ~((1U << 3) | (1U << 2));
}

static void rtc_enable(LPC_RTC_Type* const rtc) {
  /* Enable counter, release internal divider from reset */
  rtc->CCR = (rtc->CCR & ~(1U << 1)) | (1U << 0);
}

static void rtc_disable_for_set(LPC_RTC_Type* const rtc) {
  /* Disable counter, hold internal divider in reset */
  rtc->CCR = (rtc->CCR & ~(1U << 0)) | (1U << 1);
}

static bool rtc_calibration_enabled(LPC_RTC_Type* const rtc) {
  return (rtc->CCR & (1U << 4)) == 0;
}

static void rtc_calibration_enable(LPC_RTC_Type* const rtc) {
  rtc->CALIBRATION = 0;
  rtc->CCR &= ~(1U << 4);
}
#if 0
static void rtc_calibration_set(LPC_RTC_Type* const rtc, const int32_t period) {
  rtc->CALIBRATION =
      (abs(period) & 0x1ffff)
    | ((period < 0) ? 1 : 0);
}
#endif
static void rtc_interrupts_disable(LPC_RTC_Type* const rtc) {
  rtc->CIIR = 0;
}

static void rtc_interrupts_clear(LPC_RTC_Type* const rtc) {
  rtc->ILR =
      (1U << 0)
    | (1U << 1)
    ;
}

static uint_fast8_t timespec_sec(const RTCTime* const timespec) {
  return (timespec->tv_time >>  0) & 0x03f;
}

static uint_fast8_t timespec_min(const RTCTime* const timespec) {
  return (timespec->tv_time >>  8) & 0x03f;
}

static uint_fast8_t timespec_hrs(const RTCTime* const timespec) {
  return (timespec->tv_time >> 16) & 0x01f;
}

static uint_fast8_t timespec_dow(const RTCTime* const timespec) {
  return (timespec->tv_time >> 24) & 0x007;
}

static uint_fast8_t timespec_dom(const RTCTime* const timespec) {
  return (timespec->tv_date >>  0) & 0x01f;
}

static uint_fast8_t timespec_month(const RTCTime* const timespec) {
  return (timespec->tv_date >>  8) & 0x00f;
}

static uint_fast16_t timespec_year(const RTCTime* const timespec) {
  return (timespec->tv_date >> 16) & 0xfff;
}

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
    RTCD1.rtc = LPC_RTC;

    /* NOTE: Before enabling RTC, ensure that 32.768kHz clock has been enabled
     * for at least two seconds.
     */
    if( !clock_32k768_is_running() ) {
        clock_32k768_init();

        /* NOTE: This will be called while MCU clock is at 12MHz (IRC) */
        halPolledDelay(2000 * 12000);
    }

    /* Disable 32kHz output */
    LPC_CREG->CREG0 &= ~(1U << 1);

    /* Enable 1kHz output */
    LPC_CREG->CREG0 |= (1U << 0);

    rtc_interrupts_disable(LPC_RTC);
    rtc_interrupts_clear(LPC_RTC);

    if( !rtc_calibration_enabled(LPC_RTC) ) {
      rtc_calibration_enable(LPC_RTC);
    }

    rtc_enable(LPC_RTC);
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
    LPC_RTC_Type* const rtc = rtcp->rtc;

    rtc_disable_for_set(rtc);
    rtc->SEC   = timespec_sec(timespec);
    rtc->MIN   = timespec_min(timespec);
    rtc->HRS   = timespec_hrs(timespec);
    rtc->DOW   = timespec_dow(timespec);
    rtc->DOM   = timespec_dom(timespec);
    rtc->MONTH = timespec_month(timespec);
    rtc->YEAR  = timespec_year(timespec);
    rtc_enable(rtc);
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
    LPC_RTC_Type* const rtc = rtcp->rtc;

    /* Read time and date until two consecutive reads return the same values.
     */
    do {
      timespec->tv_time = rtc->CTIME0;
      timespec->tv_date = rtc->CTIME1;
    } while( (timespec->tv_time != rtc->CTIME0) || (timespec->tv_date != rtc->CTIME1) );
}

/**
 * @brief   Get current time in format suitable for usage in FatFS.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @return              FAT time value.
 *
 * @api
 */
uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp) {
  RTCTime timespec;
  rtc_lld_get_time(rtcp, &timespec);

  const int32_t year = (int32_t)timespec_year(&timespec) - 1980;

  uint32_t fattime = (year > 0) ? year : 0;
  fattime <<= 4;
  fattime |= timespec_month(&timespec);
  fattime <<= 5;
  fattime |= timespec_dom(&timespec);
  fattime <<= 5;
  fattime |= timespec_hrs(&timespec);
  fattime <<= 6;
  fattime |= timespec_min(&timespec);
  fattime <<= 5;
  fattime |= timespec_sec(&timespec) >> 1;

  return fattime;
}

#endif /* HAL_USE_RTC */

/** @} */
