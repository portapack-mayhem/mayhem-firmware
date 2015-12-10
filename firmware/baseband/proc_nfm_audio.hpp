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

#ifndef __PROC_NFM_AUDIO_H__
#define __PROC_NFM_AUDIO_H__

#include "baseband_processor.hpp"

#include "channel_decimator.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "dsp_squelch.hpp"

class NarrowbandFMAudio : public BasebandProcessor {
public:
	NarrowbandFMAudio() {
		decimator.set_decimation_factor(ChannelDecimator::DecimationFactor::By32);
		channel_filter.configure(channel_filter_taps.taps, 2);
	}

	void execute(buffer_c8_t& buffer) override;

private:
	ChannelDecimator decimator;
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_042_078_tfilter;
	dsp::decimate::FIRAndDecimateComplex channel_filter;
	dsp::demodulate::FM demod { 48000, 7500 };

	IIRBiquadFilter audio_hpf { audio_hpf_config };
	FMSquelch squelch;
};

#endif/*__PROC_NFM_AUDIO_H__*/
