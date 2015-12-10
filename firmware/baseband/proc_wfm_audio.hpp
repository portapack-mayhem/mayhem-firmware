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

#include "channel_decimator.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"

class WidebandFMAudio : public BasebandProcessor {
public:
	WidebandFMAudio() {
		decimator.set_decimation_factor(ChannelDecimator::DecimationFactor::By4);
	}

	void execute(buffer_c8_t& buffer) override;

private:
	ChannelDecimator decimator;

	dsp::demodulate::FM demod { 768000, 75000 };
	dsp::decimate::DecimateBy2CIC4Real audio_dec_1;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_2;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_3;
	const fir_taps_real<64>& audio_filter_taps = taps_64_lp_156_198;
	dsp::decimate::FIR64AndDecimateBy2Real audio_filter { audio_filter_taps.taps };

	IIRBiquadFilter audio_hpf { audio_hpf_config };
};

#endif/*__PROC_WFM_AUDIO_H__*/
