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

#include "proc_wideband_spectrum.hpp"

#include "event_m4.hpp"

#include "i2s.hpp"
using namespace lpc43xx;

#include "dsp_fft.hpp"

#include <cstdint>
#include <cstddef>

#include <array>

void WidebandSpectrum::execute(const buffer_c8_t& buffer) {
	// 2048 complex8_t samples per buffer.
	// 102.4us per buffer. 20480 instruction cycles per buffer.

	static int phase = 0;

	if( phase == 0 ) {
		std::fill(spectrum.begin(), spectrum.end(), 0);
	}

	if( (phase & 7) == 0 ) {
		// TODO: Removed window-presum windowing, due to lack of available code RAM.
		// TODO: Apply window to improve spectrum bin sidelobes.
		for(size_t i=0; i<channel_spectrum.size(); i++) {
			spectrum[i] += buffer.p[i];
		}
	}

	if( phase == 23 ) {
		if( channel_spectrum_request_update == false ) {
			fft_swap(spectrum, channel_spectrum);
			channel_spectrum_sampling_rate = buffer.sampling_rate;
			channel_filter_pass_frequency = 0;
			channel_filter_stop_frequency = 0;
			channel_spectrum_request_update = true;
			events_flag(EVT_MASK_SPECTRUM);
			phase = 0;
		}
	} else {
		phase++;
	}

	i2s::i2s0::tx_mute();
}
