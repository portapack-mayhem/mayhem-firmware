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

class GeoPos : public View {
public:
	std::function<void(void)> on_change { };
	
	GeoPos(const Point pos);
	
	void focus() override;
	
	void set_read_only(bool v);
	void set_altitude(int32_t altitude);
	void set_lat(float lat);
	void set_lon(float lon);
	float lat();
	float lon();
	int32_t altitude();

private:
	bool read_only { false };

	Labels labels_position {
		{ { 2 * 8, 0 * 16 }, "Alt:       feet", Color::light_grey() },
		{ { 2 * 8, 1 * 16 }, "Lat:     *  '  \"", Color::light_grey() },	// No ° symbol in 8x16 font
		{ { 2 * 8, 2 * 16 }, "Lon:     *  '  \"", Color::light_grey() },
	};
	
	NumberField field_altitude {
		{ 7 * 8, 0 * 16 },
		5,
		{ -1000, 50000 },
		250,
		' '
	};
	
	NumberField field_lat_degrees {
		{ 7 * 8, 1 * 16 }, 4, { -90, 90 }, 1, ' '
	};
	NumberField field_lat_minutes {
		{ 12 * 8, 1 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lat_seconds {
		{ 15 * 8, 1 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	
	NumberField field_lon_degrees {
		{ 7 * 8, 2 * 16 }, 4, { -180, 180 }, 1, ' '
	};
	NumberField field_lon_minutes {
		{ 12 * 8, 2 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lon_seconds {
		{ 15 * 8, 2 * 16 }, 2, { 0, 59 }, 1, ' '
	};
};

class GeoMapView : public View {
public:
	GeoMapView(NavigationView& nav, std::string* tag, int32_t altitude, float lat, float lon, float angle);
	GeoMapView(NavigationView& nav, int32_t altitude, float lat, float lon, const std::function<void(int32_t, float, float)> on_done);
	
	GeoMapView(const GeoMapView&) = delete;
	GeoMapView(GeoMapView&&) = delete;
	GeoMapView& operator=(const GeoMapView&) = delete;
	GeoMapView& operator=(GeoMapView&&) = delete;
	
	~GeoMapView();
	
	void focus() override;
	
	std::string title() const override { return "Map view"; };

private:
	enum Mode {
		DISPLAY,
		PROMPT
	};
	
	const Dim banner_height = 3 * 16;
	NavigationView& nav_;
	const std::function<void(int32_t, float, float)> on_done { };
	Mode mode_ { };
	std::string* tag_ { };
	int32_t altitude_ { };
	float lat_ { };
	float lon_ { };
	float angle_ { };
	
	File map_file { };
	bool file_error { false };
	uint16_t map_width { }, map_height { };
	int32_t map_center_x { }, map_center_y { };
	
	void setup();
	void move_map();
	void draw_bearing(const Point origin, const uint32_t angle, uint32_t size, const Color color);
	
	GeoPos geopos {
		{ 0, 0 }
	};
	
	Button button_ok {
		{ 20 * 8, 8, 8 * 8, 2 * 16 },
		"OK"
	};
};

} /* namespace ui */
