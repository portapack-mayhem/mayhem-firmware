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

#ifndef __OOK_HPP__
#define __OOK_HPP__

#include "phase_detector.hpp"
#include "phase_accumulator.hpp"

#include <cstdint>
#include <complex>
#include <algorithm>
#include <cmath>

class OOKSlicerMagSquaredInt {
public:
	using symbol_t = bool;

	constexpr OOKSlicerMagSquaredInt(
		const float samples_per_symbol
	) : mag2_threshold_leak_factor {
			static_cast<uint32_t>(
				factor_sq(-1.0f / (8.0f * samples_per_symbol)) * float(1ULL << 32)
			)
		}
	{
	}

	symbol_t operator()(const std::complex<int16_t> in) {
		const uint32_t real2 = in.real() * in.real();
		const uint32_t imag2 = in.imag() * in.imag();
		const uint32_t mag2 = real2 + imag2;

		const uint32_t mag2_attenuated = mag2 >> 3;	// Approximation of (-4.5dB)^2
		mag2_threshold = (uint64_t(mag2_threshold) * uint64_t(mag2_threshold_leak_factor)) >> 32;
		mag2_threshold = std::max(mag2_threshold, mag2_attenuated);
		const bool symbol = (mag2 > mag2_threshold);
		return symbol;
	}

private:
	const uint32_t mag2_threshold_leak_factor;
	uint32_t mag2_threshold = 0;

	constexpr float factor_sq(float db) {
		return std::pow(10.0f, db / (10.0f / 2));
	}
};

class OOKClockRecovery {
public:
	constexpr OOKClockRecovery(
		const float samples_per_symbol
	) : symbol_phase_inc_nominal { static_cast<uint32_t>(std::round((1ULL << 32) / samples_per_symbol)) },
		symbol_phase_inc_k { static_cast<uint32_t>(std::round(symbol_phase_inc_nominal * (2.0f / 8.0f) / samples_per_symbol)) },
		phase_detector { static_cast<size_t>(std::round(samples_per_symbol)) },
		phase_accumulator { symbol_phase_inc_nominal }
	{
	}

	template<typename SymbolHandler>
	void operator()(const uint32_t slicer_history, SymbolHandler symbol_handler) {
		if( phase_accumulator() ) {
			const auto detector_result = phase_detector(slicer_history);
			phase_accumulator.set_inc(symbol_phase_inc_nominal + detector_result.error * symbol_phase_inc_k);
			symbol_handler(detector_result.symbol);
		}
	}

private:
	const uint32_t symbol_phase_inc_nominal;
	const uint32_t symbol_phase_inc_k;
	PhaseDetectorEarlyLateGate phase_detector;
	PhaseAccumulator phase_accumulator;
};

#endif/*__OOK_HPP__*/
