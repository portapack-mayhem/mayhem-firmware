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
 * @file    LPC13xx/pal_lld.c
 * @brief   LPC13xx GPIO low level driver code.
 *
 * @addtogroup PAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

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
/**
 * @brief   LPC13xx I/O ports configuration.
 * @details GPIO unit registers initialization.
 *
 * @param[in] config    the LPC13xx ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  LPC_GPIO0->DIR = config->P0.dir;
  LPC_GPIO1->DIR = config->P1.dir;
  LPC_GPIO2->DIR = config->P2.dir;
  LPC_GPIO3->DIR = config->P3.dir;
  LPC_GPIO0->DATA = config->P0.data;
  LPC_GPIO1->DATA = config->P1.data;
  LPC_GPIO2->DATA = config->P2.data;
  LPC_GPIO3->DATA = config->P3.data;
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull output with
 *          high state.
 * @note    This function does not alter the @p PINSELx registers. Alternate
 *          functions setup must be handled by device-specific code.
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

  switch (mode) {
  case PAL_MODE_RESET:
  case PAL_MODE_INPUT:
    port->DIR &= ~mask;
    break;
  case PAL_MODE_UNCONNECTED:
    palSetPort(port, PAL_WHOLE_PORT);
  case PAL_MODE_OUTPUT_PUSHPULL:
    port->DIR |=  mask;
    break;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
