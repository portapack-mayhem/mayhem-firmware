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

#include "portapack.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

#include "string_format.hpp"
#include "complex.hpp"
#include "ui_styles.hpp"
#include "ui_font_fixed_5x8.hpp"

namespace ui {

GeoPos::GeoPos(
    const Point pos,
    const alt_unit altitude_unit,
    const spd_unit speed_unit)
    : altitude_unit_(altitude_unit), speed_unit_(speed_unit) {
    set_parent_rect({pos, {30 * 8, 3 * 16}});

    add_children({&labels_position,
                  &label_spd_position,
                  &field_altitude,
                  &field_speed,
                  &text_alt_unit,
                  &text_speed_unit,
                  &field_lat_degrees,
                  &field_lat_minutes,
                  &field_lat_seconds,
                  &text_lat_decimal,
                  &field_lon_degrees,
                  &field_lon_minutes,
                  &field_lon_seconds,
                  &text_lon_decimal});

    // Defaults
    set_altitude(0);
    set_speed(0);
    set_lat(0);
    set_lon(0);

    const auto changed_fn = [this](int32_t) {
        float lat_value = lat();
        float lon_value = lon();

        text_lat_decimal.set(to_string_decimal(lat_value, 5));
        text_lon_decimal.set(to_string_decimal(lon_value, 5));

        if (on_change && report_change)
            on_change(altitude(), lat_value, lon_value, speed());
    };

    field_altitude.on_change = changed_fn;
    field_speed.on_change = changed_fn;
    field_lat_degrees.on_change = changed_fn;
    field_lat_minutes.on_change = changed_fn;
    field_lat_seconds.on_change = changed_fn;
    field_lon_degrees.on_change = changed_fn;
    field_lon_minutes.on_change = changed_fn;
    field_lon_seconds.on_change = changed_fn;

    text_alt_unit.set(altitude_unit_ ? "m" : "ft");
    if (speed_unit_ == KMPH) text_speed_unit.set("kmph");
    if (speed_unit_ == MPH) text_speed_unit.set("mph");
    if (speed_unit_ == HIDDEN) {
        text_speed_unit.hidden(true);
        label_spd_position.hidden(true);
        field_speed.hidden(true);
    }
}

void GeoPos::set_read_only(bool v) {
    // only setting altitude to read-only (allow manual panning via lat/lon fields)
    field_altitude.set_focusable(!v);
    field_speed.set_focusable(!v);
}

// Stupid hack to avoid an event loop
void GeoPos::set_report_change(bool v) {
    report_change = v;
}

void GeoPos::focus() {
    if (field_altitude.focusable())
        field_altitude.focus();
    else
        field_lat_degrees.focus();
}

void GeoPos::hide_altandspeed() {
    // Color altitude grey to indicate it's not updated in manual panning mode
    field_altitude.set_style(&Styles::grey);
    field_speed.set_style(&Styles::grey);
}

void GeoPos::set_altitude(int32_t altitude) {
    field_altitude.set_value(altitude);
}
void GeoPos::set_speed(int32_t speed) {
    field_speed.set_value(speed);
}

void GeoPos::set_lat(float lat) {
    field_lat_degrees.set_value(lat);
    field_lat_minutes.set_value((uint32_t)abs(lat / (1.0 / 60)) % 60);
    field_lat_seconds.set_value((uint32_t)abs(lat / (1.0 / 3600)) % 60);
}

void GeoPos::set_lon(float lon) {
    field_lon_degrees.set_value(lon);
    field_lon_minutes.set_value((uint32_t)abs(lon / (1.0 / 60)) % 60);
    field_lon_seconds.set_value((uint32_t)abs(lon / (1.0 / 3600)) % 60);
}

float GeoPos::lat() {
    if (field_lat_degrees.value() < 0) {
        return -1 * (-1 * field_lat_degrees.value() + (field_lat_minutes.value() / 60.0) + (field_lat_seconds.value() / 3600.0));
    } else {
        return field_lat_degrees.value() + (field_lat_minutes.value() / 60.0) + (field_lat_seconds.value() / 3600.0);
    }
};

float GeoPos::lon() {
    if (field_lon_degrees.value() < 0) {
        return -1 * (-1 * field_lon_degrees.value() + (field_lon_minutes.value() / 60.0) + (field_lon_seconds.value() / 3600.0));
    } else {
        return field_lon_degrees.value() + (field_lon_minutes.value() / 60.0) + (field_lon_seconds.value() / 3600.0);
    }
};

int32_t GeoPos::altitude() {
    return field_altitude.value();
};

int32_t GeoPos::speed() {
    return field_speed.value();
};

GeoMap::GeoMap(
    Rect parent_rect)
    : Widget{parent_rect}, markerListLen(0) {
}

bool GeoMap::on_encoder(const EncoderEvent delta) {
    // Valid map_zoom values are -2 to -MAX_MAP_ZOOM_OUT, and +1 to +MAX_MAP_ZOOM_IN (values of 0 and -1 are not permitted)
    if (delta > 0) {
        if (map_zoom < MAX_MAP_ZOOM_IN) {
            if (map_zoom == -2) {
                map_zoom = 1;
            } else {
                map_zoom++;

                // When zooming in, ensure that MOD(240,map_zoom)==0 for the map_zoom_line() function
                if ((map_zoom > 1) && (geomap_rect_width % map_zoom != 0)) {
                    map_zoom--;
                }
            }
        }
    } else if (delta < 0) {
        if (map_zoom > -MAX_MAP_ZOOM_OUT) {
            if (map_zoom == 1) {
                map_zoom = -2;
            } else {
                map_zoom--;
            }
        }
    } else {
        return false;
    }

    // Trigger map redraw
    markerListUpdated = true;
    set_dirty();
    return true;
}

void GeoMap::map_read_line(ui::Color* buffer, uint16_t pixels) {
    if (map_zoom == 1) {
        map_file.read(buffer, pixels << 1);
    } else if (map_zoom > 1) {
        map_file.read(buffer, (pixels / map_zoom) << 1);

        // Zoom in: Expand each pixel to "map_zoom" number of pixels.
        // Future TODO:  Add dithering to smooth out the pixelation.
        // As long as MOD(width,map_zoom)==0 then we don't need to check buffer overflow case when stretching last pixel;
        // For 240 width, than means no check is needed for map_zoom values up to 6
        for (int i = (geomap_rect_width / map_zoom) - 1; i >= 0; i--) {
            for (int j = 0; j < map_zoom; j++) {
                buffer[(i * map_zoom) + j] = buffer[i];
            }
        }
    } else {
        ui::Color* zoom_out_buffer = new ui::Color[(pixels * (-map_zoom))];
        map_file.read(zoom_out_buffer, (pixels * (-map_zoom)) << 1);

        // Zoom out:  Collapse each group of "-map_zoom" pixels into one pixel.
        // Future TODO: Average each group of pixels (in both X & Y directions if possible).
        for (int i = 0; i < geomap_rect_width; i++) {
            buffer[i] = zoom_out_buffer[i * (-map_zoom)];
        }
        delete zoom_out_buffer;
    }
}

void GeoMap::draw_markers(Painter& painter) {
    const auto r = screen_rect();

    for (int i = 0; i < markerListLen; ++i) {
        GeoMarker& item = markerList[i];
        double lat_rad = sin(item.lat * pi / 180);
        int x = (map_width * (item.lon + 180) / 360) - x_pos;
        int y = (map_height - ((map_world_lon / 2 * log((1 + lat_rad) / (1 - lat_rad))) - map_offset)) - y_pos;  // Offset added for the GUI

        if (map_zoom > 1) {
            x = ((x - (r.width() / 2)) * map_zoom) + (r.width() / 2);
            y = ((y - (r.height() / 2)) * map_zoom) + (r.height() / 2);
        } else if (map_zoom < 0) {
            x = ((x - (r.width() / 2)) / (-map_zoom)) + (r.width() / 2);
            y = ((y - (r.height() / 2)) / (-map_zoom)) + (r.height() / 2);
        }

        if ((x >= 0) && (x < r.width()) &&
            (y > 10) && (y < r.height()))  // Dont draw within symbol size of top
        {
            ui::Point itemPoint(x, y + r.top());
            if (y >= 32) {  // Dont draw text if it would overlap top
                // Text and symbol
                draw_marker(painter, itemPoint, item.angle, item.tag, Color::blue(), Color::blue(), Color::magenta());
            } else {
                // Only symbol
                draw_bearing(itemPoint, item.angle, 10, Color::blue());
            }
        }
    }
}

void GeoMap::paint(Painter& painter) {
    const auto r = screen_rect();
    std::array<ui::Color, 240> map_line_buffer;

    // Ony redraw map if it moved by at least 1 pixel
    // or the markers list was updated
    int x_diff = abs(x_pos - prev_x_pos);
    int y_diff = abs(y_pos - prev_y_pos);

    if (markerListUpdated || (x_diff >= 3) || (y_diff >= 3)) {
        int32_t zoom_seek_x = x_pos;
        int32_t zoom_seek_y = y_pos;

        // Adjust starting corner position of map per zoom setting
        if (map_zoom > 1) {
            zoom_seek_x += (r.width() - (r.width() / map_zoom)) / 2;
            zoom_seek_y += (r.height() - (r.height() / map_zoom)) / 2;
        } else if (map_zoom < 0) {
            zoom_seek_x += (r.width() - (r.width() * (-map_zoom))) / 2;
            zoom_seek_y += (r.height() - (r.height() * (-map_zoom))) / 2;
        }

        // Read from map file and display to zoomed scale
        int duplicate_lines = (map_zoom < 0) ? 1 : map_zoom;
        for (uint16_t line = 0; line < (r.height() / duplicate_lines); line++) {
            uint16_t seek_line = zoom_seek_y + ((map_zoom >= 0) ? line : line * (-map_zoom));
            map_file.seek(4 + ((zoom_seek_x + (map_width * seek_line)) << 1));
            map_read_line(map_line_buffer.data(), r.width());

            for (uint16_t j = 0; j < duplicate_lines; j++) {
                display.draw_pixels({0, r.top() + (line * duplicate_lines) + j, r.width(), 1}, map_line_buffer);
            }
        }

        prev_x_pos = x_pos;
        prev_y_pos = y_pos;

        // Draw crosshairs in center in manual panning mode
        if (manual_panning_) {
            display.fill_rectangle({r.center() - Point(16, 1), {32, 2}}, Color::red());
            display.fill_rectangle({r.center() - Point(1, 16), {2, 32}}, Color::red());
        }

        // Draw the other markers
        draw_markers(painter);
        draw_scale(painter);
        markerListUpdated = false;
        set_clean();
    }

    // Draw the marker in the center
    if (!manual_panning_) {
        draw_marker(painter, r.center(), angle_, tag_, Color::red(), Color::white(), Color::black());
    }
}

bool GeoMap::on_keyboard(KeyboardEvent key) {
    if (key == '+' || key == ' ') return on_encoder(1);
    if (key == '-') return on_encoder(-1);

    return false;
}

bool GeoMap::on_touch(const TouchEvent event) {
    if ((event.type == TouchEvent::Type::Start) && (mode_ == PROMPT)) {
        set_highlighted(true);
        if (on_move) {
            Point p = event.point - screen_rect().center();
            on_move(p.x() / 2.0 * lon_ratio, p.y() / 2.0 * lat_ratio);
            return true;
        }
    }
    return false;
}

void GeoMap::move(const float lon, const float lat) {
    lon_ = lon;
    lat_ = lat;

    // Using WGS 84/Pseudo-Mercator projection
    x_pos = map_width * (lon_ + 180) / 360 - (geomap_rect_width / 2);

    // Latitude calculation based on https://stackoverflow.com/a/10401734/2278659
    double lat_rad = sin(lat * pi / 180);
    y_pos = map_height - ((map_world_lon / 2 * log((1 + lat_rad) / (1 - lat_rad))) - map_offset) - 128;  // Offset added for the GUI

    // Cap position
    if (x_pos > (map_width - geomap_rect_width))
        x_pos = map_width - geomap_rect_width;
    if (y_pos > (map_height + geomap_rect_height))
        y_pos = map_height - geomap_rect_height;

    // Scale calculation
    float km_per_deg_lon = cos(lat * pi / 180) * 111.321;  // 111.321 km/deg longitude at equator, and 0 km at poles
    pixels_per_km = (geomap_rect_width / 2) / km_per_deg_lon;
}

bool GeoMap::init() {
    auto result = map_file.open("ADSB/world_map.bin");
    if (result.is_valid())
        return false;

    map_file.read(&map_width, 2);
    map_file.read(&map_height, 2);

    map_center_x = map_width >> 1;
    map_center_y = map_height >> 1;

    lon_ratio = 180.0 / map_center_x;
    lat_ratio = -90.0 / map_center_y;

    map_bottom = sin(-85.05 * pi / 180);  // Map bitmap only goes from about -85 to 85 lat
    map_world_lon = map_width / (2 * pi);
    map_offset = (map_world_lon / 2 * log((1 + map_bottom) / (1 - map_bottom)));

    return true;
}

void GeoMap::set_mode(GeoMapMode mode) {
    mode_ = mode;
}

void GeoMap::set_manual_panning(bool v) {
    manual_panning_ = v;
}

bool GeoMap::manual_panning() {
    return manual_panning_;
}

void GeoMap::draw_scale(Painter& painter) {
    uint16_t km = 800;
    uint16_t scale_width = (map_zoom > 0) ? km * pixels_per_km * map_zoom : km * pixels_per_km / (-map_zoom);

    while (scale_width > screen_width / 2) {
        scale_width /= 2;
        km /= 2;
    }

    std::string km_string = to_string_dec_uint(km) + " km";

    display.fill_rectangle({{screen_width - 5 - scale_width, screen_height - 4}, {scale_width, 2}}, Color::black());
    display.fill_rectangle({{screen_width - 5, screen_height - 8}, {2, 6}}, Color::black());
    display.fill_rectangle({{screen_width - 5 - scale_width, screen_height - 8}, {2, 6}}, Color::black());

    painter.draw_string({(uint16_t)(screen_width - 25 - scale_width - km_string.length() * 5 / 2), screen_height - 10}, ui::font::fixed_5x8, Color::black(), Color::white(), km_string);
}

void GeoMap::draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color) {
    Point arrow_a, arrow_b, arrow_c;

    for (size_t thickness = 0; thickness < 3; thickness++) {
        arrow_a = fast_polar_to_point((int)angle, size) + origin;
        arrow_b = fast_polar_to_point((int)(angle + 180 - 35), size) + origin;
        arrow_c = fast_polar_to_point((int)(angle + 180 + 35), size) + origin;

        display.draw_line(arrow_a, arrow_b, color);
        display.draw_line(arrow_b, arrow_c, color);
        display.draw_line(arrow_c, arrow_a, color);

        size--;
    }
}

