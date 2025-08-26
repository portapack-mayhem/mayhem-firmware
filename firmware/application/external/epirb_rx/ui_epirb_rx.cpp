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

namespace ui::external_app::epirb_rx {

EPIRBBeacon EPIRBDecoder::decode_packet(const baseband::Packet& packet) {
    EPIRBBeacon beacon;

    if (packet.size() < 112) {
        return beacon;  // Invalid packet
    }

    // Convert packet bits to byte array for easier processing
    std::array<uint8_t, 16> data{};
    for (size_t i = 0; i < std::min(packet.size() / 8, data.size()); i++) {
        uint8_t byte_val = 0;
        for (int bit = 0; bit < 8 && (i * 8 + bit) < packet.size(); bit++) {
            if (packet[i * 8 + bit]) {
                byte_val |= (1 << (7 - bit));
            }
        }
        data[i] = byte_val;
    }

    // Perform BCH error detection and correction
    uint8_t error_count = 0;
    beacon.packet_status = perform_bch_check(data, error_count);
    beacon.error_count = error_count;

    // Extract beacon ID (bits 26-85, 15 hex digits)
    beacon.beacon_id = 0;
    for (int i = 3; i < 11; i++) {
        beacon.beacon_id = (beacon.beacon_id << 8) | data[i];
    }

    // Extract beacon type (bits 86-88)
    uint8_t type_bits = (data[10] >> 5) & 0x07;
    beacon.beacon_type = decode_beacon_type(type_bits);

    // Extract emergency type (bits 91-94 for some beacon types)
    uint8_t emergency_bits = (data[11] >> 4) & 0x0F;
    beacon.emergency_type = decode_emergency_type(emergency_bits);

    // Extract location if encoded (depends on beacon type and protocol)
    beacon.location = decode_location(data);

    // Extract country code (bits 1-10)
    beacon.country_code = decode_country_code(data);

    // Set timestamp
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    beacon.timestamp = datetime;

    return beacon;
}

EPIRBLocation EPIRBDecoder::decode_location(const std::array<uint8_t, 16>& data) {
    // EPIRB location encoding varies by protocol version
    // This is a simplified decoder for the most common format

    // Check for location data presence (bit patterns vary)
    if ((data[12] & 0x80) == 0) {
        return EPIRBLocation();  // No location data
    }

    // Extract latitude (simplified - actual encoding is more complex)
    int32_t lat_raw = ((data[12] & 0x7F) << 10) | (data[13] << 2) | ((data[14] >> 6) & 0x03);
    if (lat_raw & 0x10000) lat_raw |= 0xFFFE0000;  // Sign extend
    float latitude = lat_raw * (180.0f / 131072.0f);

    // Extract longitude (simplified - actual encoding is more complex)
    int32_t lon_raw = ((data[14] & 0x3F) << 12) | (data[15] << 4) | ((data[0] >> 4) & 0x0F);
    if (lon_raw & 0x20000) lon_raw |= 0xFFFC0000;  // Sign extend
    float longitude = lon_raw * (360.0f / 262144.0f);

    // Validate coordinates
    if (latitude < -90.0f || latitude > 90.0f || longitude < -180.0f || longitude > 180.0f) {
        return EPIRBLocation();  // Invalid coordinates
    }

    return EPIRBLocation(latitude, longitude);
}

BeaconType EPIRBDecoder::decode_beacon_type(uint8_t type_bits) {
    switch (type_bits) {
        case 0:
            return BeaconType::OrbitingLocationBeacon;
        case 1:
            return BeaconType::PersonalLocatorBeacon;
        case 2:
            return BeaconType::EmergencyLocatorTransmitter;
        case 3:
            return BeaconType::SerialELT;
        case 4:
            return BeaconType::NationalELT;
        default:
            return BeaconType::Other;
    }
}

EmergencyType EPIRBDecoder::decode_emergency_type(uint8_t emergency_bits) {
    switch (emergency_bits) {
        case 0:
            return EmergencyType::Fire;
        case 1:
            return EmergencyType::Flooding;
        case 2:
            return EmergencyType::Collision;
        case 3:
            return EmergencyType::Grounding;
        case 4:
            return EmergencyType::Sinking;
        case 5:
            return EmergencyType::Disabled;
        case 6:
            return EmergencyType::Abandoning;
        case 7:
            return EmergencyType::Piracy;
        case 8:
            return EmergencyType::Man_Overboard;
        default:
            return EmergencyType::Other;
    }
}

uint32_t EPIRBDecoder::decode_country_code(const std::array<uint8_t, 16>& data) {
    // Country code is in bits 1-10 (ITU country code)
    return ((data[0] & 0x03) << 8) | data[1];
}

std::string EPIRBDecoder::decode_vessel_name(const std::array<uint8_t, 16>& /* data */) {
    // Vessel name extraction depends on beacon type and protocol
    // This is a placeholder - actual implementation would be more complex
    return "";
}

PacketStatus EPIRBDecoder::perform_bch_check(std::array<uint8_t, 16>& data, uint8_t& error_count) {
    // Make a copy to detect changes
    std::array<uint8_t, 16> original_data = data;

    // Calculate BCH syndrome
    uint32_t syndrome = calculate_bch_syndrome(data);

    if (syndrome == 0) {
        // No errors detected
        error_count = 0;
        return PacketStatus::Valid;
    }

    // Try to correct single-bit error
    if (correct_single_error(data, syndrome)) {
        // Successfully corrected
        error_count = count_bit_errors(original_data, data);
        return PacketStatus::Corrected;
    }

    // Multiple errors or uncorrectable
    error_count = 255;  // Indicate unknown error count
    return PacketStatus::Error;
}

uint32_t EPIRBDecoder::calculate_bch_syndrome(const std::array<uint8_t, 16>& data) {
    // BCH(127,92,5) polynomial for EPIRB: x^35 + x^2 + x + 1
    // This is a simplified implementation - actual EPIRB uses BCH(63,21,6)
    uint32_t syndrome = 0;
    uint32_t polynomial = 0x80000007;  // x^31 + x^2 + x + 1 (simplified)

    // Process each byte of the data
    for (int i = 0; i < 14; i++) {  // Only data bits, not parity
        uint32_t byte_val = data[i];
        for (int bit = 7; bit >= 0; bit--) {
            syndrome <<= 1;
            if (byte_val & (1 << bit)) {
                syndrome |= 1;
            }

            // XOR with polynomial if MSB is set
            if (syndrome & 0x80000000) {
                syndrome ^= polynomial;
            }
        }
    }

    // XOR with parity bits
    syndrome ^= (data[14] << 8) | data[15];

    return syndrome & 0xFFFF;  // 16-bit syndrome
}

bool EPIRBDecoder::correct_single_error(std::array<uint8_t, 16>& data, uint32_t syndrome) {
    // Simplified single-error correction
    // This is a basic implementation - real BCH correction is more complex

    if (syndrome == 0) return true;  // No error

    // Look up table for single-bit error patterns (simplified)
    // In a real implementation, this would be a proper BCH syndrome table
    for (int byte_idx = 0; byte_idx < 14; byte_idx++) {
        for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
            // Create test error pattern
            std::array<uint8_t, 16> test_data = data;
            test_data[byte_idx] ^= (1 << bit_idx);

            // Check if this correction produces zero syndrome
            if (calculate_bch_syndrome(test_data) == 0) {
                // Found the error location, apply correction
                data[byte_idx] ^= (1 << bit_idx);
                return true;
            }
        }
    }

