/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file Win32/hal_lld.c
 * @brief Win32 HAL subsystem low level driver code.
 * @addtogroup WIN32_HAL
 * @{
 */

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static LARGE_INTEGER nextcnt;
static LARGE_INTEGER slice;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief Low level HAL driver initialization.
 */
void hal_lld_init(void) {
  WSADATA wsaData;

  /* Initialization.*/
  if (WSAStartup(2, &wsaData) != 0) {
    printf("Unable to locate a winsock DLL\n");
    exit(1);
  }

  printf("ChibiOS/RT simulator (Win32)\n");
  if (!QueryPerformanceFrequency(&slice)) {
    printf("QueryPerformanceFrequency() error");
    exit(1);
  }
  slice.QuadPart /= CH_FREQUENCY;
  QueryPerformanceCounter(&nextcnt);
  nextcnt.QuadPart += slice.QuadPart;

  fflush(stdout);
}

/**
 * @brief Interrupt simulation.
 */
void ChkIntSources(void) {
  LARGE_INTEGER n;

#if HAL_USE_SERIAL
  if (sd_lld_interrupt_pending()) {
    dbg_check_lock();
    if (chSchIsPreemptionRequired())
      chSchDoReschedule();
    dbg_check_unlock();
    return;
  }
#endif

  /* Interrupt Timer simulation (10ms interval).*/
  QueryPerformanceCounter(&n);
  if (n.QuadPart > nextcnt.QuadPart) {
    nextcnt.QuadPart += slice.QuadPart;

    CH_IRQ_PROLOGUE();

    chSysLockFromIsr();
    chSysTimerHandlerI();
    chSysUnlockFromIsr();

    CH_IRQ_EPILOGUE();

    dbg_check_lock();
    if (chSchIsPreemptionRequired())
      chSchDoReschedule();
    dbg_check_unlock();
  }
}

/** @} */
