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

#include "baseband_cpld.hpp"

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

namespace baseband {

void CPLD::init() {
	set_decimation_by(1);
	gpios_baseband_decimation[0].output();
	gpios_baseband_decimation[1].output();
	gpios_baseband_decimation[2].output();

	set_q_invert(false);
	gpio_baseband_q_invert.output();
}

void CPLD::set_decimation_by(const uint8_t n) {
	const uint8_t skip_n = n - 1;
	const uint8_t value = skip_n ^ 7;
	gpios_baseband_decimation[0].write(value & 1);
	gpios_baseband_decimation[1].write(value & 2);
	gpios_baseband_decimation[2].write(value & 4);
}

void CPLD::set_q_invert(const bool invert) {
	gpio_baseband_q_invert.write(invert);
}

}
