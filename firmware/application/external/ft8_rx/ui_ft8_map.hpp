/*
 * Copyright (C) 2026 Dmytro Onyshko
 * Copyright (C) 2026 Khanfar
 * Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2026 Mohammad Moghtader (Xmoo26)
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

#ifndef __UI_FT8_MAP_H__
#define __UI_FT8_MAP_H__

#include "ui.hpp"
#include "ui_geomap.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"

#include <functional>
#include <string>

namespace ui::external_app::ft8_rx {

/* A globe: outline, two meridians and two parallels. Rows of two bytes, and within a
 * byte the lowest bit is the leftmost pixel, as draw_bitmap() reads it. */
static constexpr uint8_t bitmap_globe_data[] = {
    0x00, 0x00, 0xE0, 0x07, 0x98, 0x19, 0x44, 0x22, 0x22, 0x44, 0xFE, 0x7F, 0x21, 0x84, 0x11, 0x88,
    0x11, 0x88, 0x21, 0x84, 0xFE, 0x7F, 0x22, 0x44, 0x44, 0x22, 0x98, 0x19, 0xE0, 0x07, 0x00, 0x00};
static constexpr Bitmap bitmap_globe{{16, 16}, bitmap_globe_data};

/* A station heard on the band, placed at the center of the 4-character Maidenhead
 * square its message carried. */
struct FT8Spot {
    float lat{0};
    float lon{0};
    char call[12]{};
    char grid[5]{};
    bool cq{false};
    int16_t freq{0};  // audio frequency of the last decode, Hz (0 = unknown)
};

/* The stations heard this session, oldest first. A station heard again moves to the
 * end, and a full store drops its oldest station. Fixed size, so the decoder view that
 * owns it costs the same whatever the band is doing. */
struct FT8Spots {
    static constexpr size_t max_spots = 24;
    FT8Spot spot[max_spots]{};
    size_t count{0};

    void add(const FT8Spot& s);
    int find(const char* call) const;
    void clear() { count = 0; }
};

/* Fills spot from a decoded message that carries a locator ("CQ ON1AEY JO11",
 * "EK7DY DA0O JO30"). False when the message carries none, or the station sending it
 * is only known by its hash. */
bool parse_spot(const char* text, FT8Spot& spot);

/* Center of a 4-character Maidenhead square. False when grid is not one. */
bool locator_to_lat_lon(const char* grid, float& lat, float& lon);

/* GeoMap that zooms through the view's presets instead of its own ladder, pans on a
 * touch drag and lets the view draw the stations over it. */
class FT8Map : public GeoMap {
   public:
    using GeoMap::GeoMap;

    std::function<void(int)> on_zoom_step{};
    std::function<void(Painter&)> on_paint_overlay{};

    void paint(Painter& painter) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    // Pan with a touch drag only. The D-pad is left to the focus manager so it can
    // move off the map (down to the station list, up to the toolbar); GeoMap::on_key
    // would otherwise consume every arrow to pan and trap focus on the map, which
    // stops the list from ever being scrolled.
    bool on_key(const KeyEvent) override { return false; }

   private:
    Point drag_start_{};
};

/* The last-heard list under the map, newest at the bottom. The rotary moves the
 * selection when the list has focus and a touch picks a row; the selected station is
 * picked out on the map. */
class FT8SpotList : public Widget {
   public:
    FT8SpotList(Rect parent_rect, const FT8Spots& spots)
        : Widget{parent_rect}, spots_{spots} {
        set_focusable(true);
    }

    void paint(Painter& painter) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    bool on_key(const KeyEvent key) override;  // Select opens the detail view

    /* Index of the selected station, or -1. The selection follows the callsign, so it
     * stays on the same station while the store reorders. */
    int selected() const;
    void clear_selection() { selected_call_[0] = '\0'; }

    bool have_home{false};
    float home_lat{0};
    float home_lon{0};
    std::function<void()> on_change{};
    std::function<void(int)> on_open{};  // open the detail view for a station index

