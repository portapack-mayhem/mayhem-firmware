/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PROC_REPLAY_HPP__
#define __PROC_REPLAY_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "stream_output.hpp"

#include <array>
#include <memory>

class ReplayProcessor : public BasebandProcessor {
public:
	ReplayProcessor();

	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;

private:
	// TODO: Repeated value needs to be transmitted from application side.
	static constexpr size_t baseband_fs = 500000;
	//static constexpr auto spectrum_rate_hz = 50.0f;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	//RSSIThread rssi_thread { NORMALPRIO + 10 };

	std::array<complex16_t, 512> iq { };	// 2048 doesn't fit in allocated RAM
	const buffer_c16_t iq_buffer {
		iq.data(),
		iq.size()
	};

	std::unique_ptr<StreamOutput> stream { };

	/*SpectrumCollector channel_spectrum;
	size_t spectrum_interval_samples = 0;
	size_t spectrum_samples = 0;*/

	void replay_config(const ReplayConfigMessage& message);
};

#endif/*__PROC_REPLAY_HPP__*/
