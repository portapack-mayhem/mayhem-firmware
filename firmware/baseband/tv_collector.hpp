/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 Shao
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

#ifndef __TV_COLLECTOR_H__
#define __TV_COLLECTOR_H__

#include "dsp_types.hpp"
#include "complex.hpp"

#include "block_decimator.hpp"

#include <cstdint>
#include <array>

#include "message.hpp"

class TvCollector {
public:
	void on_message(const Message* const message);

	void set_decimation_factor(const size_t decimation_factor);

	void feed(
		const buffer_c16_t& channel
	);

private:
	BlockDecimator<complex16_t, 256> channel_spectrum_decimator { 1 };
	ChannelSpectrum fifo_data[1 << ChannelSpectrumConfigMessage::fifo_k] { };
	ChannelSpectrumFIFO fifo { fifo_data, ChannelSpectrumConfigMessage::fifo_k };

	volatile bool channel_spectrum_request_update { false };
	bool streaming { false };
	std::array<std::complex<float>, 256> channel_spectrum { };
	uint32_t channel_spectrum_sampling_rate { 0 };

	void post_message(const buffer_c16_t& data);

	void set_state(const SpectrumStreamingConfigMessage& message);
	void start();
	void stop();

	void update();

};

#endif/*__TV_COLLECTOR_H__*/
