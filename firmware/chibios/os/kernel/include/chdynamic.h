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
 * @file    chdynamic.h
 * @brief   Dynamic threads macros and structures.
 *
 * @addtogroup dynamic_threads
 * @{
 */

#ifndef _CHDYNAMIC_H_
#define _CHDYNAMIC_H_

#if CH_USE_DYNAMIC || defined(__DOXYGEN__)

/*
 * Module dependencies check.
 */
#if CH_USE_DYNAMIC && !CH_USE_WAITEXIT
#error "CH_USE_DYNAMIC requires CH_USE_WAITEXIT"
#endif
#if CH_USE_DYNAMIC && !CH_USE_HEAP && !CH_USE_MEMPOOLS
#error "CH_USE_DYNAMIC requires CH_USE_HEAP and/or CH_USE_MEMPOOLS"
#endif

/*
 * Dynamic threads APIs.
 */
#ifdef __cplusplus
extern "C" {
#endif
  Thread *chThdAddRef(Thread *tp);
  void chThdRelease(Thread *tp);
#if CH_USE_HEAP
  Thread *chThdCreateFromHeap(MemoryHeap *heapp, size_t size,
                              tprio_t prio, tfunc_t pf, void *arg);
#endif
#if CH_USE_MEMPOOLS
  Thread *chThdCreateFromMemoryPool(MemoryPool *mp, tprio_t prio,
                                    tfunc_t pf, void *arg);
#endif
#ifdef __cplusplus
}
#endif

#endif /* CH_USE_DYNAMIC */

#endif /* _CHDYNAMIC_H_ */

/** @} */
