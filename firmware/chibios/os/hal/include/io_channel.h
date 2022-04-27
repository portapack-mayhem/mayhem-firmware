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
 * @file    io_channel.h
 * @brief   I/O channels access.
 * @details This header defines an abstract interface useful to access generic
 *          I/O serial devices in a standardized way.
 *
 * @addtogroup IO_CHANNEL
 * @details This module defines an abstract interface for I/O channels by
 *          extending the @p BaseSequentialStream interface.<br>
 *          Note that no code is present, I/O channels are just abstract
 *          interface like structures, you should look at the systems as
 *          to a set of abstract C++ classes (even if written in C).
 *          Specific device drivers can use/extend the interface and
 *          implement them.<br>
 *          This system has the advantage to make the access to channels
 *          independent from the implementation logic.
 * @{
 */

#ifndef _IO_CHANNEL_H_
#define _IO_CHANNEL_H_

/**
 * @brief   @p BaseChannel specific methods.
 */
#define _base_channel_methods                                               \
  _base_sequential_stream_methods                                           \
  /* Channel put method with timeout specification.*/                       \
  msg_t (*putt)(void *instance, uint8_t b, systime_t time);                 \
  /* Channel get method with timeout specification.*/                       \
  msg_t (*gett)(void *instance, systime_t time);                            \
  /* Channel write method with timeout specification.*/                     \
  size_t (*writet)(void *instance, const uint8_t *bp,                       \
                   size_t n, systime_t time);                               \
  /* Channel read method with timeout specification.*/                      \
  size_t (*readt)(void *instance, uint8_t *bp, size_t n, systime_t time);

/**
 * @brief   @p BaseChannel specific data.
 * @note    It is empty because @p BaseChannel is only an interface without
 *          implementation.
 */
#define _base_channel_data                                                  \
  _base_sequential_stream_data

/**
 * @extends BaseSequentialStreamVMT
 *
 * @brief   @p BaseChannel virtual methods table.
 */
struct BaseChannelVMT {
  _base_channel_methods
};

/**
 * @extends BaseSequentialStream
 *
 * @brief   Base channel class.
 * @details This class represents a generic, byte-wide, I/O channel. This class
 *          introduces generic I/O primitives with timeout specification.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseChannelVMT *vmt;
  _base_channel_data
} BaseChannel;

/**
 * @name    Macro Functions (BaseChannel)
 * @{
 */
/**
 * @brief   Channel blocking byte write with timeout.
 * @details This function writes a byte value to a channel. If the channel
 *          is not ready to accept data then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[in] b         the byte value to be written to the channel
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval Q_OK         if the operation succeeded.
 * @retval Q_TIMEOUT    if the specified time expired.
 * @retval Q_RESET      if the channel associated queue (if any) was reset.
 *
 * @api
 */
#define chnPutTimeout(ip, b, time) ((ip)->vmt->putt(ip, b, time))

/**
 * @brief   Channel blocking byte read with timeout.
 * @details This function reads a byte value from a channel. If the data
 *          is not available then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A byte value from the queue.
 * @retval Q_TIMEOUT    if the specified time expired.
 * @retval Q_RESET      if the channel associated queue (if any) has been
 *                      reset.
 *
 * @api
 */
#define chnGetTimeout(ip, time) ((ip)->vmt->gett(ip, time))

/**
 * @brief   Channel blocking write.
 * @details The function writes data from a buffer to a channel. If the channel
 *          is not ready to accept data then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 *
 * @return              The number of bytes transferred.
 *
 * @api
 */
#define chnWrite(ip, bp, n) chSequentialStreamWrite(ip, bp, n)

/**
 * @brief   Channel blocking write with timeout.
 * @details The function writes data from a buffer to a channel. If the channel
 *          is not ready to accept data then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes transferred.
 *
 * @api
 */
#define chnWriteTimeout(ip, bp, n, time) ((ip)->vmt->writet(ip, bp, n, time))

/**
 * @brief   Channel blocking read.
 * @details The function reads data from a channel into a buffer. If the data
 *          is not available then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 *
 * @return              The number of bytes transferred.
 *
 * @api
 */
#define chnRead(ip, bp, n) chSequentialStreamRead(ip, bp, n)

/**
 * @brief   Channel blocking read with timeout.
 * @details The function reads data from a channel into a buffer. If the data
 *          is not available then the calling thread is suspended.
 *
 * @param[in] ip        pointer to a @p BaseChannel or derived class
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes transferred.
 *
 * @api
 */
#define chnReadTimeout(ip, bp, n, time) ((ip)->vmt->readt(ip, bp, n, time))
/** @} */

#if CH_USE_EVENTS || defined(__DOXYGEN__)
/**
 * @name    I/O status flags added to the event listener
 * @{
 */
/** @brief No pending conditions.*/
#define CHN_NO_ERROR            0
/** @brief Connection happened.*/
#define CHN_CONNECTED           1
/** @brief Disconnection happened.*/
#define CHN_DISCONNECTED        2
/** @brief Data available in the input queue.*/
#define CHN_INPUT_AVAILABLE     4
/** @brief Output queue empty.*/
#define CHN_OUTPUT_EMPTY        8
/** @brief Transmission end.*/
#define CHN_TRANSMISSION_END    16
/** @} */

/**
 * @brief   @p BaseAsynchronousChannel specific methods.
 */
#define _base_asynchronous_channel_methods                                  \
  _base_channel_methods                                                     \

/**
 * @brief   @p BaseAsynchronousChannel specific data.
 */
#define _base_asynchronous_channel_data                                     \
  _base_channel_data                                                        \
  /* I/O condition event source.*/                                          \
  EventSource           event;

/**
 * @extends BaseChannelVMT
 *
 * @brief   @p BaseAsynchronousChannel virtual methods table.
 */
struct BaseAsynchronousChannelVMT {
  _base_asynchronous_channel_methods
};

/**
 * @extends BaseChannel
 *
 * @brief   Base asynchronous channel class.
 * @details This class extends @p BaseChannel by adding event sources fields
 *          for asynchronous I/O for use in an event-driven environment.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseAsynchronousChannelVMT *vmt;
  _base_asynchronous_channel_data
} BaseAsynchronousChannel;

/**
 * @name    Macro Functions (BaseAsynchronousChannel)
 * @{
 */
/**
 * @brief   Returns the I/O condition event source.
 * @details The event source is broadcasted when an I/O condition happens.
 *
 * @param[in] ip        pointer to a @p BaseAsynchronousChannel or derived
 *                      class
 * @return              A pointer to an @p EventSource object.
 *
 * @api
 */
#define chnGetEventSource(ip) (&((ip)->event))

/**
 * @brief   Adds status flags to the listeners's flags mask.
 * @details This function is usually called from the I/O ISRs in order to
 *          notify I/O conditions such as data events, errors, signal
 *          changes etc.
 *
 * @param[in] ip        pointer to a @p BaseAsynchronousChannel or derived
 *                      class
 * @param[in] flags     condition flags to be added to the listener flags mask
 *
 * @iclass
 */
#define chnAddFlagsI(ip, flags) {                                           \
  chEvtBroadcastFlagsI(&(ip)->event, flags);                                \
}
/** @} */

#endif /* CH_USE_EVENTS */

#endif /* _IO_CHANNEL_H_ */

/** @} */
