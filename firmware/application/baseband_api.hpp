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

#ifndef __BASEBAND_API_H__
#define __BASEBAND_API_H__

#include "message.hpp"

#include "dsp_fir_taps.hpp"

#include "spi_image.hpp"

#include <cstddef>

namespace baseband {

struct AMConfig {
	const fir_taps_complex<64> channel;
	const AMConfigureMessage::Modulation modulation;

	void apply() const;
};

struct NBFMConfig {
	const fir_taps_real<24> decim_0;
	const fir_taps_real<32> decim_1;
	const fir_taps_real<32> channel;
	const size_t deviation;

	void apply() const;
};

struct WFMConfig {
	void apply() const;
};

void set_ccir_data(	const uint32_t samples_per_tone, const uint16_t tone_count);
void set_audiotx_data(const uint32_t divider, const uint32_t bw, const bool ctcss_enabled, const uint32_t ctcss_phase_inc);
void set_fifo_data(const int8_t * data);
void set_pwmrssi(int32_t avg, bool enabled);
void set_afsk_data(const uint32_t afsk_samples_per_bit, const uint32_t afsk_phase_inc_mark, const uint32_t afsk_phase_inc_space,
					const uint8_t afsk_repeat, const uint32_t afsk_bw, const bool afsk_alt_format);
void set_ook_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint8_t repeat,
					const uint32_t pause_symbols);
void set_pocsag();
void set_adsb();
void set_dtmf_data(const uint32_t bw, const uint32_t tone_length, const uint32_t pause_length);

void run_image(const portapack::spi_flash::image_tag_t image_tag);
void shutdown();

void spectrum_streaming_start();
void spectrum_streaming_stop();

void capture_start(CaptureConfig* const config);
void capture_stop();

} /* namespace baseband */

#endif/*__BASEBAND_API_H__*/
