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

#include "ui_text.hpp"

namespace ui {

Glyph Font::glyph(const char c) const {
	if( c < c_start ) {
		return { w, h, data };
	}
	const size_t index = c - c_start;
	if( index >= c_count ) {
		return { w, h, data };
	} else {
		return { w, h, &data[index * data_stride] };
	}
}

Dim Font::line_height() const {
	return h;
}

Size Font::size_of(const std::string s) const {
	Size size;

	for(const auto c : s) {
		const auto glyph_data = glyph(c);
		size = {
			size.width() + glyph_data.w(),
			std::max(size.height(), glyph_data.h())
		};
	}

	return size;
}

} /* namespace ui */
