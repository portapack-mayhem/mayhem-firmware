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
 * @file    xxx.h
 * @brief   XXX Driver macros and structures.
 *
 * @addtogroup XXX
 * @{
 */

#ifndef _XXX_H_
#define _XXX_H_

#if HAL_USE_XXX || defined(__DOXYGEN__)

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
  XXX_UNINIT = 0,                   /**< Not initialized.                   */
  XXX_STOP = 1,                     /**< Stopped.                           */
  XXX_READY = 2,                    /**< Ready.                             */
} xxxstate_t;

/**
 * @brief   Type of a structure representing a XXX driver.
 */
typedef struct XXXDriver XXXDriver;

#include "xxx_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void xxxInit(void);
  void xxxObjectInit(XXXDriver *xxxp);
  void xxxStart(XXXDriver *xxxp, const XXXConfig *config);
  void xxxStop(XXXDriver *xxxp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_XXX */

#endif /* _XXX_H_ */

/** @} */
