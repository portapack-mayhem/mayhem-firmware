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
 * @file    GCC/ARMCMx/LPC8xx/cmparams.h
 * @brief   ARM Cortex-M0+ parameters for the LPC8xx.
 *
 * @defgroup ARMCMx_LPC8xx LPC8xx Specific Parameters
 * @ingroup ARMCMx_SPECIFIC
 * @details This file contains the Cortex-M0+ specific parameters for the
 *          LPC8xx platform.
 *          (Taken from the device header file where possible)
 * @{
 */

#ifndef _CMPARAMS_H_
#define _CMPARAMS_H_

#include "LPC8xx.h"

/**
 * @brief   Cortex core model.
 */
#define CORTEX_MODEL __CORTEX_M

/**
 * @brief   Systick unit presence.
 */
#define CORTEX_HAS_ST TRUE

/**
 * @brief   Memory Protection unit presence.
 */
#define CORTEX_HAS_MPU __MPU_PRESENT

/**
 * @brief   Number of bits in priority masks.
 */
#define CORTEX_PRIORITY_BITS __NVIC_PRIO_BITS

#endif /* _CMPARAMS_H_ */

/** @} */
