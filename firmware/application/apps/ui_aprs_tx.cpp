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

#include "ui_aprs_tx.hpp"
#include "ui_alphanum.hpp"

#include "aprs.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_geomap.hpp"

#include <cstring>
#include <stdio.h>

using namespace aprs;
using namespace portapack;

namespace ui {

void APRSTXView::focus() {
    tx_view.focus();
}

APRSTXView::~APRSTXView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void APRSTXView::start_tx() {
    // TODO: Clean up this API to take string_views to avoid allocations.
    std::string new_payload = payload;

    std::string gps = text_gps_coord.get();
    std::string token = "?GPS?";
    size_t pos = 0;
    while ((pos = new_payload.find(token, pos)) != std::string::npos) {
        new_payload.replace(pos, token.size(), gps);
        pos += gps.size();
    }

    text_payload.set(new_payload);

    std::string path = text_path.get();

    make_aprs_frame(
        sym_source.to_string().c_str(), num_ssid_source.value(),
        sym_dest.to_string().c_str(), num_ssid_dest.value(),
        new_payload, path);

    // uint8_t * bb_data_ptr = shared_memory.bb_data.data;
    // text_payload.set(to_string_hex_array(bb_data_ptr + 56, 15));

    transmitter_model.enable();

    baseband::set_afsk_data(
        AFSK_TX_SAMPLERATE / 1200,
        1200,
        2200,
        1,
        10000,  // APRS uses fixed 10k bandwidth
        8);
}

void APRSTXView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;

    if (done) {
        transmitter_model.disable();
        tx_view.set_transmitting(false);
    }
}

void APRSTXView::process_coordinates(float lat_, float lon_) {
    std::string s;

    char ns = lat_ >= 0 ? 'N' : 'S';
    float lat = (lat_ >= 0) ? lat_ : -lat_;
    uint32_t lat_d = (uint32_t)lat;
    uint32_t lat_m = (uint32_t)((lat - lat_d) * 6000.0f + 0.5f);

    if (lat_m >= 6000) {
        lat_m = 0;
        lat_d++;
    }

    s += to_string_dec_uint(lat_d, 2, '0');
    s += to_string_dec_uint(lat_m / 100, 2, '0');
    s += ".";
    s += to_string_dec_uint(lat_m % 100, 2, '0');
    s += ns;

    s += "/";

    char ew = lon_ >= 0 ? 'E' : 'W';
    float lon = (lon_ >= 0) ? lon_ : -lon_;
    uint32_t lon_d = (uint32_t)lon;
    uint32_t lon_m = (uint32_t)((lon - lon_d) * 6000.0f + 0.5f);

    if (lon_m >= 6000) {
        lon_m = 0;
        lon_d++;
    }

    s += to_string_dec_uint(lon_d, 3, '0');
    s += to_string_dec_uint(lon_m / 100, 2, '0');
    s += ".";
    s += to_string_dec_uint(lon_m % 100, 2, '0');
    s += ew;
    text_gps_coord.set(s);
}

void APRSTXView::on_gps(const GPSPosDataMessage* message) {
    if (manual_gps_mode) {
        return;  // no more update once set manually
    }
    if (message->lat < -90.0 || message->lat > 90.0 ||
        message->lon < -180.0 || message->lon > 180.0) {
        return;
    }
    if (message->lat == 0.0f && message->lon == 0.0f) {
        return;
    }
    last_lat = message->lat;
    last_lon = message->lon;
    process_coordinates(message->lat, message->lon);
}

APRSTXView::APRSTXView(NavigationView& nav) {
    baseband::run_image(portapack::spi_flash::image_tag_afsk);

    add_children({&labels,
                  &sym_source,
                  &num_ssid_source,
                  &sym_dest,
                  &num_ssid_dest,
                  &text_payload,
                  &button_set,
                  &text_gps_coord,
                  &button_mangps,
                  &gps_is_manual,
                  &text_path,
                  &button_setpath,
                  &tx_view});

    sym_source.set_value(symsrc);
    num_ssid_source.set_value(ssidsrc);
    sym_dest.set_value(symdst);
    num_ssid_dest.set_value(ssiddst);
    text_payload.set(payload);
    text_path.set(path_cache);

    sym_source.on_change = [this](SymField&) {
        symsrc = sym_source.to_string();
    };
    num_ssid_source.on_change = [this](int32_t v) {
        ssidsrc = v;
    };
    sym_dest.on_change = [this](SymField&) {
        symdst = sym_dest.to_string();
    };
    num_ssid_dest.on_change = [this](int32_t v) {
        ssiddst = v;
    };

    button_set.on_select = [this, &nav](Button&) {
        text_prompt(
            nav,
            payload,
            30,
            ENTER_KEYBOARD_MODE_ALPHA,
            [this](std::string& s) {
                text_payload.set(s);
            });
    };
    button_setpath.on_select = [this, &nav](Button&) {
        text_prompt(
            nav,
            path_cache,
            60,
            ENTER_KEYBOARD_MODE_ALPHA,
            [this](std::string& s) {
                text_path.set(s);
                path_cache = s;
            });
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        start_tx();
        tx_view.set_transmitting(true);
    };

    tx_view.on_stop = [this]() {
        tx_view.set_transmitting(false);
        transmitter_model.disable();
    };

    button_mangps.on_select = [this, &nav](Button&) {
        nav.push<GeoMapView>(
            0,
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::HIDDEN,
            last_lat,
            last_lon,
            [this](int32_t, float lat, float lon, int32_t) {
                last_lat = lat;
                last_lon = lon;
                manual_gps_mode = true;
                gps_is_manual.set("MANUAL");
                process_coordinates(lat, lon);
            });
    };
    // process_coordinates(last_lat, last_lon); //don't load last, so won't confuse users
}

} /* namespace ui */
