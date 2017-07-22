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

#include "ui.hpp"
#include "file.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "portapack.hpp"

namespace ui {

class GeoMapView : public View {
public:
	GeoMapView(NavigationView& nav);
	~GeoMapView();
	
	void focus() override;
	
	std::string title() const override { return "Map view"; };

private:
	NavigationView& nav_;
	
	File map_file { };
	bool file_error { false };
	uint16_t map_width { }, map_height { };
	int32_t map_center_x { }, map_center_y { };
	
	void move_map();
	Point polar_to_point(const uint8_t angle, const uint32_t size);
	void draw_bearing(const Point origin, const uint8_t angle, uint32_t size, const Color color);
	
	Labels labels {
		{ { 0 * 8, 0 * 8 }, "Test", Color::light_grey() }
	};
	
	NumberField field_xpos {
		{ 0, 0 },
		4,
		{ -180, 180 },
		1,
		'0'
	};
	NumberField field_ypos {
		{ 6 * 8, 0 },
		4,
		{ -90, 90 },
		1,
		'0'
	};
	NumberField field_angle {
		{ 12 * 8, 0 },
		3,
		{ 0, 255 },
		1,
		'0'
	};
};

} /* namespace ui */
