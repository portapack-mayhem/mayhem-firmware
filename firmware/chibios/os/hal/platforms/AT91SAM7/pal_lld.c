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
 * @file    AT91SAM7/pal_lld.c
 * @brief   AT91SAM7 PIO low level driver code.
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
 * @brief   AT91SAM7 I/O ports configuration.
 * @details PIO registers initialization.
 *
 * @param[in] config    the AT91SAM7 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  uint32_t ports = (1 << AT91C_ID_PIOA);
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
  ports |= (1 << AT91C_ID_PIOB);
#endif
  AT91C_BASE_PMC->PMC_PCER = ports;

  /*
   * PIOA setup.
   */
  AT91C_BASE_PIOA->PIO_PPUER  = config->P0Data.pusr;    /* Pull-up as spec.*/
  AT91C_BASE_PIOA->PIO_PPUDR  = ~config->P0Data.pusr;
  AT91C_BASE_PIOA->PIO_PER  = 0xFFFFFFFF;               /* PIO enabled.*/
  AT91C_BASE_PIOA->PIO_ODSR = config->P0Data.odsr;      /* Data as specified.*/
  AT91C_BASE_PIOA->PIO_OER  = config->P0Data.osr;       /* Dir. as specified.*/
  AT91C_BASE_PIOA->PIO_ODR  = ~config->P0Data.osr;
  AT91C_BASE_PIOA->PIO_IFDR = 0xFFFFFFFF;               /* Filter disabled.*/
  AT91C_BASE_PIOA->PIO_IDR  = 0xFFFFFFFF;               /* Int. disabled.*/
  AT91C_BASE_PIOA->PIO_MDDR = 0xFFFFFFFF;               /* Push Pull drive.*/
  AT91C_BASE_PIOA->PIO_ASR  = 0xFFFFFFFF;               /* Peripheral A.*/
  AT91C_BASE_PIOA->PIO_OWER = 0xFFFFFFFF;               /* Write enabled.*/

  /*
   * PIOB setup.
   */
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
  AT91C_BASE_PIOB->PIO_PPUER  = config->P1Data.pusr;    /* Pull-up as spec.*/
  AT91C_BASE_PIOB->PIO_PPUDR  = ~config->P1Data.pusr;
  AT91C_BASE_PIOB->PIO_PER  = 0xFFFFFFFF;               /* PIO enabled.*/
  AT91C_BASE_PIOB->PIO_ODSR = config->P1Data.odsr;      /* Data as specified.*/
  AT91C_BASE_PIOB->PIO_OER  = config->P1Data.osr;       /* Dir. as specified.*/
  AT91C_BASE_PIOB->PIO_ODR  = ~config->P1Data.osr;
  AT91C_BASE_PIOB->PIO_IFDR = 0xFFFFFFFF;               /* Filter disabled.*/
  AT91C_BASE_PIOB->PIO_IDR  = 0xFFFFFFFF;               /* Int. disabled.*/
  AT91C_BASE_PIOB->PIO_MDDR = 0xFFFFFFFF;               /* Push Pull drive.*/
  AT91C_BASE_PIOB->PIO_ASR  = 0xFFFFFFFF;               /* Peripheral A.*/
  AT91C_BASE_PIOB->PIO_OWER = 0xFFFFFFFF;               /* Write enabled.*/
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    This function is not meant to be invoked directly from the
 *          application code.
 * @note    @p PAL_MODE_RESET is implemented as input with pull-up.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull output with
 *          high state.
 * @note    @p PAL_MODE_OUTPUT_OPENDRAIN also enables the pull-up resistor.
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

  if (mode & AT91_PAL_OPENDRAIN)
	port->PIO_MDER = mask;
  else
	port->PIO_MDDR = mask;
  if (mode & AT91_PAL_PULLUP)
	port->PIO_PPUER = mask;
  else
	port->PIO_PPUDR = mask;
  if (mode & AT91_PAL_OUT_SET)
	port->PIO_SODR = mask;
  if (mode & AT91_PAL_OUT_CLEAR)
	port->PIO_CODR = mask;

  switch (mode & AT91_PAL_DIR_MASK) {
  case AT91_PAL_DIR_INPUT:
	port->PIO_ODR = mask;
	port->PIO_PER = mask;
	break;
  case AT91_PAL_DIR_OUTPUT:
	port->PIO_OER = mask;
	port->PIO_PER = mask;
	break;
  case AT91_PAL_DIR_PERIPH_A:
	port->PIO_OER = mask;
	port->PIO_PDR = mask;
	port->PIO_ASR = mask;
	break;
  case AT91_PAL_DIR_PERIPH_B:
	port->PIO_OER = mask;
	port->PIO_PDR = mask;
	port->PIO_BSR = mask;
	break;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
