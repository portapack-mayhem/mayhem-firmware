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

;/* <<< Use Configuration Wizard in Context Menu >>> */

;// <h> Main Stack Configuration (IRQ Stack)
;//   <o> Main Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h>
main_stack_size EQU     0x00000400

;// <h> Process Stack Configuration
;//   <o> Process Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h>
proc_stack_size EQU     0x00000400

;// <h> C-runtime heap size
;//   <o> C-runtime heap size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h>
heap_size       EQU     0x00000400

                AREA    MSTACK, NOINIT, READWRITE, ALIGN=3
main_stack_mem  SPACE   main_stack_size
                EXPORT  __initial_msp
__initial_msp

                AREA    CSTACK, NOINIT, READWRITE, ALIGN=3
__main_thread_stack_base__
                EXPORT  __main_thread_stack_base__
proc_stack_mem  SPACE   proc_stack_size
                EXPORT  __initial_sp
__initial_sp

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   heap_size
__heap_limit

CONTROL_MODE_PRIVILEGED     EQU     0
CONTROL_MODE_UNPRIVILEGED   EQU     1
CONTROL_USE_MSP             EQU     0
CONTROL_USE_PSP             EQU     2

                PRESERVE8
                THUMB

                AREA    |.text|, CODE, READONLY

/*
 * Reset handler.
 */
                IMPORT  __main
                EXPORT  Reset_Handler
Reset_Handler   PROC
                cpsid   i
                ldr     r0, =__initial_sp
                msr     PSP, r0
                movs    r0, #CONTROL_MODE_PRIVILEGED :OR: CONTROL_USE_PSP
                msr     CONTROL, r0
                isb
                bl      __early_init

                IF      {CPU} = "Cortex-M4.fp"
                LDR     R0, =0xE000ED88           ; Enable CP10,CP11
                LDR     R1, [R0]
                ORR     R1, R1, #(0xF << 20)
                STR     R1, [R0]
                ENDIF

                ldr     r0, =__main
                bx      r0
                ENDP

__early_init    PROC
                EXPORT  __early_init            [WEAK]
                bx      lr
                ENDP

                ALIGN

/*
 * User Initial Stack & Heap.
 */
                IF      :DEF:__MICROLIB
                
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                
                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap
                ldr     r0, =Heap_Mem
                ldr     r1, =(proc_stack_mem + proc_stack_size)
                ldr     r2, =(Heap_Mem + heap_size)
                ldr     r3, =proc_stack_mem
                bx      lr

                ALIGN

                ENDIF

                END
