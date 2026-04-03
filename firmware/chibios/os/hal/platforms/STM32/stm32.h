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
 * @file    STM32/stm32.h
 * @brief   STM32 common header.
 * @pre     One of the following macros must be defined before including
 *          this header, the macro selects the inclusion of the appropriate
 *          vendor header:
 *          - STM32F0XX for Entry Level devices.
 *          - STM32F10X_LD_VL for Value Line Low Density devices.
 *          - STM32F10X_MD_VL for Value Line Medium Density devices.
 *          - STM32F10X_LD for Performance Low Density devices.
 *          - STM32F10X_MD for Performance Medium Density devices.
 *          - STM32F10X_HD for Performance High Density devices.
 *          - STM32F10X_XL for Performance eXtra Density devices.
 *          - STM32F10X_CL for Connectivity Line devices.
 *          - STM32F2XX for High-performance STM32 F-2 devices.
 *          - STM32F30X for Analog & DSP devices.
 *          - STM32F37X for Analog & DSP devices.
 *          - STM32F4XX for High-performance STM32 F-4 devices.
 *          - STM32L1XX_MD for Ultra Low Power Medium-density devices.
 *          - STM32L1XX_MDP for Ultra Low Power Medium-density Plus devices.
 *          - STM32L1XX_HD for Ultra Low Power High-density devices.
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _STM32_H_
#define _STM32_H_

#if defined(STM32F030)         || defined(STM32F0XX_LD)    ||               \
    defined(STM32F0XX_MD)
#include "stm32f0xx.h"

#elif defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL) ||               \
      defined(STM32F10X_HD_VL) || defined(STM32F10X_LD)    ||               \
      defined(STM32F10X_MD)    || defined(STM32F10X_HD)    ||               \
      defined(STM32F10X_XL)    || defined(STM32F10X_CL)    ||               \
      defined(__DOXYGEN__)
#include "stm32f10x.h"

#elif defined(STM32F2XX)
#include "stm32f2xx.h"

#elif defined(STM32F30X)
#include "stm32f30x.h"

#elif defined(STM32F37X)
#include "stm32f37x.h"

#elif defined(STM32F401xx)     || defined(STM32F40_41xxx)  ||               \
      defined(STM32F427_437xx) || defined(STM32F429_439xx)
#include "stm32f4xx.h"

#elif defined(STM32L1XX_MD)    || defined(STM32L1XX_MDP)   ||               \
      defined(STM32L1XX_HD)
#include "stm32l1xx.h"

#else
#error "STM32 device not specified"
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#endif /* _STM32_H_ */

/** @} */
