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

#include "recent_entries.hpp"

namespace ui {

RecentEntriesColumns::RecentEntriesColumns(
	const std::initializer_list<RecentEntriesColumn> columns
) : _columns { columns }
{
}

RecentEntriesHeader::RecentEntriesHeader(
	const RecentEntriesColumns& columns
) : _columns { columns }
{
}

void RecentEntriesHeader::paint(Painter& painter) {
	const auto r = screen_rect();
	const auto& parent_style = style();

	const Style style {
		.font = parent_style.font,
		.background = Color::blue(),
		.foreground = parent_style.foreground,
	};

	auto p = r.location();
	for(const auto& column : _columns) {
		const auto width = column.second;
		auto text = column.first;
		if( width > text.length() ) {
			text.append(width - text.length(), ' ');
		}

		painter.draw_string(p, style, text);
		p += { static_cast<Coord>((width * 8) + 8), 0 };
	}
}

} /* namespace ui */
