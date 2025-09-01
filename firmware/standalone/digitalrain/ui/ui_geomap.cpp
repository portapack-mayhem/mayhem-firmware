/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 Mark Thompson
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
#include <cstring>
#include <stdio.h>
#include <string_view>

#include "string_format.hpp"
#include "complex.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "file_path.hpp"

namespace ui {
GeoPos::GeoPos(
    const Point pos,
    const alt_unit altitude_unit,
    const spd_unit speed_unit)
    : altitude_unit_(altitude_unit), speed_unit_(speed_unit) {
    set_parent_rect({pos, {screen_width, 3 * 16}});

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
        // Convert degrees/minutes/seconds fields to decimal (floating point) lat/lon degree
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

    const auto wrapped_lat_seconds = [this](int32_t v) {
        field_lat_minutes.on_encoder(v);
    };

    const auto wrapped_lat_minutes = [this](int32_t v) {
        field_lat_degrees.on_encoder((field_lat_degrees.value() >= 0) ? v : -v);
    };

    const auto wrapped_lon_seconds = [this](int32_t v) {
        field_lon_minutes.on_encoder(v);
    };

    const auto wrapped_lon_minutes = [this](int32_t v) {
        field_lon_degrees.on_encoder((field_lon_degrees.value() >= 0) ? v : -v);
    };

    field_lat_seconds.on_wrap = wrapped_lat_seconds;
    field_lat_minutes.on_wrap = wrapped_lat_minutes;
    field_lon_seconds.on_wrap = wrapped_lon_seconds;
    field_lon_minutes.on_wrap = wrapped_lon_minutes;

    text_alt_unit.set(altitude_unit_ ? "m" : "ft");
    if (speed_unit_ == KMPH) text_speed_unit.set("kmph");
    if (speed_unit_ == MPH) text_speed_unit.set("mph");
    if (speed_unit_ == KNOTS) text_speed_unit.set("knots");
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
    field_altitude.set_style(Theme::getInstance()->fg_medium);
    field_speed.set_style(Theme::getInstance()->fg_medium);
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
    has_osm = use_osm = find_osm_file_tile();
}

bool GeoMap::on_encoder(const EncoderEvent delta) {
    // Valid map_zoom values are -2 to -MAX_MAP_ZOOM_OUT, and +1 to +MAX_MAP_ZOOM_IN (values of 0 and -1 are not permitted)
    if (delta > 0) {
        if (map_zoom < MAX_MAP_ZOOM_IN) {
            if (map_zoom == -2) {
                map_zoom = 1;
            } else {
                // zoom in faster after exceeding the map resolution limit
                map_zoom += (map_zoom >= MAP_ZOOM_RESOLUTION_LIMIT) ? map_zoom : 1;
            }
        }
        map_osm_zoom++;
        if (has_osm) set_osm_max_zoom();
    } else if (delta < 0) {
        if (map_zoom > -MAX_MAP_ZOOM_OUT) {
            if (map_zoom == 1) {
                map_zoom = -2;
            } else {
                if (map_zoom > MAP_ZOOM_RESOLUTION_LIMIT)
                    map_zoom /= 2;
                else
                    map_zoom--;
            }
        }
        if (map_osm_zoom > 0) map_osm_zoom--;
    } else {
        return false;
    }

    map_visible = map_opened && (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT);
    if (use_osm) {
        map_visible = true;
        zoom_pixel_offset = 0;
    } else {
        zoom_pixel_offset = (map_visible && (map_zoom > 1)) ? (float)map_zoom / 2 : 0.0f;
    }

    // Trigger map redraw
    redraw_map = true;
    set_dirty();
    return true;
}

void GeoMap::map_read_line_bin(ui::Color* buffer, uint16_t pixels) {
    if (map_zoom == 1) {
        map_file.read(buffer, pixels << 1);
    } else if (map_zoom > 1) {
        map_file.read(buffer, (pixels / map_zoom) << 1);

        // Zoom in: Expand each pixel to "map_zoom" number of pixels.
        // Future TODO:  Add dithering to smooth out the pixelation.
        // As long as MOD(width,map_zoom)==0 then we don't need to check buffer overflow case when stretching last pixel;
        // For 240 width, than means no check is needed for map_zoom values up to 6.
        // (Rectangle height must also divide evenly into map_zoom or we get black lines at end of screen)
        // Note that zooming in results in a map offset of (1/map_zoom) pixels to the right & downward directions (see zoom_pixel_offset).
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
        delete[] zoom_out_buffer;
    }
}

void GeoMap::draw_markers(Painter& painter) {
    for (int i = 0; i < markerListLen; ++i) {
        draw_marker_item(painter, markerList[i], Color::blue(), Color::blue(), Color::magenta());
    }
}

void GeoMap::draw_marker_item(Painter& painter, GeoMarker& item, const Color color, const Color fontColor, const Color backColor) {
    const auto r = screen_rect();
    const ui::Point itemPoint = item_rect_pixel(item);
    if ((itemPoint.x() >= 0) && (itemPoint.x() < r.width()) &&
        (itemPoint.y() > 10) && (itemPoint.y() < r.height()))  // Dont draw within symbol size of top
    {
        draw_marker(painter, {itemPoint.x() + r.left(), itemPoint.y() + r.top()}, item.angle, item.tag, color, fontColor, backColor);
    }
}

// Calculate screen position of item, adjusted for zoom factor.
ui::Point GeoMap::item_rect_pixel(GeoMarker& item) {
    if (!use_osm) {
        const auto r = screen_rect();
        const auto geomap_rect_half_width = r.width() / 2;
        const auto geomap_rect_half_height = r.height() / 2;
        GeoPoint mapPoint = lat_lon_to_map_pixel(item.lat, item.lon);
        float x = mapPoint.x - x_pos;
        float y = mapPoint.y - y_pos;
        if (map_zoom > 1) {
            x = x * map_zoom + zoom_pixel_offset;
            y = y * map_zoom + zoom_pixel_offset;
        } else if (map_zoom < 0) {
            x = x / (-map_zoom);
            y = y / (-map_zoom);
        }
        x += geomap_rect_half_width;
        y += geomap_rect_half_height;
        return {(int16_t)x, (int16_t)y};
    }
    // osm calculation
    double y = lat_to_pixel_y_tile(item.lat, map_osm_zoom) - viewport_top_left_py;
    double x = lon_to_pixel_x_tile(item.lon, map_osm_zoom) - viewport_top_left_px;
    return {(int16_t)x, (int16_t)y};
}

/**
 * @brief Converts longitude to a map tile's X-coordinate.
 * @param lon The longitude in degrees.
 * @param zoom The zoom level.
 * @return The X-coordinate of the tile.
 */
int GeoMap::lon2tile(double lon, int zoom) {
    return (int)floor((lon + 180.0) / 360.0 * pow(2.0, zoom));
}

/**
 * @brief Converts latitude to a map tile's Y-coordinate.
 * @param lat The latitude in degrees.
 * @param zoom The zoom level.
 * @return The Y-coordinate of the tile.
 */
int GeoMap::lat2tile(double lat, int zoom) {
    // Convert latitude from degrees to radians for trigonometric functions
    double lat_rad = lat * M_PI / 180.0;
    // Perform the Mercator projection calculation
    return (int)floor((1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * pow(2.0, zoom));
}

void GeoMap::set_osm_max_zoom() {
    if (map_osm_zoom > 20) map_osm_zoom = 20;
    for (uint8_t i = map_osm_zoom; i > 0; i--) {
        int tile_x = lon2tile(lon_, i);
        int tile_y = lat2tile(lat_, i);
        std::string filename = "/OSM/" + to_string_dec_int(i) + "/" + to_string_dec_int(tile_x) + "/" + to_string_dec_int(tile_y) + ".bmp";
        std::filesystem::path file_path(filename);
        if (file_exists(file_path)) {
            map_osm_zoom = i;
            return;
        }
    }
    map_osm_zoom = 0;  // should not happen
}

// checks if the tile file presents or not. to determine if we got osm or not
uint8_t GeoMap::find_osm_file_tile() {
    std::string filename = "/OSM/" + to_string_dec_int(0) + "/" + to_string_dec_int(0) + "/" + to_string_dec_int(0) + ".bmp";
    std::filesystem::path file_path(filename);
    if (file_exists(file_path)) return 1;
    return 0;  // not found
}

// Converts latitude/longitude to pixel coordinates in binary map file.
// (Note that when map_zoom==1, one pixel in map file corresponds to 1 pixel on screen)
GeoPoint GeoMap::lat_lon_to_map_pixel(float lat, float lon) {
    // Using WGS 84/Pseudo-Mercator projection
    float x = (map_width * (lon + 180) / 360);

    // Latitude calculation based on https://stackoverflow.com/a/10401734/2278659
    double lat_rad = sin(lat * pi / 180);
    float y = (map_height - ((map_world_lon / 2 * log((1 + lat_rad) / (1 - lat_rad))) - map_offset));

    return {x, y};
}

// Draw grid in place of map (when zoom-in level is too high).
void GeoMap::draw_map_grid(ui::Rect r, Painter& painter) {
    // Grid spacing is just based on zoom at the moment, and centered on screen.
    // TODO: Maybe align with latitude/longitude seconds instead?
    int grid_spacing = map_zoom * 2;
    int x = (r.width() / 2) % grid_spacing;
    int y = (r.height() / 2) % grid_spacing;

    if (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT)
        return;

    painter.fill_rectangle({{0, r.top()}, {r.width(), r.height()}}, Theme::getInstance()->bg_darkest->background);

    for (uint16_t line = y; line < r.height(); line += grid_spacing) {
        painter.fill_rectangle({{0, r.top() + line}, {r.width(), 1}}, Theme::getInstance()->bg_darker->background);
    }
    for (uint16_t column = x; column < r.width(); column += grid_spacing) {
        painter.fill_rectangle({{column, r.top()}, {1, r.height()}}, Theme::getInstance()->bg_darker->background);
    }
}

double GeoMap::tile_pixel_x_to_lon(int x, int zoom) {
    double map_width = pow(2.0, zoom) * TILE_SIZE;
    return (x / map_width * 360.0) - 180.0;
}

double GeoMap::tile_pixel_y_to_lat(int y, int zoom) {
    double map_height = pow(2.0, zoom) * TILE_SIZE;
    double n = M_PI * (1.0 - 2.0 * y / map_height);
    return atan(sinh(n)) * 180.0 / M_PI;
}

double GeoMap::lon_to_pixel_x_tile(double lon, int zoom) {
    return ((lon + 180.0) / 360.0) * pow(2.0, zoom) * TILE_SIZE;
}

double GeoMap::lat_to_pixel_y_tile(double lat, int zoom) {
    double lat_rad = lat * M_PI / 180.0;
    double sin_lat = sin(lat_rad);
    return ((1.0 - log((1.0 + sin_lat) / (1.0 - sin_lat)) / (2.0 * M_PI)) / 2.0) * pow(2.0, zoom) * TILE_SIZE;
}

bool GeoMap::draw_osm_file(int zoom, int tile_x, int tile_y, int relative_x, int relative_y, Painter& painter) {
    const ui::Rect r = screen_rect();
    // Early exit if the tile is completely outside the viewport
    if (relative_x >= r.width() || relative_y >= r.height() ||
        relative_x + TILE_SIZE <= 0 || relative_y + TILE_SIZE <= 0) {
        return true;
    }

    BMPFile bmp{};
    bmp.open("/OSM/" + to_string_dec_int(zoom) + "/" + to_string_dec_int(tile_x) + "/" + to_string_dec_int(tile_y) + ".bmp", true);
    // 1. Define the source and destination areas, starting with the full tile.
    int src_x = 0;
    int src_y = 0;
    int dest_x = relative_x;
    int dest_y = relative_y;
    int clip_w = TILE_SIZE;
    int clip_h = TILE_SIZE;
    // 2. Clip left edge
    if (dest_x < 0) {
        src_x = -dest_x;
        clip_w += dest_x;
        dest_x = 0;
    }
    // 3. Clip top edge
    if (dest_y < 0) {
        src_y = -dest_y;
        clip_h += dest_y;
        dest_y = 0;
    }
    // 4. Clip right edge
    if (dest_x + clip_w > r.width()) {
        clip_w = r.width() - dest_x;
    }
    // 5. Clip bottom edge
    if (dest_y + clip_h > r.height()) {
        clip_h = r.height() - dest_y;
    }
    // 6. If clipping resulted in no visible area, we're done.
    if (clip_w <= 0 || clip_h <= 0) {
        return true;
    }

    if (!bmp.is_loaded()) {
        // Draw an error rectangle using the calculated clipped dimensions
        ui::Rect error_rect{{dest_x + r.left(), dest_y + r.top()}, {clip_w, clip_h}};
        painter.fill_rectangle(error_rect, Theme::getInstance()->bg_lightest->background);
        return false;
    }
    std::vector<ui::Color> line(clip_w);
    for (int y = 0; y < clip_h; ++y) {
        int source_row = src_y + y;
        int dest_row = dest_y + y;
        bmp.seek(src_x, source_row);
        for (int x = 0; x < clip_w; ++x) {
            bmp.read_next_px(line[x], true);
        }
        painter.draw_pixels({dest_x + r.left(), dest_row + r.top(), clip_w, 1}, line);
    }
    return true;
}

void GeoMap::paint(Painter& painter) {
    const auto r = screen_rect();
    std::array<ui::Color, geomap_rect_width> map_line_buffer;
    int16_t zoom_seek_x, zoom_seek_y;

    if (!use_osm) {
        // Ony redraw map if it moved by at least 1 pixel or the markers list was updated
        if (map_zoom <= 1) {
            // Zooming out, or no zoom
            const int min_diff = abs(map_zoom);
            if ((int)abs(x_pos - prev_x_pos) >= min_diff)
                redraw_map = true;
            else if ((int)abs(y_pos - prev_y_pos) >= min_diff)
                redraw_map = true;
        } else {
            // Zooming in; magnify position differences (utilizing zoom_pixel_offset)
            if ((int)(abs(x_pos - prev_x_pos) * map_zoom) >= 1)
                redraw_map = true;
            else if ((int)(abs(y_pos - prev_y_pos) * map_zoom) >= 1)
                redraw_map = true;
        }
    } else {
        // using osm; needs to be stricter with the redraws, it'll be checked on move
    }

    if (redraw_map) {
        redraw_map = false;
        if (map_visible) {
            if (!use_osm) {
                prev_x_pos = x_pos;  // Note x_pos/y_pos pixel position in map file now correspond to screen rect CENTER pixel
                prev_y_pos = y_pos;
                // Adjust starting corner position of map per zoom setting;
                // When zooming in the map should technically by shifted left & up by another map_zoom/2 pixels but
                // the map_read_line_bin() function doesn't handle that yet so we're adjusting markers instead (see zoom_pixel_offset).
                if (map_zoom > 1) {
                    zoom_seek_x = x_pos - (float)r.width() / (2 * map_zoom);
                    zoom_seek_y = y_pos - (float)r.height() / (2 * map_zoom);
                } else {
                    zoom_seek_x = x_pos - (r.width() * abs(map_zoom)) / 2;
                    zoom_seek_y = y_pos - (r.height() * abs(map_zoom)) / 2;
                }
                // Read from map file and display to zoomed scale
                int duplicate_lines = (map_zoom < 0) ? 1 : map_zoom;
                for (uint16_t line = 0; line < (r.height() / duplicate_lines); line++) {
                    uint16_t seek_line = zoom_seek_y + ((map_zoom >= 0) ? line : line * (-map_zoom));
                    map_file.seek(4 + ((zoom_seek_x + (map_width * seek_line)) << 1));
                    map_read_line_bin(map_line_buffer.data(), r.width());
                    for (uint16_t j = 0; j < duplicate_lines; j++) {
                        painter.draw_pixels({0, r.top() + (line * duplicate_lines) + j, r.width(), 1}, map_line_buffer);
                    }
                }

            } else {
                // display osm tiles
                //  Convert center GPS to a global pixel coordinate
                double global_center_px = lon_to_pixel_x_tile(lon_, map_osm_zoom);
                double global_center_py = lat_to_pixel_y_tile(lat_, map_osm_zoom);

                // Find the top-left corner of the screen (viewport) in global pixel coordinates
                viewport_top_left_px = global_center_px - (r.width() / 2.0);
                viewport_top_left_py = global_center_py - (r.height() / 2.0);

                // Find the tile ID that contains the top-left corner of the viewport
                int start_tile_x = floor(viewport_top_left_px / TILE_SIZE);
                int start_tile_y = floor(viewport_top_left_py / TILE_SIZE);

                // Calculate the crucial render offset.
                // This determines how much the first tile is shifted to align the map correctly.
                // This value will almost always be negative or zero.
                double render_offset_x = -(viewport_top_left_px - (start_tile_x * TILE_SIZE));
                double render_offset_y = -(viewport_top_left_py - (start_tile_y * TILE_SIZE));

                // Determine how many tiles we need to draw to fill the screen
                int tiles_needed_x = (r.width() / TILE_SIZE) + 2;
                int tiles_needed_y = (r.height() / TILE_SIZE) + 2;

                for (int y = 0; y < tiles_needed_y; ++y) {
                    for (int x = 0; x < tiles_needed_x; ++x) {
                        int current_tile_x = start_tile_x + x;
                        int current_tile_y = start_tile_y + y;

                        // Calculate the final on-screen drawing position for this tile.
                        // For the first tile (x=0, y=0), this will be the negative offset.
                        int draw_pos_x = round(render_offset_x + x * TILE_SIZE);
                        int draw_pos_y = round(render_offset_y + y * TILE_SIZE);
                        if (!draw_osm_file(map_osm_zoom, current_tile_x, current_tile_y, draw_pos_x, draw_pos_y, painter)) {
                            // already blanked it.
                        }
                    }
                }
            }

        } else {
            // No map data or excessive zoom; just draw a grid
            draw_map_grid(r, painter);
        }
        // Draw crosshairs in center in manual panning mode
        if (manual_panning_) {
            painter.fill_rectangle({r.center() - Point(16, 1) + Point(zoom_pixel_offset, zoom_pixel_offset), {32, 2}}, Color::red());
            painter.fill_rectangle({r.center() - Point(1, 16) + Point(zoom_pixel_offset, zoom_pixel_offset), {2, 32}}, Color::red());
        }

        // Draw the other markers
        draw_markers(painter);
        if (!use_osm) draw_scale(painter);
        draw_mypos(painter);
        if (has_osm) draw_switcher(painter);
        set_clean();
    }

    // Draw the marker in the center
    if (!manual_panning_ && !hide_center_marker_) {
        draw_marker(painter, r.center() + Point(zoom_pixel_offset, zoom_pixel_offset), angle_, tag_, Color::red(), Color::white(), Color::black());
    }
}

void GeoMap::draw_switcher(Painter& painter) {
    painter.fill_rectangle({screen_rect().left(), screen_rect().top(), 3 * 20, 20}, Theme::getInstance()->bg_darker->background);
    std::string_view txt = (use_osm) ? "B I N" : "O S M";
    painter.draw_string({screen_rect().left() + 5, screen_rect().top() + 2}, *Theme::getInstance()->fg_light, txt);
}

bool GeoMap::on_keyboard(KeyboardEvent key) {
    if (key == '+' || key == ' ') return on_encoder(1);
    if (key == '-') return on_encoder(-1);

    return false;
}

bool GeoMap::on_touch(const TouchEvent event) {
    if (has_osm && event.type == TouchEvent::Type::Start && event.point.x() < screen_rect().left() + 3 * 20 && event.point.y() < screen_rect().top() + 20) {
        use_osm = !use_osm;
        move(lon_, lat_);  // to re calculate the center for each map type
        if (use_osm) set_osm_max_zoom();
        redraw_map = true;
        set_dirty();
        return false;  // false, because with true this hits 2 times
    }

    if ((event.type == TouchEvent::Type::Start) && (mode_ == PROMPT)) {
        Point p;
        set_highlighted(true);
        if (on_move) {
            if (!use_osm) {
                p = event.point - screen_rect().center();
                on_move(p.x() / 2.0 * lon_ratio, p.y() / 2.0 * lat_ratio, false);
            } else {
                p = event.point - screen_rect().location();
                on_move(tile_pixel_x_to_lon(p.x() + viewport_top_left_px, map_osm_zoom), tile_pixel_y_to_lat(p.y() + viewport_top_left_py, map_osm_zoom), true);
            }
            return true;
        }
    }
    return false;
}

void GeoMap::move(const float lon, const float lat) {
    const auto r = screen_rect();
    bool is_changed = (lon_ != lon || lat_ != lat);
    lon_ = lon;
    lat_ = lat;
    if (!use_osm) {
        // Calculate x_pos/y_pos in map file corresponding to CENTER pixel of screen rect
        // (Note there is a 1:1 correspondence between map file pixels and screen pixels when map_zoom=1)
        GeoPoint mapPoint = lat_lon_to_map_pixel(lat_, lon_);
        x_pos = mapPoint.x;
        y_pos = mapPoint.y;
        // Cap position
        if (x_pos > (map_width - r.width() / 2))
            x_pos = map_width - r.width() / 2;
        if (y_pos > (map_height + r.height() / 2))
            y_pos = map_height - r.height() / 2;

        // Scale calculation
        float km_per_deg_lon = cos(lat * pi / 180) * 111.321;  // 111.321 km/deg longitude at equator, and 0 km at poles
        pixels_per_km = (r.width() / 2) / km_per_deg_lon;
    } else {
        if (is_changed) {
            set_osm_max_zoom();
            redraw_map = true;
        }
    }
}

bool GeoMap::init() {
    auto result = map_file.open(adsb_dir / u"world_map.bin");
    map_opened = !result.is_valid();

    if (map_opened) {
        map_file.read(&map_width, 2);
        map_file.read(&map_height, 2);
    } else {
        map_width = 32768;
        map_height = 32768;
    }

    map_visible = map_opened || has_osm;
    map_center_x = map_width >> 1;
    map_center_y = map_height >> 1;

    lon_ratio = 180.0 / map_center_x;
    lat_ratio = -90.0 / map_center_y;

    map_bottom = sin(-85.05 * pi / 180);  // Map bitmap only goes from about -85 to 85 lat
    map_world_lon = map_width / (2 * pi);
    map_offset = (map_world_lon / 2 * log((1 + map_bottom) / (1 - map_bottom)));
    return map_opened;
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
    const auto r = screen_rect();
    uint32_t m = 800000;
    uint32_t scale_width = (map_zoom > 0) ? m * map_zoom * pixels_per_km : m * pixels_per_km / (-map_zoom);
    ui::Color scale_color = (map_visible) ? Color::black() : Color::white();
    std::string km_string;

    while (scale_width > (uint32_t)r.width() * (1000 / 2)) {
        scale_width /= 2;
        m /= 2;
    }
    scale_width /= 1000;
    if (m < 1000) {
        km_string = to_string_dec_uint(m) + "m";
    } else {
        m += 50;  // (add rounding factor for div by 100 below)
        uint32_t km = m / 1000;
        m -= km * 1000;
        if (m == 0) {
            km_string = to_string_dec_uint(km) + " km";
        } else {
            km_string = to_string_dec_uint(km) + "." + to_string_dec_uint(m / 100, 1) + "km";
        }
    }

    painter.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 4}, {(uint16_t)scale_width, 2}}, scale_color);
    painter.fill_rectangle({{r.right() - 5, r.bottom() - 8}, {2, 6}}, scale_color);
    painter.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 8}, {2, 6}}, scale_color);
    std::string_view sw = km_string;
    ui::Point pos = {(uint16_t)(r.right() - 25 - scale_width - sw.length() * 5 / 2), r.bottom() - 10};
    painter.draw_string(pos, *Theme::getInstance()->fg_light, sw);
}

