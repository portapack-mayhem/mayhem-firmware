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

#ifndef __GEOMAP_H__
#define __GEOMAP_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "file.hpp"
#include "bmpfile.hpp"
#include "mathdef.hpp"
#include <string_view>

namespace ui {

#define MAX_MAP_ZOOM_IN 4000
#define MAX_MAP_ZOOM_OUT 10
#define MAP_ZOOM_RESOLUTION_LIMIT 5  // Max zoom-in to show map; rect height & width must divide into this evenly

#define INVALID_LAT_LON 200
#define INVALID_ANGLE 400

#define GEOMAP_BANNER_HEIGHT (3 * 16)
#define GEOMAP_RECT_WIDTH 240
#define GEOMAP_RECT_HEIGHT (320 - 16 - GEOMAP_BANNER_HEIGHT)

#define TILE_SIZE 256

enum GeoMapMode {
    DISPLAY,
    PROMPT
};

struct GeoPoint {
   public:
    float x{0};
    float y{0};
};

struct GeoMarker {
   public:
    float lat{0};
    float lon{0};
    uint16_t angle{0};
    std::string tag{""};

    GeoMarker& operator=(GeoMarker& rhs) {
        lat = rhs.lat;
        lon = rhs.lon;
        angle = rhs.angle;
        tag = rhs.tag;

        return *this;
    }
};

class GeoPos : public View {
   public:
    enum alt_unit {
        FEET = 0,
        METERS
    };
    enum spd_unit {
        NONE = 0,
        MPH,
        KMPH,
        KNOTS,
        HIDDEN = 255
    };

    std::function<void(int32_t, float, float, int32_t)> on_change{};

    GeoPos(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit);

    void focus() override;

    void set_read_only(bool v);
    void set_altitude(int32_t altitude);
    void set_speed(int32_t speed);
    void set_lat(float lat);
    void set_lon(float lon);
    int32_t altitude();
    int32_t speed();
    void hide_altandspeed();
    float lat();
    float lon();

    void set_report_change(bool v);

   private:
    bool read_only{false};
    bool report_change{true};
    alt_unit altitude_unit_{};
    spd_unit speed_unit_{};

    Labels labels_position{
        {{1 * 8, 0 * 16}, "Alt:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 1 * 16}, "Lat:    \xB0  '  \"", Theme::getInstance()->fg_light->foreground},  // 0xB0 is degree ° symbol in our 8x16 font
        {{1 * 8, 2 * 16}, "Lon:    \xB0  '  \"", Theme::getInstance()->fg_light->foreground},
    };
    Labels label_spd_position{
        {{15 * 8, 0 * 16}, "Spd:", Theme::getInstance()->fg_light->foreground},
    };
    NumberField field_altitude{
        {6 * 8, 0 * 16},
        5,
        {-1000, 50000},
        250,
        ' '};

    NumberField field_speed{
        {19 * 8, 0 * 16},
        4,
        {0, 5000},
        1,
        ' '};
    Text text_alt_unit{
        {12 * 8, 0 * 16, 2 * 8, 16},
        ""};
    Text text_speed_unit{
        {25 * 8, 0 * 16, 5 * 8, 16},
        ""};

    NumberField field_lat_degrees{
        {5 * 8, 1 * 16},
        4,
        {-90, 90},
        1,
        ' '};
    NumberField field_lat_minutes{
        {10 * 8, 1 * 16},
        2,
        {0, 59},
        1,
        ' ',
        true};
    NumberField field_lat_seconds{
        {13 * 8, 1 * 16},
        2,
        {0, 59},
        1,
        ' ',
        true};
    Text text_lat_decimal{
        {17 * 8, 1 * 16, 13 * 8, 1 * 16},
        ""};

    NumberField field_lon_degrees{
        {5 * 8, 2 * 16},
        4,
        {-180, 180},
        1,
        ' '};
    NumberField field_lon_minutes{
        {10 * 8, 2 * 16},
        2,
        {0, 59},
        1,
        ' ',
        true};
    NumberField field_lon_seconds{
        {13 * 8, 2 * 16},
        2,
        {0, 59},
        1,
        ' ',
        true};
    Text text_lon_decimal{
        {17 * 8, 2 * 16, 13 * 8, 1 * 16},
        ""};
};

enum MapMarkerStored {
    MARKER_NOT_STORED,
    MARKER_STORED,
    MARKER_LIST_FULL
};

enum MapType {
    MAP_TYPE_OSM,
    MAP_TYPE_BIN
};

class GeoMap : public Widget {
   public:
    std::function<void(float, float, bool)> on_move{};

    GeoMap(Rect parent_rect);

    void paint(Painter& painter) override;

    bool on_touch(const TouchEvent event) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void update_my_position(float lat, float lon, int32_t altitude);
    void update_my_orientation(uint16_t angle, bool refresh = false);

    bool init();
    void set_mode(GeoMapMode mode);
    void set_manual_panning(bool v);
    bool manual_panning();
    void move(const float lon, const float lat);
    void set_tag(std::string new_tag) {
        tag_ = new_tag;
    }
    MapType get_map_type();

    void set_angle(uint16_t new_angle) {
        angle_ = new_angle;
    }