    return false;  // Could not correct
}

uint8_t EPIRBDecoder::count_bit_errors(const std::array<uint8_t, 16>& original, const std::array<uint8_t, 16>& corrected) {
    uint8_t count = 0;
    for (size_t i = 0; i < 16; i++) {
        uint8_t diff = original[i] ^ corrected[i];
        // Count set bits in diff
        while (diff) {
            count += diff & 1;
            diff >>= 1;
        }
    }
    return count;
}

void EPIRBLogger::on_packet(const EPIRBBeacon& beacon) {
    std::string entry = "EPIRB," +
                        to_string_dec_uint(beacon.beacon_id, 15, '0') + "," +
                        to_string_dec_uint(static_cast<uint8_t>(beacon.beacon_type)) + "," +
                        to_string_dec_uint(static_cast<uint8_t>(beacon.emergency_type)) + ",";

    if (beacon.location.valid) {
        entry += to_string_decimal(beacon.location.latitude, 6) + "," +
                 to_string_decimal(beacon.location.longitude, 6);
    } else {
        entry += ",";
    }

    entry += "," + to_string_dec_uint(beacon.country_code) + "," +
             format_packet_status(beacon.packet_status) + "," +
             to_string_dec_uint(beacon.error_count) + "\n";

    log_file.write_entry(beacon.timestamp, entry);
}