void GeoMap::draw_marker(Painter& painter, const ui::Point itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color, const Color fontColor, const Color backColor) {
    int tagOffset = 10;
    if (mode_ == PROMPT) {
        // Cross
        display.fill_rectangle({itemPoint - Point(16, 1), {32, 2}}, color);
        display.fill_rectangle({itemPoint - Point(1, 16), {2, 32}}, color);
        tagOffset = 16;
    } else if (angle_ < 360) {
        // if we have a valid angle draw bearing
        draw_bearing(itemPoint, itemAngle, 10, color);
        tagOffset = 10;
    } else {
        // draw a small cross
        display.fill_rectangle({itemPoint - Point(8, 1), {16, 2}}, color);
        display.fill_rectangle({itemPoint - Point(1, 8), {2, 16}}, color);
        tagOffset = 8;
    }
    // center tag above point
    if (itemTag.find_first_not_of(' ') != itemTag.npos) {  // only draw tag if we have something other than spaces
        painter.draw_string(itemPoint - Point(((int)itemTag.length() * 8 / 2), 14 + tagOffset),
                            style().font, fontColor, backColor, itemTag);
    }
}

void GeoMap::clear_markers() {
    markerListLen = 0;
}

MapMarkerStored GeoMap::store_marker(GeoMarker& marker) {
    MapMarkerStored ret;

    // Check if it could be on screen
    // Only checking one direction to reduce CPU
    double lat_rad = sin(marker.lat * pi / 180);
    int x = (map_width * (marker.lon + 180) / 360) - x_pos;
    int y = (map_height - ((map_world_lon / 2 * log((1 + lat_rad) / (1 - lat_rad))) - map_offset)) - y_pos;  // Offset added for the GUI
    if (false == ((x >= 0) && (x < geomap_rect_width) && (y > 10) && (y < geomap_rect_height)))              // Dont draw within symbol size of top
    {
        ret = MARKER_NOT_STORED;
    } else if (markerListLen < NumMarkerListElements) {
        markerList[markerListLen] = marker;
        markerListLen++;
        markerListUpdated = true;
        ret = MARKER_STORED;
    } else {
        ret = MARKER_LIST_FULL;
    }
    return ret;
}

