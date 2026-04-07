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
 * @file    templates/xxx_lld.h
 * @brief   XXX Driver subsystem low level driver header template.
 *
 * @addtogroup XXX
 * @{
 */

#ifndef _XXX_LLD_H_
#define _XXX_LLD_H_

#if HAL_USE_XXX || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   XXX driver enable switch.
 * @details If set to @p TRUE the support for XXX1 is included.
 */
#if !defined(PLATFORM_XXX_USE_XXX1) || defined(__DOXYGEN__)
#define PLATFORM_XXX_USE_XXX1             FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an XXX driver.
 */
typedef struct XXXDriver XXXDriver;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {

} XXXConfig;

/**
 * @brief   Structure representing an XXX driver.
 */
struct XXXDriver {
  /**
   * @brief Driver state.
   */
  xxxstate_t                state;
  /**
   * @brief Current configuration data.
   */
  const XXXConfig           *config;
  /* End of the mandatory fields.*/
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if PLATFORM_XXX_USE_XXX1 && !defined(__DOXYGEN__)
extern XXXDriver XXXD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void xxx_lld_init(void);
  void xxx_lld_start(XXXDriver *xxxp);
  void xxx_lld_stop(XXXDriver *xxxp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_XXX */

#endif /* _XXX_LLD_H_ */

/** @} */
