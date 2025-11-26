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
    bool logging{false};
    bool use_crc{false};
    app_settings::SettingsManager settings_{
        "rx_sonde",
        app_settings::Mode::RX,
        {
            {"logging"sv, &logging},
            {"use_crc"sv, &use_crc},
        }};

    std::unique_ptr<SondeLogger> logger{};

    sonde::GPS_data gps_info{};
    sonde::temp_humid temp_humid_info{};
    std::string sonde_id{};

    // AudioOutput audio_output { };

    Labels labels{
        {{UI_POS_X(4), UI_POS_Y(2)}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(6), UI_POS_Y(3)}, "ID:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(4)}, "DateTime:", Theme::getInstance()->fg_light->foreground},

        {{UI_POS_X(3), UI_POS_Y(5)}, "Vbatt:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(3), UI_POS_Y(6)}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(4), UI_POS_Y(7)}, "Temp:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "Humidity:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(9)}, "Pressure:", Theme::getInstance()->fg_light->foreground}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};

    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{
        {UI_POS_X(21), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(24), 4},
    };

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    Checkbox check_log{
        {UI_POS_X_RIGHT(8), UI_POS_Y(8)},
        3,
        "Log"};

    Checkbox check_crc{
        {UI_POS_X_RIGHT(8), UI_POS_Y(10)},
        3,
        "CRC"};

    Text text_signature{
        {UI_POS_X(9), UI_POS_Y(2), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};

    Text text_serial{
        {UI_POS_X(9), UI_POS_Y(3), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "..."};

    Text text_timestamp{
        {UI_POS_X(9), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)},
        "..."};

    Text text_voltage{
        {UI_POS_X(9), UI_POS_Y(5), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};

    Text text_frame{
        {UI_POS_X(9), UI_POS_Y(6), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};

    Text text_temp{
        {UI_POS_X(9), UI_POS_Y(7), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};

    Text text_humid{
        {UI_POS_X(9), UI_POS_Y(8), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};
    Text text_press{
        {UI_POS_X(9), UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "..."};

    GeoPos geopos{
        {UI_POS_X(0), UI_POS_Y(12)},
        GeoPos::alt_unit::METERS,
        GeoPos::spd_unit::HIDDEN};

    Button button_see_qr{
        {UI_POS_X_CENTER(12) - UI_POS_WIDTH(8), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
        "See QR"};

    Button button_see_map{
        {UI_POS_X_CENTER(12) + UI_POS_WIDTH(8), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
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
    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void on_freqchg(int64_t freq);

    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);
    void on_packet(const sonde::Packet& packet);
};

} /* namespace ui */

#endif /*__UI_SONDE_H__*/
