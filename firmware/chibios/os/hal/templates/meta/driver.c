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
 * @file    xxx.c
 * @brief   XXX Driver code.
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

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   XXX Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void xxxInit(void) {

  xxx_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p XXXDriver structure.
 *
 * @param[out] xxxp     pointer to the @p XXXDriver object
 *
 * @init
 */
void xxxObjectInit(XXXDriver *xxxp) {

  xxxp->state  = XXX_STOP;
  xxxp->config = NULL;
}

/**
 * @brief   Configures and activates the XXX peripheral.
 *
 * @param[in] xxxp      pointer to the @p XXXDriver object
 * @param[in] config    pointer to the @p XXXConfig object
 *
 * @api
 */
void xxxStart(XXXDriver *xxxp, const XXXConfig *config) {

  chDbgCheck((xxxp != NULL) && (config != NULL), "xxxStart");

  chSysLock();
  chDbgAssert((xxxp->state == XXX_STOP) || (xxxp->state == XXX_READY),
              "xxxStart(), #1", "invalid state");
  xxxp->config = config;
  xxx_lld_start(xxxp);
  xxxp->state = XXX_READY;
  chSysUnlock();
}

/**
 * @brief   Deactivates the XXX peripheral.
 *
 * @param[in] xxxp      pointer to the @p XXXDriver object
 *
 * @api
 */
void xxxStop(XXXDriver *xxxp) {

  chDbgCheck(xxxp != NULL, "xxxStop");

  chSysLock();
  chDbgAssert((xxxp->state == XXX_STOP) || (xxxp->state == XXX_READY),
              "xxxStop(), #1", "invalid state");
  xxx_lld_stop(xxxp);
  xxxp->state = XXX_STOP;
  chSysUnlock();
}

#endif /* HAL_USE_XXX */

/** @} */
