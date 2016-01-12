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

#ifndef __PROC_AM_AUDIO_H__
#define __PROC_AM_AUDIO_H__

#include "baseband_processor.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "spectrum_collector.hpp"

#include <cstdint>

class NarrowbandAMAudio : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const message) override;

private:
	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	const buffer_f32_t work_audio_buffer {
		(float*)dst.data(),
		sizeof(dst) / sizeof(float)
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0;
	dsp::decimate::FIRC16xR16x32Decim8 decim_1;
	dsp::decimate::FIRAndDecimateComplex channel_filter;
	uint32_t channel_filter_pass_f;
	uint32_t channel_filter_stop_f;

	dsp::demodulate::AM demod;

	SpectrumCollector channel_spectrum;

	bool configured { false };
	void configure(const AMConfigureMessage& message);

	void streaming_config(const SpectrumStreamingConfigMessage& message);
};

#endif/*__PROC_AM_AUDIO_H__*/
