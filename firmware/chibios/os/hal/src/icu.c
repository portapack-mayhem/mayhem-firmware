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
 * @file    icu.c
 * @brief   ICU Driver code.
 *
 * @addtogroup ICU
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_ICU || defined(__DOXYGEN__)

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
 * @brief   ICU Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void icuInit(void) {

  icu_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p ICUDriver structure.
 *
 * @param[out] icup     pointer to the @p ICUDriver object
 *
 * @init
 */
void icuObjectInit(ICUDriver *icup) {

  icup->state  = ICU_STOP;
  icup->config = NULL;
}

/**
 * @brief   Configures and activates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 * @param[in] config    pointer to the @p ICUConfig object
 *
 * @api
 */
void icuStart(ICUDriver *icup, const ICUConfig *config) {

  chDbgCheck((icup != NULL) && (config != NULL), "icuStart");

  chSysLock();
  chDbgAssert((icup->state == ICU_STOP) || (icup->state == ICU_READY),
              "icuStart(), #1", "invalid state");
  icup->config = config;
  icu_lld_start(icup);
  icup->state = ICU_READY;
  chSysUnlock();
}

/**
 * @brief   Deactivates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @api
 */
void icuStop(ICUDriver *icup) {

  chDbgCheck(icup != NULL, "icuStop");

  chSysLock();
  chDbgAssert((icup->state == ICU_STOP) || (icup->state == ICU_READY),
              "icuStop(), #1", "invalid state");
  icu_lld_stop(icup);
  icup->state = ICU_STOP;
  chSysUnlock();
}

/**
 * @brief   Enables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @api
 */
void icuEnable(ICUDriver *icup) {

  chDbgCheck(icup != NULL, "icuEnable");

  chSysLock();
  chDbgAssert(icup->state == ICU_READY, "icuEnable(), #1", "invalid state");
  icu_lld_enable(icup);
  icup->state = ICU_WAITING;
  chSysUnlock();
}

/**
 * @brief   Disables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @api
 */
void icuDisable(ICUDriver *icup) {

  chDbgCheck(icup != NULL, "icuDisable");

  chSysLock();
  chDbgAssert((icup->state == ICU_READY) || (icup->state == ICU_WAITING) ||
              (icup->state == ICU_ACTIVE) || (icup->state == ICU_IDLE),
              "icuDisable(), #1", "invalid state");
  icu_lld_disable(icup);
  icup->state = ICU_READY;
  chSysUnlock();
}

#endif /* HAL_USE_ICU */

/** @} */
