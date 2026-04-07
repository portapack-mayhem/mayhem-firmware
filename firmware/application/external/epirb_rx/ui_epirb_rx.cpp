/*
 * Copyright (C) 2024 EPIRB Decoder Implementation
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

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

#include "ui_epirb_rx.hpp"

using namespace portapack;

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui.hpp"

#include "message.hpp"

#include "usb_serial_asyncmsg.hpp"

namespace ui::external_app::epirb_rx {

std::string EPIRBAppView::beacon_to_hex_string(const baseband::Packet& packet) {
    const char hex[] = "0123456789ABCDEF";

    std::string out;
    out.resize(36);
    size_t frame_size = std::min(packet.size(), (size_t)144);

    for (size_t i = 0; i < frame_size / 8; i++) {
        uint8_t byte_val = 0;
        for (size_t bit = 0; bit < 8 && (i * 8 + bit) < frame_size; bit++) {
            if (packet[i * 8 + bit]) {
                byte_val |= (1 << (7 - bit));
            }
        }

        out[i * 2] = hex[byte_val >> 4];
        out[i * 2 + 1] = hex[byte_val & 0x0F];
    }

    return out;
}

Beacon EPIRBAppView::decode_packet(const baseband::Packet& packet) {
    Beacon beacon;

    // Convert packet bits to byte array for easier processing
    UsbSerialAsyncmsg::asyncmsg("Start Decode");

    uint8_t data[18];
    for (size_t i = 0; i < std::min(packet.size() / 8, (size_t)BEACON_DATA_SIZE); i++) {
        uint8_t byte_val = 0;
        for (int bit = 0; bit < 8 && (i * 8 + bit) < packet.size(); bit++) {
            if (packet[i * 8 + bit]) {
                byte_val |= (1 << (7 - bit));
            }
        }
        data[i] = byte_val;
    }
    beacon.setFrame(data);
    // Set timestamp
    UsbSerialAsyncmsg::asyncmsg("Get time");
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    beacon.date = datetime;

    return beacon;
    /*
    UsbSerialAsyncmsg::asyncmsg("Start bch check");
    // Perform BCH error detection and correction
    uint8_t error_count = 0;
    beacon.packet_status = perform_bch_check(data, error_count);
    beacon.error_count = error_count;

    // Extract beacon ID (bits 26-85, 15 hex digits)
    UsbSerialAsyncmsg::asyncmsg("Get beacon Id");
    beacon.beacon_id = 0;
    for (int i = 3; i < 11; i++) {
        beacon.beacon_id = (beacon.beacon_id << 8) | data[i];
    }

    // Extract beacon type (bits 86-88)
    UsbSerialAsyncmsg::asyncmsg("Get beacon type");
    uint8_t type_bits = (data[10] >> 5) & 0x07;
    beacon.beacon_type = decode_beacon_type(type_bits);

    // Extract emergency type (bits 91-94 for some beacon types)
    UsbSerialAsyncmsg::asyncmsg("Get eamercency type");
    uint8_t emergency_bits = (data[11] >> 4) & 0x0F;
    beacon.emergency_type = decode_emergency_type(emergency_bits);

    // Extract location if encoded (depends on beacon type and protocol)
    UsbSerialAsyncmsg::asyncmsg("Get location");
    beacon.location = decode_location(data);

    // Extract country code (bits 1-10)
    UsbSerialAsyncmsg::asyncmsg("Get country code");
    beacon.country_code = decode_country_code(data);

    // Set timestamp
    UsbSerialAsyncmsg::asyncmsg("Get time");
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    beacon.timestamp = datetime;

    return beacon;*/
}

void EPIRBLogger::on_packet(Beacon& beacon) {
    std::string entry = "EPIRB," +
                        beacon.hexId + "," +
                        beacon.getProtocolName() + ",";  // ;
                                                         // to_string_dec_uint(static_cast<uint8_t>(beacon.emergency_type)) + ",";

    if (!beacon.location.isUnknown()) {
        entry += beacon.location.toString(Location::LocationFormat::DECIMAL);
    } else {
        entry += ",";
    }

    entry += "," + std::string(beacon.country.alphaCode) + "," +
             format_packet_status(beacon) + "\n";

    log_file.write_entry(beacon.date, entry);
}

