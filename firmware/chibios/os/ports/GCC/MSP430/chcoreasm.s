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

#include "chconf.h"

#define FALSE 0
#define TRUE 1

        .text
        .p2align 1, 0
        .weak   _port_switch
_port_switch:
        push    r11
        push    r10
        push    r9
        push    r8
        push    r7
        push    r6
        push    r5
        push    r4
        mov r1, 6(r14)
        mov 6(r15), r1
        pop     r4
        pop     r5
        pop     r6
        pop     r7
        pop     r8
        pop     r9
        pop     r10
        pop     r11
        ret

        .p2align 1, 0
        .weak   _port_thread_start
_port_thread_start:
#if CH_DBG_SYSTEM_STATE_CHECK
        call    #dbg_check_unlock
#endif
        eint
        mov     r11, r15
        call    r10
        call    #chThdExit
        ; Falls into _port_halt

        .p2align 1, 0
        .weak   _port_halt
_port_halt:
        dint
.L1:    jmp     .L1
