/*
 * Copyright (C) 2024 HTotoo
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

#ifndef __UI_WARDRIVEMAP_H__
#define __UI_WARDRIVEMAP_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_geomap.hpp"
#include "app_settings.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::wardrivemap {

class WardriveMapView : public View {
   public:
    WardriveMapView(NavigationView& nav);
    ~WardriveMapView();

    void focus() override;

    std::string title() const override {
        return "WardriveMap";
    };

   private:
    const std::filesystem::path flippersub_dir = u"subghz";
    NavigationView& nav_;

    Text text_info{{UI_POS_X(0), 0 * 8, 20 * 8, 16 * 1}, "0 / 30"};
    Text text_notfound{{UI_POS_X(0), 0 * 8, screen_width, 16 * 1}, "No GeoTagged captures found"};
    GeoPos geopos{
        {0, 20},
        GeoPos::alt_unit::METERS,
        GeoPos::spd_unit::HIDDEN};
    GeoMap geomap{{0, 75, screen_width, screen_height - 75}};

    Button btn_back{{UI_POS_X_RIGHT(8), 0 * 8, 3 * 8, 16}, "<-"};
    Button btn_next{{UI_POS_X_RIGHT(4), 0 * 8, 3 * 8, 16}, "->"};

    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);

    void load_markers();  // returns true if any exists, false if none.

    bool first_init = false;       // to center map to first marker, before callback is set
    bool markers_counted = false;  // to iterate all files on first, but only on first.
    uint16_t marker_start = 0;     // for paginator, this will be the first displayed
    uint16_t marker_cntall = 0;    // all geotagged marker count

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
    MessageHandlerRegistration message_handler_orientation{
        Message::ID::OrientationData,
        [this](Message* const p) {
            const auto message = static_cast<const OrientationDataMessage*>(p);
            this->on_orientation(message);
        }};
};
};  // namespace ui::external_app::wardrivemap

#endif /*__UI_WARDRIVEMAP_H__*/
