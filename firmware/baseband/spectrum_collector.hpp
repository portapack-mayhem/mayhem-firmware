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

#ifndef __SPECTRUM_COLLECTOR_H__
#define __SPECTRUM_COLLECTOR_H__

#include "dsp_types.hpp"
#include "complex.hpp"

#include "block_decimator.hpp"

#include <cstdint>
#include <array>

class SpectrumCollector {
public:
	constexpr SpectrumCollector(
	) : channel_spectrum_decimator { 1 }
	{
	}

	void set_decimation_factor(const size_t decimation_factor);

	void feed(
		const buffer_c16_t& channel,
		const uint32_t filter_pass_frequency,
		const uint32_t filter_stop_frequency
	);

	void update();

private:
	BlockDecimator<256> channel_spectrum_decimator;

	volatile bool channel_spectrum_request_update { false };
	std::array<std::complex<float>, 256> channel_spectrum;
	uint32_t channel_spectrum_sampling_rate { 0 };
	uint32_t channel_filter_pass_frequency { 0 };
	uint32_t channel_filter_stop_frequency { 0 };

	void post_message(const buffer_c16_t& data);
};

#endif/*__SPECTRUM_COLLECTOR_H__*/
