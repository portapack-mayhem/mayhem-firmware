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
 * @file    AVR/pal_lld.c
 * @brief   AVR GPIO low level driver code.
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
 * @brief   AVR GPIO ports configuration.
 * @details GPIO registers initialization.
 *
 * @param[in] config    the AVR ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

#if defined(PORTA) || defined(__DOXYGEN__)
  PORTA = config->porta.out;
  DDRA = config->porta.dir;
#endif

#if defined(PORTB) || defined(__DOXYGEN__)
  PORTB = config->portb.out;
  DDRB = config->portb.dir;
#endif

#if defined(PORTC) || defined(__DOXYGEN__)
  PORTC = config->portc.out;
  DDRC = config->portc.dir;
#endif

#if defined(PORTD) || defined(__DOXYGEN__)
  PORTD = config->portd.out;
  DDRD = config->portd.dir;
#endif

#if defined(PORTE) || defined(__DOXYGEN__)
  PORTE = config->porte.out;
  DDRE = config->porte.dir;
#endif

#if defined(PORTF) || defined(__DOXYGEN__)
  PORTF = config->portf.out;
  DDRF = config->portf.dir;
#endif

#if defined(PORTG) || defined(__DOXYGEN__)
  PORTG = config->portg.out;
  DDRG = config->portg.dir;
#endif

#if defined(PORTH) || defined(__DOXYGEN__)
  PORTH = config->porth.out;
  DDRH = config->porth.dir;
#endif

#if defined(PORTJ) || defined(__DOXYGEN__)
  PORTJ = config->portj.out;
  DDRJ = config->portj.dir;
#endif

#if defined(PORTK) || defined(__DOXYGEN__)
  PORTK = config->portk.out;
  DDRK = config->portk.dir;
#endif

#if defined(PORTL) || defined(__DOXYGEN__)
  PORTL = config->portl.out;
  DDRL = config->portl.dir;
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port the port identifier
 * @param[in] mask the group mask
 * @param[in] mode the mode
 *
 * @note This function is not meant to be invoked directly by the application
 *       code.
 * @note @p PAL_MODE_UNCONNECTED is implemented as output as recommended by
 *       the AVR Family User's Guide. Unconnected pads are set to input
 *       with pull-up by default.
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  switch (mode) {
  case PAL_MODE_RESET:
  case PAL_MODE_INPUT:
  case PAL_MODE_INPUT_ANALOG:
    port->dir &= ~mask;
    port->out &= ~mask;
	break;
  case PAL_MODE_UNCONNECTED:
  case PAL_MODE_INPUT_PULLUP:
    port->dir &= ~mask;
    port->out |= mask;
    break;
  case PAL_MODE_OUTPUT_PUSHPULL:
    port->dir |= mask;
    break;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
