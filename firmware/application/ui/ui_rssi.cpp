/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_rssi.hpp"

#include "baseband_api.hpp"
#include "utility.hpp"

#include <algorithm>

namespace ui {

void RSSI::paint(Painter& painter) {
	const auto r = screen_rect();

	constexpr int rssi_sample_range = 256;
	constexpr float rssi_voltage_min = 0.4;
	constexpr float rssi_voltage_max = 2.2;
	constexpr float adc_voltage_max = 3.3;
	constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
	constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
	constexpr int raw_delta = raw_max - raw_min;
	const range_t<int> x_avg_range { 0, r.width() - 1 };
	const auto x_avg = x_avg_range.clip((avg_ - raw_min) * r.width() / raw_delta);
	const range_t<int> x_min_range { 0, x_avg };
	const auto x_min = x_min_range.clip((min_ - raw_min) * r.width() / raw_delta);
	const range_t<int> x_max_range { x_avg + 1, r.width() };
	const auto x_max = x_max_range.clip((max_ - raw_min) * r.width() / raw_delta);

	const Rect r0 { r.left(), r.top(), x_min, r.height() };
	painter.fill_rectangle(
		r0,
		Color::blue()
	);

	const Rect r1 { r.left() + x_min, r.top(), x_avg - x_min, r.height() };
	painter.fill_rectangle(
		r1,
		Color::red()
	);

	const Rect r2 { r.left() + x_avg, r.top(), 1, r.height() };
	painter.fill_rectangle(
		r2,
		Color::white()
	);

	const Rect r3 { r.left() + x_avg + 1, r.top(), x_max - (x_avg + 1), r.height() };
	painter.fill_rectangle(
		r3,
		Color::red()
	);

	const Rect r4 { r.left() + x_max, r.top(), r.width() - x_max, r.height() };
	painter.fill_rectangle(
		r4,
		Color::black()
	);
	
	if (pitch_rssi_enabled)
		baseband::set_pitch_rssi((avg_ - raw_min) * 2000 / raw_delta, true);
}

void RSSI::set_pitch_rssi(bool enabled) {
	pitch_rssi_enabled = enabled;
	if (!enabled) baseband::set_pitch_rssi(0, false);
}

void RSSI::on_statistics_update(const RSSIStatistics& statistics) {
	min_ = statistics.min;
	avg_ = statistics.accumulator / statistics.count;
	max_ = statistics.max;
	set_dirty();
}

} /* namespace ui */
