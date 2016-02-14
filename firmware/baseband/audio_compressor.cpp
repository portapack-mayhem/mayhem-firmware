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

#include "audio_compressor.hpp"

float GainComputer::operator()(const float x) const {
	const auto abs_x = std::abs(x);
	const auto db = (abs_x < lin_floor) ? db_floor : log2_db_k * fast_log2(abs_x);
	const auto overshoot_db = db - threshold_db;
	if( knee_width_db > 0.0f ) {
		const auto w2 = knee_width_db / 2.0f;
		const auto a = w2 / (knee_width_db * knee_width_db);
		const auto in_transition = (overshoot_db > -w2) && (overshoot_db < w2);
		const auto rectified_overshoot = in_transition ? (a * std::pow(overshoot_db + w2, 2.0f)) : std::max(overshoot_db, 0.0f);
		return rectified_overshoot * slope;
	} else {
		const auto rectified_overshoot = std::max(overshoot_db, 0.0f);
		return rectified_overshoot * slope;
	}
}

void FeedForwardCompressor::execute_in_place(const buffer_f32_t& buffer) {
	constexpr float makeup_gain = std::pow(10.0f, (threshold - (threshold / ratio)) / -20.0f);
	for(size_t i=0; i<buffer.count; i++) {
		buffer.p[i] = execute_once(buffer.p[i]) * makeup_gain;
	}
}

float FeedForwardCompressor::execute_once(const float x) {
	const auto gain_db = gain_computer(x);
	const auto peak_db = -peak_detector(-gain_db);
	const auto gain = fast_pow2(peak_db * (3.321928094887362f / 20.0f));
	return x * gain;
}
