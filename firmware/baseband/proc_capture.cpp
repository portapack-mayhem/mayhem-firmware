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

#include "proc_capture.hpp"

#include "dsp_fir_taps.hpp"

#include "utility.hpp"

CaptureProcessor::CaptureProcessor() {
	decim_0.configure(taps_200k_decim_0.taps, 33554432);
	decim_1.configure(taps_200k_decim_1.taps, 131072);

	stream = std::make_unique<StreamInput>(15);
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
	/* 2.4576MHz, 2048 samples */
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto decimator_out = decim_1_out;

	if( stream ) {
		const size_t bytes_to_write = sizeof(*decimator_out.p) * decimator_out.count;
		const auto result = stream->write(decimator_out.p, bytes_to_write);
	}
}
