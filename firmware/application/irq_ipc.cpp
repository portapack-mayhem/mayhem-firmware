/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "irq_ipc.hpp"

#include "event.hpp"

#include "ch.h"
#include "hal.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

void m4txevent_interrupt_enable() {
	nvicEnableVector(M4CORE_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M4TXEVENT_IRQ_PRIORITY));
}

extern "C" {

CH_IRQ_HANDLER(M4Core_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	events_flag_isr(EVT_MASK_APPLICATION);
	chSysUnlockFromIsr();

	creg::m4txevent::clear();

	CH_IRQ_EPILOGUE();
}

}
