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

#include "event_m0.hpp"

#include <algorithm>

namespace ui {

void RSSI::on_show() {
	EventDispatcher::message_map().register_handler(Message::ID::RSSIStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const RSSIStatisticsMessage*>(p)->statistics);
		}
	);
}

void RSSI::on_hide() {
	EventDispatcher::message_map().unregister_handler(Message::ID::RSSIStatistics);
}

void RSSI::paint(Painter& painter) {
	const auto r = screen_rect();
	/*
	constexpr int32_t rssi_min = 0.# * 256 / 3.3;
	constexpr int32_t rssi_max = 2.5 * 256 / 3.3;
	// (23 - 194) / 2
	*/
	/* TODO: Clip maximum */
	constexpr int32_t raw_min = 23;
	const int32_t x_0 = 0;
	const int32_t x_min = std::max(x_0, (min_ - raw_min) / 2);
	const int32_t x_avg = std::max(x_min, (avg_ - raw_min) / 2);
	const int32_t x_max = std::max(x_avg + 1, (max_ - raw_min) / 2);
	const int32_t x_lim = r.width();

	const Rect r0 {
		static_cast<ui::Coord>(r.left() + x_0), r.top(),
		static_cast<ui::Dim>(x_min - x_0), r.height()
	};
	painter.fill_rectangle(
		r0,
		Color::blue()
	);

	const Rect r1 {
		static_cast<ui::Coord>(r.left() + x_min), r.top(),
		static_cast<ui::Dim>(x_avg - x_min), r.height()
	};
	painter.fill_rectangle(
		r1,
		Color::red()
	);

	const Rect r2 {
		static_cast<ui::Coord>(r.left() + x_avg), r.top(),
		1, r.height()
	};
	painter.fill_rectangle(
		r2,
		Color::white()
	);

	const Rect r3 {
		static_cast<ui::Coord>(r.left() + x_avg + 1), r.top(),
		static_cast<ui::Dim>(x_max - (x_avg + 1)), r.height()
	};
	painter.fill_rectangle(
		r3,
		Color::red()
	);

	const Rect r4 {
		static_cast<ui::Coord>(r.left() + x_max), r.top(),
		static_cast<ui::Dim>(x_lim - x_max), r.height()
	};
	painter.fill_rectangle(
		r4,
		Color::black()
	);
}

void RSSI::on_statistics_update(const RSSIStatistics& statistics) {
	min_ = statistics.min;
	avg_ = statistics.accumulator / statistics.count;
	max_ = statistics.max;
	set_dirty();
}

} /* namespace ui */
