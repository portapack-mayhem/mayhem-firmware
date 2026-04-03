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
 * @file    chvt.h
 * @brief   Time macros and structures.
 *
 * @addtogroup time
 * @{
 */

#ifndef _CHVT_H_
#define _CHVT_H_

/**
 * @name    Time conversion utilities
 * @{
 */
/**
 * @brief   Seconds to system ticks.
 * @details Converts from seconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] sec       number of seconds
 * @return              The number of ticks.
 *
 * @api
 */
#define S2ST(sec)                                                           \
  ((systime_t)((sec) * CH_FREQUENCY))

/**
 * @brief   Milliseconds to system ticks.
 * @details Converts from milliseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] msec      number of milliseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define MS2ST(msec)                                                         \
  ((systime_t)(((((uint32_t)(msec)) * ((uint32_t)CH_FREQUENCY) - 1UL) /     \
                1000UL) + 1UL))

/**
 * @brief   Microseconds to system ticks.
 * @details Converts from microseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] usec      number of microseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define US2ST(usec)                                                         \
  ((systime_t)(((((uint32_t)(usec)) * ((uint32_t)CH_FREQUENCY) - 1UL) /     \
                1000000UL) + 1UL))
/** @} */

/**
 * @brief   Virtual Timer callback function.
 */
typedef void (*vtfunc_t)(void *);

/**
 * @brief   Virtual Timer structure type.
 */
typedef struct VirtualTimer VirtualTimer;

/**
 * @extends VTList
 *
 * @brief   Virtual Timer descriptor structure.
 */
struct VirtualTimer {
  VirtualTimer          *vt_next;   /**< @brief Next timer in the delta
                                                list.                       */
  VirtualTimer          *vt_prev;   /**< @brief Previous timer in the delta
                                                list.                       */
  systime_t             vt_time;    /**< @brief Time delta before timeout.  */
  vtfunc_t              vt_func;    /**< @brief Timer callback function
                                                pointer.                    */
  void                  *vt_par;    /**< @brief Timer callback function
                                                parameter.                  */
};

/**
 * @brief   Virtual timers list header.
 * @note    The delta list is implemented as a double link bidirectional list
 *          in order to make the unlink time constant, the reset of a virtual
 *          timer is often used in the code.
 */
typedef struct {
  VirtualTimer          *vt_next;   /**< @brief Next timer in the delta
                                                list.                       */
  VirtualTimer          *vt_prev;   /**< @brief Last timer in the delta
                                                list.                       */
  systime_t             vt_time;    /**< @brief Must be initialized to -1.  */
  volatile systime_t    vt_systime; /**< @brief System Time counter.        */
} VTList;

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Virtual timers ticker.
 * @note    The system lock is released before entering the callback and
 *          re-acquired immediately after. It is callback's responsibility
 *          to acquire the lock if needed. This is done in order to reduce
 *          interrupts jitter when many timers are in use.
 *
 * @iclass
 */
#define chVTDoTickI() {                                                     \
  vtlist.vt_systime++;                                                      \
  if (&vtlist != (VTList *)vtlist.vt_next) {                                \
    VirtualTimer *vtp;                                                      \
                                                                            \
    --vtlist.vt_next->vt_time;                                              \
    while (!(vtp = vtlist.vt_next)->vt_time) {                              \
      vtfunc_t fn = vtp->vt_func;                                           \
      vtp->vt_func = (vtfunc_t)NULL;                                        \
      vtp->vt_next->vt_prev = (void *)&vtlist;                              \
      (&vtlist)->vt_next = vtp->vt_next;                                    \
      chSysUnlockFromIsr();                                                 \
      fn(vtp->vt_par);                                                      \
      chSysLockFromIsr();                                                   \
    }                                                                       \
  }                                                                         \
}

/**
 * @brief   Returns @p TRUE if the specified timer is armed.
 *
 * @iclass
 */
#define chVTIsArmedI(vtp) ((vtp)->vt_func != NULL)

/**
 * @brief   Enables a virtual timer.
 * @note    The associated function is invoked from interrupt context.
 *
 * @param[out] vtp      the @p VirtualTimer structure pointer
 * @param[in] time      the number of ticks before the operation timeouts, the
 *                      special values are handled as follow:
 *                      - @a TIME_INFINITE is allowed but interpreted as a
 *                        normal time specification.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 * @param[in] vtfunc    the timer callback function. After invoking the
 *                      callback the timer is disabled and the structure can
 *                      be disposed or reused.
 * @param[in] par       a parameter that will be passed to the callback
 *                      function
 *
 * @api
 */
#define chVTSet(vtp, time, vtfunc, par) {                                   \
  chSysLock();                                                              \
  chVTSetI(vtp, time, vtfunc, par);                                         \
  chSysUnlock();                                                            \
}

/**
 * @brief   Disables a Virtual Timer.
 * @note    The timer is first checked and disabled only if armed.
 *
 * @param[in] vtp       the @p VirtualTimer structure pointer
 *
 * @api
 */
#define chVTReset(vtp) {                                                    \
  chSysLock();                                                              \
  if (chVTIsArmedI(vtp))                                                    \
    chVTResetI(vtp);                                                        \
  chSysUnlock();                                                            \
}

/**
 * @brief   Current system time.
 * @details Returns the number of system ticks since the @p chSysInit()
 *          invocation.
 * @note    The counter can reach its maximum and then restart from zero.
 * @note    This function is designed to work with the @p chThdSleepUntil().
 *
 * @return              The system time in ticks.
 *
 * @api
 */
#define chTimeNow() (vtlist.vt_systime)

/**
 * @brief   Returns the elapsed time since the specified start time.
 *
 * @param[in] start     start time
 * @return              The elapsed time.
 *
 * @api
 */
#define chTimeElapsedSince(start) (chTimeNow() - (start))

/**
 * @brief   Checks if the current system time is within the specified time
 *          window.
 * @note    When start==end then the function returns always true because the
 *          whole time range is specified.
 *
 * @param[in] start     the start of the time window (inclusive)
 * @param[in] end       the end of the time window (non inclusive)
 * @retval TRUE         current time within the specified time window.
 * @retval FALSE        current time not within the specified time window.
 *
 * @api
 */
#define chTimeIsWithin(start, end)                                          \
  (chTimeElapsedSince(start) < ((end) - (start)))
/** @} */

extern VTList vtlist;

/*
 * Virtual Timers APIs.
 */
#ifdef __cplusplus
extern "C" {
#endif
  void _vt_init(void);
  void chVTSetI(VirtualTimer *vtp, systime_t time, vtfunc_t vtfunc, void *par);
  void chVTResetI(VirtualTimer *vtp);
#ifdef __cplusplus
}
#endif

#endif /* _CHVT_H_ */

/** @} */
