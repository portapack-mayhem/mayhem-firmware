/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_AUDIOTX_H__
#define __PROC_AUDIOTX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "tone_gen.hpp"
#include "stream_output.hpp"

class AudioTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const msg) override;

private:
	size_t baseband_fs = 0;
	
	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	std::array<uint8_t, 64> audio { };
	const buffer_t<uint8_t> audio_buffer {
		audio.data(),
		audio.size(),
		baseband_fs / 8
	};
	
	std::unique_ptr<StreamOutput> stream { };
	
	ToneGen tone_gen { };
	
	uint32_t fm_delta { 0 };
	uint32_t phase { 0 }, sphase { 0 };
	int8_t out_sample { };
	int32_t sample { 0 }, audio_sample { 0 }, delta { };
	int8_t re { 0 }, im { 0 };
	
	size_t spectrum_interval_samples = 0;
	size_t spectrum_samples = 0;

	//int16_t audio_data[64];
	/*const buffer_s16_t preview_audio_buffer {
		audio_data,
		sizeof(int16_t)*64
	};*/
	
	bool configured { false };
	uint32_t bytes_read { 0 };
	
	void samplerate_config(const SamplerateConfigMessage& message);
	void audio_config(const AudioTXConfigMessage& message);
	void replay_config(const ReplayConfigMessage& message);
	
	TXProgressMessage txprogress_message { };
	RequestSignalMessage sig_message { RequestSignalMessage::Signal::FillRequest };
};

#endif
