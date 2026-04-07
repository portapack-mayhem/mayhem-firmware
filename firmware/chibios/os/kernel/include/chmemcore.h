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
 * @file    chmemcore.h
 * @brief   Core memory manager macros and structures.
 *
 * @addtogroup memcore
 * @{
 */

#ifndef _CHMEMCORE_H_
#define _CHMEMCORE_H_

/**
 * @brief   Memory get function.
 * @note    This type must be assignment compatible with the @p chMemAlloc()
 *          function.
 */
typedef void *(*memgetfunc_t)(size_t size);

/**
 * @name    Alignment support macros
 */
/**
 * @brief   Alignment size constant.
 */
#define MEM_ALIGN_SIZE      sizeof(stkalign_t)

/**
 * @brief   Alignment mask constant.
 */
#define MEM_ALIGN_MASK      (MEM_ALIGN_SIZE - 1)

/**
 * @brief   Alignment helper macro.
 */
#define MEM_ALIGN_PREV(p)   ((size_t)(p) & ~MEM_ALIGN_MASK)

/**
 * @brief   Alignment helper macro.
 */
#define MEM_ALIGN_NEXT(p)   MEM_ALIGN_PREV((size_t)(p) + MEM_ALIGN_MASK)

/**
 * @brief   Returns whatever a pointer or memory size is aligned to
 *          the type @p align_t.
 */
#define MEM_IS_ALIGNED(p)   (((size_t)(p) & MEM_ALIGN_MASK) == 0)
/** @} */

#if CH_USE_MEMCORE || defined(__DOXYGEN__)

#ifdef __cplusplus
extern "C" {
#endif
  void _core_init(void);
  void *chCoreAlloc(size_t size);
  void *chCoreAllocI(size_t size);
  size_t chCoreStatus(void);
#ifdef __cplusplus
}
#endif

#endif /* CH_USE_MEMCORE */

#endif /* _CHMEMCORE_H_ */

/** @} */
