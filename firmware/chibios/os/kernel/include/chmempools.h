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
 * @file    chmempools.h
 * @brief   Memory Pools macros and structures.
 *
 * @addtogroup pools
 * @{
 */

#ifndef _CHMEMPOOLS_H_
#define _CHMEMPOOLS_H_

#if CH_USE_MEMPOOLS || defined(__DOXYGEN__)

/**
 * @brief   Memory pool free object header.
 */
struct pool_header {
  struct pool_header    *ph_next;       /**< @brief Pointer to the next pool
                                                    header in the list.     */
};

/**
 * @brief   Memory pool descriptor.
 */
typedef struct {
  struct pool_header    *mp_next;       /**< @brief Pointer to the header.  */
  size_t                mp_object_size; /**< @brief Memory pool objects
                                                    size.                   */
  memgetfunc_t          mp_provider;    /**< @brief Memory blocks provider for
                                                    this pool.              */
} MemoryPool;

/**
 * @brief   Data part of a static memory pool initializer.
 * @details This macro should be used when statically initializing a
 *          memory pool that is part of a bigger structure.
 *
 * @param[in] name      the name of the memory pool variable
 * @param[in] size      size of the memory pool contained objects
 * @param[in] provider  memory provider function for the memory pool
 */
#define _MEMORYPOOL_DATA(name, size, provider)                              \
  {NULL, size, provider}

/**
 * @brief Static memory pool initializer in hungry mode.
 * @details Statically initialized memory pools require no explicit
 *          initialization using @p chPoolInit().
 *
 * @param[in] name the name of the memory pool variable
 * @param[in] size size of the memory pool contained objects
 * @param[in] provider memory provider function for the memory pool or @p NULL
 *                     if the pool is not allowed to grow automatically
 */
#define MEMORYPOOL_DECL(name, size, provider)                               \
  MemoryPool name = _MEMORYPOOL_DATA(name, size, provider)

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Adds an object to a memory pool.
 * @pre     The memory pool must be already been initialized.
 * @pre     The added object must be of the right size for the specified
 *          memory pool.
 * @pre     The added object must be memory aligned to the size of
 *          @p stkalign_t type.
 * @note    This function is just an alias for @p chPoolFree() and has been
 *          added for clarity.
 *
 * @param[in] mp        pointer to a @p MemoryPool structure
 * @param[in] objp      the pointer to the object to be added
 *
 * @api
 */
#define chPoolAdd(mp, objp) chPoolFree(mp, objp)

/**
 * @brief   Adds an object to a memory pool.
 * @pre     The memory pool must be already been initialized.
 * @pre     The added object must be of the right size for the specified
 *          memory pool.
 * @pre     The added object must be memory aligned to the size of
 *          @p stkalign_t type.
 * @note    This function is just an alias for @p chPoolFree() and has been
 *          added for clarity.
 *
 * @param[in] mp        pointer to a @p MemoryPool structure
 * @param[in] objp      the pointer to the object to be added
 *
 * @iclass
 */
#define chPoolAddI(mp, objp) chPoolFreeI(mp, objp)
/** @} */

#ifdef __cplusplus
extern "C" {
#endif
  void chPoolInit(MemoryPool *mp, size_t size, memgetfunc_t provider);
  void chPoolLoadArray(MemoryPool *mp, void *p, size_t n);
  void *chPoolAllocI(MemoryPool *mp);
  void *chPoolAlloc(MemoryPool *mp);
  void chPoolFreeI(MemoryPool *mp, void *objp);
  void chPoolFree(MemoryPool *mp, void *objp);
#ifdef __cplusplus
}
#endif

#endif /* CH_USE_MEMPOOLS */

#endif /* _CHMEMPOOLS_H_ */

/** @} */
