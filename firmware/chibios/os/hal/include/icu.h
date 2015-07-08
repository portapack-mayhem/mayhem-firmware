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

/**
 * @file    icu.h
 * @brief   ICU Driver macros and structures.
 *
 * @addtogroup ICU
 * @{
 */

#ifndef _ICU_H_
#define _ICU_H_

#if HAL_USE_ICU || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

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
 * @brief   Driver state machine possible states.
 */
typedef enum {
  ICU_UNINIT = 0,                   /**< Not initialized.                   */
  ICU_STOP = 1,                     /**< Stopped.                           */
  ICU_READY = 2,                    /**< Ready.                             */
  ICU_WAITING = 3,                  /**< Waiting first edge.                */
  ICU_ACTIVE = 4,                   /**< Active cycle phase.                */
  ICU_IDLE = 5,                     /**< Idle cycle phase.                  */
} icustate_t;

/**
 * @brief   Type of a structure representing an ICU driver.
 */
typedef struct ICUDriver ICUDriver;

/**
 * @brief   ICU notification callback type.
 *
 * @param[in] icup      pointer to a @p ICUDriver object
 */
typedef void (*icucallback_t)(ICUDriver *icup);

#include "icu_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Enables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @iclass
 */
#define icuEnableI(icup) icu_lld_enable(icup)

/**
 * @brief   Disables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @iclass
 */
#define icuDisableI(icup) icu_lld_disable(icup)

/**
 * @brief   Returns the width of the latest pulse.
 * @details The pulse width is defined as number of ticks between the start
 *          edge and the stop edge.
 * @note    This function is meant to be invoked from the width capture
 *          callback only.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 * @return              The number of ticks.
 *
 * @special
 */
#define icuGetWidth(icup) icu_lld_get_width(icup)

/**
 * @brief   Returns the width of the latest cycle.
 * @details The cycle width is defined as number of ticks between a start
 *          edge and the next start edge.
 * @note    This function is meant to be invoked from the width capture
 *          callback only.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 * @return              The number of ticks.
 *
 * @special
 */
#define icuGetPeriod(icup) icu_lld_get_period(icup)
/** @} */

/**
 * @name    Low Level driver helper macros
 * @{
 */
/**
 * @brief   Common ISR code, ICU width event.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
#define _icu_isr_invoke_width_cb(icup) {                                    \
  if ((icup)->state != ICU_WAITING) {                                       \
    (icup)->state = ICU_IDLE;                                               \
    (icup)->config->width_cb(icup);                                         \
  }                                                                         \
}

/**
 * @brief   Common ISR code, ICU period event.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
#define _icu_isr_invoke_period_cb(icup) {                                   \
  icustate_t previous_state = (icup)->state;                                \
  (icup)->state = ICU_ACTIVE;                                               \
  if (previous_state != ICU_WAITING)                                        \
    (icup)->config->period_cb(icup);                                        \
}

/**
 * @brief   Common ISR code, ICU timer overflow event.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
#define _icu_isr_invoke_overflow_cb(icup) {                                 \
  (icup)->config->overflow_cb(icup);                                        \
}
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void icuInit(void);
  void icuObjectInit(ICUDriver *icup);
  void icuStart(ICUDriver *icup, const ICUConfig *config);
  void icuStop(ICUDriver *icup);
  void icuEnable(ICUDriver *icup);
  void icuDisable(ICUDriver *icup);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ICU */

#endif /* _ICU_H_ */

/** @} */
