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

#ifndef __UI_SONDE_H__
#define __UI_SONDE_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "ui_qrcode.hpp"
#include "ui_geomap.hpp"

#include "event_m0.hpp"

#include "log_file.hpp"

#include "sonde_packet.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include <cstddef>
#include <string>

class SondeLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(const sonde::Packet& packet);

   private:
    LogFile log_file{};
};

namespace ui {

class SondeView : public View {
   public:
    static constexpr uint32_t initial_target_frequency = 402700000;

    SondeView(NavigationView& nav);
    ~SondeView();
    SondeView(const SondeView& other) = delete;
    SondeView& operator=(const SondeView& other) = delete;

    void focus() override;

    std::string title() const override { return "Radiosnd RX"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{
        402700000 /* frequency */,
        1750000 /* bandwidth */,
        2457600 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "rx_sonde", app_settings::Mode::RX};

    std::unique_ptr<SondeLogger> logger{};
    bool logging{false};
    bool use_crc{false};
    bool beep{false};

    char geo_uri[32] = {};

    sonde::GPS_data gps_info{};
    sonde::temp_humid temp_humid_info{};
    std::string sonde_id{};

    // AudioOutput audio_output { };

    Labels labels{
        {{4 * 8, 2 * 16}, "Type:", Color::light_grey()},
        {{6 * 8, 3 * 16}, "ID:", Color::light_grey()},
        {{0 * 8, 4 * 16}, "DateTime:", Color::light_grey()},

        {{3 * 8, 5 * 16}, "Vbatt:", Color::light_grey()},
        {{3 * 8, 6 * 16}, "Frame:", Color::light_grey()},
        {{4 * 8, 7 * 16}, "Temp:", Color::light_grey()},
        {{0 * 8, 8 * 16}, "Humidity:", Color::light_grey()}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 8},
        nav_};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};

    LNAGainField field_lna{
        {15 * 8, 0 * 16}};

    VGAGainField field_vga{
        {18 * 8, 0 * 16}};

    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    Checkbox check_beep{
        {22 * 8, 6 * 16},
        3,
        "Beep"};

    Checkbox check_log{
        {22 * 8, 8 * 16},
        3,
        "Log"};

    Checkbox check_crc{
        {22 * 8, 10 * 16},
        3,
        "CRC"};

    Text text_signature{
        {9 * 8, 2 * 16, 10 * 8, 16},
        "..."};

    Text text_serial{
        {9 * 8, 3 * 16, 11 * 8, 16},
        "..."};

    Text text_timestamp{
        {9 * 8, 4 * 16, 11 * 8, 16},
        "..."};

    Text text_voltage{
        {9 * 8, 5 * 16, 10 * 8, 16},
        "..."};

    Text text_frame{
        {9 * 8, 6 * 16, 10 * 8, 16},
        "..."};

    Text text_temp{
        {9 * 8, 7 * 16, 10 * 8, 16},
        "..."};

    Text text_humid{
        {9 * 8, 8 * 16, 10 * 8, 16},
        "..."};

    GeoPos geopos{
        {0, 12 * 16},
        GeoPos::alt_unit::METERS,
        GeoPos::spd_unit::HIDDEN};

    Button button_see_qr{
        {2 * 8, 15 * 16, 12 * 8, 3 * 16},
        "See QR"};

    Button button_see_map{
        {16 * 8, 15 * 16, 12 * 8, 3 * 16},
        "See on map"};

    GeoMapView* geomap_view_{nullptr};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::SondePacket,
        [this](Message* const p) {
            const auto message = static_cast<const SondePacketMessage*>(p);
            const sonde::Packet packet{message->packet, message->type};
            this->on_packet(packet);
        }};

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

    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);
    void on_packet(const sonde::Packet& packet);
    char* float_to_char(float x, char* p);
};

} /* namespace ui */

#endif /*__UI_SONDE_H__*/
