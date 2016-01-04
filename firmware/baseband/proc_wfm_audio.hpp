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

#ifndef __PROC_WFM_AUDIO_H__
#define __PROC_WFM_AUDIO_H__

#include "baseband_processor.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"

class WidebandFMAudio : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;

private:
	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(dst) / sizeof(int16_t)
	};

	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0;
	dsp::decimate::FIRC16xR16x16Decim2 decim_1;

	dsp::demodulate::FM demod;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_1;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_2;
	dsp::decimate::FIR64AndDecimateBy2Real audio_filter;

	IIRBiquadFilter audio_hpf { audio_hpf_30hz_config };
	IIRBiquadFilter audio_deemph { audio_deemph_2122_6_config };

	bool configured { false };
	void configure(const WFMConfigureMessage& message);
};

#endif/*__PROC_WFM_AUDIO_H__*/