void GeoMap::draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color, Painter& painter) {
    Point arrow_a, arrow_b, arrow_c;

    for (size_t thickness = 0; thickness < 3; thickness++) {
        arrow_a = fast_polar_to_point((int)angle, size) + origin;
        arrow_b = fast_polar_to_point((int)(angle + 180 - 35), size) + origin;
        arrow_c = fast_polar_to_point((int)(angle + 180 + 35), size) + origin;

        painter.draw_line(arrow_a, arrow_b, color);
        painter.draw_line(arrow_b, arrow_c, color);
        painter.draw_line(arrow_c, arrow_a, color);

        size--;
    }

    painter.draw_pixel(origin, color);  // 1 pixel indicating center pivot point of bearing symbol
}

void GeoMap::draw_marker(Painter& painter, const ui::Point itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color, const Color fontColor, const Color backColor) {
    const auto r = screen_rect();

    int tagOffset = 10;
    if (mode_ == PROMPT) {
        // Cross
        painter.fill_rectangle({itemPoint - Point(16, 1), {32, 2}}, color);
        painter.fill_rectangle({itemPoint - Point(1, 16), {2, 32}}, color);
        tagOffset = 16;
    } else if (angle_ < 360) {
        // if we have a valid angle draw bearing
        draw_bearing(itemPoint, itemAngle, 10, color, painter);
        tagOffset = 10;
    } else {
        // draw a small cross
        painter.fill_rectangle({itemPoint - Point(8, 1), {16, 2}}, color);
        painter.fill_rectangle({itemPoint - Point(1, 8), {2, 16}}, color);
        tagOffset = 8;
    }
    // center tag above point
    if ((itemPoint.y() - r.top() >= 32) && (itemTag.find_first_not_of(' ') != itemTag.npos)) {  // only draw tag if doesn't overlap top & not just spaces
        painter.draw_string(itemPoint - Point(((int)itemTag.length() * 8 / 2), 14 + tagOffset),
                            style().font, fontColor, backColor, itemTag);
    }
}

