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
#include "pocsag_packet.hpp"
#include "jammer.hpp"

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

	void apply(const uint8_t squelch_level) const;
};

struct WFMConfig {
	void apply() const;
};

void set_tone(const uint32_t index, const uint32_t delta, const uint32_t duration);
void set_tones_config(const uint32_t bw, const uint32_t pre_silence, const uint16_t tone_count,
					const bool dual_tone, const bool audio_out);
void kill_tone();
void set_sstv_data(const uint8_t vis_code, const uint32_t pixel_duration);
void set_audiotx_config(const uint32_t divider, const float deviation_hz, const float audio_gain,
					const uint32_t tone_key_delta);
void set_fifo_data(const int8_t * data);
void set_pitch_rssi(int32_t avg, bool enabled);
void set_afsk_data(const uint32_t afsk_samples_per_bit, const uint32_t afsk_phase_inc_mark, const uint32_t afsk_phase_inc_space,
					const uint8_t afsk_repeat, const uint32_t afsk_bw, const uint8_t symbol_count);
void kill_afsk();
void set_afsk(const uint32_t baudrate, const uint32_t word_length, const uint32_t trigger_value, const bool trigger_word);

void set_btle(const uint32_t baudrate, const uint32_t word_length, const uint32_t trigger_value, const bool trigger_word);

void set_nrf(const uint32_t baudrate, const uint32_t word_length, const uint32_t trigger_value, const bool trigger_word);

void set_ook_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint8_t repeat,
					const uint32_t pause_symbols);
void set_fsk_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint32_t shift,
					const uint32_t progress_notice);
void set_pocsag(const pocsag::BitRate bitrate, bool phase);
void set_adsb();
void set_jammer(const bool run, const jammer::JammerType type, const uint32_t speed);
void set_rds_data(const uint16_t message_length);
void set_spectrum(const size_t sampling_rate, const size_t trigger);
void set_siggen_tone(const uint32_t tone);
void set_siggen_config(const uint32_t bw, const uint32_t shape, const uint32_t duration);
void request_beep();

void run_image(const portapack::spi_flash::image_tag_t image_tag);
void shutdown();

void spectrum_streaming_start();
void spectrum_streaming_stop();

void set_sample_rate(const uint32_t sample_rate);
void capture_start(CaptureConfig* const config);
void capture_stop();
void replay_start(ReplayConfig* const config);
void replay_stop();

} /* namespace baseband */

#endif/*__BASEBAND_API_H__*/
