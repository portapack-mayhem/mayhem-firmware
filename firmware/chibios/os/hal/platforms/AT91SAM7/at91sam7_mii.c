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
 * @file    AT91SAM7/at91sam7_mii.c
 * @brief   AT91SAM7 low level MII driver code.
 *
 * @addtogroup AT91SAM7_MII
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "at91sam7_mii.h"

#if HAL_USE_MAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
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
 * @brief   Low level MII driver initialization.
 *
 * @notapi
 */
void miiInit(void) {

}

/**
 * @brief   Resets a PHY device.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void miiReset(MACDriver *macp) {

  (void)macp;

  /*
   * Disables the pullups on all the pins that are latched on reset by the PHY.
   */
  AT91C_BASE_PIOB->PIO_PPUDR = PHY_LATCHED_PINS;

#ifdef PIOB_PHY_PD_MASK
  /*
   * PHY power control.
   */
  AT91C_BASE_PIOB->PIO_OER = PIOB_PHY_PD_MASK;  /* Becomes an output.       */
  AT91C_BASE_PIOB->PIO_PPUDR = PIOB_PHY_PD_MASK;/* Default pullup disabled. */
#if (PHY_HARDWARE == PHY_DAVICOM_9161)
  AT91C_BASE_PIOB->PIO_CODR = PIOB_PHY_PD_MASK; /* Output to low level.     */
#else
  AT91C_BASE_PIOB->PIO_SODR = PIOB_PHY_PD_MASK; /* Output to high level.    */
#endif
#endif

  /*
   * PHY reset by pulsing the NRST pin.
   */
  AT91C_BASE_RSTC->RSTC_RMR = 0xA5000100;
  AT91C_BASE_RSTC->RSTC_RCR = 0xA5000000 | AT91C_RSTC_EXTRST;
  while (!(AT91C_BASE_RSTC->RSTC_RSR & AT91C_RSTC_NRSTL))
    ;
}

/**
 * @brief   Reads a PHY register through the MII interface.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] addr      the register address
 * @return              The register value.
 *
 * @notapi
 */
phyreg_t miiGet(MACDriver *macp, phyaddr_t addr) {

  (void)macp;
  AT91C_BASE_EMAC->EMAC_MAN = (0b01 << 30) |            /* SOF */
                              (0b10 << 28) |            /* RW */
                              (PHY_ADDRESS << 23) |     /* PHYA */
                              (addr << 18) |            /* REGA */
                              (0b10 << 16);             /* CODE */
  while (!( AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE))
    ;
  return (phyreg_t)(AT91C_BASE_EMAC->EMAC_MAN & 0xFFFF);
}

/**
 * @brief   Writes a PHY register through the MII interface.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] addr      the register address
 * @param[in] value     the new register value
 *
 * @notapi
 */
void miiPut(MACDriver *macp, phyaddr_t addr, phyreg_t value) {

  (void)macp;
  AT91C_BASE_EMAC->EMAC_MAN = (0b01 << 30) |            /* SOF */
                              (0b01 << 28) |            /* RW */
                              (PHY_ADDRESS << 23) |     /* PHYA */
                              (addr << 18) |            /* REGA */
                              (0b10 << 16) |            /* CODE */
                              value;
  while (!( AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE))
    ;
}

#endif /* HAL_USE_MAC */

/** @} */
