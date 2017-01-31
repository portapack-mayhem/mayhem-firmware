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

extern "C" {

CH_IRQ_HANDLER(RTC_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	chEvtSignalI(thread_rtc_event, EVT_MASK_RTC_TICK);
	chSysUnlockFromIsr();

	rtc::interrupt::clear_all();

	CH_IRQ_EPILOGUE();
}

}
