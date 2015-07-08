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
 * @file    chqueues.c
 * @brief   I/O Queues code.
 *
 * @addtogroup io_queues
 * @details ChibiOS/RT queues are mostly used in serial-like device drivers.
 *          The device drivers are usually designed to have a lower side
 *          (lower driver, it is usually an interrupt service routine) and an
 *          upper side (upper driver, accessed by the application threads).<br>
 *          There are several kind of queues:<br>
 *          - <b>Input queue</b>, unidirectional queue where the writer is the
 *            lower side and the reader is the upper side.
 *          - <b>Output queue</b>, unidirectional queue where the writer is the
 *            upper side and the reader is the lower side.
 *          - <b>Full duplex queue</b>, bidirectional queue. Full duplex queues
 *            are implemented by pairing an input queue and an output queue
 *            together.
 *          .
 * @pre     In order to use the I/O queues the @p CH_USE_QUEUES option must
 *          be enabled in @p chconf.h.
 * @{
 */

#include "ch.h"

#if CH_USE_QUEUES || defined(__DOXYGEN__)

/**
 * @brief   Puts the invoking thread into the queue's threads queue.
 *
 * @param[out] qp       pointer to an @p GenericQueue structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A message specifying how the invoking thread has been
 *                      released from threads queue.
 * @retval Q_OK         is the normal exit, thread signaled.
 * @retval Q_RESET      if the queue has been reset.
 * @retval Q_TIMEOUT    if the queue operation timed out.
 */
static msg_t qwait(GenericQueue *qp, systime_t time) {

  if (TIME_IMMEDIATE == time)
    return Q_TIMEOUT;
  currp->p_u.wtobjp = qp;
  queue_insert(currp, &qp->q_waiting);
  return chSchGoSleepTimeoutS(THD_STATE_WTQUEUE, time);
}

/**
 * @brief   Initializes an input queue.
 * @details A Semaphore is internally initialized and works as a counter of
 *          the bytes contained in the queue.
 * @note    The callback is invoked from within the S-Locked system state,
 *          see @ref system_states.
 *
 * @param[out] iqp      pointer to an @p InputQueue structure
 * @param[in] bp        pointer to a memory area allocated as queue buffer
 * @param[in] size      size of the queue buffer
 * @param[in] infy      pointer to a callback function that is invoked when
 *                      data is read from the queue. The value can be @p NULL.
 * @param[in] link      application defined pointer
 *
 * @init
 */
void chIQInit(InputQueue *iqp, uint8_t *bp, size_t size, qnotify_t infy,
              void *link) {

  queue_init(&iqp->q_waiting);
  iqp->q_counter = 0;
  iqp->q_buffer = iqp->q_rdptr = iqp->q_wrptr = bp;
  iqp->q_top = bp + size;
  iqp->q_notify = infy;
  iqp->q_link = link;
}

/**
 * @brief   Resets an input queue.
 * @details All the data in the input queue is erased and lost, any waiting
 *          thread is resumed with status @p Q_RESET.
 * @note    A reset operation can be used by a low level driver in order to
 *          obtain immediate attention from the high level layers.
 *
 * @param[in] iqp       pointer to an @p InputQueue structure
 *
 * @iclass
 */
void chIQResetI(InputQueue *iqp) {

  chDbgCheckClassI();

  iqp->q_rdptr = iqp->q_wrptr = iqp->q_buffer;
  iqp->q_counter = 0;
  while (notempty(&iqp->q_waiting))
    chSchReadyI(fifo_remove(&iqp->q_waiting))->p_u.rdymsg = Q_RESET;
}

/**
 * @brief   Input queue write.
 * @details A byte value is written into the low end of an input queue.
 *
 * @param[in] iqp       pointer to an @p InputQueue structure
 * @param[in] b         the byte value to be written in the queue
 * @return              The operation status.
 * @retval Q_OK         if the operation has been completed with success.
 * @retval Q_FULL       if the queue is full and the operation cannot be
 *                      completed.
 *
 * @iclass
 */
msg_t chIQPutI(InputQueue *iqp, uint8_t b) {

  chDbgCheckClassI();

  if (chIQIsFullI(iqp))
    return Q_FULL;

  iqp->q_counter++;
  *iqp->q_wrptr++ = b;
  if (iqp->q_wrptr >= iqp->q_top)
    iqp->q_wrptr = iqp->q_buffer;

  if (notempty(&iqp->q_waiting))
    chSchReadyI(fifo_remove(&iqp->q_waiting))->p_u.rdymsg = Q_OK;

  return Q_OK;
}

/**
 * @brief   Input queue read with timeout.
 * @details This function reads a byte value from an input queue. If the queue
 *          is empty then the calling thread is suspended until a byte arrives
 *          in the queue or a timeout occurs.
 * @note    The callback is invoked before reading the character from the
 *          buffer or before entering the state @p THD_STATE_WTQUEUE.
 *
 * @param[in] iqp       pointer to an @p InputQueue structure
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A byte value from the queue.
 * @retval Q_TIMEOUT    if the specified time expired.
 * @retval Q_RESET      if the queue has been reset.
 *
 * @api
 */
msg_t chIQGetTimeout(InputQueue *iqp, systime_t time) {
  uint8_t b;

  chSysLock();
  if (iqp->q_notify)
    iqp->q_notify(iqp);

  while (chIQIsEmptyI(iqp)) {
    msg_t msg;
    if ((msg = qwait((GenericQueue *)iqp, time)) < Q_OK) {
      chSysUnlock();
      return msg;
    }
  }

  iqp->q_counter--;
  b = *iqp->q_rdptr++;
  if (iqp->q_rdptr >= iqp->q_top)
    iqp->q_rdptr = iqp->q_buffer;

  chSysUnlock();
  return b;
}

/**
 * @brief   Input queue read with timeout.
 * @details The function reads data from an input queue into a buffer. The
 *          operation completes when the specified amount of data has been
 *          transferred or after the specified timeout or if the queue has
 *          been reset.
 * @note    The function is not atomic, if you need atomicity it is suggested
 *          to use a semaphore or a mutex for mutual exclusion.
 * @note    The callback is invoked before reading each character from the
 *          buffer or before entering the state @p THD_STATE_WTQUEUE.
 *
 * @param[in] iqp       pointer to an @p InputQueue structure
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
size_t chIQReadTimeout(InputQueue *iqp, uint8_t *bp,
                       size_t n, systime_t time) {
  qnotify_t nfy = iqp->q_notify;
  size_t r = 0;

  chDbgCheck(n > 0, "chIQReadTimeout");

  chSysLock();
  while (TRUE) {
    if (nfy)
      nfy(iqp);

    while (chIQIsEmptyI(iqp)) {
      if (qwait((GenericQueue *)iqp, time) != Q_OK) {
        chSysUnlock();
        return r;
      }
    }

    iqp->q_counter--;
    *bp++ = *iqp->q_rdptr++;
    if (iqp->q_rdptr >= iqp->q_top)
      iqp->q_rdptr = iqp->q_buffer;

    chSysUnlock(); /* Gives a preemption chance in a controlled point.*/
    r++;
    if (--n == 0)
      return r;

    chSysLock();
  }
}