std::string format_beacon_type(BeaconType type) {
    switch (type) {
        case BeaconType::OrbitingLocationBeacon:
            return "OLB";
        case BeaconType::PersonalLocatorBeacon:
            return "PLB";
        case BeaconType::EmergencyLocatorTransmitter:
            return "ELT";
        case BeaconType::SerialELT:
            return "S-ELT";
        case BeaconType::NationalELT:
            return "N-ELT";
        default:
            return "Other";
    }
}

std::string format_emergency_type(EmergencyType type) {
    switch (type) {
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
    }
}

std::string format_packet_status(PacketStatus status) {
    switch (status) {
        case PacketStatus::Valid:
            return "OK";
        case PacketStatus::Corrected:
            return "CORR";
        case PacketStatus::Error:
            return "ERR";
        default:
            return "UNK";
    }
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

EPIRBBeaconDetailView::EPIRBBeaconDetailView(ui::NavigationView& nav) {
    add_children({&button_done,
                  &button_see_map});

    button_done.on_select = [this](Button&) {
        if (on_close) on_close();
    };

    button_see_map.on_select = [this, &nav](Button&) {
        if (beacon_.location.valid) {
            nav.push<GeoMapView>(
                to_string_hex(beacon_.beacon_id, 8),  // tag as string
                0,                                    // altitude
                GeoPos::alt_unit::METERS,
                GeoPos::spd_unit::NONE,
                beacon_.location.latitude,
                beacon_.location.longitude,
                0,  // angle
                [this]() {
                    if (on_close) on_close();
                });
        }
    };
}

void EPIRBBeaconDetailView::set_beacon(const EPIRBBeacon& beacon) {
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
                             "Beacon ID", to_string_hex(beacon_.beacon_id, 15))
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Type", format_beacon_type(beacon_.beacon_type))
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Emergency", format_emergency_type(beacon_.emergency_type))
                      .location();

    if (beacon_.location.valid) {
        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Latitude", to_string_decimal(beacon_.location.latitude, 6) + "°")
                          .location();

        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Longitude", to_string_decimal(beacon_.location.longitude, 6) + "°")
                          .location();
    } else {
        draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                                 "Location", "Unknown")
                          .location();
    }

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Country", to_string_dec_uint(beacon_.country_code))
                      .location();

    draw_cursor = draw_field(painter, {draw_cursor, {200, 16}}, s,
                             "Time", to_string_datetime(beacon_.timestamp, HMS))
                      .location();

    // Show packet status with appropriate color
    std::string status_text = format_packet_status(beacon_.packet_status);
    if (beacon_.error_count > 0 && beacon_.packet_status == PacketStatus::Corrected) {
        status_text += " (" + to_string_dec_uint(beacon_.error_count) + " err)";
    }
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

    // Configure receiver for default EPIRB frequency (406.028 MHz)
    receiver_model.set_target_frequency(406028000);
    receiver_model.set_rf_amp(true);
    receiver_model.set_lna(32);
    receiver_model.set_vga(32);
    receiver_model.set_sampling_rate(2457600);
    receiver_model.enable();

    logger = std::make_unique<EPIRBLogger>();
    if (logger) {
        logger->append(logs_dir / "epirb_rx.txt");
    }
}

EPIRBAppView::~EPIRBAppView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;

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
    // Decode the EPIRB packet
    auto beacon = EPIRBDecoder::decode_packet(packet);

    if (beacon.beacon_id != 0) {  // Valid beacon decoded
        on_beacon_decoded(beacon);
    }
}

