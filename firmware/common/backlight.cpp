/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
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

#include "backlight.hpp"

#include "portapack_io.hpp"

namespace portapack {

void BacklightOnOff::on() {
	if( !is_on() ) {
		io.lcd_backlight(true);
		on_ = true;
	}
}

void BacklightOnOff::off() {
	if( is_on() ) {
		io.lcd_backlight(false);
		on_ = false;
	}
}

void BacklightCAT4004::set_level(const value_t value) {		
	auto target = value;

	// Clip target value to valid range.
	if( target < 0 ) {
		target = 0;
	}
	if( target > maximum_level ) {
		target = maximum_level;
	}

	if( is_on() ) {
		pulses(target);
	} else {
		level_ = target;
	}
}

void BacklightCAT4004::on() {
	if( !is_on() ) {
		io.lcd_backlight(true);
		halPolledDelay(ticks_setup);
		on_ = true;

		// Just enabled driver, initial value is maximum.
		const auto target_level = level();
		level_ = maximum_level;

		pulses(target_level);
	}
}

void BacklightCAT4004::off() {
	if( is_on() ) {
		io.lcd_backlight(false);
		chThdSleepMilliseconds(ms_pwrdwn);
		on_ = false;
	}
}

void BacklightCAT4004::pulses(value_t target) {
	while(level() != target) {
		pulse();
	}
}

void BacklightCAT4004::pulse() {
	io.lcd_backlight(false);
	halPolledDelay(ticks_lo);
	io.lcd_backlight(true);
	halPolledDelay(ticks_hi);

	level_ -= 1;
	if( level_ < 0 ) {
		level_ = levels() - 1;
	}
}

} /* namespace portapack */
