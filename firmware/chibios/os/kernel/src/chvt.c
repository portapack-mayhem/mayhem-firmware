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
 * @file    chvt.c
 * @brief   Time and Virtual Timers related code.
 *
 * @addtogroup time
 * @details Time and Virtual Timers related APIs and services.
 * @{
 */

#include "ch.h"

/**
 * @brief   Virtual timers delta list header.
 */
VTList vtlist;

/**
 * @brief   Virtual Timers initialization.
 * @note    Internal use only.
 *
 * @notapi
 */
void _vt_init(void) {

  vtlist.vt_next = vtlist.vt_prev = (void *)&vtlist;
  vtlist.vt_time = (systime_t)-1;
  vtlist.vt_systime = 0;
}

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
 * @iclass
 */
void chVTSetI(VirtualTimer *vtp, systime_t time, vtfunc_t vtfunc, void *par) {
  VirtualTimer *p;

  chDbgCheckClassI();
  chDbgCheck((vtp != NULL) && (vtfunc != NULL) && (time != TIME_IMMEDIATE),
             "chVTSetI");

  vtp->vt_par = par;
  vtp->vt_func = vtfunc;
  p = vtlist.vt_next;
  while (p->vt_time < time) {
    time -= p->vt_time;
    p = p->vt_next;
  }

  vtp->vt_prev = (vtp->vt_next = p)->vt_prev;
  vtp->vt_prev->vt_next = p->vt_prev = vtp;
  vtp->vt_time = time;
  if (p != (void *)&vtlist)
    p->vt_time -= time;
}

/**
 * @brief   Disables a Virtual Timer.
 * @note    The timer MUST be active when this function is invoked.
 *
 * @param[in] vtp       the @p VirtualTimer structure pointer
 *
 * @iclass
 */
void chVTResetI(VirtualTimer *vtp) {

  chDbgCheckClassI();
  chDbgCheck(vtp != NULL, "chVTResetI");
  chDbgAssert(vtp->vt_func != NULL,
              "chVTResetI(), #1",
              "timer not set or already triggered");

  if (vtp->vt_next != (void *)&vtlist)
    vtp->vt_next->vt_time += vtp->vt_time;
  vtp->vt_prev->vt_next = vtp->vt_next;
  vtp->vt_next->vt_prev = vtp->vt_prev;
  vtp->vt_func = (vtfunc_t)NULL;
}

/** @} */
