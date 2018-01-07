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

#include "utility.hpp"

#include <algorithm>

namespace ui {

void Channel::paint(Painter& painter) {
	const auto r = screen_rect();

	constexpr int db_min = -96;
	constexpr int db_max = 0;
	constexpr int db_delta = db_max - db_min;
	const range_t<int> x_max_range { 0, r.width() - 1 };
	const auto x_max = x_max_range.clip((max_db_ - db_min) * r.width() / db_delta);

	const Rect r0 { r.left(), r.top(), x_max, r.height() };
	painter.fill_rectangle(
		r0,
		Color::blue()
	);

	const Rect r1 { r.left() + x_max, r.top(), 1, r.height() };
	painter.fill_rectangle(
		r1,
		Color::white()
	);

	const Rect r2 { r.left() + x_max + 1, r.top(), r.width() - (x_max + 1), r.height() };
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