/**
 * @brief   Initializes an output queue.
 * @details A Semaphore is internally initialized and works as a counter of
 *          the free bytes in the queue.
 * @note    The callback is invoked from within the S-Locked system state,
 *          see @ref system_states.
 *
 * @param[out] oqp      pointer to an @p OutputQueue structure
 * @param[in] bp        pointer to a memory area allocated as queue buffer
 * @param[in] size      size of the queue buffer
 * @param[in] onfy      pointer to a callback function that is invoked when
 *                      data is written to the queue. The value can be @p NULL.
 * @param[in] link      application defined pointer
 *
 * @init
 */
void chOQInit(OutputQueue *oqp, uint8_t *bp, size_t size, qnotify_t onfy,
              void *link) {

  queue_init(&oqp->q_waiting);
  oqp->q_counter = size;
  oqp->q_buffer = oqp->q_rdptr = oqp->q_wrptr = bp;
  oqp->q_top = bp + size;
  oqp->q_notify = onfy;
  oqp->q_link = link;
}

/**
 * @brief   Resets an output queue.
 * @details All the data in the output queue is erased and lost, any waiting
 *          thread is resumed with status @p Q_RESET.
 * @note    A reset operation can be used by a low level driver in order to
 *          obtain immediate attention from the high level layers.
 *
 * @param[in] oqp       pointer to an @p OutputQueue structure
 *
 * @iclass
 */
