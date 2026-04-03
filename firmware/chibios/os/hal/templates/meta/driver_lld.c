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

/**
 * @file    templates/xxx_lld.c
 * @brief   XXX Driver subsystem low level driver source template.
 *
 * @addtogroup XXX
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_XXX || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   XXX1 driver identifier.
 */
#if PLATFORM_XXX_USE_XXX1 || defined(__DOXYGEN__)
XXXDriver XXXD1;
#endif

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level XXX driver initialization.
 *
 * @notapi
 */
void xxx_lld_init(void) {

#if PLATFORM_XXX_USE_XXX1
  /* Driver initialization.*/
  xxxObjectInit(&XXXD1);
#endif /* PLATFORM_XXX_USE_XXX1 */
}

/**
 * @brief   Configures and activates the XXX peripheral.
 *
 * @param[in] xxxp      pointer to the @p XXXDriver object
 *
 * @notapi
 */
void xxx_lld_start(XXXDriver *xxxp) {

  if (xxxp->state == XXX_STOP) {
    /* Enables the peripheral.*/
#if PLATFORM_XXX_USE_XXX1
    if (&XXXD1 == xxxp) {

    }
#endif /* PLATFORM_XXX_USE_XXX1 */
  }
  /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the XXX peripheral.
 *
 * @param[in] xxxp      pointer to the @p XXXDriver object
 *
 * @notapi
 */
void xxx_lld_stop(XXXDriver *xxxp) {

  if (xxxp->state == XXX_READY) {
    /* Resets the peripheral.*/

    /* Disables the peripheral.*/
#if PLATFORM_XXX_USE_XXX1
    if (&XXXD1 == xxxp) {

    }
#endif /* PLATFORM_XXX_USE_XXX1 */
  }
}

#endif /* HAL_USE_XXX */

/** @} */
