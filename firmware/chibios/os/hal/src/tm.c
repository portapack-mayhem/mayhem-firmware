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
 * @file    tm.c
 * @brief   Time Measurement driver code.
 *
 * @addtogroup TM
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_TM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Subsystem calibration value.
 */
static halrtcnt_t measurement_offset;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Starts a measurement.
 *
 * @param[in,out] tmp   pointer to a @p TimeMeasurement structure
 *
 * @notapi
 */
static void tm_start(TimeMeasurement *tmp) {

  tmp->last = halGetCounterValue();
}

/**
 * @brief   Stops a measurement.
 *
 * @param[in,out] tmp   pointer to a @p TimeMeasurement structure
 *
 * @notapi
 */
static void tm_stop(TimeMeasurement *tmp) {

  halrtcnt_t now = halGetCounterValue();
  tmp->last = now - tmp->last - measurement_offset;
  if (tmp->last > tmp->worst)
      tmp->worst = tmp->last;
  else if (tmp->last < tmp->best)
      tmp->best = tmp->last;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the Time Measurement unit.
 *
 * @init
 */
void tmInit(void) {
  TimeMeasurement tm;

  /* Time Measurement subsystem calibration, it does a null measurement
     and calculates the call overhead which is subtracted to real
     measurements.*/
  measurement_offset = 0;
  tmObjectInit(&tm);
  tmStartMeasurement(&tm);
  tmStopMeasurement(&tm);
  measurement_offset = tm.last;
}

/**
 * @brief   Initializes a @p TimeMeasurement object.
 *
 * @param[out] tmp      pointer to a @p TimeMeasurement structure
 *
 * @init
 */
void tmObjectInit(TimeMeasurement *tmp) {

  tmp->start = tm_start;
  tmp->stop  = tm_stop;
  tmp->last  = (halrtcnt_t)0;
  tmp->worst = (halrtcnt_t)0;
  tmp->best  = (halrtcnt_t)-1;
}

#endif /* HAL_USE_TM */

/** @} */