void GeoMap::draw_mypos(Painter& painter) {
    if ((my_pos.lat < INVALID_LAT_LON) && (my_pos.lon < INVALID_LAT_LON))
        draw_marker_item(painter, my_pos, Color::yellow());
}

void GeoMap::clear_markers() {
    markerListLen = 0;
}

MapMarkerStored GeoMap::store_marker(GeoMarker& marker) {
    const auto r = screen_rect();
    MapMarkerStored ret;

    // Check if it could be on screen
    // (Shows more distant planes when zoomed out)
    GeoPoint mapPoint = lat_lon_to_map_pixel(marker.lat, marker.lon);
    int x_dist = abs((int)mapPoint.x - (int)x_pos);
    int y_dist = abs((int)mapPoint.y - (int)y_pos);
    int zoom_out = (map_zoom < 0) ? -map_zoom : 1;

    if ((x_dist >= (zoom_out * r.width() / 2)) || (y_dist >= (zoom_out * r.height() / 2))) {
        ret = MARKER_NOT_STORED;
    } else if (markerListLen < NumMarkerListElements) {
        markerList[markerListLen] = marker;
        markerListLen++;
        redraw_map = true;
        ret = MARKER_STORED;
    } else {
        ret = MARKER_LIST_FULL;
    }
    return ret;
}

