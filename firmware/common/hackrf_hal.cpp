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

#include "hackrf_hal.hpp"

#include "lpc43xx_cpp.hpp"

using namespace lpc43xx;

namespace hackrf {
namespace one {

void reset() {
	/* "The reset delay is counted in IRC clock cycles. If the core frequency
	 * CCLK is much higher than the IRC frequency, add a software delay of
	 * fCCLK/fIRC clock cycles between resetting and accessing any of the
	 * peripheral blocks."
	 */
	rgu::reset_mask(
		/* Don't reset SCU, may trip up SPIFI pins if running from SPIFI
		 * memory.
		 */
		/*rgu::Reset::SCU */
		  rgu::Reset::LCD
		| rgu::Reset::USB0
		| rgu::Reset::USB1
		| rgu::Reset::DMA
		| rgu::Reset::SDIO
		| rgu::Reset::EMC
		| rgu::Reset::ETHERNET
		| rgu::Reset::GPIO
		| rgu::Reset::TIMER0
		| rgu::Reset::TIMER1
		| rgu::Reset::TIMER2
		| rgu::Reset::TIMER3
		| rgu::Reset::RITIMER
		| rgu::Reset::SCT
		| rgu::Reset::MOTOCONPWM
		| rgu::Reset::QEI
		| rgu::Reset::ADC0
		| rgu::Reset::ADC1
		| rgu::Reset::DAC
		| rgu::Reset::UART0
		| rgu::Reset::UART1
		| rgu::Reset::UART2
		| rgu::Reset::UART3
		| rgu::Reset::I2C0
		| rgu::Reset::I2C1
		| rgu::Reset::SSP0
		| rgu::Reset::SSP1
		| rgu::Reset::I2S
		/* Don't reset SPIFI if running from SPIFI memory */
		/*| rgu::Reset::SPIFI*/
		| rgu::Reset::CAN1
		| rgu::Reset::CAN0
		| rgu::Reset::M0APP
		| rgu::Reset::SGPIO
		| rgu::Reset::SPI
	);
}

} /* namespace one */
} /* namespace hackrf */
