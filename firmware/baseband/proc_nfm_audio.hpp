/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_NFM_AUDIO_H__
#define __PROC_NFM_AUDIO_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

#include <cstdint>

class NarrowbandFMAudio : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;

private:
	static constexpr size_t baseband_fs = 3072000;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };

	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(dst) / sizeof(int16_t)
	};
	
	std::array<int16_t, 16> audio { };
	const buffer_s16_t audio_buffer {
		(int16_t*)audio.data(),
		sizeof(audio) / sizeof(int16_t)
	};
	
	std::array<int16_t, 16> tone { };
	const buffer_s16_t tone_buffer {
		(int16_t*)tone.data(),
		sizeof(tone) / sizeof(int16_t)
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::decimate::FIRAndDecimateComplex channel_filter { };
	uint32_t channel_filter_pass_f = 0;
	uint32_t channel_filter_stop_f = 0;
	
	// For CTCSS decoding
	dsp::decimate::FIR64AndDecimateBy2Real ctcss_filter { };
	IIRBiquadFilter hpf { };

	dsp::demodulate::FM demod { };

	AudioOutput audio_output { };

	SpectrumCollector channel_spectrum { };
	
	uint32_t tone_phase { 0 };
	uint32_t tone_delta { 0 };
	bool pitch_rssi_enabled { false };
	
	float cur_sample { }, prev_sample { };
	uint32_t z_acc { 0}, z_timer { 0 }, z_count { 0 };
	bool ctcss_detect_enabled { true };
	static constexpr float k = 32768.0f;
	static constexpr float ki = 1.0f / k;

	bool configured { false };
	void pitch_rssi_config(const PitchRSSIConfigureMessage& message);
	void configure(const NBFMConfigureMessage& message);
	void capture_config(const CaptureConfigMessage& message);
	
	//RequestSignalMessage sig_message { RequestSignalMessage::Signal::Squelched };
	CodedSquelchMessage ctcss_message { 0 };
};

#endif/*__PROC_NFM_AUDIO_H__*/
