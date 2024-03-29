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

#include "ui_sonde.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "app_settings.hpp"
#include "file_path.hpp"

#include "portapack.hpp"
#include <cstring>
#include <stdio.h>

using namespace portapack;
namespace pmem = portapack::persistent_memory;

#include "string_format.hpp"
#include "complex.hpp"

void SondeLogger::on_packet(const sonde::Packet& packet) {
    const auto formatted = packet.symbols_formatted();
    log_file.write_entry(packet.received_at(), formatted.data);
}

namespace ui {

SondeView::SondeView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_sonde);

    add_children({&labels,
                  &field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &check_log,
                  &check_crc,
                  &text_signature,
                  &text_serial,
                  &text_timestamp,
                  &text_voltage,
                  &text_frame,
                  &text_temp,
                  &text_humid,
                  &geopos,
                  &button_see_qr,
                  &button_see_map});

    field_frequency.set_step(500);  // euquiq: was 10000, but we are using this for fine-tunning

    geopos.set_read_only(true);

    check_log.set_value(logging);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
    };

    check_crc.set_value(use_crc);
    check_crc.on_select = [this](Checkbox&, bool v) {
        use_crc = v;
    };

    receiver_model.enable();

    // QR code with geo URI
    button_see_qr.on_select = [this, &nav](Button&) {
        std::string geo_uri = "geo:" + to_string_decimal(geopos.lat(), 5) + "," + to_string_decimal(geopos.lon(), 5);  // 5 decimal digits for ~1 meter accuracy
        nav.push<QRCodeView>(geo_uri.data());
    };

    button_see_map.on_select = [this, &nav](Button&) {
        geomap_view_ = nav.push<GeoMapView>(
            sonde_id,
            geopos.altitude(),
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::HIDDEN,
            geopos.lat(),
            geopos.lon(),
            999);  // set a dummy heading out of range to draw a cross...probably not ideal?
        nav.set_on_pop([this]() {
            geomap_view_ = nullptr;
        });
    };

    logger = std::make_unique<SondeLogger>();
    if (logger)
        logger->append(logs_dir / u"SONDE.TXT");

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }

    // inject a PitchRSSIConfigureMessage in order to arm
    // the pitch rssi events that will be used by the
    // processor:
    const PitchRSSIConfigureMessage message{true, 0};

    shared_memory.application_queue.push(message);

    baseband::set_pitch_rssi(0, true);
}

SondeView::~SondeView() {
    baseband::set_pitch_rssi(0, false);

    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void SondeView::on_gps(const GPSPosDataMessage* msg) {
    if (!geomap_view_)
        return;
    geomap_view_->update_my_position(msg->lat, msg->lon, msg->altitude);
}
void SondeView::on_orientation(const OrientationDataMessage* msg) {
    if (!geomap_view_)
        return;
    geomap_view_->update_my_orientation(msg->angle, true);
}

void SondeView::focus() {
    field_frequency.focus();
}

void SondeView::on_packet(const sonde::Packet& packet) {
    if (!use_crc || packet.crc_ok())  // euquiq: Reject bad packet if crc is on
    {
        text_signature.set(packet.type_string());

        sonde_id = packet.serial_number();  // used also as tag on the geomap
        text_serial.set(sonde_id);

        text_timestamp.set(to_string_timestamp(packet.received_at()));

        text_voltage.set(unit_auto_scale(packet.battery_voltage(), 2, 2) + "V");

        text_frame.set(to_string_dec_uint(packet.frame(), 0));  // euquiq: integrate frame #, temp & humid.

        temp_humid_info = packet.get_temp_humid();
        if (temp_humid_info.humid != 0) {
            double decimals = abs(get_decimals(temp_humid_info.humid, 10, true));
            text_humid.set(to_string_dec_int((int)temp_humid_info.humid) + "." + to_string_dec_uint(decimals, 1) + "%");
        }

        if (temp_humid_info.temp != 0) {
            double decimals = abs(get_decimals(temp_humid_info.temp, 10, true));
            text_temp.set(to_string_dec_int((int)temp_humid_info.temp) + "." + to_string_dec_uint(decimals, 1) + STR_DEGREES_C);
        }

        gps_info = packet.get_GPS_data();

        geopos.set_altitude(gps_info.alt);
        geopos.set_lat(gps_info.lat);
        geopos.set_lon(gps_info.lon);

        if (logger && logging) {
            logger->on_packet(packet);
        }

        if (pmem::beep_on_packets()) {
            baseband::request_rssi_beep();
        }
    }
}

} /* namespace ui */
