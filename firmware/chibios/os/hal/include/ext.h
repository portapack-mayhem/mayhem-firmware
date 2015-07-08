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
 * @file    ext.h
 * @brief   EXT Driver macros and structures.
 *
 * @addtogroup EXT
 * @{
 */

#ifndef _EXT_H_
#define _EXT_H_

#if HAL_USE_EXT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    EXT channel modes
 * @{
 */
#define EXT_CH_MODE_EDGES_MASK      3   /**< @brief Mask of edges field.    */
#define EXT_CH_MODE_DISABLED        0   /**< @brief Channel disabled.       */
#define EXT_CH_MODE_RISING_EDGE     1   /**< @brief Rising edge callback.   */
#define EXT_CH_MODE_FALLING_EDGE    2   /**< @brief Falling edge callback.  */
#define EXT_CH_MODE_BOTH_EDGES      3   /**< @brief Both edges callback.    */

#define EXT_CH_MODE_AUTOSTART       4   /**< @brief Channel started
                                             automatically on driver start. */
/** @} */

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
  EXT_UNINIT = 0,                   /**< Not initialized.                   */
  EXT_STOP = 1,                     /**< Stopped.                           */
  EXT_ACTIVE = 2,                   /**< Active.                            */
} extstate_t;

/**
 * @brief   Type of a structure representing a EXT driver.
 */
typedef struct EXTDriver EXTDriver;

#include "ext_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Enables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be enabled
 *
 * @iclass
 */
#define extChannelEnableI(extp, channel) ext_lld_channel_enable(extp, channel)

/**
 * @brief   Disables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be disabled
 *
 * @iclass
 */
#define extChannelDisableI(extp, channel) ext_lld_channel_disable(extp, channel)

/**
 * @brief   Changes the operation mode of a channel.
 * @note    This function attempts to write over the current configuration
 *          structure that must have been not declared constant. This
 *          violates the @p const qualifier in @p extStart() but it is
 *          intentional. This function cannot be used if the configuration
 *          structure is declared @p const.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be changed
 * @param[in] extcp     new configuration for the channel
 *
 * @api
 */
#define extSetChannelMode(extp, channel, extcp) {                           \
  chSysLock();                                                              \
  extSetChannelModeI(extp, channel, extcp);                                 \
  chSysUnlock();                                                            \
}

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void extInit(void);
  void extObjectInit(EXTDriver *extp);
  void extStart(EXTDriver *extp, const EXTConfig *config);
  void extStop(EXTDriver *extp);
  void extChannelEnable(EXTDriver *extp, expchannel_t channel);
  void extChannelDisable(EXTDriver *extp, expchannel_t channel);
  void extSetChannelModeI(EXTDriver *extp,
                          expchannel_t channel,
                          const EXTChannelConfig *extcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EXT */

#endif /* _EXT_H_ */

/** @} */
