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
/*
 * **** This file incorporates work covered by the following copyright and ****
 * **** permission notice:                                                 ****
 *
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

// see http://lwip.wikia.com/wiki/Porting_for_an_OS for instructions

#include "ch.h"

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

#include "arch/cc.h"
#include "arch/sys_arch.h"

void sys_init(void) {

}

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {

  *sem = chHeapAlloc(NULL, sizeof(Semaphore));
  if (*sem == 0) {
    SYS_STATS_INC(sem.err);
    return ERR_MEM;
  }
  else {
    chSemInit(*sem, (cnt_t)count);
    SYS_STATS_INC_USED(sem);
    return ERR_OK;
  }
}

void sys_sem_free(sys_sem_t *sem) {

  chHeapFree(*sem);
  *sem = SYS_SEM_NULL;
  SYS_STATS_DEC(sem.used);
}

void sys_sem_signal(sys_sem_t *sem) {

  chSemSignal(*sem);
}

/* CHIBIOS FIX: specific variant of this call to be called from within
   a lock.*/
void sys_sem_signal_S(sys_sem_t *sem) {

  chSemSignalI(*sem);
  chSchRescheduleS();
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
  systime_t time, tmo;

  chSysLock();
  tmo = timeout > 0 ? (systime_t)timeout : TIME_INFINITE;
  time = chTimeNow();
  if (chSemWaitTimeoutS(*sem, tmo) != RDY_OK)
    time = SYS_ARCH_TIMEOUT;
  else
    time = chTimeNow() - time;
  chSysUnlock();
  return time;
}

int sys_sem_valid(sys_sem_t *sem) {
  return *sem != SYS_SEM_NULL;
}

// typically called within lwIP after freeing a semaphore
// to make sure the pointer is not left pointing to invalid data
void sys_sem_set_invalid(sys_sem_t *sem) {
  *sem = SYS_SEM_NULL;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
  
  *mbox = chHeapAlloc(NULL, sizeof(Mailbox) + sizeof(msg_t) * size);
  if (*mbox == 0) {
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }
  else {
    chMBInit(*mbox, (void *)(((uint8_t *)*mbox) + sizeof(Mailbox)), size);
    SYS_STATS_INC(mbox.used);
    return ERR_OK;
  }
}

void sys_mbox_free(sys_mbox_t *mbox) {

  if (chMBGetUsedCountI(*mbox) != 0) {
    // If there are messages still present in the mailbox when the mailbox
    // is deallocated, it is an indication of a programming error in lwIP
    // and the developer should be notified.
    SYS_STATS_INC(mbox.err);
    chMBReset(*mbox);
  }
  chHeapFree(*mbox);
  *mbox = SYS_MBOX_NULL;
  SYS_STATS_DEC(mbox.used);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {

  chMBPost(*mbox, (msg_t)msg, TIME_INFINITE);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {

  if (chMBPost(*mbox, (msg_t)msg, TIME_IMMEDIATE) == RDY_TIMEOUT) {
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }
  return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
  systime_t time, tmo;

  chSysLock();
  tmo = timeout > 0 ? (systime_t)timeout : TIME_INFINITE;
  time = chTimeNow();
  if (chMBFetchS(*mbox, (msg_t *)msg, tmo) != RDY_OK)
    time = SYS_ARCH_TIMEOUT;
  else
    time = chTimeNow() - time;
  chSysUnlock();
  return time;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {

  if (chMBFetch(*mbox, (msg_t *)msg, TIME_IMMEDIATE) == RDY_TIMEOUT)
    return SYS_MBOX_EMPTY;
  return 0;
}

int sys_mbox_valid(sys_mbox_t *mbox) {
  return *mbox != SYS_MBOX_NULL;
}

// typically called within lwIP after freeing an mbox
// to make sure the pointer is not left pointing to invalid data
void sys_mbox_set_invalid(sys_mbox_t *mbox) {
  *mbox = SYS_MBOX_NULL;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread,
                            void *arg, int stacksize, int prio) {

  size_t wsz;
  void *wsp;

  (void)name;
  wsz = THD_WA_SIZE(stacksize);
  wsp = chCoreAlloc(wsz);
  if (wsp == NULL)
    return NULL;
  return (sys_thread_t)chThdCreateStatic(wsp, wsz, prio, (tfunc_t)thread, arg);
}

sys_prot_t sys_arch_protect(void) {

  chSysLock();
  return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {

  (void)pval;
  chSysUnlock();
}

u32_t sys_now(void) {

#if CH_FREQUENCY == 1000
  return (u32_t)chTimeNow();
#elif (CH_FREQUENCY / 1000) >= 1 && (CH_FREQUENCY % 1000) == 0
  return ((u32_t)chTimeNow() - 1) / (CH_FREQUENCY / 1000) + 1;
#elif (1000 / CH_FREQUENCY) >= 1 && (1000 % CH_FREQUENCY) == 0
  return ((u32_t)chTimeNow() - 1) * (1000 / CH_FREQUENCY) + 1;
#else
  return (u32_t)(((u64_t)(chTimeNow() - 1) * 1000) / CH_FREQUENCY) + 1;
#endif
}