void chOQResetI(OutputQueue *oqp) {

  chDbgCheckClassI();

  oqp->q_rdptr = oqp->q_wrptr = oqp->q_buffer;
  oqp->q_counter = chQSizeI(oqp);
  while (notempty(&oqp->q_waiting))
    chSchReadyI(fifo_remove(&oqp->q_waiting))->p_u.rdymsg = Q_RESET;
}

/**
 * @brief   Output queue write with timeout.
 * @details This function writes a byte value to an output queue. If the queue
 *          is full then the calling thread is suspended until there is space
 *          in the queue or a timeout occurs.
 * @note    The callback is invoked after writing the character into the
 *          buffer.
 *
 * @param[in] oqp       pointer to an @p OutputQueue structure
 * @param[in] b         the byte value to be written in the queue
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval Q_OK         if the operation succeeded.
 * @retval Q_TIMEOUT    if the specified time expired.
 * @retval Q_RESET      if the queue has been reset.
 *
 * @api
 */
msg_t chOQPutTimeout(OutputQueue *oqp, uint8_t b, systime_t time) {

  chSysLock();
  while (chOQIsFullI(oqp)) {
    msg_t msg;

    if ((msg = qwait((GenericQueue *)oqp, time)) < Q_OK) {
      chSysUnlock();
      return msg;
    }
  }

  oqp->q_counter--;
  *oqp->q_wrptr++ = b;
  if (oqp->q_wrptr >= oqp->q_top)
    oqp->q_wrptr = oqp->q_buffer;

  if (oqp->q_notify)
    oqp->q_notify(oqp);

  chSysUnlock();
  return Q_OK;
}

/**
 * @brief   Output queue read.
 * @details A byte value is read from the low end of an output queue.
 *
 * @param[in] oqp       pointer to an @p OutputQueue structure
 * @return              The byte value from the queue.
 * @retval Q_EMPTY      if the queue is empty.
 *
 * @iclass
 */
msg_t chOQGetI(OutputQueue *oqp) {
  uint8_t b;

  chDbgCheckClassI();

  if (chOQIsEmptyI(oqp))
    return Q_EMPTY;

  oqp->q_counter++;
  b = *oqp->q_rdptr++;
  if (oqp->q_rdptr >= oqp->q_top)
    oqp->q_rdptr = oqp->q_buffer;

  if (notempty(&oqp->q_waiting))
    chSchReadyI(fifo_remove(&oqp->q_waiting))->p_u.rdymsg = Q_OK;

  return b;
}

/**
 * @brief   Output queue write with timeout.
 * @details The function writes data from a buffer to an output queue. The
 *          operation completes when the specified amount of data has been
 *          transferred or after the specified timeout or if the queue has
 *          been reset.
 * @note    The function is not atomic, if you need atomicity it is suggested
 *          to use a semaphore or a mutex for mutual exclusion.
 * @note    The callback is invoked after writing each character into the
 *          buffer.
 *
 * @param[in] oqp       pointer to an @p OutputQueue structure
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
size_t chOQWriteTimeout(OutputQueue *oqp, const uint8_t *bp,
                        size_t n, systime_t time) {
  qnotify_t nfy = oqp->q_notify;
  size_t w = 0;

  chDbgCheck(n > 0, "chOQWriteTimeout");

  chSysLock();
  while (TRUE) {
    while (chOQIsFullI(oqp)) {
      if (qwait((GenericQueue *)oqp, time) != Q_OK) {
        chSysUnlock();
        return w;
      }
    }
    oqp->q_counter--;
    *oqp->q_wrptr++ = *bp++;
    if (oqp->q_wrptr >= oqp->q_top)
      oqp->q_wrptr = oqp->q_buffer;

    if (nfy)
      nfy(oqp);

    chSysUnlock(); /* Gives a preemption chance in a controlled point.*/
    w++;
    if (--n == 0)
      return w;
    chSysLock();
  }
}
#endif  /* CH_USE_QUEUES */

/** @} */
