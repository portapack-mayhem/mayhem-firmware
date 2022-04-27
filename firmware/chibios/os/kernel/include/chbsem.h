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
 * @file    chbsem.h
 * @brief   Binary semaphores structures and macros.
 *
 * @addtogroup binary_semaphores
 * @details Binary semaphores related APIs and services.
 *
 *          <h2>Operation mode</h2>
 *          Binary semaphores are implemented as a set of macros that use the
 *          existing counting semaphores primitives. The difference between
 *          counting and binary semaphores is that the counter of binary
 *          semaphores is not allowed to grow above the value 1. Repeated
 *          signal operation are ignored. A binary semaphore can thus have
 *          only two defined states:
 *          - <b>Taken</b>, when its counter has a value of zero or lower
 *            than zero. A negative number represent the number of threads
 *            queued on the binary semaphore.
 *          - <b>Not taken</b>, when its counter has a value of one.
 *          .
 *          Binary semaphores are different from mutexes because there is no
 *          the concept of ownership, a binary semaphore can be taken by a
 *          thread and signaled by another thread or an interrupt handler,
 *          mutexes can only be taken and released by the same thread. Another
 *          difference is that binary semaphores, unlike mutexes, do not
 *          implement the priority inheritance protocol.<br>
 *          In order to use the binary semaphores APIs the @p CH_USE_SEMAPHORES
 *          option must be enabled in @p chconf.h.
 * @{
 */

#ifndef _CHBSEM_H_
#define _CHBSEM_H_

#if CH_USE_SEMAPHORES || defined(__DOXYGEN__)

/**
 * @extends Semaphore
 *
 * @brief   Binary semaphore type.
 */
typedef struct  {
  Semaphore             bs_sem;
} BinarySemaphore;

/**
 * @brief   Data part of a static semaphore initializer.
 * @details This macro should be used when statically initializing a semaphore
 *          that is part of a bigger structure.
 *
 * @param[in] name      the name of the semaphore variable
 * @param[in] taken     the semaphore initial state
 */
#define _BSEMAPHORE_DATA(name, taken)                                       \
  {_SEMAPHORE_DATA(name.bs_sem, ((taken) ? 0 : 1))}

/**
 * @brief   Static semaphore initializer.
 * @details Statically initialized semaphores require no explicit
 *          initialization using @p chBSemInit().
 *
 * @param[in] name      the name of the semaphore variable
 * @param[in] taken     the semaphore initial state
 */
#define BSEMAPHORE_DECL(name, taken)                                        \
  BinarySemaphore name = _BSEMAPHORE_DATA(name, taken)

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Initializes a binary semaphore.
 *
 * @param[out] bsp      pointer to a @p BinarySemaphore structure
 * @param[in] taken     initial state of the binary semaphore:
 *                      - @a FALSE, the initial state is not taken.
 *                      - @a TRUE, the initial state is taken.
 *                      .
 *
 * @init
 */
#define chBSemInit(bsp, taken) chSemInit(&(bsp)->bs_sem, (taken) ? 0 : 1)

/**
 * @brief   Wait operation on the binary semaphore.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the binary semaphore has been successfully taken.
 * @retval RDY_RESET    if the binary semaphore has been reset using
 *                      @p bsemReset().
 *
 * @api
 */
#define chBSemWait(bsp) chSemWait(&(bsp)->bs_sem)

/**
 * @brief   Wait operation on the binary semaphore.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the binary semaphore has been successfully taken.
 * @retval RDY_RESET    if the binary semaphore has been reset using
 *                      @p bsemReset().
 *
 * @sclass
 */
#define chBSemWaitS(bsp) chSemWaitS(&(bsp)->bs_sem)

/**
 * @brief   Wait operation on the binary semaphore.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the binary semaphore has been successfully taken.
 * @retval RDY_RESET    if the binary semaphore has been reset using
 *                      @p bsemReset().
 * @retval RDY_TIMEOUT  if the binary semaphore has not been signaled or reset
 *                      within the specified timeout.
 *
 * @api
 */
#define chBSemWaitTimeout(bsp, time) chSemWaitTimeout(&(bsp)->bs_sem, (time))

/**
 * @brief   Wait operation on the binary semaphore.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from the semaphore.
 * @retval RDY_OK       if the binary semaphore has been successfully taken.
 * @retval RDY_RESET    if the binary semaphore has been reset using
 *                      @p bsemReset().
 * @retval RDY_TIMEOUT  if the binary semaphore has not been signaled or reset
 *                      within the specified timeout.
 *
 * @sclass
 */
#define chBSemWaitTimeoutS(bsp, time) chSemWaitTimeoutS(&(bsp)->bs_sem, (time))

/**
 * @brief   Reset operation on the binary semaphore.
 * @note    The released threads can recognize they were waked up by a reset
 *          rather than a signal because the @p bsemWait() will return
 *          @p RDY_RESET instead of @p RDY_OK.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @param[in] taken     new state of the binary semaphore
 *                      - @a FALSE, the new state is not taken.
 *                      - @a TRUE, the new state is taken.
 *                      .
 *
 * @api
 */
#define chBSemReset(bsp, taken) chSemReset(&(bsp)->bs_sem, (taken) ? 0 : 1)

/**
 * @brief   Reset operation on the binary semaphore.
 * @note    The released threads can recognize they were waked up by a reset
 *          rather than a signal because the @p bsemWait() will return
 *          @p RDY_RESET instead of @p RDY_OK.
 * @note    This function does not reschedule.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @param[in] taken     new state of the binary semaphore
 *                      - @a FALSE, the new state is not taken.
 *                      - @a TRUE, the new state is taken.
 *                      .
 *
 * @iclass
 */
#define chBSemResetI(bsp, taken) chSemResetI(&(bsp)->bs_sem, (taken) ? 0 : 1)

/**
 * @brief   Performs a signal operation on a binary semaphore.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 *
 * @api
 */
#define chBSemSignal(bsp) {                                                 \
  chSysLock();                                                              \
  chBSemSignalI((bsp));                                                     \
  chSchRescheduleS();                                                       \
  chSysUnlock();                                                            \
}

/**
 * @brief   Performs a signal operation on a binary semaphore.
 * @note    This function does not reschedule.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 *
 * @iclass
 */
#define chBSemSignalI(bsp) {                                                \
  if ((bsp)->bs_sem.s_cnt < 1)                                              \
    chSemSignalI(&(bsp)->bs_sem);                                           \
}

/**
 * @brief   Returns the binary semaphore current state.
 *
 * @param[in] bsp       pointer to a @p BinarySemaphore structure
 * @return              The binary semaphore current state.
 * @retval FALSE        if the binary semaphore is not taken.
 * @retval TRUE         if the binary semaphore is taken.
 *
 * @iclass
 */
#define chBSemGetStateI(bsp) ((bsp)->bs_sem.s_cnt > 0 ? FALSE : TRUE)
/** @} */

#endif /* CH_USE_SEMAPHORES */

#endif /* _CHBSEM_H_ */

/** @} */
