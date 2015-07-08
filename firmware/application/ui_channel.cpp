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

#include "ui_channel.hpp"

#include <algorithm>

namespace ui {

void Channel::on_show() {
	context().message_map[Message::ID::ChannelStatistics] = [this](const Message* const p) {
		this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
	};
}

void Channel::on_hide() {
	context().message_map[Message::ID::ChannelStatistics] = nullptr;
}

void Channel::paint(Painter& painter) {
	const auto r = screen_rect();

	const int32_t db_min = -r.width();
	const int32_t x_0 = 0;
	const int32_t x_max = std::max(x_0, max_db_ - db_min);
	const int32_t x_lim = r.width();

	const Rect r0 {
		static_cast<ui::Coord>(r.left() + x_0), r.top(),
		static_cast<ui::Dim>(x_max - x_0), r.height()
	};
	painter.fill_rectangle(
		r0,
		Color::blue()
	);

	const Rect r1 {
		static_cast<ui::Coord>(r.left() + x_max), r.top(),
		1, r.height()
	};
	painter.fill_rectangle(
		r1,
		Color::white()
	);

	const Rect r2 {
		static_cast<ui::Coord>(r.left() + x_max + 1), r.top(),
		static_cast<ui::Dim>(x_lim - (x_max + 1)), r.height()
	};
	painter.fill_rectangle(
		r2,
		Color::black()
	);
}

void Channel::on_statistics_update(const ChannelStatistics& statistics) {
	max_db_ = statistics.max_db;
	set_dirty();
}

} /* namespace ui */
