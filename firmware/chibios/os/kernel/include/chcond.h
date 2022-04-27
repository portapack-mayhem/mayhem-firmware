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
/*
   Concepts and parts of this file have been contributed by Leon Woestenberg.
 */

/**
 * @file    chcond.h
 * @brief   Condition Variables macros and structures.
 *
 * @addtogroup condvars
 * @{
 */

#ifndef _CHCOND_H_
#define _CHCOND_H_

#if CH_USE_CONDVARS || defined(__DOXYGEN__)

/*
 * Module dependencies check.
 */
#if !CH_USE_MUTEXES
#error "CH_USE_CONDVARS requires CH_USE_MUTEXES"
#endif

/**
 * @brief   CondVar structure.
 */
typedef struct CondVar {
  ThreadsQueue          c_queue;        /**< @brief CondVar threads queue.*/
} CondVar;

#ifdef __cplusplus
extern "C" {
#endif
  void chCondInit(CondVar *cp);
  void chCondSignal(CondVar *cp);
  void chCondSignalI(CondVar *cp);
  void chCondBroadcast(CondVar *cp);
  void chCondBroadcastI(CondVar *cp);
  msg_t chCondWait(CondVar *cp);
  msg_t chCondWaitS(CondVar *cp);
#if CH_USE_CONDVARS_TIMEOUT
  msg_t chCondWaitTimeout(CondVar *cp, systime_t time);
  msg_t chCondWaitTimeoutS(CondVar *cp, systime_t time);
#endif
#ifdef __cplusplus
}
#endif

/**
 * @brief Data part of a static condition variable initializer.
 * @details This macro should be used when statically initializing a condition
 *          variable that is part of a bigger structure.
 *
 * @param[in] name      the name of the condition variable
 */
#define _CONDVAR_DATA(name) {_THREADSQUEUE_DATA(name.c_queue)}

/**
 * @brief Static condition variable initializer.
 * @details Statically initialized condition variables require no explicit
 *          initialization using @p chCondInit().
 *
 * @param[in] name      the name of the condition variable
 */
#define CONDVAR_DECL(name) CondVar name = _CONDVAR_DATA(name)

#endif /* CH_USE_CONDVARS */

#endif /* _CHCOND_H_ */

/** @} */