void GeoMapView::focus() {
    geopos.focus();

    if (!map_opened)
        nav_.display_modal("No map", "No world_map.bin file in\n/ADSB/ directory", ABORT);
}

void GeoMapView::update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed) {
    if (geomap.manual_panning()) {
        geomap.set_dirty();
        return;
    }

    lat_ = lat;
    lon_ = lon;
    altitude_ = altitude;
    speed_ = speed;

    // Stupid hack to avoid an event loop
    geopos.set_report_change(false);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);
    geopos.set_altitude(altitude_);
    geopos.set_speed(speed_);
    geopos.set_report_change(true);

    geomap.set_angle(angle);
    geomap.move(lon_, lat_);
    geomap.set_dirty();
}

void GeoMapView::update_tag(const std::string tag) {
    geomap.set_tag(tag);
}

void GeoMapView::setup() {
    add_child(&geomap);

    geopos.set_altitude(altitude_);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);

    geopos.on_change = [this](int32_t altitude, float lat, float lon, int32_t speed) {
        altitude_ = altitude;
        lat_ = lat;
        lon_ = lon;
        speed_ = speed;
        geopos.hide_altandspeed();
        geomap.set_manual_panning(true);
        geomap.move(lon_, lat_);
        geomap.set_dirty();
    };

    geomap.on_move = [this](float move_x, float move_y) {
        lon_ += move_x;
        lat_ += move_y;

        // Stupid hack to avoid an event loop
        geopos.set_report_change(false);
        geopos.set_lon(lon_);
        geopos.set_lat(lat_);
        geopos.set_report_change(true);

        geomap.move(lon_, lat_);
        geomap.set_dirty();
    };
}

