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

#ifndef __CLOCK_RECOVERY_H__
#define __CLOCK_RECOVERY_H__

#include <cstdint>

class ClockRecovery {
public:
	void configure(
		const uint32_t symbol_rate,
		const uint32_t sampling_rate
	);

	template<typename SymbolHandler>
	void execute(
		const float in,
		SymbolHandler symbol_handler
	) {
		const bool phase_0 = (phase_last >> 31) & (!(phase >> 31));
		const bool phase_180 = (!(phase_last >> 31)) & (phase >> 31);

		if( phase_0 || phase_180 ) {
			t2 = t1;
			t1 = t0;

			const uint32_t phase_boundary = phase_180 ? (1U << 31) : 0;
			const float alpha = (phase_boundary - phase_last) / float(phase_increment + phase_adjustment);
			const float t = last_sample + alpha * (in - last_sample);
			t0 = t;
		}

		if( phase_0 ) {
			symbol_handler(t0);

			const float error = (t0 - t2) * t1;
			// + error == late == decrease/slow phase
			// - error == early == increase/fast phase

			error_filtered = 0.75f * error_filtered + 0.25f * error;

			// Correct phase (don't change frequency!)
			phase_adjustment = -phase_increment * error_filtered / 200.0f;
		}

		phase_last = phase;
		phase += phase_increment + phase_adjustment;
		last_sample = in;
	}

private:
	uint32_t phase { 0 };
	uint32_t phase_last { 0 };
	uint32_t phase_adjustment { 0 };
	uint32_t phase_increment { 0 };
	float last_sample { 0 };
	float t0 { 0 };
	float t1 { 0 };
	float t2 { 0 };
	float error_filtered { 0 };

	static constexpr float fractional_symbol_rate(
		const uint32_t symbol_rate,
		const uint32_t sampling_rate
	) {
		return float(symbol_rate) / float(sampling_rate);
	}

	static constexpr uint32_t phase_increment_u32(const float fractional_symbol_rate) {
		return 4294967296.0f * fractional_symbol_rate;
	}
};

#endif/*__CLOCK_RECOVERY_H__*/
