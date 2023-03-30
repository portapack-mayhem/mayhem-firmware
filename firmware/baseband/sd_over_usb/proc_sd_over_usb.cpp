/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
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

#include "ch.h"
#include "hal.h"

#include "proc_sd_over_usb.hpp"

#include "debug.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

extern "C" {
void start_usb(void);
void stop_usb(void);
void irq_usb(void);

CH_IRQ_HANDLER(Vector60) {
	irq_usb();
}
}

int main() {
	//EventDispatcher event_dispatcher { std::make_unique<USBProcessor>() };

	//HALT_UNTIL_DEBUGGING();

	//LPC_CGU->PLL0USB_CTRL.PD = 1;
	//LPC_CGU->PLL0USB_CTRL.AUTOBLOCK = 1;
	//LPC_CGU->PLL0USB_CTRL.CLK_SEL = 0x06;
//
	//while (LPC_CGU->PLL0USB_STAT.LOCK) {}
 //
	//LPC_CGU->PLL0USB_MDIV = 0x06167FFA; // 0x71A7FAA; //
	//LPC_CGU->PLL0USB_NP_DIV = 0x00302062;
//
	////LPC_CGU->PLL0USB_CTRL.PD |= 1;
	//LPC_CGU->PLL0USB_CTRL.DIRECTI = 1;
	//LPC_CGU->PLL0USB_CTRL.DIRECTO = 1;
	//LPC_CGU->PLL0USB_CTRL.CLKEN = 1;
//
	//LPC_CGU->PLL0USB_CTRL.PD = 0;
//
	//while (!(LPC_CGU->PLL0USB_STAT.LOCK)) {}
	////chThdSleepMilliseconds(800);
//
	//LPC_CGU->BASE_USB0_CLK.AUTOBLOCK = 1;
	//LPC_CGU->BASE_USB0_CLK.CLK_SEL = 0x07;

	start_usb();
	//event_dispatcher.run();
	while (true);
	stop_usb();
	return 0;
}
