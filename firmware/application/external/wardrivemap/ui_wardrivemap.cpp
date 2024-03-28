/*
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

#include "ui_wardrivemap.hpp"

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "metadata_file.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::wardrivemap {

void WardriveMapView::focus() {
    geopos.focus();
}

// needs to load on every map change, because won't store or draw all while marker is not in the current view.
// todo optimize this somehow
bool WardriveMapView::load_markers() {
    uint16_t cnt = 0;
    uint16_t displayed_cnt = 0;
    geomap.clear_markers();
    // serach for files with geotag, and add it to geomap as marker with tag. limit to N bc of mem limit.
    for (const auto& entry : std::filesystem::directory_iterator(captures_dir, u"*.txt")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            if (displayed_cnt > 30) break;
            std::filesystem::path pth = captures_dir;
            pth += u"/" + entry.path();
            auto metadata_path = get_metadata_path(pth);
            auto metadata = read_metadata_file(metadata_path);

            if (metadata) {
                if (metadata.value().latitude != 0 && metadata.value().longitude != 0 && metadata.value().latitude < 400 && metadata.value().longitude < 400) {
                    if (first_init == false) {
                        // move map there before add, so will display this.
                        geopos.set_report_change(false);
                        geopos.set_lat(metadata.value().latitude);
                        geopos.set_lon(metadata.value().longitude);
                        geopos.set_report_change(true);
                        geomap.move(metadata.value().longitude, metadata.value().latitude);
                        first_init = true;
                    }
                    GeoMarker tmp{metadata.value().latitude, metadata.value().longitude, 400, entry.path().filename().string()};
                    if (geomap.store_marker(tmp) == MapMarkerStored::MARKER_STORED) displayed_cnt++;
                    cnt++;
                }
            }
        }
    }
    return (cnt > 0);
}

WardriveMapView::WardriveMapView(NavigationView& nav)
    : nav_{nav} {
    add_children({&text_info,
                  &geomap,
                  &geopos,
                  &text_notfound});

    geomap.set_mode(DISPLAY);
    geomap.set_manual_panning(false);
    geomap.set_focusable(true);
    geomap.set_hide_center_marker(true);
    geomap.clear_markers();

    geopos.set_report_change(false);
    geopos.set_lat(0);
    geopos.set_lon(0);
    geopos.set_altitude(0);
    geopos.set_speed(0);
    geopos.set_read_only(true);
    geopos.hide_altandspeed();
    geopos.set_report_change(true);

    geopos.on_change = [this](int32_t altitude, float lat, float lon, int32_t speed) {
        (void)altitude;
        (void)speed;
        geomap.set_manual_panning(true);
        geomap.move(lon, lat);
        load_markers();
        geomap.set_dirty();
    };
    text_notfound.hidden(true);
    geomap.init();
    // load markers
    if (load_markers()) {
        text_notfound.hidden(true);
        geomap.set_dirty();
    } else {
        geomap.hidden(true);
        geopos.hidden(true);
    }
    geomap.on_move = [this](float lon, float lat) {
        (void)lon;
        (void)lat;
        load_markers();
    };
}

WardriveMapView::~WardriveMapView() {
}

void WardriveMapView::on_gps(const GPSPosDataMessage* msg) {
    geomap.update_my_position(msg->lat, msg->lon, msg->altitude);
    if (geomap.manual_panning() == false) {
        geopos.set_report_change(false);
        geopos.set_lat(msg->lat);
        geopos.set_lon(msg->lon);
        geopos.set_altitude(msg->altitude);
        geopos.set_speed(msg->speed);
        geopos.set_report_change(true);
        geomap.move(msg->lon, msg->lat);
        load_markers();
    }
    geomap.set_dirty();
}

void WardriveMapView::on_orientation(const OrientationDataMessage* msg) {
    geomap.set_angle(msg->angle);
    geomap.update_my_orientation(msg->angle, true);
}

}  // namespace ui::external_app::wardrivemap