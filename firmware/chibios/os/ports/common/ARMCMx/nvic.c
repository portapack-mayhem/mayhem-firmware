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
 * @file    common/ARMCMx/nvic.c
 * @brief   Cortex-Mx NVIC support code.
 *
 * @addtogroup COMMON_ARMCMx_NVIC
 * @{
 */

#include "ch.h"
#include "nvic.h"

/**
 * @brief   Sets the priority of an interrupt handler and enables it.
 * @note    The parameters are not tested for correctness.
 *
 * @param[in] n         the interrupt number
 * @param[in] prio      the interrupt priority mask
 */
void nvicEnableVector(uint32_t n, uint32_t prio) {
  unsigned sh = (n & 3) << 3;

  NVIC_IPR(n >> 2) = (NVIC_IPR(n >> 2) & ~(0xFF << sh)) | (prio << sh);
  NVIC_ICPR(n >> 5) = 1 << (n & 0x1F);
  NVIC_ISER(n >> 5) = 1 << (n & 0x1F);
}

/**
 * @brief   Disables an interrupt handler.
 * @note    The parameters are not tested for correctness.
 *
 * @param[in] n         the interrupt number
 */
void nvicDisableVector(uint32_t n) {
  unsigned sh = (n & 3) << 3;

  NVIC_ICER(n >> 5) = 1 << (n & 0x1F);
  NVIC_IPR(n >> 2) = NVIC_IPR(n >> 2) & ~(0xFF << sh);
}

/**
 * @brief   Changes the priority of a system handler.
 * @note    The parameters are not tested for correctness.
 *
 * @param[in] handler   the system handler number
 * @param[in] prio      the system handler priority mask
 */
void nvicSetSystemHandlerPriority(uint32_t handler, uint32_t prio) {
  unsigned sh = (handler & 3) * 8;

  SCB_SHPR(handler >> 2) = (SCB_SHPR(handler >> 2) &
                           ~(0xFF << sh)) | (prio << sh);
}

/** @} */
