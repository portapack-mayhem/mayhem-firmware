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

#ifndef __TEMPERATURE_LOGGER_H__
#define __TEMPERATURE_LOGGER_H__

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

class TemperatureLogger {
public:	
	using sample_t = uint8_t;

	void second_tick();

	size_t size() const;
	size_t capacity() const;
	
	std::vector<sample_t> history() const;

private:
	std::array<sample_t, 128> samples { };

	static constexpr size_t sample_interval = 5;
	size_t sample_phase = 0;
	size_t samples_count = 0;

	sample_t read_sample();
	void push_sample(const sample_t sample);
};

#endif/*__TEMPERATURE_LOGGER_H__*/