std::string format_beacon_type(Beacon& beacon) {
    return beacon.getProtocolDesciption();
}

std::string format_emergency_type(Beacon& /*beacon*/) {
    // TODO
    /*switch (EmergencyType::Fire) {
        case EmergencyType::Fire:
            return "Fire";
        case EmergencyType::Flooding:
            return "Flooding";
        case EmergencyType::Collision:
            return "Collision";
        case EmergencyType::Grounding:
            return "Grounding";
        case EmergencyType::Sinking:
            return "Sinking";
        case EmergencyType::Disabled:
            return "Disabled";
        case EmergencyType::Abandoning:
            return "Abandoning";
        case EmergencyType::Piracy:
            return "Piracy";
        case EmergencyType::Man_Overboard:
            return "MOB";
        default:
            return "Other";
    }*/
    return "other";
}

std::string format_packet_status(Beacon& beacon) {
    if (beacon.isFrameValid())
        return "OK";
    else
        return "ERR";
}

ui::Color get_packet_status_color(PacketStatus status) {
    switch (status) {
        case PacketStatus::Valid:
            return ui::Color::green();
        case PacketStatus::Corrected:
            return ui::Color::yellow();
        case PacketStatus::Error:
            return ui::Color::red();
        default:
            return ui::Color::white();
    }
}
/*
EPIRBBeaconDetailView::EPIRBBeaconDetailView(ui::NavigationView& nav) {
    add_children({&button_done,
                  &button_see_map});

    button_done.on_select = [this](Button&) {
        if (on_close) on_close();
    };

    button_see_map.on_select = [this, &nav](Button&) {
        if (!beacon_.location.isUnknown()) {
            nav.push<GeoMapView>(
                beacon_.hexId,  // tag as string
                0,              // altitude
                GeoPos::alt_unit::METERS,
                GeoPos::spd_unit::NONE,
                beacon_.location.latitude.getFloatValue(),
                beacon_.location.longitude.getFloatValue(),
                0,  // angle
                [this]() {
                    if (on_close) on_close();
                });
        }
    };
}

void EPIRBBeaconDetailView::set_beacon(const Beacon& beacon) {
    beacon_ = beacon;
    set_dirty();
}

void EPIRBBeaconDetailView::focus() {
    button_see_map.focus();
}

void EPIRBBeaconDetailView::paint(ui::Painter& painter) {
    View::paint(painter);

    const auto rect = screen_rect();
    const auto s = style();

    auto draw_cursor = rect.location();
    draw_cursor += {8, 8};

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Beacon ID", beacon_.hexId)
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Type", beacon_.getProtocolName())
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Emergency", format_emergency_type(beacon_))
                      .location();

    if (!beacon_.location.isUnknown()) {
        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Latitude", to_string_decimal(beacon_.location.latitude.getFloatValue(), 6) + "°")
                          .location();

        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Longitude", to_string_decimal(beacon_.location.longitude.getFloatValue(), 6) + "°")
                          .location();
    } else {
        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Location", "Unknown")
                          .location();
    }

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Country", beacon_.country.alphaCode)
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Time", to_string_datetime(beacon_.date, HMS))
                      .location();

    // Show packet status with appropriate color
    std::string status_text = format_packet_status(beacon_);
    //if (beacon_.error_count > 0 && beacon_.packet_status == PacketStatus::Corrected) {
    //    status_text += " (" + to_string_dec_uint(beacon_.error_count) + " err)";
    //}
    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Status", status_text)
                      .location();
}

ui::Rect EPIRBBeaconDetailView::draw_field(
    ui::Painter& painter,
    const ui::Rect& draw_rect,
    const ui::Style& style,
    const std::string& label,
    const std::string& value) {
    const auto label_width = 8 * 8;

    painter.draw_string({draw_rect.location()}, style, label + ":");
    painter.draw_string({draw_rect.location() + ui::Point{label_width, 0}}, style, value);

    return {draw_rect.location() + ui::Point{0, draw_rect.height()}, draw_rect.size()};
}
*/
EPIRBAppView::EPIRBAppView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&label_frequency,
                  &options_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &channel,
                  &label_status,
                  &label_beacons_count,
                  &label_latest,
                  &text_latest_info,
                  &label_packet_stats,
                  //&options_algo,
                  &console,
                  &button_map,
                  &button_clear,
                  &button_log});

    button_map.on_select = [this](Button&) {
        this->on_show_map();
    };

    button_clear.on_select = [this](Button&) {
        this->on_clear_beacons();
    };

    button_log.on_select = [this](Button&) {
        this->on_toggle_log();
    };

    options_frequency.on_change = [this](size_t, ui::OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_frequency.set_by_value(receiver_model.target_frequency());

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };

    options_algo.on_change = [this](size_t, ui::OptionsField::value_t v) {
        // Send config to baseband
        epirb_tx_message.data_len = v;

        baseband::set_epirb_tx_config(epirb_tx_message);
    };

    // Configure receiver for default EPIRB frequency (406.028 MHz)
    receiver_model.set_target_frequency(406028000);
    receiver_model.set_rf_amp(true);
    receiver_model.set_lna(32);
    receiver_model.set_vga(32);
    receiver_model.set_sampling_rate(3072000);
    // receiver_model.set_sampling_rate(2457600);
    // receiver_model.set_baseband_bandwidth(10000);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.enable();

    /*logger = std::make_unique<EPIRBLogger>();
    if (logger) {
        logger->append(logs_dir / "epirb_rx.txt");
    }*/
    std::string mess = "EPIRB App Started!\n";
    UsbSerialAsyncmsg::asyncmsg(mess);
}

