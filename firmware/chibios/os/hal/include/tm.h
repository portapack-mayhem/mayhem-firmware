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
 * @file    tm.h
 * @brief   Time Measurement driver header.
 *
 * @addtogroup TM
 * @{
 */

#ifndef _TM_H_
#define _TM_H_

#if HAL_USE_TM || defined(__DOXYGEN__)

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
 * @brief   Type of a Time Measurement object.
 * @note    Start/stop of measurements is performed through the function
 *          pointers in order to avoid inlining of those functions which
 *          could compromise measurement accuracy.
 * @note    The maximum measurable time period depends on the implementation
 *          of the realtime counter in the HAL driver.
 * @note    The measurement is not 100% cycle-accurate, it can be in excess
 *          of few cycles depending on the compiler and target architecture.
 * @note    Interrupts can affect measurement if the measurement is performed
 *          with interrupts enabled.
 */
typedef struct TimeMeasurement TimeMeasurement;

/**
 * @brief   Time Measurement structure.
 */
struct TimeMeasurement {
  void (*start)(TimeMeasurement *tmp);  /**< @brief Starts a measurement.   */
  void (*stop)(TimeMeasurement *tmp);   /**< @brief Stops a measurement.    */
  halrtcnt_t           last;            /**< @brief Last measurement.       */
  halrtcnt_t           worst;           /**< @brief Worst measurement.      */
  halrtcnt_t           best;            /**< @brief Best measurement.       */
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Starts a measurement.
 * @pre     The @p TimeMeasurement must be initialized.
 * @note    This function can be invoked in any context.
 *
 * @param[in,out] tmp   pointer to a @p TimeMeasurement structure
 *
 * @special
 */
#define tmStartMeasurement(tmp) (tmp)->start(tmp)

/**
 * @brief   Stops a measurement.
 * @pre     The @p TimeMeasurement must be initialized.
 * @note    This function can be invoked in any context.
 *
 * @param[in,out] tmp   pointer to a @p TimeMeasurement structure
 *
 * @special
 */
#define tmStopMeasurement(tmp) (tmp)->stop(tmp)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void tmInit(void);
  void tmObjectInit(TimeMeasurement *tmp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_TM */

#endif /* _TM_H_ */

/** @} */
