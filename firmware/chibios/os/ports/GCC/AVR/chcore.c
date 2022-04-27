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
 * @file    AVR/chcore.c
 * @brief   AVR architecture port code.
 *
 * @addtogroup AVR_CORE
 * @{
 */

#include "ch.h"

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 * @note    The function is declared as a weak symbol, it is possible to
 *          redefine it in your application code.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 */
#if !defined(__DOXYGEN__)
__attribute__((naked, weak))
#endif
void port_switch(Thread *ntp, Thread *otp) {

  asm volatile ("push    r2");
  asm volatile ("push    r3");
  asm volatile ("push    r4");
  asm volatile ("push    r5");
  asm volatile ("push    r6");
  asm volatile ("push    r7");
  asm volatile ("push    r8");
  asm volatile ("push    r9");
  asm volatile ("push    r10");
  asm volatile ("push    r11");
  asm volatile ("push    r12");
  asm volatile ("push    r13");
  asm volatile ("push    r14");
  asm volatile ("push    r15");
  asm volatile ("push    r16");
  asm volatile ("push    r17");
  asm volatile ("push    r28");
  asm volatile ("push    r29");

  asm volatile ("movw    r30, r22");
  asm volatile ("in      r0, 0x3d");
  asm volatile ("std     Z+5, r0");
  asm volatile ("in      r0, 0x3e");
  asm volatile ("std     Z+6, r0");

  asm volatile ("movw    r30, r24");
  asm volatile ("ldd     r0, Z+5");
  asm volatile ("out     0x3d, r0");
  asm volatile ("ldd     r0, Z+6");
  asm volatile ("out     0x3e, r0");

  asm volatile ("pop     r29");
  asm volatile ("pop     r28");
  asm volatile ("pop     r17");
  asm volatile ("pop     r16");
  asm volatile ("pop     r15");
  asm volatile ("pop     r14");
  asm volatile ("pop     r13");
  asm volatile ("pop     r12");
  asm volatile ("pop     r11");
  asm volatile ("pop     r10");
  asm volatile ("pop     r9");
  asm volatile ("pop     r8");
  asm volatile ("pop     r7");
  asm volatile ("pop     r6");
  asm volatile ("pop     r5");
  asm volatile ("pop     r4");
  asm volatile ("pop     r3");
  asm volatile ("pop     r2");
  asm volatile ("ret");
}

/**
 * @brief   Halts the system.
 * @details This function is invoked by the operating system when an
 *          unrecoverable error is detected (for example because a programming
 *          error in the application code that triggers an assertion while in
 *          debug mode).
 * @note    The function is declared as a weak symbol, it is possible to
 *          redefine it in your application code.
 */
#if !defined(__DOXYGEN__)
__attribute__((weak))
#endif
void port_halt(void) {

  port_disable();
  while (TRUE) {
  }
}

/**
 * @brief   Start a thread by invoking its work function.
 * @details If the work function returns @p chThdExit() is automatically
 *          invoked.
 */
void _port_thread_start(void) {

  chSysUnlock();
  asm volatile ("movw    r24, r4");
  asm volatile ("movw    r30, r2");
  asm volatile ("icall");
  asm volatile ("call    chThdExit");
}

/** @} */
