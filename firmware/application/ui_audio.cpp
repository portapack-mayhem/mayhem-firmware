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

#include "ui_audio.hpp"

#include "event_m0.hpp"

#include <algorithm>

namespace ui {

void Audio::on_show() {
	EventDispatcher::message_map().register_handler(Message::ID::AudioStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const AudioStatisticsMessage*>(p)->statistics);
		}
	);
}

void Audio::on_hide() {
	EventDispatcher::message_map().unregister_handler(Message::ID::AudioStatistics);
}

void Audio::paint(Painter& painter) {
	const auto r = screen_rect();

	const int32_t db_min = -r.width();
	const int32_t x_0 = 0;
	const int32_t x_rms = std::max(x_0, rms_db_ - db_min);
	const int32_t x_max = std::max(x_rms + 1, max_db_ - db_min);
	const int32_t x_lim = r.width();

	const Rect r0 {
		static_cast<ui::Coord>(r.left() + x_0), r.top(),
		static_cast<ui::Dim>(x_rms - x_0), r.height()
	};
	painter.fill_rectangle(
		r0,
		Color::green()
	);

	const Rect r1 {
		static_cast<ui::Coord>(r.left() + x_rms), r.top(),
		1, r.height()
	};
	painter.fill_rectangle(
		r1,
		Color::black()
	);

	const Rect r2 {
		static_cast<ui::Coord>(r.left() + x_rms + 1), r.top(),
		static_cast<ui::Dim>(x_max - (x_rms + 1)), r.height()
	};
	painter.fill_rectangle(
		r2,
		Color::red()
	);

	const Rect r3 {
		static_cast<ui::Coord>(r.left() + x_max), r.top(),
		static_cast<ui::Dim>(x_lim - x_max), r.height()
	};
	painter.fill_rectangle(
		r3,
		Color::black()
	);
}

void Audio::on_statistics_update(const AudioStatistics& statistics) {
	rms_db_ = statistics.rms_db;
	max_db_ = statistics.max_db;
	set_dirty();
}

} /* namespace ui */
