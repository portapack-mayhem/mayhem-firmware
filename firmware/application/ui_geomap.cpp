/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_geomap.hpp"

#include "adsb.hpp"
//#include "string_format.hpp"
#include "sine_table_int8.hpp"
#include "portapack.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void GeoMapView::focus() {
	if (!file_error) {
		field_xpos.focus();
		move_map();
	} else
		nav_.display_modal("No map", "No world_map.bin file in\n/ADSB/ directory", ABORT, nullptr);
}

GeoMapView::~GeoMapView() {
	
}

Point GeoMapView::polar_to_point(const uint8_t angle, const uint32_t size) {
	return { (Coord)(sine_table_i8[(angle + 64) & 0xFF] * size) >> 7, (Coord)(sine_table_i8[angle] * size) >> 7 };
}

void GeoMapView::draw_bearing(const Point origin, const uint8_t angle, uint32_t size, const Color color) {
	Point arrow_a, arrow_b, arrow_c;
	
	arrow_a = polar_to_point(angle, size) + origin;
	arrow_b = polar_to_point(angle + 128 - 16, size) + origin;
	arrow_c = polar_to_point(angle + 128 + 16, size) + origin;
	display.draw_line(arrow_a, arrow_b, color);
	display.draw_line(arrow_b, arrow_c, color);
	display.draw_line(arrow_c, arrow_a, color);
	
	size--;
	arrow_a = polar_to_point(angle, size) + origin;
	arrow_b = polar_to_point(angle + 128 - 16, size) + origin;
	arrow_c = polar_to_point(angle + 128 + 16, size) + origin;
	display.draw_line(arrow_a, arrow_b, color);
	display.draw_line(arrow_b, arrow_c, color);
	display.draw_line(arrow_c, arrow_a, color);
}

void GeoMapView::move_map() {
	Coord line;
	int32_t x_pos, y_pos;
	std::array<ui::Color, 240> map_buffer;
	
	// Map is in Equidistant "Plate Carrée" projection
	x_pos = map_center_x - 120 + ((((field_xpos.value() * map_center_x) << 8) / 180) >> 8);
	y_pos = map_center_y - 144 + ((((field_ypos.value() * map_center_y) << 8) / 90) >> 8);
	
	if (x_pos > (map_width - 240))
		x_pos = map_width - 240;
	if (y_pos > (map_height + 288))
		y_pos = map_height - 288;
	
	for (line = 0; line < 288; line++) {
		map_file.seek(4 + ((x_pos + (map_width * (y_pos + line))) << 1));
		map_file.read(map_buffer.data(), 240 * 2);
		display.draw_pixels({ 0, 32 + line, 240, 1 }, map_buffer);
	}
	
	draw_bearing({ 120, 32 + 144 }, field_angle.value(), 16, Color::red());
	
	//display.fill_rectangle({ 120-16, 176-1, 32, 2 }, Color::red());
	//display.fill_rectangle({ 120-1, 176-16, 2, 32 }, Color::red());
}

GeoMapView::GeoMapView(
	NavigationView& nav
) : nav_ (nav)
{
	auto result = map_file.open("ADSB/world_map.bin");
	if (result.is_valid()) {
		file_error = true;
		return;
	}
	
	map_file.read(&map_width, 2);
	map_file.read(&map_height, 2);
	
	map_center_x = map_width >> 1;
	map_center_y = map_height >> 1;
	
	add_children({
		&field_xpos,
		&field_ypos,
		&field_angle
	});
	
	field_xpos.on_change = [this](int32_t) {
		move_map();
	};
	field_ypos.on_change = [this](int32_t) {
		move_map();
	};
	field_angle.on_change = [this](int32_t) {
		move_map();
	};
}

} /* namespace ui */