GeoMapView::~GeoMapView() {
    if (on_close_)
        on_close_();
}

// Display mode
GeoMapView::GeoMapView(
    NavigationView& nav,
    const std::string& tag,
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    uint16_t angle,
    const std::function<void(void)> on_close)
    : nav_(nav),
      altitude_(altitude),
      altitude_unit_(altitude_unit),
      speed_unit_(speed_unit),
      lat_(lat),
      lon_(lon),
      angle_(angle),
      on_close_(on_close) {
    mode_ = DISPLAY;

    add_child(&geopos);

    map_opened = geomap.init();
    if (!map_opened) return;

    setup();

    geomap.set_mode(mode_);
    geomap.set_tag(tag);
    geomap.set_angle(angle);
    geomap.move(lon_, lat_);
    geomap.set_focusable(true);

    geopos.set_read_only(true);
}

// Prompt mode
GeoMapView::GeoMapView(
    NavigationView& nav,
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    const std::function<void(int32_t, float, float, int32_t)> on_done)
    : nav_(nav),
      altitude_(altitude),
      altitude_unit_(altitude_unit),
      speed_unit_(speed_unit),
      lat_(lat),
      lon_(lon) {
    mode_ = PROMPT;

    add_child(&geopos);

    map_opened = geomap.init();
    if (!map_opened) return;

    setup();
    add_child(&button_ok);

    geomap.set_mode(mode_);
    geomap.move(lon_, lat_);
    geomap.set_focusable(true);

    button_ok.on_select = [this, on_done, &nav](Button&) {
        if (on_done)
            on_done(altitude_, lat_, lon_, speed_);
        nav.pop();
    };
}

void GeoMapView::clear_markers() {
    geomap.clear_markers();
}

MapMarkerStored GeoMapView::store_marker(GeoMarker& marker) {
    return geomap.store_marker(marker);
}

} /* namespace ui */
