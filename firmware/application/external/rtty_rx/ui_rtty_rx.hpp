/*
 * Copyright (C) 2026 HTotoo
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

#ifndef __UI_RTTY_RX_H__
#define __UI_RTTY_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include "baudot.hpp"

using namespace ui;

namespace ui::external_app::rtty_rx {

class RttyRxView : public View {
   public:
    RttyRxView(NavigationView& nav);
    ~RttyRxView();

    void focus() override;

    std::string title() const override { return "RTTY"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    uint32_t baud_conf = 0;
    app_settings::SettingsManager settings_{
        "rx_rtty",
        app_settings::Mode::RX,
        {
            {"baud_conf"sv, &baud_conf},
        }};

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
    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Baud:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_baud{
        {UI_POS_X(7), UI_POS_Y(1)},
        5,
        {{"Auto", 0},
         {"45", 4500},
         {"45.45", 4545},
         {"50", 5000},
         {"75", 7500},
         {"100", 10000},
         {"110", 11000},
         {"150", 15000},
         {"200", 20000}}};

    Console console{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}};

    BaudotCoder baudot_decoder{};

    void on_data(const RTTYDataMessage* message);
    void got_message(std::string msg);

    MessageHandlerRegistration message_handler_data{
        Message::ID::RTTYData,
        [this](Message* const p) {
            const auto message = static_cast<const RTTYDataMessage*>(p);
            this->on_data(message);
        }};
};

}  // namespace ui::external_app::rtty_rx

#endif /*__UI_RTTY_RX_H__*/