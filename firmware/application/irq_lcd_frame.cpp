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

#include "irq_lcd_frame.hpp"

#include "event_m0.hpp"

#include "ch.h"
#include "hal.h"

#include "portapack_hal.hpp"

static Thread* thread_lcd_frame_event = NULL;

static void pin_int4_interrupt_enable() {
	thread_lcd_frame_event = chThdSelf();
	nvicEnableVector(PIN_INT4_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_PIN_INT4_IRQ_PRIORITY));
}

void lcd_frame_sync_configure() {
	/* Positive edge sensitivity */
	LPC_GPIO_INT->ISEL &= ~(1U << 4);
	LPC_GPIO_INT->SIENR = (1U << 4);
	LPC_GPIO_INT->CIENF = (1U << 4);
	LPC_GPIO_INT->IST = (1U << 4);

	LPC_SCU->PINTSEL1 =
		  (LPC_SCU->PINTSEL1 & ~(0xffU << 0))
		| (portapack::gpio_lcd_te.pad() << 0)
		| (portapack::gpio_lcd_te.port() << 5)
		;

	pin_int4_interrupt_enable();
}

extern "C" {

CH_IRQ_HANDLER(PIN_INT4_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	chEvtSignalI(thread_lcd_frame_event, EVT_MASK_LCD_FRAME_SYNC);
	chSysUnlockFromIsr();

	LPC_GPIO_INT->IST = (1U << 4);

	CH_IRQ_EPILOGUE();
}

}
