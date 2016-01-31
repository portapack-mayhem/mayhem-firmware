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

#include "channel_decimator.hpp"

buffer_c16_t ChannelDecimator::execute_decimation(const buffer_c8_t& buffer) {
	const buffer_c16_t work_baseband_buffer {
		work_baseband.data(),
		work_baseband.size()
	};

	const buffer_s16_t work_audio_buffer {
		(int16_t*)work_baseband.data(),
		sizeof(work_baseband) / sizeof(int16_t)
	};

	/* 3.072MHz complex<int8_t>[2048], [-128, 127]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
	 *                   -0.1dB @ 86kHz,   -1dB @ 270kHz,  -60dB @ 1.44MHz
	 * -> gain of 256
	 * -> decimation by 2
	 * -> 1.544MHz complex<int16_t>[1024], [-32768, 32512] */
	auto stage_0_out = execute_stage_0(buffer, work_baseband_buffer);
	if( decimation_factor == DecimationFactor::By2 ) {
		return stage_0_out;
	}

	/* 1.536MHz complex<int16_t>[1024], [-32768, 32512]
	 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
	 *                   -0.1dB @ 43kHz,   -1dB @ 136kHz,  -60dB @ 723kHz
	 * -> gain of 1
	 * -> decimation by 2
	 * -> 768kHz complex<int16_t>[512], [-8192, 8128] */
	auto cic_1_out = cic_1.execute(stage_0_out, work_baseband_buffer);
	if( decimation_factor == DecimationFactor::By4 ) {
		return cic_1_out;
	}

	/* 768kHz complex<int16_t>[512], [-32768, 32512]
	 * -> 3rd order CIC decimation by 2, gain of 1
	 * -> 384kHz complex<int16_t>[256], [-32768, 32512] */
	auto cic_2_out = cic_2.execute(cic_1_out, work_baseband_buffer);
	if( decimation_factor == DecimationFactor::By8 ) {
		return cic_2_out;
	}

	/* 384kHz complex<int16_t>[256], [-32768, 32512]
	 * -> 3rd order CIC decimation by 2, gain of 1
	 * -> 192kHz complex<int16_t>[128], [-32768, 32512] */
	auto cic_3_out = cic_3.execute(cic_2_out, work_baseband_buffer);
	if( decimation_factor == DecimationFactor::By16 ) {
		return cic_3_out;
	}

	/* 192kHz complex<int16_t>[128], [-32768, 32512]
	 * -> 3rd order CIC decimation by 2, gain of 1
	 * -> 96kHz complex<int16_t>[64], [-32768, 32512] */
	auto cic_4_out = cic_4.execute(cic_3_out, work_baseband_buffer);

	return cic_4_out;
}

buffer_c16_t ChannelDecimator::execute_stage_0(
	const buffer_c8_t& buffer,
	const buffer_c16_t& work_baseband_buffer
) {
	if( fs_over_4_downconvert ) {
		return translate.execute(buffer, work_baseband_buffer);
	} else {
		return cic_0.execute(buffer, work_baseband_buffer);
	}
}
