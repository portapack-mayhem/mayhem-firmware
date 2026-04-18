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
#include "nvic.h"
#include "lpc43xx.inc"

static Thread* thread_rtc_event = NULL;

void rtc_interrupt_enable() {
    thread_rtc_event = chThdSelf();
    rtc::interrupt::enable_second_inc();
    nvicEnableVector(RTC_IRQn, CORTEX_PRIORITY_MASK(LPC_RTC_IRQ_PRIORITY));
}

void rtc_reset_default() {
    // 1. FULL RTC CLEANUP/RESET
    LPC_RTC->CIIR = 0;
    LPC_RTC->AMR = 0xFF;  // Disable all alarms
    LPC_RTC->ILR = 3;     // Clear stuck interrupt flags
    LPC_RTC->ASEC = 0;
    LPC_RTC->AMIN = 0;
    LPC_RTC->AHRS = 0;

    // 2. RESET EVENT ROUTER (Using absolute addresses)

    volatile uint32_t* evrt_edge = (volatile uint32_t*)(0x40044000 + 0x004);
    volatile uint32_t* evrt_clr_en = (volatile uint32_t*)(0x40044000 + 0x008);
    volatile uint32_t* evrt_clr_stat = (volatile uint32_t*)(0x40044000 + 0x018);

    *evrt_edge |= (1 << 5);       // Reset to edge-triggered
    *evrt_clr_en = (1 << 5);      // DISABLE RTC channel routing
    *evrt_clr_stat = 0xFFFFFFFF;  // Clear pending events

    // 3. RE-ENABLE USB PHY (CREG0 is also battery-backed!)
    LPC_CREG->CREG0 &= ~(1 << 5);
}

void rtc_wakeup_init() {
    rtc_reset_default();

    // 1. Force RTC Clock (In case it was modified by the app)
    // Bit 0: Enable, Bit 4: Calibration OFF
    LPC_RTC->CCR = (1 << 0);

    // 2. EVENT ROUTER - Link the RTC with the EVRT
    // This is the 0x010 register in EVRT (ERCONTROL)
    // Bit 0: RTC event enable
    *(volatile uint32_t*)(0x40044010) |= (1 << 0);

    // 3. EVENT ROUTER CONFIGURATION
    volatile uint32_t* evrt_hilo = (volatile uint32_t*)(0x40044000 + 0x000);
    volatile uint32_t* evrt_edge = (volatile uint32_t*)(0x40044000 + 0x004);
    volatile uint32_t* evrt_set_en = (volatile uint32_t*)(0x40044000 + 0x00C);
    volatile uint32_t* evrt_clr_stat = (volatile uint32_t*)(0x40044000 + 0x018);

    *evrt_hilo |= (1 << 5);
    *evrt_edge &= ~(1 << 5);  // Level-triggered (Better for wakeup)
    *evrt_clr_stat = 0xFFFFFFFF;
    *evrt_set_en = (1 << 5);

    // 4. NVIC Cleanup
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_ClearPendingIRQ(EVENTROUTER_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_EnableIRQ(EVENTROUTER_IRQn);
}

void rtc_wakeup(uint32_t sleep_seconds) {
    // 1. Safety cleanup after application tasks
    LPC_RTC->CCR &= ~(1 << 4);  // Disable calibration (Important!)

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

    // 2. Write values to alarm registers
    LPC_RTC->ASEC = sec;
    LPC_RTC->AMIN = min;
    LPC_RTC->AHRS = hrs;

    // Mask for SEC, MIN, HRS (Disable all other alarm triggers)
    uint32_t mask = 0xFF ^ ((1 << 0) | (1 << 1) | (1 << 2));
    LPC_RTC->AMR = mask;

    // Verify write (Sync with RTC domain)
    while (LPC_RTC->ASEC != sec);
    while (LPC_RTC->AMR != mask);

    // Brief extra delay to allow internal logic to settle/latch
    for (volatile int i = 0; i < 5000; i++) __asm__("nop");
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
