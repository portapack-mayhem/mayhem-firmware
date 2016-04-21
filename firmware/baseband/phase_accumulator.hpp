/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PHASE_ACCUMULATOR_HPP__
#define __PHASE_ACCUMULATOR_HPP__

#include <cstdint>

class PhaseAccumulator {
public:
	constexpr PhaseAccumulator(
		const uint32_t phase_inc
	) : phase_inc { phase_inc }
	{
	}

	bool operator()() {
		const auto last_phase = phase;
		phase += phase_inc;
		return (phase < last_phase);
	}

	void set_inc(const uint32_t new_phase_inc) {
		phase_inc = new_phase_inc;
	}

private:
	uint32_t phase { 0 };
	uint32_t phase_inc;
};

#endif/*__PHASE_ACCUMULATOR_HPP__*/
