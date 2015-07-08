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
 * @file    chmtx.h
 * @brief   Mutexes macros and structures.
 *
 * @addtogroup mutexes
 * @{
 */

#ifndef _CHMTX_H_
#define _CHMTX_H_

#if CH_USE_MUTEXES || defined(__DOXYGEN__)

/**
 * @brief   Mutex structure.
 */
typedef struct Mutex {
  ThreadsQueue          m_queue;    /**< @brief Queue of the threads sleeping
                                                on this Mutex.              */
  Thread                *m_owner;   /**< @brief Owner @p Thread pointer or
                                                @p NULL.                    */
  struct Mutex          *m_next;    /**< @brief Next @p Mutex into an
                                                owner-list or @p NULL.      */
} Mutex;

#ifdef __cplusplus
extern "C" {
#endif
  void chMtxInit(Mutex *mp);
  void chMtxLock(Mutex *mp);
  void chMtxLockS(Mutex *mp);
  bool_t chMtxTryLock(Mutex *mp);
  bool_t chMtxTryLockS(Mutex *mp);
  Mutex *chMtxUnlock(void);
  Mutex *chMtxUnlockS(void);
  void chMtxUnlockAll(void);
#ifdef __cplusplus
}
#endif

/**
 * @brief   Data part of a static mutex initializer.
 * @details This macro should be used when statically initializing a mutex
 *          that is part of a bigger structure.
 *
 * @param[in] name      the name of the mutex variable
 */
#define _MUTEX_DATA(name) {_THREADSQUEUE_DATA(name.m_queue), NULL, NULL}

/**
 * @brief   Static mutex initializer.
 * @details Statically initialized mutexes require no explicit initialization
 *          using @p chMtxInit().
 *
 * @param[in] name      the name of the mutex variable
 */
#define MUTEX_DECL(name) Mutex name = _MUTEX_DATA(name)

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Returns @p TRUE if the mutex queue contains at least a waiting
 *          thread.
 *
 * @sclass
 */
#define chMtxQueueNotEmptyS(mp) notempty(&(mp)->m_queue)
/** @} */

#endif /* CH_USE_MUTEXES */

#endif /* _CHMTX_H_ */

/** @} */
