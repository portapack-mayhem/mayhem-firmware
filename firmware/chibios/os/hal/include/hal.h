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
 * @file    hal.h
 * @brief   HAL subsystem header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_H_
#define _HAL_H_

#include "ch.h"
#include "board.h"
#include "halconf.h"

#include "hal_lld.h"

/* Abstract interfaces.*/
#include "io_channel.h"
#include "io_block.h"

/* Shared headers.*/
#include "mmcsd.h"

/* Layered drivers.*/
#include "tm.h"
#include "pal.h"
#include "adc.h"
#include "can.h"
#include "ext.h"
#include "gpt.h"
#include "i2c.h"
#include "icu.h"
#include "mac.h"
#include "pwm.h"
#include "rtc.h"
#include "serial.h"
#include "sdc.h"
#include "spi.h"
#include "uart.h"
#include "usb.h"

/* Complex drivers.*/
#include "mmc_spi.h"
#include "serial_usb.h"

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

#if HAL_IMPLEMENTS_COUNTERS || defined(__DOXYGEN__)
/**
 * @name    Time conversion utilities for the realtime counter
 * @{
 */
/**
 * @brief   Seconds to realtime ticks.
 * @details Converts from seconds to realtime ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] sec       number of seconds
 * @return              The number of ticks.
 *
 * @api
 */
#define S2RTT(sec) (halGetCounterFrequency() * (sec))

/**
 * @brief   Milliseconds to realtime ticks.
 * @details Converts from milliseconds to realtime ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] msec      number of milliseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define MS2RTT(msec) (((halGetCounterFrequency() + 999UL) / 1000UL) * (msec))

/**
 * @brief   Microseconds to realtime ticks.
 * @details Converts from microseconds to realtime ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] usec      number of microseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define US2RTT(usec) (((halGetCounterFrequency() + 999999UL) / 1000000UL) * \
                      (usec))

/**
 * @brief   Realtime ticks to seconds to.
 * @details Converts from realtime ticks number to seconds.
 *
 * @param[in] ticks     number of ticks
 * @return              The number of seconds.
 *
 * @api
 */
#define RTT2S(ticks) ((ticks) / halGetCounterFrequency())

/**
 * @brief   Realtime ticks to milliseconds.
 * @details Converts from realtime ticks number to milliseconds.
 *
 * @param[in] ticks     number of ticks
 * @return              The number of milliseconds.
 *
 * @api
 */
#define RTT2MS(ticks) ((ticks) / (halGetCounterFrequency() / 1000UL))

/**
 * @brief   Realtime ticks to microseconds.
 * @details Converts from realtime ticks number to microseconds.
 *
 * @param[in] ticks     number of ticks
 * @return              The number of microseconds.
 *
 * @api
 */
#define RTT2US(ticks) ((ticks) / (halGetCounterFrequency() / 1000000UL))
/** @} */

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Returns the current value of the system free running counter.
 * @note    This is an optional service that could not be implemented in
 *          all HAL implementations.
 * @note    This function can be called from any context.
 *
 * @return              The value of the system free running counter of
 *                      type halrtcnt_t.
 *
 * @special
 */
#define halGetCounterValue() hal_lld_get_counter_value()

/**
 * @brief   Realtime counter frequency.
 * @note    This is an optional service that could not be implemented in
 *          all HAL implementations.
 * @note    This function can be called from any context.
 *
 * @return              The realtime counter frequency of type halclock_t.
 *
 * @special
 */
#define halGetCounterFrequency() hal_lld_get_counter_frequency()
/** @} */
#endif /* HAL_IMPLEMENTS_COUNTERS */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void halInit(void);
#if HAL_IMPLEMENTS_COUNTERS
  bool_t halIsCounterWithin(halrtcnt_t start, halrtcnt_t end);
  void halPolledDelay(halrtcnt_t ticks);
#endif /* HAL_IMPLEMENTS_COUNTERS */
#ifdef __cplusplus
}
#endif

#endif /* _HAL_H_ */

/** @} */