   private:
    const FT8Spots& spots_;
    char selected_call_[sizeof(FT8Spot::call)]{};
    int scroll_{0};

    int rows() const { return (screen_rect().height() - 8) / 8; }
    void select(int index);
};

/* A single station shown full-screen in the normal (readable) font: call, grid,
 * country and, with a home locator set, distance and bearing. Opened by tapping or
 * selecting a row of the last-heard list, which is too small to read comfortably. */
class FT8SpotDetailView : public View {
   public:
    FT8SpotDetailView(NavigationView& nav, const FT8Spot& spot, bool have_home, float home_lat, float home_lon);

    void paint(Painter& painter) override;
    void focus() override;
    std::string title() const override { return "Station"; }

   private:
    NavigationView& nav_;
    FT8Spot spot_{};
    bool have_home_{false};
    float home_lat_{0};
    float home_lon_{0};

    // y accounts for the 16 px status bar so the button sits inside the view, not 8 px
    // off the bottom of the screen.
    Button button_done{
        {screen_width - 96 - 8, screen_height - 16 - 40, 96, 32},
        "Back"};
};

/* Map of the stations heard, drawn from home, which is the 4-character locator set in
 * the top row. Reads the light /ADSB/world_map_2048.bin: the full-size world_map.bin is
 * far too slow to read at the zoom a whole-band view needs. */
class FT8MapView : public View {
   public:
    FT8MapView(NavigationView& nav, std::string& qth, FT8Spots& spots, std::function<void()> on_close);
    ~FT8MapView();

    FT8MapView(const FT8MapView&) = delete;
    FT8MapView& operator=(const FT8MapView&) = delete;

    void focus() override;
    std::string title() const override { return "FT8 Map"; }

    /* Called by the decoder view when a station was added. */
    void spots_changed();

   private:
    /* Presets for a 2048 px map, most zoomed in first: roughly 4, 8, 20, 40, 85, 170
     * and 340 degrees of longitude across the screen. 10 is the most GeoMap magnifies
     * a map, and at -8 the view is 1920 px, just inside the map. */
    static constexpr int16_t zoom_presets[] = {10, 5, 2, 1, -2, -4, -8};
    static constexpr size_t zoom_preset_count = sizeof(zoom_presets) / sizeof(zoom_presets[0]);

    NavigationView& nav_;
    std::string& qth_;
    FT8Spots& spots_;
    std::function<void()> on_close_;
    size_t preset_{5};
    bool have_home_{false};
    float home_lat_{0};
    float home_lon_{0};

    void set_home(bool recenter);
    void step_zoom(int dir);
    void draw_overlay(Painter& painter);
    void draw_field_letters(Painter& painter, const Rect& r);

    Text text_qth{
        {0, 0, 3 * 8, 16},
        "QTH"};

    /* A locator is two field letters A-R then two square digits, so it takes two
     * fields, each with its own symbols. */
    SymField field_qth_field{
        {4 * 8, 0},
        2,
        "ABCDEFGHIJKLMNOPQR",
        true};

    SymField field_qth_square{
        {6 * 8, 0},
        2,
        SymField::Type::Dec,
        true};

    Button button_zoom_out{
        {9 * 8, 0, 3 * 8, 16},
        "-"};

    Button button_zoom_in{
        {12 * 8, 0, 3 * 8, 16},
        "+"};

    Button button_clear{
        {16 * 8, 0, 6 * 8, 16},
        "Clear"};

    /* Height is a multiple of 10 so every zoom preset fills it with whole map lines. */
    FT8Map geomap{
        {0, 16, screen_width, ((screen_height - 5 * 16) / 10) * 10}};

    FT8SpotList spot_list{
        {0, 16 + ((screen_height - 5 * 16) / 10) * 10, screen_width, screen_height - 2 * 16 - ((screen_height - 5 * 16) / 10) * 10},
        spots_};
};

}  // namespace ui::external_app::ft8_rx

#endif  // __UI_FT8_MAP_H__
