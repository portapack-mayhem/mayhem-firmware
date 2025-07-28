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

#ifndef __UI_EPIRB_RX_H__
#define __UI_EPIRB_RX_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "ui_geomap.hpp"

#include "event_m0.hpp"
#include "log_file.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "baseband_packet.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <array>

namespace epirb {

// EPIRB 406 MHz beacon types
enum class BeaconType : uint8_t {
    OrbitingLocationBeacon = 0,
    PersonalLocatorBeacon = 1,
    EmergencyLocatorTransmitter = 2,
    SerialELT = 3,
    NationalELT = 4,
    Other = 15
};

// EPIRB distress and emergency types
enum class EmergencyType : uint8_t {
    Fire = 0,
    Flooding = 1,
    Collision = 2,
    Grounding = 3,
    Sinking = 4,
    Disabled = 5,
    Abandoning = 6,
    Piracy = 7,
    Man_Overboard = 8,
    Other = 15
};

struct EPIRBLocation {
    float latitude;   // degrees, -90 to +90
    float longitude;  // degrees, -180 to +180  
    bool valid;
    
    EPIRBLocation() : latitude(0.0f), longitude(0.0f), valid(false) {}
    EPIRBLocation(float lat, float lon) : latitude(lat), longitude(lon), valid(true) {}
};

struct EPIRBBeacon {
    uint32_t beacon_id;
    BeaconType beacon_type;
    EmergencyType emergency_type;
    EPIRBLocation location;
    uint32_t country_code;
    std::string vessel_name;
    rtc::RTC timestamp;
    uint32_t sequence_number;
    
    EPIRBBeacon() : beacon_id(0), beacon_type(BeaconType::Other), 
                   emergency_type(EmergencyType::Other), country_code(0), 
                   sequence_number(0) {}
};

class EPIRBDecoder {
public:
    static EPIRBBeacon decode_packet(const baseband::Packet& packet);
    
private:
    static EPIRBLocation decode_location(const std::array<uint8_t, 16>& data);
    static BeaconType decode_beacon_type(uint8_t type_bits);
    static EmergencyType decode_emergency_type(uint8_t emergency_bits);
    static uint32_t decode_country_code(const std::array<uint8_t, 16>& data);
    static std::string decode_vessel_name(const std::array<uint8_t, 16>& data);
};

} // namespace epirb

class EPIRBLogger {
public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }
    
    void on_packet(const epirb::EPIRBBeacon& beacon);
    
private:
    LogFile log_file{};
};

namespace ui {

class EPIRBBeaconDetailView : public View {
public:
    std::function<void(void)> on_close{};
    
    EPIRBBeaconDetailView(NavigationView& nav);
    
    void set_beacon(const epirb::EPIRBBeacon& beacon);
    const epirb::EPIRBBeacon& beacon() const { return beacon_; }
    
    void focus() override;
    void paint(Painter&) override;
    
    GeoMapView* get_geomap_view() { return geomap_view; }
    
private:
    epirb::EPIRBBeacon beacon_{};
    
    Button button_done{
        {125, 224, 96, 24},
        "Done"
    };
    Button button_see_map{
        {19, 224, 96, 24},
        "See on map"
    };
    
    GeoMapView* geomap_view{nullptr};
    
    Rect draw_field(
        Painter& painter,
        const Rect& draw_rect,
        const Style& style,
        const std::string& label,
        const std::string& value
    );
};

class EPIRBAppView : public View {
public:
    EPIRBAppView(NavigationView& nav);
    ~EPIRBAppView();
    
    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override;
    void focus() override;
    
    std::string title() const override { return "EPIRB RX"; }
    
private:
    RxRadioState radio_state_{
        406028000 /* 406.028 MHz center frequency */,
        25000 /* 25 kHz bandwidth */,
        2457600 /* 2.4576 MHz sampling rate */
    };
    app_settings::SettingsManager settings_{
        "rx_epirb", app_settings::Mode::RX
    };
    
    NavigationView& nav_;
    
    std::vector<epirb::EPIRBBeacon> recent_beacons{};
    std::unique_ptr<EPIRBLogger> logger{};
    
    EPIRBBeaconDetailView beacon_detail_view{nav_};
    
    static constexpr auto header_height = 3 * 16;
    
    Text label_frequency{
        {0 * 8, 0 * 16, 10 * 8, 1 * 16},
        "406.028 MHz"
    };
    
    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}
    };
    
    LNAGainField field_lna{
        {15 * 8, 0 * 16}
    };
    
    VGAGainField field_vga{
        {18 * 8, 0 * 16}
    };
    
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}
    };
    
    AudioVolumeField field_volume{
        {screen_width - 2 * 8, 0 * 16}
    };
    
    Channel channel{
        {21 * 8, 5, 6 * 8, 4}
    };
    
    // Status display
    Text label_status{
        {0 * 8, 1 * 16, 15 * 8, 1 * 16},
        "Listening..."
    };
    
    Text label_beacons_count{
        {16 * 8, 1 * 16, 14 * 8, 1 * 16},
        "Beacons: 0"
    };
    
    // Latest beacon info display
    Text label_latest{
        {0 * 8, 2 * 16, 8 * 8, 1 * 16},
        "Latest:"
    };
    
    Text text_latest_info{
        {8 * 8, 2 * 16, 22 * 8, 1 * 16},
        ""
    };
    
    // Beacon list
    Console console{
        {0, 3 * 16, 240, 168}
    };
    
    Button button_map{
        {0, 224, 60, 24},
        "Map"
    };
    
    Button button_clear{
        {64, 224, 60, 24},
        "Clear"
    };
    
    Button button_log{
        {128, 224, 60, 24},
        "Log"
    };
    
    SignalToken signal_token_tick_second{};
    uint32_t beacons_received = 0;
    
    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) {
            const auto message = static_cast<const EPIRBPacketMessage*>(p);
            this->on_packet(message->packet);
        }
    };
    
    void on_packet(const baseband::Packet& packet);
    void on_beacon_decoded(const epirb::EPIRBBeacon& beacon);
    void on_show_map();
    void on_clear_beacons();
    void on_toggle_log();
    void on_tick_second();
    
    void update_display();
    std::string format_beacon_summary(const epirb::EPIRBBeacon& beacon);
    std::string format_location(const epirb::EPIRBLocation& location);
    std::string format_beacon_type(epirb::BeaconType type);
    std::string format_emergency_type(epirb::EmergencyType type);
};

} // namespace ui

#endif // __UI_EPIRB_RX_H__