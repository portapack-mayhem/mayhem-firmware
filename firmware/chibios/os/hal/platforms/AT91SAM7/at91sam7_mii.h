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
 * @file    AT91SAM7/at91sam7_mii.h
 * @brief   AT91SAM7 low level MII driver header.
 *
 * @addtogroup AT91SAM7_MII
 * @{
 */

#ifndef _AT91SAM7_MII_H_
#define _AT91SAM7_MII_H_

#if HAL_USE_MAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define PHY_MICREL_KS8721       0
#define PHY_DAVICOM_9161        1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   PHY manufacturer and model.
 */
#if !defined(PHY_HARDWARE) || defined(__DOXYGEN__)
#define PHY_HARDWARE            PHY_MICREL_KS8721
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/**
 * @brief   Pins latched by the PHY at reset.
 */
#if PHY_HARDWARE == PHY_MICREL_KS8721
#define PHY_ADDRESS             1
#define PHY_ID                  MII_KS8721_ID
#define PHY_LATCHED_PINS        (AT91C_PB4_ECRS          | AT91C_PB5_ERX0  | \
                                 AT91C_PB6_ERX1          | AT91C_PB7_ERXER | \
                                 AT91C_PB13_ERX2         | AT91C_PB14_ERX3 | \
                                 AT91C_PB15_ERXDV_ECRSDV | AT91C_PB16_ECOL | \
                                 AT91C_PIO_PB26)

#elif PHY_HARDWARE == PHY_DAVICOM_9161
#define PHY_ADDRESS             0
#define PHY_ID                  MII_DM9161_ID
#define PHY_LATCHED_PINS        (AT91C_PB0_ETXCK_EREFCK  | AT91C_PB4_ECRS          | \
                                 AT91C_PB5_ERX0          | AT91C_PB6_ERX1          | \
                                 AT91C_PB7_ERXER         | AT91C_PB13_ERX2         | \
                                 AT91C_PB14_ERX3         | AT91C_PB15_ERXDV_ECRSDV | \
                                 AT91C_PB16_ECOL         | AT91C_PB17_ERXCK)
#endif /* PHY_HARDWARE */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PHY register value.
 */
typedef uint16_t phyreg_t;

/**
 * @brief   Type of a PHY register address.
 */
typedef uint8_t phyaddr_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void miiInit(void);
  void miiReset(MACDriver *macp);
  phyreg_t miiGet(MACDriver *macp, phyaddr_t addr);
  void miiPut(MACDriver *macp, phyaddr_t addr, phyreg_t value);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_MAC */

#endif /* _AT91SAM7_MII_H_ */

/** @} */
