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
 * @file    STM32/RTCv2/rtc_lld.h
 * @brief   RTC low level driver header.
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
 * @brief   Two alarm comparators available on STM32F4x and STM32F2x.
 */
#if !defined(STM32F0XX)
#define RTC_ALARMS                2
#else
#define RTC_ALARMS                1
#endif

/**
 * @brief   STM32F0x has no periodic wakeups.
 */
#if !defined(STM32F0XX)
#define RTC_HAS_PERIODIC_WAKEUPS  TRUE
#else
#define RTC_HAS_PERIODIC_WAKEUPS  FALSE
#endif

/**
 * @brief   Data offsets in RTC date and time registers.
 */
#define RTC_TR_PM_OFFSET    22
#define RTC_TR_HT_OFFSET    20
#define RTC_TR_HU_OFFSET    16
#define RTC_TR_MNT_OFFSET   12
#define RTC_TR_MNU_OFFSET   8
#define RTC_TR_ST_OFFSET    4
#define RTC_TR_SU_OFFSET    0

#define RTC_DR_YT_OFFSET    20
#define RTC_DR_YU_OFFSET    16
#define RTC_DR_WDU_OFFSET   13
#define RTC_DR_MT_OFFSET    12
#define RTC_DR_MU_OFFSET    8
#define RTC_DR_DT_OFFSET    4
#define RTC_DR_DU_OFFSET    0

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if HAL_USE_RTC && !STM32_HAS_RTC
#error "RTC not present in the selected device"
#endif

#if !(STM32_RTCSEL == STM32_RTCSEL_LSE) &&                                  \
    !(STM32_RTCSEL == STM32_RTCSEL_LSI) &&                                  \
    !(STM32_RTCSEL == STM32_RTCSEL_HSEDIV)
#error "invalid source selected for RTC clock"
#endif

#if !defined(RTC_USE_INTERRUPTS) || defined(__DOXYGEN__)
#define RTC_USE_INTERRUPTS                FALSE
#endif

#if defined(STM32_PCLK1) /* For devices without STM32_PCLK1 (STM32F0xx) */
#if STM32_PCLK1 < (STM32_RTCCLK * 7)
#error "STM32_PCLK1 frequency is too low to handle RTC without ugly workaround"
#endif
#endif /* defined(STM32_PCLK1) */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an RTC alarm time stamp.
 */
typedef struct RTCAlarm RTCAlarm;

/**
 * @brief   Type of a structure representing an RTC wakeup period.
 */
typedef struct RTCWakeup RTCWakeup;

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
 * @brief   Structure representing an RTC time stamp.
 */
struct RTCTime {
  /**
   * @brief RTC date register in STM32 BCD format.
   */
  uint32_t tv_date;
  /**
   * @brief RTC time register in STM32 BCD format.
   */
  uint32_t tv_time;
  /**
   * @brief Set this to TRUE to use 12 hour notation.
   */
  bool_t h12;
  /**
   * @brief Fractional part of time.
   */
#if STM32_RTC_HAS_SUBSECONDS
  uint32_t tv_msec;
#endif
};

/**
 * @brief   Structure representing an RTC alarm time stamp.
 */
struct RTCAlarm {
  /**
   * @brief Date and time of alarm in STM32 BCD.
   */
  uint32_t tv_datetime;
};

#if RTC_HAS_PERIODIC_WAKEUPS
/**
 * @brief   Structure representing an RTC periodic wakeup period.
 */
struct RTCWakeup {
  /**
   * @brief   RTC WUTR register.
   * @details Bits [15:0] contain value of WUTR register
   *          Bits [18:16] contain value of WUCKSEL bits in CR register
   *
   * @note    ((WUTR == 0) || (WUCKSEL == 3)) is forbidden combination.
   */
  uint32_t wakeup;
};
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

/**
 * @brief   Structure representing an RTC driver.
 */
struct RTCDriver{
  /**
   * @brief Pointer to the RTC registers block.
   */
  RTC_TypeDef               *id_rtc;
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
  void rtc_lld_init(void);
  void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec);
  void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec);
  void rtc_lld_set_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         const RTCAlarm *alarmspec);
  void rtc_lld_get_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         RTCAlarm *alarmspec);
#if RTC_HAS_PERIODIC_WAKEUPS
  void rtcSetPeriodicWakeup_v2(RTCDriver *rtcp, const RTCWakeup *wakeupspec);
  void rtcGetPeriodicWakeup_v2(RTCDriver *rtcp, RTCWakeup *wakeupspec);
#endif /* RTC_HAS_PERIODIC_WAKEUPS */
  uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* _RTC_LLD_H_ */

/** @} */
