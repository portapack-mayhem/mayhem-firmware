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

#include "rssi.hpp"

#include <cstdint>

#include "adc.hpp"
#include "rssi_dma.hpp"
#include "utility.hpp"

#include "hal.h"
using namespace lpc43xx;

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "portapack_adc.hpp"

namespace rf {
namespace rssi {

constexpr uint8_t adc1_sel = (1 << portapack::adc1_rssi_input);
const auto adc1_interrupt_mask = flp2(adc1_sel);

constexpr adc::CR adc1_cr {
	.sel = adc1_sel,
	.clkdiv = 49,		/* 400kHz sample rate, 2.5us/sample @ 200MHz PCLK */
	.resolution = 9,	/* Ten clocks */
	.edge = 0,
};
constexpr adc::Config adc1_config {
	.cr = adc1_cr,
};

void init() {
	adc1::clock_enable();
	adc1::interrupts_disable();
	adc1::power_up(adc1_config);
	adc1::interrupts_enable(adc1_interrupt_mask);

	dma::init();
}

void start() {
	dma::enable();
	adc1::start_burst();
}

void stop() {
	dma::disable();
	adc1::stop_burst();
}

} /* namespace rssi */
} /* namespace rf */