EPIRBAppView::~EPIRBAppView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void EPIRBAppView::set_parent_rect(const ui::Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    const auto console_rect = ui::Rect{
        new_parent_rect.left(),
        new_parent_rect.top() + header_height,
        new_parent_rect.width(),
        new_parent_rect.height() - header_height - 32};
    console.set_parent_rect(console_rect);
}

void EPIRBAppView::paint(ui::Painter& /* painter */) {
    // Custom painting if needed
}

void EPIRBAppView::focus() {
    options_frequency.focus();
}

void EPIRBAppView::on_packet(const baseband::Packet& packet) {
    std::string beacon_string = "Data:" + beacon_to_hex_string(packet) + "\n";
    // std::string beacon_string = "Message Received!\n";
    UsbSerialAsyncmsg::asyncmsg(beacon_string);

    // Decode the EPIRB packet
    auto beacon = decode_packet(packet);

    on_beacon_decoded(beacon);
}

void EPIRBAppView::on_beacon_decoded(Beacon& beacon) {
    beacons_received++;

    // Track packet statistics
    if (beacon.isFrameValid())
        packets_valid++;
    else
        packets_error++;

    recent_beacons[recent_beacon_pos++ % BEACON_HISTORY_SIZE] = beacon;
    recent_beacon_full |= (recent_beacon_pos == 0);

    // Update display
    update_display();

    // Log the beacon
    // if (logger) {
    //    logger->on_packet(beacon);
    //}

    // Display in console with full details and colored status
    std::string beacon_info = format_beacon_summary(beacon);
    /*if (beacon.emergency_type != EmergencyType::Other) {
        beacon_info += " [" + format_emergency_type(beacon.emergency_type) + "]";
    }*/

    // Add colored status indicator
    std::string status_color;
    if (beacon.isFrameValid())
        status_color = STR_COLOR_GREEN;
    else
        status_color = STR_COLOR_RED;

    beacon_info += " [" + status_color + format_packet_status(beacon) + STR_COLOR_WHITE + "]";
    /*if (beacon.error_count > 0 && beacon.packet_status == PacketStatus::Corrected) {
        beacon_info += " (" + to_string_dec_uint(beacon.error_count) + "e)";
    }*/

    console.write(beacon_info + "\n");
}

