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

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"

#include "event_m0.hpp"
#include "signal.hpp"
#include "message.hpp"
#include "log_file.hpp"

#include "baseband_packet.hpp"

/* #include <cstdint>
#include <cstddef>
#include <string>
#include <array> */

namespace ui::external_app::epirb_rx {

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

    EPIRBLocation()
        : latitude(0.0f), longitude(0.0f), valid(false) {}
    EPIRBLocation(float lat, float lon)
        : latitude(lat), longitude(lon), valid(true) {}
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

    EPIRBBeacon()
        : beacon_id(0), beacon_type(BeaconType::Other), emergency_type(EmergencyType::Other), location(), country_code(0), vessel_name(), timestamp(), sequence_number(0) {}
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

class EPIRBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(const EPIRBBeacon& beacon);

   private:
    LogFile log_file{};
};

// Forward declarations of formatting functions
std::string format_beacon_type(BeaconType type);
std::string format_emergency_type(EmergencyType type);

class EPIRBBeaconDetailView : public ui::View {
   public:
    std::function<void(void)> on_close{};

    EPIRBBeaconDetailView(ui::NavigationView& nav);
    EPIRBBeaconDetailView(const EPIRBBeaconDetailView&) = delete;
    EPIRBBeaconDetailView& operator=(const EPIRBBeaconDetailView&) = delete;

    void set_beacon(const EPIRBBeacon& beacon);
    const EPIRBBeacon& beacon() const { return beacon_; }

    void focus() override;
    void paint(ui::Painter&) override;

    ui::GeoMapView* get_geomap_view() { return geomap_view; }

   private:
    EPIRBBeacon beacon_{};

    ui::Button button_done{
        {125, 224, 96, 24},
        "Done"};
    ui::Button button_see_map{
        {19, 224, 96, 24},
        "See on map"};

    ui::GeoMapView* geomap_view{nullptr};

    ui::Rect draw_field(
        ui::Painter& painter,
        const ui::Rect& draw_rect,
        const ui::Style& style,
        const std::string& label,
        const std::string& value);
};

class EPIRBAppView : public ui::View {
   public:
    EPIRBAppView(ui::NavigationView& nav);
    ~EPIRBAppView();

    void set_parent_rect(const ui::Rect new_parent_rect) override;
    void paint(ui::Painter&) override;
    void focus() override;

    std::string title() const override { return "EPIRB RX"; }

   private:
    app_settings::SettingsManager settings_{
        "rx_epirb", app_settings::Mode::RX};

    ui::NavigationView& nav_;

    std::vector<EPIRBBeacon> recent_beacons{};
    std::unique_ptr<EPIRBLogger> logger{};

    EPIRBBeaconDetailView beacon_detail_view{nav_};

    static constexpr auto header_height = 3 * 16;

    ui::Text label_frequency{
        {0 * 8, 0 * 16, 10 * 8, 1 * 16},
        "406.028 MHz"};

    ui::RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};

    ui::LNAGainField field_lna{
        {15 * 8, 0 * 16}};

    ui::VGAGainField field_vga{
        {18 * 8, 0 * 16}};

    ui::RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    ui::AudioVolumeField field_volume{
        {screen_width - 2 * 8, 0 * 16}};

    ui::Channel channel{
        {21 * 8, 5, 6 * 8, 4}};

    // Status display
    ui::Text label_status{
        {0 * 8, 1 * 16, 15 * 8, 1 * 16},
        "Listening..."};

    ui::Text label_beacons_count{
        {16 * 8, 1 * 16, 14 * 8, 1 * 16},
        "Beacons: 0"};

    // Latest beacon info display
    ui::Text label_latest{
        {0 * 8, 2 * 16, 8 * 8, 1 * 16},
        "Latest:"};

    ui::Text text_latest_info{
        {8 * 8, 2 * 16, 22 * 8, 1 * 16},
        ""};

    // Beacon list
    ui::Console console{
        {0, 3 * 16, 240, 168}};

    ui::Button button_map{
        {0, 224, 60, 24},
        "Map"};

    ui::Button button_clear{
        {64, 224, 60, 24},
        "Clear"};

    ui::Button button_log{
        {128, 224, 60, 24},
        "Log"};

    SignalToken signal_token_tick_second{};
    uint32_t beacons_received = 0;

    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) {
            const auto message = static_cast<const EPIRBPacketMessage*>(p);
            this->on_packet(message->packet);
        }};

    void on_packet(const baseband::Packet& packet);
    void on_beacon_decoded(const EPIRBBeacon& beacon);
    void on_show_map();
    void on_clear_beacons();
    void on_toggle_log();
    void on_tick_second();

    void update_display();
    std::string format_beacon_summary(const EPIRBBeacon& beacon);
    std::string format_location(const EPIRBLocation& location);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __UI_EPIRB_RX_H__