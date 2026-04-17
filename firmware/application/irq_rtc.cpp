/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "irq_rtc.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "event_m0.hpp"

static Thread* thread_rtc_event = NULL;

void rtc_interrupt_enable() {
    thread_rtc_event = chThdSelf();
    rtc::interrupt::enable_second_inc();
    nvicEnableVector(RTC_IRQn, CORTEX_PRIORITY_MASK(LPC_RTC_IRQ_PRIORITY));
}

void rtc_reset_default() {
    // 1. RTC TELJES TAKARÍTÁS
    LPC_RTC->CIIR = 0;
    LPC_RTC->AMR = 0xFF;  // Riasztások tiltása
    LPC_RTC->ILR = 3;     // Beragadt megszakítási flagek törlése
    LPC_RTC->ASEC = 0;
    LPC_RTC->AMIN = 0;
    LPC_RTC->AHRS = 0;

    // 2. EVENT ROUTER VISSZAÁLLÍTÁSA (A VALÓDI CÍMEKKEL!)

    volatile uint32_t* evrt_edge = (volatile uint32_t*)(0x40044000 + 0x004);
    volatile uint32_t* evrt_clr_en = (volatile uint32_t*)(0x40044000 + 0x008);
    volatile uint32_t* evrt_clr_stat = (volatile uint32_t*)(0x40044000 + 0x018);

    *evrt_edge |= (1 << 5);       // Visszaállítás élvezéreltre
    *evrt_clr_en = (1 << 5);      // RTC csatorna routing LETILTÁSA
    *evrt_clr_stat = 0xFFFFFFFF;  // Pending Eventek törlése

    // 3. USB PHY VISSZAKAPCSOLÁSA (CREG0 is akku-védett!)
    LPC_CREG->CREG0 &= ~(1 << 5);
}

void rtc_wakeup_init() {
    rtc_reset_default();

    LPC_RGU->RESET_CTRL[1] = (1 << 24);  // M0APP_RST bit beállítása

    volatile uint32_t* evrt_hilo = (volatile uint32_t*)(0x40044000 + 0x000);
    volatile uint32_t* evrt_edge = (volatile uint32_t*)(0x40044000 + 0x004);
    volatile uint32_t* evrt_set_en = (volatile uint32_t*)(0x40044000 + 0x00C);
    volatile uint32_t* evrt_clr_stat = (volatile uint32_t*)(0x40044000 + 0x018);

    *evrt_hilo |= (1 << 5);       // Magas szint
    *evrt_edge &= ~(1 << 5);      // SZINTVEZÉRELT!
    *evrt_clr_stat = 0xFFFFFFFF;  // Flagek törlése
    *evrt_set_en = (1 << 5);      // Csatorna (RTC) engedélyezése
}

void rtc_wakeup(uint32_t sleep_seconds) {
    // --- Idő kiszámítása ---
    uint32_t sec = LPC_RTC->SEC;
    uint32_t min = LPC_RTC->MIN;
    uint32_t hrs = LPC_RTC->HRS;

    sec += sleep_seconds;
    while (sec >= 60) {
        sec -= 60;
        min++;
    }
    while (min >= 60) {
        min -= 60;
        hrs++;
    }
    while (hrs >= 24) {
        hrs -= 24;
    }

    // --- Riasztás élesítése ---
    LPC_RTC->ASEC = sec;
    LPC_RTC->AMIN = min;
    LPC_RTC->AHRS = hrs;

    // Csak a SEC, MIN, HRS egyezést figyeljük
    LPC_RTC->AMR = 0xFF ^ ((1 << 0) | (1 << 1) | (1 << 2));
}

extern "C" {

CH_IRQ_HANDLER(RTC_IRQHandler) {
    CH_IRQ_PROLOGUE();
#ifdef PRALINE
    if (thread_rtc_event) {
        chSysLockFromIsr();
        chEvtSignalI(thread_rtc_event, EVT_MASK_RTC_TICK);
        chSysUnlockFromIsr();
    }
#else
    chSysLockFromIsr();
    chEvtSignalI(thread_rtc_event, EVT_MASK_RTC_TICK);
    chSysUnlockFromIsr();
#endif

    rtc::interrupt::clear_all();

    CH_IRQ_EPILOGUE();
}
}
