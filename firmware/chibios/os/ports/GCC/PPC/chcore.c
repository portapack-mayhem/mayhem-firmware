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
 * @file    PPC/chcore.c
 * @brief   PowerPC architecture port code.
 *
 * @addtogroup PPC_CORE
 * @{
 */

#include "ch.h"

/**
 * @brief   Kernel port layer initialization.
 * @details IVOR4 and IVOR10 initialization.
 */
void port_init(void) {
#if PPC_SUPPORTS_IVORS
    /* The CPU supports IVOR registers, the kernel requires IVOR4 and IVOR10
       and the initialization is performed here.*/
    asm volatile ("li          %%r3, _IVOR4@l       \t\n"
                  "mtIVOR4     %%r3                 \t\n"
                  "li          %%r3, _IVOR10@l      \t\n"
                  "mtIVOR10    %%r3" : : : "memory");
#endif
}

/**
 * @brief   Halts the system.
 * @details This function is invoked by the operating system when an
 *          unrecoverable error is detected (for example because a programming
 *          error in the application code that triggers an assertion while
 *          in debug mode).
 */
void port_halt(void) {

  port_disable();
  while (TRUE) {
  }
}

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 */
#if !defined(__DOXYGEN__)
__attribute__((naked))
#endif
void port_dummy1(void) {

  asm (".global _port_switch");
  asm ("_port_switch:");
  asm ("subi        %sp, %sp, 80");     /* Size of the intctx structure.    */
  asm ("mflr        %r0");
  asm ("stw         %r0, 84(%sp)");     /* LR into the caller frame.        */
  asm ("mfcr        %r0");
  asm ("stw         %r0, 0(%sp)");      /* CR.                              */
  asm ("stmw        %r14, 4(%sp)");     /* GPR14...GPR31.                   */
  
  asm ("stw         %sp, 12(%r4)");     /* Store swapped-out stack.         */
  asm ("lwz         %sp, 12(%r3)");     /* Load swapped-in stack.           */
  
  asm ("lmw         %r14, 4(%sp)");     /* GPR14...GPR31.                   */
  asm ("lwz         %r0, 0(%sp)");      /* CR.                              */
  asm ("mtcr        %r0");
  asm ("lwz         %r0, 84(%sp)");     /* LR from the caller frame.        */
  asm ("mtlr        %r0");
  asm ("addi        %sp, %sp, 80");     /* Size of the intctx structure.    */
  asm ("blr");
}

/**
 * @brief   Start a thread by invoking its work function.
 * @details If the work function returns @p chThdExit() is automatically
 *          invoked.
 */
#if !defined(__DOXYGEN__)
__attribute__((naked))
#endif
void port_dummy2(void) {

  asm (".global _port_thread_start");
  asm ("_port_thread_start:");
  chSysUnlock();
  asm ("mr          %r3, %r31");        /* Thread parameter.                */
  asm ("mtctr       %r30");
  asm ("bctrl");                        /* Invoke thread function.          */
  asm ("bl          chThdExit");        /* Thread termination on exit.      */
}

/** @} */
