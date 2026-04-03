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
 * @file    PPC/crt0.s
 * @brief   Generic PowerPC startup file for ChibiOS/RT.
 *
 * @addtogroup PPC_CORE
 * @{
 */

#if !defined(__DOXYGEN__)

        .section    .crt0, "ax"
        .align		2
        .globl      _boot_address
        .type       _boot_address, @function
_boot_address:
        /*
         * Stack setup.
         */
        lis         %r1, __process_stack_end__@h
        ori         %r1, %r1, __process_stack_end__@l
        li          %r0, 0
        stwu        %r0, -8(%r1)
        /*
         * Small sections registers initialization.
         */
        lis         %r2, __sdata2_start__@h
        ori         %r2, %r2, __sdata2_start__@l
        lis         %r13, __sdata_start__@h
        ori         %r13, %r13, __sdata_start__@l
        /*
         * Early initialization.
         */
        bl          __early_init
        /*
         * BSS clearing.
         */
        lis         %r4, __bss_start__@h
        ori         %r4, %r4, __bss_start__@l
        lis         %r5, __bss_end__@h
        ori         %r5, %r5, __bss_end__@l
        li          %r7, 0
.bssloop:
        cmpl        cr0, %r4, %r5
        bge         cr0, .bssend
        stw         %r7, 0(%r4)
        addi        %r4, %r4, 4
        b           .bssloop
.bssend:
        /*
         * DATA initialization.
         */
        lis         %r4, __romdata_start__@h
        ori         %r4, %r4, __romdata_start__@l
        lis         %r5, __data_start__@h
        ori         %r5, %r5, __data_start__@l
        lis         %r6, __data_end__@h
        ori         %r6, %r6, __data_end__@l
.dataloop:
        cmpl        cr0, %r5, %r6
        bge         cr0, .dataend
        lwz         %r7, 0(%r4)
        addi        %r4, %r4, 4
        stw         %r7, 0(%r5)
        addi        %r5, %r5, 4
        b           .dataloop
.dataend:
        /*
         * Late initialization.
         */
        bl          __late_init
        /*
         * Main program invocation.
         */
        bl          main
        b           _main_exit_handler

        /*
         * Default main exit code, infinite loop.
         */
        .weak       _main_exit_handler
        .globl      _main_exit_handler
        .type       _main_exit_handler, @function
_main_exit_handler:
        b           _main_exit_handler

        /*
         * Default early initialization code, none.
         */
        .weak       __early_init
        .globl      __early_init
        .type       __early_init, @function
__early_init:
        blr

        /*
         * Default late initialization code, none.
         */
        .weak       __late_init
        .globl      __late_init
        .type       __late_init, @function
__late_init:
        blr

#endif /* !defined(__DOXYGEN__) */

/** @} */
