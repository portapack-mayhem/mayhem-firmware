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
 * @file    chdynamic.c
 * @brief   Dynamic threads code.
 *
 * @addtogroup dynamic_threads
 * @details Dynamic threads related APIs and services.
 * @{
 */

#include "ch.h"

#if CH_USE_DYNAMIC || defined(__DOXYGEN__)

/**
 * @brief   Adds a reference to a thread object.
 * @pre     The configuration option @p CH_USE_DYNAMIC must be enabled in order
 *          to use this function.
 *
 * @param[in] tp        pointer to the thread
 * @return              The same thread pointer passed as parameter
 *                      representing the new reference.
 *
 * @api
 */
Thread *chThdAddRef(Thread *tp) {

  chSysLock();
  chDbgAssert(tp->p_refs < 255, "chThdAddRef(), #1", "too many references");
  tp->p_refs++;
  chSysUnlock();
  return tp;
}

/**
 * @brief   Releases a reference to a thread object.
 * @details If the references counter reaches zero <b>and</b> the thread
 *          is in the @p THD_STATE_FINAL state then the thread's memory is
 *          returned to the proper allocator.
 * @pre     The configuration option @p CH_USE_DYNAMIC must be enabled in order
 *          to use this function.
 * @note    Static threads are not affected.
 *
 * @param[in] tp        pointer to the thread
 *
 * @api
 */
void chThdRelease(Thread *tp) {
  trefs_t refs;

  chSysLock();
  chDbgAssert(tp->p_refs > 0, "chThdRelease(), #1", "not referenced");
  refs = --tp->p_refs;
  chSysUnlock();

  /* If the references counter reaches zero and the thread is in its
     terminated state then the memory can be returned to the proper
     allocator. Of course static threads are not affected.*/
  if ((refs == 0) && (tp->p_state == THD_STATE_FINAL)) {
    switch (tp->p_flags & THD_MEM_MODE_MASK) {
#if CH_USE_HEAP
    case THD_MEM_MODE_HEAP:
#if CH_USE_REGISTRY
      REG_REMOVE(tp);
#endif
      chHeapFree(tp);
      break;
#endif
#if CH_USE_MEMPOOLS
    case THD_MEM_MODE_MEMPOOL:
#if CH_USE_REGISTRY
      REG_REMOVE(tp);
#endif
      chPoolFree(tp->p_mpool, tp);
      break;
#endif
    }
  }
}

#if CH_USE_HEAP || defined(__DOXYGEN__)
/**
 * @brief   Creates a new thread allocating the memory from the heap.
 * @pre     The configuration options @p CH_USE_DYNAMIC and @p CH_USE_HEAP
 *          must be enabled in order to use this function.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The memory allocated for the thread is not released when the thread
 *          terminates but when a @p chThdWait() is performed.
 *
 * @param[in] heapp     heap from which allocate the memory or @p NULL for the
 *                      default heap
 * @param[in] size      size of the working area to be allocated
 * @param[in] prio      the priority level for the new thread
 * @param[in] pf        the thread function
 * @param[in] arg       an argument passed to the thread function. It can be
 *                      @p NULL.
 * @return              The pointer to the @p Thread structure allocated for
 *                      the thread into the working space area.
 * @retval NULL         if the memory cannot be allocated.
 *
 * @api
 */
Thread *chThdCreateFromHeap(MemoryHeap *heapp, size_t size,
                            tprio_t prio, tfunc_t pf, void *arg) {
  void *wsp;
  Thread *tp;

  wsp = chHeapAlloc(heapp, size);
  if (wsp == NULL)
    return NULL;
  
#if CH_DBG_FILL_THREADS
  _thread_memfill((uint8_t *)wsp,
                  (uint8_t *)wsp + sizeof(Thread),
                  CH_THREAD_FILL_VALUE);
  _thread_memfill((uint8_t *)wsp + sizeof(Thread),
                  (uint8_t *)wsp + size,
                  CH_STACK_FILL_VALUE);
#endif
  
  chSysLock();
  tp = chThdCreateI(wsp, size, prio, pf, arg);
  tp->p_flags = THD_MEM_MODE_HEAP;
  chSchWakeupS(tp, RDY_OK);
  chSysUnlock();
  return tp;
}
#endif /* CH_USE_HEAP */

#if CH_USE_MEMPOOLS || defined(__DOXYGEN__)
/**
 * @brief   Creates a new thread allocating the memory from the specified
 *          memory pool.
 * @pre     The configuration options @p CH_USE_DYNAMIC and @p CH_USE_MEMPOOLS
 *          must be enabled in order to use this function.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The memory allocated for the thread is not released when the thread
 *          terminates but when a @p chThdWait() is performed.
 *
 * @param[in] mp        pointer to the memory pool object
 * @param[in] prio      the priority level for the new thread
 * @param[in] pf        the thread function
 * @param[in] arg       an argument passed to the thread function. It can be
 *                      @p NULL.
 * @return              The pointer to the @p Thread structure allocated for
 *                      the thread into the working space area.
 * @retval  NULL        if the memory pool is empty.
 *
 * @api
 */
Thread *chThdCreateFromMemoryPool(MemoryPool *mp, tprio_t prio,
                                  tfunc_t pf, void *arg) {
  void *wsp;
  Thread *tp;

  chDbgCheck(mp != NULL, "chThdCreateFromMemoryPool");

  wsp = chPoolAlloc(mp);
  if (wsp == NULL)
    return NULL;
  
#if CH_DBG_FILL_THREADS
  _thread_memfill((uint8_t *)wsp,
                  (uint8_t *)wsp + sizeof(Thread),
                  CH_THREAD_FILL_VALUE);
  _thread_memfill((uint8_t *)wsp + sizeof(Thread),
                  (uint8_t *)wsp + mp->mp_object_size,
                  CH_STACK_FILL_VALUE);
#endif

  chSysLock();
  tp = chThdCreateI(wsp, mp->mp_object_size, prio, pf, arg);
  tp->p_flags = THD_MEM_MODE_MEMPOOL;
  tp->p_mpool = mp;
  chSchWakeupS(tp, RDY_OK);
  chSysUnlock();
  return tp;
}
#endif /* CH_USE_MEMPOOLS */

#endif /* CH_USE_DYNAMIC */

/** @} */
