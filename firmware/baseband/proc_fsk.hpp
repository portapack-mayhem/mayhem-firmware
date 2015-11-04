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

#ifndef __PROC_FSK_H__
#define __PROC_FSK_H__

#include "baseband_processor.hpp"

#include "channel_decimator.hpp"
#include "dsp_decimate.hpp"
#include "matched_filter.hpp"
#include "dsp_fir_taps.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

#include "ais_baseband.hpp"

class FSKProcessor : public BasebandProcessor {
public:
	using payload_t = std::bitset<1024>;

	FSKProcessor(MessageHandlerMap& message_handlers);
	~FSKProcessor();

	void configure(const FSKConfiguration new_configuration);

	void execute(buffer_c8_t buffer) override;

private:
	const size_t sampling_rate = 76800;
	
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By16 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_031_070_tfilter;
	dsp::decimate::FIRAndDecimateComplex channel_filter { channel_filter_taps.taps, 8 };

	dsp::matched_filter::MatchedFilter mf { baseband::ais::rrc_taps_128_decim_4_p, 1 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery {
		static_cast<float>(sampling_rate / 4),
		9600,
		{ 0.0555f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};
	symbol_coding::NRZIDecoder nrzi_decode;
	PacketBuilder packet_builder {
		[this](const PacketBuilder::PayloadType& payload, const size_t bits_received) {
			this->payload_handler(payload, bits_received);
		}
	};

	MessageHandlerMap& message_handlers;

	void consume_symbol(const float symbol);
	void payload_handler(const payload_t& payload, const size_t bits_received);
};

#endif/*__PROC_FSK_H__*/
