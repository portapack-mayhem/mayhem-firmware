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
 * @file    chmsg.h
 * @brief   Messages macros and structures.
 *
 * @addtogroup messages
 * @{
 */

#ifndef _CHMSG_H_
#define _CHMSG_H_

#if CH_USE_MESSAGES || defined(__DOXYGEN__)

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Evaluates to TRUE if the thread has pending messages.
 *
 * @iclass
 */
#define chMsgIsPendingI(tp) \
        ((tp)->p_msgqueue.p_next != (Thread *)&(tp)->p_msgqueue)

/**
 * @brief   Returns the message carried by the specified thread.
 * @pre     This function must be invoked immediately after exiting a call
 *          to @p chMsgWait().
 *
 * @param[in] tp        pointer to the thread
 * @return              The message carried by the sender.
 *
 * @api
 */
#define chMsgGet(tp) ((tp)->p_msg)

/**
 * @brief   Releases the thread waiting on top of the messages queue.
 * @pre     Invoke this function only after a message has been received
 *          using @p chMsgWait().
 *
 * @param[in] tp        pointer to the thread
 * @param[in] msg       message to be returned to the sender
 *
 * @sclass
 */
#define chMsgReleaseS(tp, msg) chSchWakeupS(tp, msg)
/** @} */

#ifdef __cplusplus
extern "C" {
#endif
  msg_t chMsgSend(Thread *tp, msg_t msg);
  Thread * chMsgWait(void);
  void chMsgRelease(Thread *tp, msg_t msg);
#ifdef __cplusplus
}
#endif

#endif /* CH_USE_MESSAGES */

#endif /* _CHMSG_H_ */

/** @} */
