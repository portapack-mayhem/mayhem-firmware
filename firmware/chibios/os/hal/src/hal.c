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
 * @file    hal.c
 * @brief   HAL subsystem code.
 *
 * @addtogroup HAL
 * @{
 */

#include "ch.h"
#include "hal.h"

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
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   HAL initialization.
 * @details This function invokes the low level initialization code then
 *          initializes all the drivers enabled in the HAL. Finally the
 *          board-specific initialization is performed by invoking
 *          @p boardInit() (usually defined in @p board.c).
 *
 * @init
 */
void halInit(void) {

  hal_lld_init();

#if HAL_USE_TM || defined(__DOXYGEN__)
  tmInit();
#endif
#if HAL_USE_PAL || defined(__DOXYGEN__)
  palInit(&pal_default_config);
#endif
#if HAL_USE_ADC || defined(__DOXYGEN__)
  adcInit();
#endif
#if HAL_USE_CAN || defined(__DOXYGEN__)
  canInit();
#endif
#if HAL_USE_EXT || defined(__DOXYGEN__)
  extInit();
#endif
#if HAL_USE_GPT || defined(__DOXYGEN__)
  gptInit();
#endif
#if HAL_USE_I2C || defined(__DOXYGEN__)
  i2cInit();
#endif
#if HAL_USE_ICU || defined(__DOXYGEN__)
  icuInit();
#endif
#if HAL_USE_MAC || defined(__DOXYGEN__)
  macInit();
#endif
#if HAL_USE_PWM || defined(__DOXYGEN__)
  pwmInit();
#endif
#if HAL_USE_SERIAL || defined(__DOXYGEN__)
  sdInit();
#endif
#if HAL_USE_SDC || defined(__DOXYGEN__)
  sdcInit();
#endif
#if HAL_USE_SPI || defined(__DOXYGEN__)
  spiInit();
#endif
#if HAL_USE_UART || defined(__DOXYGEN__)
  uartInit();
#endif
#if HAL_USE_USB || defined(__DOXYGEN__)
  usbInit();
#endif
#if HAL_USE_MMC_SPI || defined(__DOXYGEN__)
  mmcInit();
#endif
#if HAL_USE_SERIAL_USB || defined(__DOXYGEN__)
  sduInit();
#endif
#if HAL_USE_RTC || defined(__DOXYGEN__)
  rtcInit();
#endif
  /* Board specific initialization.*/
  boardInit();
}

#if HAL_IMPLEMENTS_COUNTERS || defined(__DOXYGEN__)
/**
 * @brief   Realtime window test.
 * @details This function verifies if the current realtime counter value
 *          lies within the specified range or not. The test takes care
 *          of the realtime counter wrapping to zero on overflow.
 * @note    When start==end then the function returns always true because the
 *          whole time range is specified.
 * @note    This is an optional service that could not be implemented in
 *          all HAL implementations.
 * @note    This function can be called from any context.
 *
 * @par Example 1
 * Example of a guarded loop using the realtime counter. The loop implements
 * a timeout after one second.
 * @code
 *   halrtcnt_t start = halGetCounterValue();
 *   halrtcnt_t timeout  = start + S2RTT(1);
 *   while (my_condition) {
 *     if (!halIsCounterWithin(start, timeout)
 *       return TIMEOUT;
 *     // Do something.
 *   }
 *   // Continue.
 * @endcode
 *
 * @par Example 2
 * Example of a loop that lasts exactly 50 microseconds.
 * @code
 *   halrtcnt_t start = halGetCounterValue();
 *   halrtcnt_t timeout  = start + US2RTT(50);
 *   while (halIsCounterWithin(start, timeout)) {
 *     // Do something.
 *   }
 *   // Continue.
 * @endcode
 *
 * @param[in] start     the start of the time window (inclusive)
 * @param[in] end       the end of the time window (non inclusive)
 * @retval TRUE         current time within the specified time window.
 * @retval FALSE        current time not within the specified time window.
 *
 * @special
 */
bool_t halIsCounterWithin(halrtcnt_t start, halrtcnt_t end) {
  halrtcnt_t now = halGetCounterValue();

  return end > start ? (now >= start) && (now < end) :
                       (now >= start) || (now < end);
}

/**
 * @brief   Polled delay.
 * @note    The real delays is always few cycles in excess of the specified
 *          value.
 * @note    This is an optional service that could not be implemented in
 *          all HAL implementations.
 * @note    This function can be called from any context.
 *
 * @param[in] ticks     number of ticks
 *
 * @special
 */
void halPolledDelay(halrtcnt_t ticks) {
  halrtcnt_t start = halGetCounterValue();
  halrtcnt_t timeout  = start + (ticks);
  while (halIsCounterWithin(start, timeout))
    ;
}
#endif /* HAL_IMPLEMENTS_COUNTERS */

/** @} */
