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
 * @file    ARM/LPC214x/armparams.h
 * @brief   ARM7 LPC214x Specific Parameters.
 *
 * @defgroup ARM_LPC214x LPC214x Specific Parameters
 * @ingroup ARM_SPECIFIC
 * @details This file contains the ARM specific parameters for the
 *          LPC214x platform.
 * @{
 */

#ifndef _ARMPARAMS_H_
#define _ARMPARAMS_H_

/**
 * @brief   ARM core model.
 */
#define ARM_CORE                ARM_CORE_ARM7TDMI

/**
 * @brief   LPC214x-specific wait for interrupt code.
 * @details This implementation writes 1 into the PCON register.
 */
#if !defined(port_wait_for_interrupt) || defined(__DOXYGEN__)
#if ENABLE_WFI_IDLE || defined(__DOXYGEN__)
#define port_wait_for_interrupt() {                                         \
  (*((volatile uint32_t *)0xE01FC0C0)) = 1;                                 \
}
#else
#define port_wait_for_interrupt()
#endif
#endif

#endif /* _ARMPARAMS_H_ */

/** @} */
