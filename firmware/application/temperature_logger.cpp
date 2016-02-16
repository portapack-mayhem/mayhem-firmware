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

#include "temperature_logger.hpp"

#include "radio.hpp"

#include <algorithm>

void TemperatureLogger::second_tick() {
	sample_phase++;
	if( sample_phase >= sample_interval ) {
		push_sample(read_sample());
	}
}

size_t TemperatureLogger::size() const {
	return std::min(capacity(), samples_count);
}

size_t TemperatureLogger::capacity() const {
	return samples.size();
}

std::vector<TemperatureLogger::sample_t> TemperatureLogger::history() const {
	std::vector<sample_t> result;

	const auto n = size();
	result.resize(n);
	
	// Copy the last N samples from the buffer, since new samples are added at the end.
	std::copy(samples.cend() - n, samples.cend(), result.data());
	
	return result;
}

TemperatureLogger::sample_t TemperatureLogger::read_sample() {
	// MAX2837 does not return a valid temperature if in "shutdown" mode.
	return radio::debug::second_if::temp_sense();
}

void TemperatureLogger::push_sample(const TemperatureLogger::sample_t sample) {
	// Started out building a pseudo-FIFO, then got lazy.

	// Shift samples: samples[1:] -> samples[0:-1]
	// New sample goes into samples[-1]
	std::copy(samples.cbegin() + 1, samples.cend(), samples.begin());
	samples.back() = sample;
	samples_count++;
	sample_phase = 0;
}
