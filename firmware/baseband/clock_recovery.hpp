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

#include <cstddef>
#include <array>
#include <functional>

#include "linear_resampler.hpp"

namespace clock_recovery {

class GardnerTimingErrorDetector {
public:
	static constexpr size_t samples_per_symbol { 2 };

	/*
	Expects retimed samples at a rate of twice the expected symbol rate.
	Calculates timing error, sends symbol and error to handler.
	*/
	template<typename SymbolHandler>
	void operator()(
		const float in,
		SymbolHandler symbol_handler
	) {
		/* NOTE: Algorithm is sensitive to input magnitude. Timing error value
		 * will scale proportionally. Best practice is to use error sign only.
		 */
		t[2] = t[1];
		t[1] = t[0];
		t[0] = in;

		if( symbol_phase == 0 ) {
			const auto symbol = t[0];
			const float lateness = (t[0] - t[2]) * t[1];
			symbol_handler(symbol, lateness);
		}

		symbol_phase = (symbol_phase + 1) % samples_per_symbol;
	}

private:
	std::array<float, 3> t { };
	size_t symbol_phase { 0 };
};

class LinearErrorFilter {
public:
	LinearErrorFilter(
		const float filter_alpha = 0.95f,
		const float error_weight = -1.0f
	) : filter_alpha { filter_alpha },
		error_weight { error_weight }
	{
	}

	float operator()(
		const float error
	) {
		error_filtered = filter_alpha * error_filtered + (1.0f - filter_alpha) * error;
		return error_filtered * error_weight;
	}

private:
	const float filter_alpha;
	const float error_weight;
	float error_filtered { 0.0f };
};

class FixedErrorFilter {
public:
	FixedErrorFilter(
	) {
	}

	FixedErrorFilter(
		const float weight
	) : weight_ { weight }
	{
	}

	float operator()(
		const float lateness
	) const {
		return (lateness < 0.0f) ? weight() : -weight();
	}

	float weight() const {
		return weight_;
	}

private:
	float weight_ { 1.0f / 16.0f };
};

template<typename ErrorFilter>
class ClockRecovery {
public:
	using SymbolHandler = std::function<void(const float)>;

	ClockRecovery(
		const float sampling_rate,
		const float symbol_rate,
		ErrorFilter error_filter,
		SymbolHandler symbol_handler
	) : symbol_handler { std::move(symbol_handler) }
	{
		configure(sampling_rate, symbol_rate, error_filter);
	}

	ClockRecovery(
		SymbolHandler symbol_handler
	) : symbol_handler { std::move(symbol_handler) }
	{
	}

	void configure(
		const float sampling_rate,
		const float symbol_rate,
		ErrorFilter error_filter
	) {
		resampler.configure(sampling_rate, symbol_rate * timing_error_detector.samples_per_symbol);
		error_filter = error_filter;
	}

	void operator()(
		const float baseband_sample
	) {
		resampler(baseband_sample,
			[this](const float interpolated_sample) {
				this->resampler_callback(interpolated_sample);
			}
		);
	}

private:
	dsp::interpolation::LinearResampler resampler { };
	GardnerTimingErrorDetector timing_error_detector { };
	ErrorFilter error_filter { };
	const SymbolHandler symbol_handler;

	void resampler_callback(const float interpolated_sample) {
		timing_error_detector(interpolated_sample,
			[this](const float symbol, const float lateness) {
				this->symbol_callback(symbol, lateness);
			}
		);
	}

	void symbol_callback(const float symbol, const float lateness) {
		// NOTE: This check is to avoid std::function nullptr check, which
		// brings in "_ZSt25__throw_bad_function_callv" and a lot of extra code.
		// TODO: Make symbol_handler known at compile time.
		if( symbol_handler) {
			symbol_handler(symbol);
		}

		const float adjustment = error_filter(lateness);
		resampler.advance(adjustment);
	}
};

} /* namespace clock_recovery */

#endif/*__CLOCK_RECOVERY_H__*/
