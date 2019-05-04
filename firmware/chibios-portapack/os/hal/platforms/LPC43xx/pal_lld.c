/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    templates/pal_lld.c
 * @brief   PAL subsystem low level driver template.
 *
 * @addtogroup PAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

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
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/**
 * @brief   LPC43xx I/O ports configuration.
 * @details Ports 0 through 8.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {
  for(size_t i=0; i<8; i++) {
    LPC_GPIO->PIN[i] = config->P[i].data;
    LPC_GPIO->DIR[i] = config->P[i].dir;
  }

  for(size_t i=0; i<ARRAY_SIZE(config->SCU); i++) {
    LPC_SCU->SFSP[config->SCU[i].port][config->SCU[i].pin] = config->SCU[i].config.word;
  }
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {
  /* TODO: Handling pull-up, pull-down modes not implemented, as it would
   * require a map from GPIO to SCU pins. Not today.
   */
  if( mode == PAL_MODE_OUTPUT_PUSHPULL ) {
    LPC_GPIO->DIR[port] |= mask;
  } else {
    LPC_GPIO->DIR[port] &= ~mask;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
