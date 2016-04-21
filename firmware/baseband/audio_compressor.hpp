/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AUDIO_COMPRESSOR_H__
#define __AUDIO_COMPRESSOR_H__

#include "dsp_types.hpp"
#include "utility.hpp"

#include <cmath>

/* Code based on article in Journal of the Audio Engineering Society
 * Vol. 60, No. 6, 2012 June, by Dimitrios Giannoulis, Michael Massberg,
 * Joshua D. Reiss "Digital Dynamic Range Compressor Design – A Tutorial
 * and Analysis"
 */

class GainComputer {
public:
	constexpr GainComputer(
		float ratio,
		float threshold
	) : ratio { ratio },
		slope { 1.0f / ratio - 1.0f },
		threshold_db { threshold }
	{
	}

	float operator()(const float x) const;

private:
	const float ratio;
	const float slope;
	const float threshold_db;

	static constexpr float knee_width_db = 0.0f;

	static constexpr float db_floor = -120.0f;
	static constexpr float lin_floor = std::pow(10.0f, db_floor / 20.0f);
	static constexpr float log2_db_k = 20.0f * std::log10(2.0f);
};

class PeakDetectorBranchingSmooth {
public:
	constexpr PeakDetectorBranchingSmooth(
		float att_a,
		float rel_a
	) : att_a { att_a },
		rel_a { rel_a }
	{
	}

	float operator()(const float db) {
		const auto a = (db > state) ? att_a : rel_a;
		state = db + a * (state - db);
		return state;
	}

private:
	float state { 0.0f };
	const float att_a;
	const float rel_a;
};

class FeedForwardCompressor {
public:
	void execute_in_place(const buffer_f32_t& buffer);

private:
	static constexpr float fs = 12000.0f;
	static constexpr float ratio = 10.0f;
	static constexpr float threshold = -30.0f;

	GainComputer gain_computer { ratio, threshold };
	PeakDetectorBranchingSmooth peak_detector { tau_alpha(0.010f, fs), tau_alpha(0.300f, fs) };

	float execute_once(const float x);

	static constexpr float tau_alpha(const float tau, const float fs) {
		return std::exp(-1.0f / (tau * fs));
	}
};

#endif/*__AUDIO_COMPRESSOR_H__*/