void EPIRBAppView::on_beacon_decoded(const EPIRBBeacon& beacon) {
    beacons_received++;

    // Track packet statistics
    switch (beacon.packet_status) {
        case PacketStatus::Valid:
            packets_valid++;
            break;
        case PacketStatus::Corrected:
            packets_corrected++;
            break;
        case PacketStatus::Error:
            packets_error++;
            break;
    }

    recent_beacons.push_back(beacon);

    // Keep only last 50 beacons
    if (recent_beacons.size() > 50) {
        recent_beacons.erase(recent_beacons.begin());
    }

    // Update display
    update_display();

    // Log the beacon
    if (logger) {
        logger->on_packet(beacon);
    }

    // Display in console with full details and colored status
    std::string beacon_info = format_beacon_summary(beacon);
    if (beacon.emergency_type != EmergencyType::Other) {
        beacon_info += " [" + format_emergency_type(beacon.emergency_type) + "]";
    }

    // Add colored status indicator
    std::string status_color;
    switch (beacon.packet_status) {
        case PacketStatus::Valid:
            status_color = STR_COLOR_GREEN;
            break;
        case PacketStatus::Corrected:
            status_color = STR_COLOR_YELLOW;
            break;
        case PacketStatus::Error:
            status_color = STR_COLOR_RED;
            break;
        default:
            status_color = STR_COLOR_WHITE;
            break;
    }

    beacon_info += " [" + status_color + format_packet_status(beacon.packet_status) + STR_COLOR_WHITE + "]";
    if (beacon.error_count > 0 && beacon.packet_status == PacketStatus::Corrected) {
        beacon_info += " (" + to_string_dec_uint(beacon.error_count) + "e)";
    }

    console.write(beacon_info + "\n");
}

void EPIRBAppView::on_show_map() {
    if (!recent_beacons.empty()) {
        // Find latest beacon with valid location
        for (auto it = recent_beacons.rbegin(); it != recent_beacons.rend(); ++it) {
            if (it->location.valid) {
                // Create a GeoMapView with all beacon locations
                auto map_view = nav_.push<ui::GeoMapView>(
                    "EPIRB",  // tag
                    0,        // altitude
                    ui::GeoPos::alt_unit::METERS,
                    ui::GeoPos::spd_unit::NONE,
                    it->location.latitude,
                    it->location.longitude,
                    0  // angle
                );

                // Add all beacons with valid locations as markers
                for (const auto& beacon : recent_beacons) {
                    if (beacon.location.valid) {
                        ui::GeoMarker marker;
                        marker.lat = beacon.location.latitude;
                        marker.lon = beacon.location.longitude;
                        marker.angle = 0;
                        marker.tag = to_string_hex(beacon.beacon_id, 8) + " " +
                                     format_beacon_type(beacon.beacon_type);
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
    recent_beacons.clear();
    beacons_received = 0;
    packets_valid = 0;
    packets_corrected = 0;
    packets_error = 0;
    console.clear(true);
    update_display();
}

void EPIRBAppView::on_toggle_log() {
    // Toggle logging functionality
    if (logger) {
        logger.reset();
        button_log.set_text("Log");
    } else {
        logger = std::make_unique<EPIRBLogger>();
        logger->append("epirb_rx.txt");
        button_log.set_text("Stop");
    }
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

    if (!recent_beacons.empty()) {
        const auto& latest = recent_beacons.back();
        text_latest_info.set(format_beacon_summary(latest));
    }
}

std::string EPIRBAppView::format_beacon_summary(const EPIRBBeacon& beacon) {
    std::string summary = to_string_hex(beacon.beacon_id, 8) + " " +
                          format_beacon_type(beacon.beacon_type);

    if (beacon.location.valid) {
        summary += " " + format_location(beacon.location);
    }

    // Add status indicator for summary display
    summary += " " + format_packet_status(beacon.packet_status);

    return summary;
}

std::string EPIRBAppView::format_location(const EPIRBLocation& location) {
    return to_string_decimal(location.latitude, 4) + "°," +
           to_string_decimal(location.longitude, 4) + "°";
}

}  // namespace ui::external_app::epirb_rx