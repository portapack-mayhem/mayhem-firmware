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
    add_current_marker.on_select = [this](Button&) {
        GeoMarker tmp{my_lat, my_lon, my_orientation};
        geomap.store_marker(tmp);
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
    // refresh db
    if (last_max_db != statistics.max_db) {
        last_max_db = statistics.max_db;
        freq_stats_db.set("Power: " + to_string_dec_int(statistics.max_db) + " db");
    }

} /* on_statistic_updates */

void FoxhuntRxView::on_gps(const GPSPosDataMessage* msg) {
    my_lat = msg->lat;
    my_lon = msg->lon;
    geomap.update_my_position(msg->lat, msg->lon, msg->altitude);
    geomap.move(my_lon, my_lat);
    geomap.set_dirty();
}
void FoxhuntRxView::on_orientation(const OrientationDataMessage* msg) {
    my_orientation = msg->angle;
    geomap.set_angle(msg->angle);
    geomap.update_my_orientation(msg->angle, true);
}

}  // namespace ui::external_app::foxhunt_rx