void EPIRBAppView::on_show_map() {
    if (recent_beacon_full || recent_beacon_pos > 0) {
        // Find latest beacon with valid location
        size_t size = recent_beacon_full ? BEACON_HISTORY_SIZE : recent_beacon_pos;
        for (size_t i = 1; i <= size; i++) {
            int8_t pos = (recent_beacon_pos - i);
            if (pos < 0) pos = BEACON_HISTORY_SIZE + pos;
            if (!recent_beacons[pos].location.isUnknown()) {
                // Create a GeoMapView with all beacon locations
                auto map_view = nav_.push<ui::GeoMapView>(
                    "EPIRB",  // tag
                    0,        // altitude
                    ui::GeoPos::alt_unit::METERS,
                    ui::GeoPos::spd_unit::NONE,
                    recent_beacons[pos].location.latitude.getFloatValue(),
                    recent_beacons[pos].location.longitude.getFloatValue(),
                    0  // angle
                );

                // Add all beacons with valid locations as markers
                for (auto& beacon : recent_beacons) {
                    if (!beacon.location.isUnknown()) {
                        ui::GeoMarker marker;
                        marker.lat = beacon.location.latitude.getFloatValue();
                        marker.lon = beacon.location.longitude.getFloatValue();
                        marker.angle = 0;
                        marker.tag = beacon.hexId + " " +
                                     format_beacon_type(beacon);
                        map_view->store_marker(marker);
                    }
                }
                return;
            }
        }
    }

    // No valid location found
    nav_.display_modal("No Location", "No beacons with valid\nlocation data found.");
}

void EPIRBAppView::on_clear_beacons() {
    recent_beacon_pos = 0;
    beacons_received = 0;
    packets_valid = 0;
    packets_corrected = 0;
    packets_error = 0;
    console.clear(true);
    update_display();
}

void EPIRBAppView::on_toggle_log() {
    // Toggle logging functionality
    /*if (logger) {
        logger.reset();
        button_log.set_text("Log");
    } else {
        logger = std::make_unique<EPIRBLogger>();
        logger->append("epirb_rx.txt");
        button_log.set_text("Stop");
    }*/
}

void EPIRBAppView::on_tick_second() {
    // Update status display every second
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);

    label_status.set("Listening... " + to_string_datetime(datetime, HM));
}

void EPIRBAppView::update_display() {
    label_beacons_count.set("Beacons: " + to_string_dec_uint(beacons_received));

    // Update packet statistics display
    std::string stats = std::string("Stats: ") +
                        STR_COLOR_GREEN + to_string_dec_uint(packets_valid) + "OK " +
                        STR_COLOR_YELLOW + to_string_dec_uint(packets_corrected) + "CORR " +
                        STR_COLOR_RED + to_string_dec_uint(packets_error) + "ERR" + STR_COLOR_WHITE;
    label_packet_stats.set(stats);

    if (recent_beacon_pos > 0) {
        auto& latest = recent_beacons[recent_beacon_pos - 1];
        text_latest_info.set(format_beacon_summary(latest));
    }
}

std::string EPIRBAppView::format_beacon_summary(Beacon& beacon) {
    std::string summary = beacon.hexId + " " +
                          format_beacon_type(beacon);

    if (!beacon.location.isUnknown()) {
        summary += " " + format_location(beacon.location);
    }

    // Add status indicator for summary display
    summary += " " + format_packet_status(beacon);

    return summary;
}

std::string EPIRBAppView::format_location(Location& location) {
    return to_string_decimal(location.latitude.getFloatValue(), 4) + "°," +
           to_string_decimal(location.longitude.getFloatValue(), 4) + "°";
}

}  // namespace ui::external_app::epirb_rx