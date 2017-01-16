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

#ifndef __BLOCK_DECIMATOR_H__
#define __BLOCK_DECIMATOR_H__

#include <cstdint>
#include <cstddef>
#include <array>

#include "dsp_types.hpp"
#include "complex.hpp"

template<typename T, size_t N>
class BlockDecimator {
public:
	constexpr BlockDecimator(
		const size_t factor
	) : factor_ { factor }
	{
	}

	void set_input_sampling_rate(const uint32_t new_sampling_rate) {
		if( new_sampling_rate != input_sampling_rate() ) {
			input_sampling_rate_ = new_sampling_rate;
			reset_state();
		}
	}

	uint32_t input_sampling_rate() const {
		return input_sampling_rate_;
	}

	void set_factor(const size_t new_factor) {
		if( new_factor != factor() ) {
			factor_ = new_factor;
			reset_state();
		}
	}

	size_t factor() const {
		return factor_;
	}

	uint32_t output_sampling_rate() const {
		return input_sampling_rate() / factor();
	}

	template<typename BlockCallback>
	void feed(const buffer_t<T>& src, BlockCallback callback) {
		/* NOTE: Input block size must be >= factor */

		set_input_sampling_rate(src.sampling_rate);

		while( src_i < src.count ) {
			buffer[dst_i++] = src.p[src_i];
			if( dst_i == buffer.size() ) {
				callback({ buffer.data(), buffer.size(), output_sampling_rate() });
				reset_state();
				dst_i = 0;
			}

			src_i += factor();
		}

		src_i -= src.count;
	}

private:
	std::array<T, N> buffer { };
	uint32_t input_sampling_rate_ { 0 };
	size_t factor_ { 1 };
	size_t src_i { 0 };
	size_t dst_i { 0 };

	void reset_state() {
		src_i = 0;
		dst_i = 0;
	}
};

#endif/*__BLOCK_DECIMATOR_H__*/
