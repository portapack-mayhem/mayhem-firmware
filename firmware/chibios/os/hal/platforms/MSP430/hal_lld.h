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
 * @file    MSP430/hal_lld.h
 * @brief   MSP430 HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "msp430.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS FALSE

/**
 * @brief   Platform name.
 */
#define PLATFORM_NAME   "MSP430"

#define MSP430_CLOCK_SOURCE_XT2CLK  0   /**< @brief XT2CLK clock selector.  */
#define MSP430_CLOCK_SOURCE_DCOCLK  1   /**< @brief DCOCLK clock selector.  */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Clock source.
 * @details The clock source can be selected from:
 *          - @p MSP430_CLOCK_SOURCE_XT2CLK.
 *          - @p MSP430_CLOCK_SOURCE_DCOCLK.
 *          .
 */
#if !defined(MSP430_USE_CLOCK) || defined(__DOXYGEN__)
#define MSP430_USE_CLOCK            MSP430_CLOCK_SOURCE_XT2CLK
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Calculating the derived clock constants.
 */
#define ACLK                        LFXT1CLK
#if MSP430_USE_CLOCK == MSP430_CLOCK_SOURCE_XT2CLK
#define MCLK                        XT2CLK
#define SMCLK                       (XT2CLK / 8)
#elif MSP430_USE_CLOCK == MSP430_CLOCK_SOURCE_DCOCLK
#define MCLK                        DCOCLK
#define SMCLK                       DCOCLK
#else
#error "unknown clock source specified"
#endif

/*
 * Calculating the initialization values.
 */
#define VAL_DCOCTL                  (DCO0 | DCO1)
#if MSP430_USE_CLOCK == MSP430_CLOCK_SOURCE_XT2CLK
#define VAL_BCSCTL1                 (RSEL2)
#define VAL_BCSCTL2                 (SELM_2 | DIVM_0 | DIVS_3 | SELS)
#endif
#if MSP430_USE_CLOCK == MSP430_CLOCK_SOURCE_DCOCLK
#define VAL_BCSCTL1                 (XT2OFF | RSEL2)
#define VAL_BCSCTL2                 (SELM_0 | DIVM_0 | DIVS_0)
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
