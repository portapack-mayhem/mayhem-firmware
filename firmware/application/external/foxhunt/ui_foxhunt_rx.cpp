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

#include "ui_foxhunt_rx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::foxhunt_rx {

void FoxhuntRxView::focus() {
    field_frequency.focus();
}

FoxhuntRxView::FoxhuntRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_am_audio);

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &freq_stats_db,
                  &rssi_graph,
                  &geomap,
                  &clear_markers,
                  &add_current_marker});

    clear_markers.on_select = [this](Button&) {
        geomap.clear_markers();
    };
    add_current_marker.on_select = [this, &nav](Button&) {
        // Check if manual position mode is active (status_flags bit 0)
        if (status_flags & 0x01) {
            // In manual mode: mark current position
            GeoMarker tmp{my_lat, my_lon, my_orientation};
            geomap.store_marker(tmp);
        } else {
            // In GPS mode: open position picker to enter manual mode
            float start_lat = (my_lat != 200) ? my_lat : 0.0f;
            float start_lon = (my_lon != 200) ? my_lon : 0.0f;
            nav.push<GeoMapView>(
                0,
                GeoPos::alt_unit::METERS,
                GeoPos::spd_unit::HIDDEN,
                start_lat,
                start_lon,
                [this](int32_t, float lat, float lon, int32_t) {
                    my_lat = lat;
                    my_lon = lon;
                    status_flags |= 0x01;  // Set manual_pos_mode bit
                    add_current_marker.set_text("Mark");
                    update_position_display();
                    geomap.update_my_position(lat, lon, 0);
                    geomap.move(lon, lat);
                    geomap.set_dirty();
                });
        }
    };
    geomap.set_mode(DISPLAY);
    geomap.set_manual_panning(false);
    // geomap.set_hide_center_marker(true); //todo test if needed
    geomap.set_focusable(true);
    geomap.clear_markers();
    receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
    field_frequency.set_step(100);
    receiver_model.enable();
    audio::output::start();
    rssi_graph.set_nb_columns(64);
    add_current_marker.set_text("SetPos");  // Initial text when in GPS mode
    update_position_display();
    geomap.init();
}

FoxhuntRxView::~FoxhuntRxView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void FoxhuntRxView::on_statistics_update(const ChannelStatistics& statistics) {
    static int16_t last_max_db = -1000;
    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);
    // refresh db and position mode
    if (last_max_db != statistics.max_db) {
        last_max_db = statistics.max_db;
        update_position_display(statistics.max_db);
    }

} /* on_statistic_updates */

void FoxhuntRxView::on_gps(const GPSPosDataMessage* msg) {
    if (!(status_flags & 0x01)) {  // Check manual_pos_mode bit
        my_lat = msg->lat;
        my_lon = msg->lon;
        geomap.update_my_position(msg->lat, msg->lon, msg->altitude);
        geomap.move(my_lon, my_lat);
        geomap.set_dirty();
    }
}
void FoxhuntRxView::on_orientation(const OrientationDataMessage* msg) {
    my_orientation = msg->angle;
    geomap.set_angle(msg->angle);
    geomap.update_my_orientation(msg->angle, true);
}

void FoxhuntRxView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void FoxhuntRxView::update_position_display(int16_t db) {
    static int16_t cached_db = -1000;
    if (db != -1000) {
        cached_db = db;
    }
    const char* mode = (status_flags & 0x01) ? "MANUAL" : "GPS";
    freq_stats_db.set("Power: " + to_string_dec_int(cached_db) + " db [" + std::string(mode) + "]");
}

}  // namespace ui::external_app::foxhunt_rx
