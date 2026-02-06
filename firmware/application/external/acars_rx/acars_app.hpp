/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __ACARS_APP_H__
#define __ACARS_APP_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "log_file.hpp"

namespace ui::external_app::acars_rx {

class ACARSLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }
    void log_str(std::string msg);

   private:
    LogFile log_file{};
};

class ACARSAppView : public View {
   public:
    ACARSAppView(NavigationView& nav);
    ~ACARSAppView();

    void focus() override;

    std::string title() const override { return "ACARS"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{
        131825000 /* frequency */,
        1750000 /* bandwidth */,
        2457600 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "rx_acars", app_settings::Mode::RX};

    bool logging{false};
    uint32_t packet_counter{0};

    RFAmpField field_rf_amp{
        {13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{
        {15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{
        {18 * 8, UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), 0 * 8},
        nav_};
    Checkbox check_log{
        {16 * 8, 1 * 16},
        3,
        "LOG",
        true};

    Console console{
        {0, 3 * 16, screen_width, 256}};

    AudioVolumeField field_volume{
        {screen_width - 2 * 8, 1 * 16}};

    std::unique_ptr<ACARSLogger> logger{};

    void on_packet(const ACARSPacketMessage* packet);

    MessageHandlerRegistration message_handler_packet{
        Message::ID::ACARSPacket,
        [this](Message* const p) {
            const auto message = static_cast<const ACARSPacketMessage*>(p);
            this->on_packet(message);
        }};
};

}  // namespace ui::external_app::acars_rx

#endif /*__ACARS_APP_H__*/