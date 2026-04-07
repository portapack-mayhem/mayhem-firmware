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
 * @file    chlists.h
 * @brief   Thread queues/lists macros and structures.
 * @note    All the macros present in this module, while public, are not
 *          an OS API and should not be directly used in the user applications
 *          code.
 *
 * @addtogroup internals
 * @{
 */

#ifndef _CHLISTS_H_
#define _CHLISTS_H_

typedef struct Thread Thread;

/**
 * @brief   Threads queue initialization.
 *
 * @notapi
 */
#define queue_init(tqp) ((tqp)->p_next = (tqp)->p_prev = (Thread *)(tqp));

/**
 * @brief   Threads list initialization.
 *
 * @notapi
 */
#define list_init(tlp) ((tlp)->p_next = (Thread *)(tlp))

/**
 * @brief   Evaluates to @p TRUE if the specified threads queue or list is
 * empty.
 *
 * @notapi
 */
#define isempty(p)      ((p)->p_next == (Thread *)(p))

/**
 * @brief   Evaluates to @p TRUE if the specified threads queue or list is
 *          not empty.
 *
 * @notapi
 */
#define notempty(p)     ((p)->p_next != (Thread *)(p))

/**
 * @brief   Data part of a static threads queue initializer.
 * @details This macro should be used when statically initializing a threads
 *          queue that is part of a bigger structure.
 *
 * @param[in] name      the name of the threads queue variable
 */
#define _THREADSQUEUE_DATA(name) {(Thread *)&name, (Thread *)&name}

/**
 * @brief   Static threads queue initializer.
 * @details Statically initialized threads queues require no explicit
 *          initialization using @p queue_init().
 *
 * @param[in] name      the name of the threads queue variable
 */
#define THREADSQUEUE_DECL(name) ThreadsQueue name = _THREADSQUEUE_DATA(name)

/**
 * @extends ThreadsList
 *
 * @brief   Generic threads bidirectional linked list header and element.
 */
typedef struct {
  Thread                *p_next;        /**< First @p Thread in the queue, or
                                             @p ThreadQueue when empty.     */
  Thread                *p_prev;        /**< Last @p Thread in the queue, or
                                             @p ThreadQueue when empty.     */
} ThreadsQueue;

/**
 * @brief   Generic threads single link list, it works like a stack.
 */
typedef struct {

  Thread            *p_next;            /**< Last pushed @p Thread on the stack
                                             list, or pointer to itself if
                                             empty.                         */
} ThreadsList;

#if !CH_OPTIMIZE_SPEED

#ifdef __cplusplus
extern "C" {
#endif
  void prio_insert(Thread *tp, ThreadsQueue *tqp);
  void queue_insert(Thread *tp, ThreadsQueue *tqp);
  Thread *fifo_remove(ThreadsQueue *tqp);
  Thread *lifo_remove(ThreadsQueue *tqp);
  Thread *dequeue(Thread *tp);
  void list_insert(Thread *tp, ThreadsList *tlp);
  Thread *list_remove(ThreadsList *tlp);
#ifdef __cplusplus
}
#endif

#endif /* !CH_OPTIMIZE_SPEED */

#endif /* _CHLISTS_H_ */

/** @} */