void GeoMap::update_my_position(float lat, float lon, int32_t altitude) {
    bool is_changed = (my_pos.lat != lat) || (my_pos.lon != lon);
    my_pos.lat = lat;
    my_pos.lon = lon;
    my_altitude = altitude;
    redraw_map = is_changed;
    set_dirty();
}

void GeoMap::update_my_orientation(uint16_t angle, bool refresh) {
    bool is_changed = (my_pos.angle != angle);
    my_pos.angle = angle;
    if (refresh && is_changed) {
        redraw_map = true;
        set_dirty();
    }
}

MapType GeoMap::get_map_type() {
    return use_osm ? MAP_TYPE_OSM : MAP_TYPE_BIN;
}

void GeoMapView::focus() {
    geopos.focus();
    if (!geomap.map_file_opened()) {
        // nav_.display_modal("No map", "No world_map.bin file in\n/" + adsb_dir.string() + "/ directory", ABORT);
        // TODO crate an error display
    }
}

void GeoMapView::update_my_position(float lat, float lon, int32_t altitude) {
    geomap.update_my_position(lat, lon, altitude);
}
void GeoMapView::update_my_orientation(uint16_t angle, bool refresh) {
    geomap.update_my_orientation(angle, refresh);
}

void GeoMapView::update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed) {
    if (geomap.manual_panning()) {
        geomap.set_dirty();
        return;
    }
    bool is_changed = lat_ != lat || lon_ != lon || altitude_ != altitude || speed_ != speed || angle_ != angle;
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
    if (is_changed) geomap.move(lon_, lat_);
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
        bool is_changed = (altitude_ != altitude) || (lat_ != lat) || (lon_ != lon) || (speed_ != speed);
        altitude_ = altitude;
        lat_ = lat;
        lon_ = lon;
        speed_ = speed;
        geopos.hide_altandspeed();
        geomap.set_manual_panning(true);
        if (is_changed) geomap.move(lon_, lat_);
        geomap.set_dirty();
    };

    geomap.on_move = [this](float move_x, float move_y, bool absolute) {
        if (absolute) {
            lon_ = move_x;
            lat_ = move_y;
        } else {
            lon_ += move_x;
            lat_ += move_y;
        }

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
    const std::string& tag,
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    uint16_t angle,
    const std::function<void(void)> on_close)
    : altitude_(altitude),
      altitude_unit_(altitude_unit),
      speed_unit_(speed_unit),
      lat_(lat),
      lon_(lon),
      angle_(angle),
      on_close_(on_close) {
    mode_ = DISPLAY;

    add_child(&geopos);

    geomap.init();

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
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    const std::function<void(int32_t, float, float, int32_t)> on_done)
    : altitude_(altitude),
      altitude_unit_(altitude_unit),
      speed_unit_(speed_unit),
      lat_(lat),
      lon_(lon) {
    mode_ = PROMPT;

    add_child(&geopos);

    geomap.init();

    setup();
    add_child(&button_ok);

    geomap.set_mode(mode_);
    geomap.move(lon_, lat_);
    geomap.set_focusable(true);

    button_ok.on_select = [this, on_done](Button&) {
        if (on_done)
            on_done(altitude_, lat_, lon_, speed_);
        // exit handled on caller side
    };
}

void GeoMapView::clear_markers() {
    geomap.clear_markers();
}

MapMarkerStored GeoMapView::store_marker(GeoMarker& marker) {
    return geomap.store_marker(marker);
}

MapType GeoMapView::get_map_type() {
    return geomap.get_map_type();
}

} /* namespace ui */
