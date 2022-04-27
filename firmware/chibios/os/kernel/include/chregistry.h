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
 * @file    chregistry.h
 * @brief   Threads registry macros and structures.
 *
 * @addtogroup registry
 * @{
 */

#ifndef _CHREGISTRY_H_
#define _CHREGISTRY_H_

#if CH_USE_REGISTRY || defined(__DOXYGEN__)

/**
 * @brief   ChibiOS/RT memory signature record.
 */
typedef struct {
  char      ch_identifier[4];       /**< @brief Always set to "main".       */
  uint8_t   ch_zero;                /**< @brief Must be zero.               */
  uint8_t   ch_size;                /**< @brief Size of this structure.     */
  uint16_t  ch_version;             /**< @brief Encoded ChibiOS/RT version. */
  uint8_t   ch_ptrsize;             /**< @brief Size of a pointer.          */
  uint8_t   ch_timesize;            /**< @brief Size of a @p systime_t.     */
  uint8_t   ch_threadsize;          /**< @brief Size of a @p Thread struct. */
  uint8_t   cf_off_prio;            /**< @brief Offset of @p p_prio field.  */
  uint8_t   cf_off_ctx;             /**< @brief Offset of @p p_ctx field.   */
  uint8_t   cf_off_newer;           /**< @brief Offset of @p p_newer field. */
  uint8_t   cf_off_older;           /**< @brief Offset of @p p_older field. */
  uint8_t   cf_off_name;            /**< @brief Offset of @p p_name field.  */
  uint8_t   cf_off_stklimit;        /**< @brief Offset of @p p_stklimit
                                                field.                      */
  uint8_t   cf_off_state;           /**< @brief Offset of @p p_state field. */
  uint8_t   cf_off_flags;           /**< @brief Offset of @p p_flags field. */
  uint8_t   cf_off_refs;            /**< @brief Offset of @p p_refs field.  */
  uint8_t   cf_off_preempt;         /**< @brief Offset of @p p_preempt
                                                field.                      */
  uint8_t   cf_off_time;            /**< @brief Offset of @p p_time field.  */
} chdebug_t;

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Sets the current thread name.
 * @pre     This function only stores the pointer to the name if the option
 *          @p CH_USE_REGISTRY is enabled else no action is performed.
 *
 * @param[in] p         thread name as a zero terminated string
 *
 * @api
 */
#define chRegSetThreadName(p) (currp->p_name = (p))

/**
 * @brief   Returns the name of the specified thread.
 * @pre     This function only returns the pointer to the name if the option
 *          @p CH_USE_REGISTRY is enabled else @p NULL is returned.
 *
 * @param[in] tp        pointer to the thread
 *
 * @return              Thread name as a zero terminated string.
 * @retval NULL         if the thread name has not been set.
 */
#define chRegGetThreadName(tp) ((tp)->p_name)
/** @} */
#else /* !CH_USE_REGISTRY */
#define chRegSetThreadName(p)
#define chRegGetThreadName(tp) NULL
#endif /* !CH_USE_REGISTRY */

#if CH_USE_REGISTRY || defined(__DOXYGEN__)
/**
 * @brief   Removes a thread from the registry list.
 * @note    This macro is not meant for use in application code.
 *
 * @param[in] tp        thread to remove from the registry
 */
#define REG_REMOVE(tp) {                                                    \
  (tp)->p_older->p_newer = (tp)->p_newer;                                   \
  (tp)->p_newer->p_older = (tp)->p_older;                                   \
}

/**
 * @brief   Adds a thread to the registry list.
 * @note    This macro is not meant for use in application code.
 *
 * @param[in] tp        thread to add to the registry
 */
#define REG_INSERT(tp) {                                                    \
  (tp)->p_newer = (Thread *)&rlist;                                         \
  (tp)->p_older = rlist.r_older;                                            \
  (tp)->p_older->p_newer = rlist.r_older = (tp);                            \
}

#ifdef __cplusplus
extern "C" {
#endif
  extern ROMCONST chdebug_t ch_debug;
  Thread *chRegFirstThread(void);
  Thread *chRegNextThread(Thread *tp);
#ifdef __cplusplus
}
#endif

#endif /* CH_USE_REGISTRY */

#endif /* _CHREGISTRY_H_ */

/** @} */
