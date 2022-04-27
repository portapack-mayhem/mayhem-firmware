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
 * @file    SPC560Dxx/bam.s
 * @brief   SPC560Dxx boot assistant record.
 *
 * @addtogroup PPC_CORE
 * @{
 */

#if !defined(__DOXYGEN__)

        /* BAM record.*/
        .section    .bam, "ax"
        .long       0x015A0000
        .long       _reset_address

        .align      2
        .globl      _reset_address
        .type       _reset_address, @function
_reset_address:
        bl          _coreinit
        bl          _ivinit

        b           _boot_address

#endif /* !defined(__DOXYGEN__) */

/** @} */
