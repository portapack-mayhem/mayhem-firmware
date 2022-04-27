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
 * @file    ARM/LPC214x/vectors.s
 * @brief   Interrupt vectors for the LPC214x family.
 *
 * @defgroup ARM_LPC214x_VECTORS LPC214x Interrupt Vectors
 * @ingroup ARM_SPECIFIC
 * @details Interrupt vectors for the LPC214x family.
 * @{
 */

#if defined(__DOXYGEN__)
/**
 * @brief   Unhandled exceptions handler.
 * @details Any undefined exception vector points to this function by default.
 *          This function simply stops the system into an infinite loop.
 *
 * @notapi
 */
void _unhandled_exception(void) {}
#endif

#if !defined(__DOXYGEN__)

.section vectors
.code 32
.balign 4
/*
 * System entry points.
 */
_start:
        ldr     pc, _reset
        ldr     pc, _undefined
        ldr     pc, _swi
        ldr     pc, _prefetch
        ldr     pc, _abort
        nop
        ldr     pc, [pc,#-0xFF0]        /* VIC - IRQ Vector Register */
        ldr     pc, _fiq

_reset:
        .word   ResetHandler            /* In crt0.s */
_undefined:
        .word   UndHandler
_swi:
        .word   SwiHandler
_prefetch:
        .word   PrefetchHandler
_abort:
        .word   AbortHandler
_fiq:
        .word   FiqHandler
        .word   0
        .word   0

/*
 * Default exceptions handlers. The handlers are declared weak in order to be
 * replaced by the real handling code. Everything is defaulted to an infinite
 * loop.
 */
.weak UndHandler
UndHandler:

.weak SwiHandler
SwiHandler:

.weak PrefetchHandler
PrefetchHandler:

.weak AbortHandler
AbortHandler:

.weak FiqHandler
FiqHandler:

.global _unhandled_exception
_unhandled_exception:
        b       _unhandled_exception

#endif

/** @} */