    bool map_file_opened() { return map_opened; }

    void set_hide_center_marker(bool hide) {
        hide_center_marker_ = hide;
    }
    bool hide_center_marker() { return hide_center_marker_; }

    static const int NumMarkerListElements = 30;

    void clear_markers();
    MapMarkerStored store_marker(GeoMarker& marker);

    static const Dim banner_height = GEOMAP_BANNER_HEIGHT;
    static const Dim geomap_rect_width = GEOMAP_RECT_WIDTH;
    static const Dim geomap_rect_height = GEOMAP_RECT_HEIGHT;

   private:
    void draw_scale(Painter& painter);
    ui::Point item_rect_pixel(GeoMarker& item);
    GeoPoint lat_lon_to_map_pixel(float lat, float lon);
    void draw_marker_item(Painter& painter, GeoMarker& item, const Color color, const Color fontColor = Color::white(), const Color backColor = Color::black());
    void draw_marker(Painter& painter, const ui::Point itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color = Color::red(), const Color fontColor = Color::white(), const Color backColor = Color::black());
    void draw_markers(Painter& painter);
    void draw_mypos(Painter& painter);
    void draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color, Painter& painter);
    void draw_map_grid(ui::Rect r, Painter& painter);
    void draw_switcher(Painter& painter);
    void map_read_line_bin(ui::Color* buffer, uint16_t pixels);
    // open street map related
    uint8_t find_osm_file_tile();
    void set_osm_max_zoom();
    bool draw_osm_file(int zoom, int tile_x, int tile_y, int relative_x, int relative_y, Painter& painter);
    int lon2tile(double lon, int zoom);
    int lat2tile(double lat, int zoom);
    double lon_to_pixel_x_tile(double lon, int zoom);
    double lat_to_pixel_y_tile(double lat, int zoom);
    double tile_pixel_x_to_lon(int x, int zoom);
    double tile_pixel_y_to_lat(int y, int zoom);
    uint8_t map_osm_zoom{3};
    double viewport_top_left_px = 0;
    double viewport_top_left_py = 0;

    bool manual_panning_{false};
    bool hide_center_marker_{false};
    GeoMapMode mode_{};
    File map_file{};
    bool map_opened{};
    bool map_visible{};
    uint16_t map_width{}, map_height{};
    int32_t map_center_x{}, map_center_y{};
    int16_t map_zoom{1};

    float lon_ratio{}, lat_ratio{};
    double map_bottom{};
    double map_world_lon{};
    double map_offset{};
    float x_pos{}, y_pos{};
    float prev_x_pos{32767.0f}, prev_y_pos{32767.0f};
    float lat_{};
    float lon_{};
    float zoom_pixel_offset{0.0f};
    float pixels_per_km{};
    uint16_t angle_{};
    std::string tag_{};

    // the portapack's position data ( for example injected from serial )
    GeoMarker my_pos{INVALID_LAT_LON, INVALID_LAT_LON, INVALID_ANGLE, ""};  // lat, lon, angle, tag
    int32_t my_altitude{0};

    int markerListLen{0};
    GeoMarker markerList[NumMarkerListElements];
    bool redraw_map{true};
    bool use_osm{false};
    bool has_osm{false};
};

class GeoMapView : public View {
   public:
    GeoMapView(
        const std::string& tag,
        int32_t altitude,
        GeoPos::alt_unit altitude_unit,
        GeoPos::spd_unit speed_unit,
        float lat,
        float lon,
        uint16_t angle,
        const std::function<void(void)> on_close = nullptr);
    GeoMapView(
        int32_t altitude,
        GeoPos::alt_unit altitude_unit,
        GeoPos::spd_unit speed_unit,
        float lat,
        float lon,
        const std::function<void(int32_t, float, float, int32_t)> on_done);
    ~GeoMapView();

    GeoMapView(const GeoMapView&) = delete;
    GeoMapView(GeoMapView&&) = delete;
    GeoMapView& operator=(const GeoMapView&) = delete;
    GeoMapView& operator=(GeoMapView&&) = delete;

    void focus() override;

    void update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed = 0);
    void update_my_position(float lat, float lon, int32_t altitude);
    void update_my_orientation(uint16_t angle, bool refresh = false);

    MapType get_map_type();

    std::string title() const override { return "Map view"; };

    void clear_markers();
    MapMarkerStored store_marker(GeoMarker& marker);

    void update_tag(const std::string tag);

   private:
    void setup();

    const std::function<void(int32_t, float, float, int32_t)> on_done{};
    GeoMapMode mode_{};
    int32_t altitude_{};
    int32_t speed_{};
    GeoPos::alt_unit altitude_unit_{};
    GeoPos::spd_unit speed_unit_{};
    float lat_{};
    float lon_{};
    uint16_t angle_{};
    std::function<void(void)> on_close_{nullptr};

    GeoPos geopos{
        {0, 0},
        altitude_unit_,
        speed_unit_};

    GeoMap geomap{
        {0, GeoMap::banner_height, GeoMap::geomap_rect_width, GeoMap::geomap_rect_height}};

    Button button_ok{
        {screen_width - 15 * 8, 0, 15 * 8, 1 * 16},
        "OK"};
};

} /* namespace ui */

#endif
