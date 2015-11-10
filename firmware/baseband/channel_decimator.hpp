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

#ifndef __CHANNEL_DECIMATOR_H__
#define __CHANNEL_DECIMATOR_H__

#include "buffer.hpp"
#include "complex.hpp"

#include "dsp_decimate.hpp"

#include <array>

class ChannelDecimator {
public:
	enum class DecimationFactor {
		By4,
		By8,
		By16,
		By32,
	};

	constexpr ChannelDecimator(
	) : decimation_factor { DecimationFactor::By32 }
	{
	}
	
	constexpr ChannelDecimator(
		const DecimationFactor decimation_factor
	) : decimation_factor { decimation_factor }
	{
	}

	void set_decimation_factor(const DecimationFactor f) {
		decimation_factor = f;
	}

	buffer_c16_t execute(buffer_c8_t buffer) {
		auto decimated = execute_decimation(buffer);

		return decimated;
	}

private:
	std::array<complex16_t, 1024> work_baseband;

	const buffer_c16_t work_baseband_buffer {
		work_baseband.data(),
		work_baseband.size()
	};
	const buffer_s16_t work_audio_buffer {
		(int16_t*)work_baseband.data(),
		sizeof(work_baseband) / sizeof(int16_t)
	};

	//const bool fs_over_4_downconvert = true;

	dsp::decimate::TranslateByFSOver4AndDecimateBy2CIC3 translate;
	//dsp::decimate::DecimateBy2CIC3 cic_0;
	dsp::decimate::DecimateBy2CIC3 cic_1;
	dsp::decimate::DecimateBy2CIC3 cic_2;
	dsp::decimate::DecimateBy2CIC3 cic_3;
	dsp::decimate::DecimateBy2CIC3 cic_4;

	DecimationFactor decimation_factor;

	buffer_c16_t execute_decimation(buffer_c8_t buffer);
};

#endif/*__CHANNEL_DECIMATOR_H__*/
