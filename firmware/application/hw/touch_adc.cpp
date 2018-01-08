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

#include "touch.hpp"

#include <cstdint>

#include "adc.hpp"
#include "utility.hpp"

#include "hal.h"
using namespace lpc43xx;

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "portapack_adc.hpp"
#include "portapack_shared_memory.hpp"

namespace touch {
namespace adc {

constexpr uint8_t adc0_sel =
	  (1 << portapack::adc0_touch_xp_input)
	| (1 << portapack::adc0_touch_xn_input)
	| (1 << portapack::adc0_touch_yp_input)
	| (1 << portapack::adc0_touch_yn_input)
	;
const auto adc0_interrupt_mask = flp2(adc0_sel);

constexpr lpc43xx::adc::CR adc0_cr {
	.sel = adc0_sel,
	.clkdiv = 49,		/* 400kHz sample rate, 2.5us/sample @ 200MHz PCLK */
	.resolution = 9,	/* Ten clocks */
	.edge = 0,
};
constexpr lpc43xx::adc::Config adc0_config {
	.cr = adc0_cr,
};

void init() {
	adc0::clock_enable();
	adc0::interrupts_disable();
	adc0::power_up(adc0_config);
	adc0::interrupts_enable(adc0_interrupt_mask);
}

void start() {
	adc0::start_burst();
}

// static constexpr bool monitor_overruns_and_not_dones = false;

Samples get() {
	const auto xp_reg = LPC_ADC0->DR[portapack::adc0_touch_xp_input];
	const auto xn_reg = LPC_ADC0->DR[portapack::adc0_touch_xn_input];
	const auto yp_reg = LPC_ADC0->DR[portapack::adc0_touch_yp_input];
	const auto yn_reg = LPC_ADC0->DR[portapack::adc0_touch_yn_input];
	return {
		(xp_reg >> 6) & 0x3ff,
		(xn_reg >> 6) & 0x3ff,
		(yp_reg >> 6) & 0x3ff,
		(yn_reg >> 6) & 0x3ff,
	};
}

} /* namespace adc */
} /* namespace touch */
