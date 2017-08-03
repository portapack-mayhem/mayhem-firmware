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
#include "portapack.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

GeoPos::GeoPos(
	const Point pos
) {
	set_parent_rect({pos, {30 * 8, 3 * 16}});
	
	add_children({
		&labels_position,
		&field_altitude,
		&field_lat_degrees,
		&field_lat_minutes,
		&field_lat_seconds,
		&field_lon_degrees,
		&field_lon_minutes,
		&field_lon_seconds
	});
	
	// Defaults
	set_altitude(0);
	set_lat(0);
	set_lon(0);
	
	const auto changed = [this](int32_t) {
		if (on_change)
			on_change();
	};
	
	field_altitude.on_change = changed;
	field_lat_degrees.on_change = changed;
	field_lat_minutes.on_change = changed;
	field_lat_seconds.on_change = changed;
	field_lon_degrees.on_change = changed;
	field_lon_minutes.on_change = changed;
	field_lon_seconds.on_change = changed;
}

void GeoPos::focus() {
	field_altitude.focus();
}

void GeoPos::set_altitude(int32_t altitude) {
	field_altitude.set_value(altitude);
}

void GeoPos::set_lat(float lat) {
	field_lat_degrees.set_value(lat);
	field_lat_minutes.set_value((uint32_t)(lat / (1.0 / 60)) % 60);
	field_lat_seconds.set_value((uint32_t)(lat / (1.0 / 3600)) % 60);
}

void GeoPos::set_lon(float lon) {
	field_lon_degrees.set_value(lon);
	field_lon_minutes.set_value((uint32_t)(lon / (1.0 / 60)) % 60);
	field_lon_seconds.set_value((uint32_t)(lon / (1.0 / 3600)) % 60);
}

float GeoPos::lat() {
	return field_lat_degrees.value() + (field_lat_minutes.value() / 60.0) + (field_lat_seconds.value() / 3600.0);
};

float GeoPos::lon() {
	return field_lon_degrees.value() + (field_lon_minutes.value() / 60.0) + (field_lon_seconds.value() / 3600.0);
};

int32_t GeoPos::altitude() {
	return field_altitude.value();
};

void GeoPos::set_read_only(bool v) {
	set_focusable(~v);
};

void GeoMapView::focus() {
	if (!file_error) {
		geopos.focus();
		move_map();
	} else
		nav_.display_modal("No map", "No world_map.bin file in\n/ADSB/ directory", ABORT, nullptr);
}

GeoMapView::~GeoMapView() {
	
}

void GeoMapView::draw_bearing(const Point origin, const uint32_t angle, uint32_t size, const Color color) {
	Point arrow_a, arrow_b, arrow_c;
	
	for (size_t thickness = 0; thickness < 3; thickness++) {
		arrow_a = polar_to_point(angle, size) + origin;
		arrow_b = polar_to_point(angle + 180 - 30, size) + origin;
		arrow_c = polar_to_point(angle + 180 + 30, size) + origin;
		
		display.draw_line(arrow_a, arrow_b, color);
		display.draw_line(arrow_b, arrow_c, color);
		display.draw_line(arrow_c, arrow_a, color);
		
		size--;
	}
}

void GeoMapView::move_map() {
	Coord line;
	int32_t x_pos, y_pos;
	std::array<ui::Color, 240> map_line_buffer;
	
	auto r = screen_rect();
	Rect map_rect = { r.left(), r.top() + banner_height, r.width(), r.height() - banner_height };
	
	altitude_ = geopos.altitude();
	lat_ = geopos.lat();
	lon_ = geopos.lon();
	
	// Map is in Equidistant "Plate Carrée" projection
	x_pos = map_center_x - (map_rect.width() / 2) + ((lat_ * map_center_x) / 180);
	y_pos = map_center_y - (map_rect.height() / 2) + ((lon_ * map_center_y) / 90);
	
	if (x_pos > (map_width - map_rect.width()))
		x_pos = map_width - map_rect.width();
	if (y_pos > (map_height + map_rect.height()))
		y_pos = map_height - map_rect.height();
	
	for (line = 0; line < map_rect.height(); line++) {
		map_file.seek(4 + ((x_pos + (map_width * (y_pos + line))) << 1));
		map_file.read(map_line_buffer.data(), map_rect.width() << 1);
		display.draw_pixels({ 0, map_rect.top() + line, map_rect.width(), 1 }, map_line_buffer);
	}
	
	if (mode_ == PROMPT) {
		display.fill_rectangle({ map_rect.center() - Point(16, 1), { 32, 2 } }, Color::red());
		display.fill_rectangle({ map_rect.center() - Point(1, 16), { 2, 32 } }, Color::red());
	} else {
		draw_bearing({ 120, 32 + 144 }, angle_, 16, Color::red());
	}
}

void GeoMapView::setup() {
	auto result = map_file.open("ADSB/world_map.bin");
	if (result.is_valid()) {
		file_error = true;
		return;
	}
	
	map_file.read(&map_width, 2);
	map_file.read(&map_height, 2);
	
	map_center_x = map_width >> 1;
	map_center_y = map_height >> 1;
	
	add_child(&geopos);
	
	geopos.set_altitude(altitude_);
	geopos.set_lat(lat_);
	geopos.set_lon(lon_);
	
	geopos.on_change = [this]() {
		move_map();
	};
}

// Display mode
GeoMapView::GeoMapView(
	NavigationView& nav,
	std::string* tag,
	int32_t altitude,
	float lat,
	float lon,
	float angle
) : nav_ (nav),
	tag_ (tag),
	altitude_ (altitude),
	lat_ (lat),
	lon_ (lon),
	angle_ (angle)
{
	mode_ = DISPLAY;
	setup();
	
	geopos.set_read_only(true);
}

// Prompt mode
GeoMapView::GeoMapView(
	NavigationView& nav,
	int32_t altitude,
	float lat,
	float lon,
	const std::function<void(int32_t, float, float)> on_done
) : nav_ (nav),
	altitude_ (altitude),
	lat_ (lat),
	lon_ (lon)
{
	mode_ = PROMPT;
	setup();
	
	add_child(&button_ok);
	
	button_ok.on_select = [this, on_done, &nav](Button&) {
		if (on_done)
			on_done(altitude_, lat_, lon_);
		nav.pop();
	};
}

} /* namespace ui */
