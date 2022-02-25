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
 * @file    ext.c
 * @brief   EXT Driver code.
 *
 * @addtogroup EXT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

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
 * @brief   EXT Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void extInit(void) {

  ext_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p EXTDriver structure.
 *
 * @param[out] extp     pointer to the @p EXTDriver object
 *
 * @init
 */
void extObjectInit(EXTDriver *extp) {

  extp->state  = EXT_STOP;
  extp->config = NULL;
}

/**
 * @brief   Configures and activates the EXT peripheral.
 * @post    After activation all EXT channels are in the disabled state,
 *          use @p extChannelEnable() in order to activate them.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] config    pointer to the @p EXTConfig object
 *
 * @api
 */
void extStart(EXTDriver *extp, const EXTConfig *config) {

  chDbgCheck((extp != NULL) && (config != NULL), "extStart");

  chSysLock();
  chDbgAssert((extp->state == EXT_STOP) || (extp->state == EXT_ACTIVE),
              "extStart(), #1", "invalid state");
  extp->config = config;
  ext_lld_start(extp);
  extp->state = EXT_ACTIVE;
  chSysUnlock();
}

/**
 * @brief   Deactivates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @api
 */
void extStop(EXTDriver *extp) {

  chDbgCheck(extp != NULL, "extStop");

  chSysLock();
  chDbgAssert((extp->state == EXT_STOP) || (extp->state == EXT_ACTIVE),
              "extStop(), #1", "invalid state");
  ext_lld_stop(extp);
  extp->state = EXT_STOP;
  chSysUnlock();
}

/**
 * @brief   Enables an EXT channel.
 * @pre     The channel must not be in @p EXT_CH_MODE_DISABLED mode.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be enabled
 *
 * @api
 */
void extChannelEnable(EXTDriver *extp, expchannel_t channel) {

  chDbgCheck((extp != NULL) && (channel < EXT_MAX_CHANNELS),
             "extChannelEnable");

  chSysLock();
  chDbgAssert((extp->state == EXT_ACTIVE) &&
              ((extp->config->channels[channel].mode &
                EXT_CH_MODE_EDGES_MASK) != EXT_CH_MODE_DISABLED),
              "extChannelEnable(), #1", "invalid state");
  extChannelEnableI(extp, channel);
  chSysUnlock();
}

/**
 * @brief   Disables an EXT channel.
 * @pre     The channel must not be in @p EXT_CH_MODE_DISABLED mode.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be disabled
 *
 * @api
 */
void extChannelDisable(EXTDriver *extp, expchannel_t channel) {

  chDbgCheck((extp != NULL) && (channel < EXT_MAX_CHANNELS),
             "extChannelDisable");

  chSysLock();
  chDbgAssert((extp->state == EXT_ACTIVE) &&
              ((extp->config->channels[channel].mode &
                EXT_CH_MODE_EDGES_MASK) != EXT_CH_MODE_DISABLED),
              "extChannelDisable(), #1", "invalid state");
  extChannelDisableI(extp, channel);
  chSysUnlock();
}

/**
 * @brief   Changes the operation mode of a channel.
 * @note    This function attempts to write over the current configuration
 *          structure that must have been not declared constant. This
 *          violates the @p const qualifier in @p extStart() but it is
 *          intentional.
 * @note    This function cannot be used if the configuration structure is
 *          declared @p const.
 * @note    The effect of this function on constant configuration structures
 *          is not defined.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be changed
 * @param[in] extcp     new configuration for the channel
 *
 * @iclass
 */
void extSetChannelModeI(EXTDriver *extp,
                        expchannel_t channel,
                        const EXTChannelConfig *extcp) {
  EXTChannelConfig *oldcp;

  chDbgCheck((extp != NULL) && (channel < EXT_MAX_CHANNELS) &&
             (extcp != NULL), "extSetChannelModeI");

  chDbgAssert(extp->state == EXT_ACTIVE,
              "extSetChannelModeI(), #1", "invalid state");

  /* Note that here the access is enforced as non-const, known access
     violation.*/
  oldcp = (EXTChannelConfig *)&extp->config->channels[channel];

  /* Overwiting the old channels configuration then the channel is reconfigured
     by the low level driver.*/
  *oldcp = *extcp;
  ext_lld_channel_enable(extp, channel);
}

#endif /* HAL_USE_EXT */

/** @} */
