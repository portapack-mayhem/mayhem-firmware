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
 * @file    MSP430/pal_lld.c
 * @brief   MSP430 Digital I/O low level driver code.
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
 * @brief   MSP430 I/O ports configuration.
 * @note    The @p PxIFG, @p PxIE and @p PxSEL registers are cleared. @p PxOUT
 *          and @p PxDIR are configured as specified.
 *
 * @param[in] config the MSP430 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

#if defined(__MSP430_HAS_PORT1__) || defined(__MSP430_HAS_PORT1_R__)
  IOPORT1->iop_full.ie = 0;
  IOPORT1->iop_full.ifg = 0;
  IOPORT1->iop_full.sel = 0;
  IOPORT1->iop_common.out = config->P1Data.out;
  IOPORT1->iop_common.dir = config->P1Data.dir;
#endif

#if defined(__MSP430_HAS_PORT2__) || defined(__MSP430_HAS_PORT2_R__)
  IOPORT2->iop_full.ie = 0;
  IOPORT2->iop_full.ifg = 0;
  IOPORT2->iop_full.sel = 0;
  IOPORT2->iop_common.out = config->P2Data.out;
  IOPORT2->iop_common.dir = config->P2Data.dir;
#endif

#if defined(__MSP430_HAS_PORT3__) || defined(__MSP430_HAS_PORT3_R__)
  IOPORT3->iop_simple.sel = 0;
  IOPORT3->iop_common.out = config->P3Data.out;
  IOPORT3->iop_common.dir = config->P3Data.dir;
#endif

#if defined(__MSP430_HAS_PORT4__) || defined(__MSP430_HAS_PORT4_R__)
  IOPORT4->iop_simple.sel = 0;
  IOPORT4->iop_common.out = config->P4Data.out;
  IOPORT4->iop_common.dir = config->P4Data.dir;
#endif

#if defined(__MSP430_HAS_PORT5__) || defined(__MSP430_HAS_PORT5_R__)
  IOPORT5->iop_simple.sel = 0;
  IOPORT5->iop_common.out = config->P5Data.out;
  IOPORT5->iop_common.dir = config->P5Data.dir;
#endif

#if defined(__MSP430_HAS_PORT6__) || defined(__MSP430_HAS_PORT6_R__)
  IOPORT6->iop_simple.sel = 0;
  IOPORT6->iop_common.out = config->P6Data.out;
  IOPORT6->iop_common.dir = config->P6Data.dir;
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as output as recommended by
 *          the MSP430x1xx Family User's Guide. Unconnected pads are set to
 *          high logic state by default.
 * @note    This function does not alter the @p PxSEL registers. Alternate
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
    port->iop_common.dir &= ~mask;
    break;
  case PAL_MODE_UNCONNECTED:
    port->iop_common.out |= mask;
  case PAL_MODE_OUTPUT_PUSHPULL:
    port->iop_common.dir |= mask;
    break;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
