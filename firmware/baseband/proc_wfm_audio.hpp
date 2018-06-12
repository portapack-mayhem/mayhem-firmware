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

#ifndef __PROC_WFM_AUDIO_H__
#define __PROC_WFM_AUDIO_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_types.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "block_decimator.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

class WidebandFMAudio : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;

private:
	static constexpr size_t baseband_fs = 3072000;
	static constexpr auto spectrum_rate_hz = 50.0f;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };

	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	// work_audio_buffer and dst_buffer use the same data pointer
	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(dst) / sizeof(int16_t)
	};

	std::array<complex16_t, 64> complex_audio { };
	const buffer_c16_t complex_audio_buffer {
		complex_audio.data(),
		complex_audio.size()
	};
	
	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0 { };
	dsp::decimate::FIRC16xR16x16Decim2 decim_1 { };
	uint32_t channel_filter_pass_f = 0;
	uint32_t channel_filter_stop_f = 0;

	dsp::demodulate::FM demod { };
	dsp::decimate::DecimateBy2CIC4Real audio_dec_1 { };
	dsp::decimate::DecimateBy2CIC4Real audio_dec_2 { };
	dsp::decimate::FIR64AndDecimateBy2Real audio_filter { };

	AudioOutput audio_output { };
	
	// For fs=96kHz FFT streaming
	BlockDecimator<complex16_t, 256> audio_spectrum_decimator { 1 };
	std::array<std::complex<float>, 256> audio_spectrum { };
	uint32_t audio_spectrum_timer { 0 };
	enum AudioSpectrumState {
		IDLE = 0,
		FEED,
		FFT
	};
	AudioSpectrumState audio_spectrum_state { IDLE };
	AudioSpectrum spectrum { };
	uint32_t fft_step { 0 };

	SpectrumCollector channel_spectrum { };
	size_t spectrum_interval_samples = 0;
	size_t spectrum_samples = 0;

	bool configured { false };
	void configure(const WFMConfigureMessage& message);
	void capture_config(const CaptureConfigMessage& message);
	void post_message(const buffer_c16_t& data);
};

#endif/*__PROC_WFM_AUDIO_H__*/
