/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2020 Shao
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

#ifndef __UI_BTLE_RX_H__
#define __UI_BTLE_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_record_view.hpp"

#include "utility.hpp"

namespace ui {

class BTLERxView : public View {
   public:
    BTLERxView(NavigationView& nav);
    ~BTLERxView();

    void focus() override;

    std::string title() const override { return "BTLE RX"; };

   private:
    void on_data(uint32_t value, bool is_data);

    NavigationView& nav_;
    RxRadioState radio_state_{
        2426000000 /* frequency */,
        4000000 /* bandwidth */,
        4000000 /* sampling rate */,
        ReceiverModel::Mode::WidebandFMAudio};
    app_settings::SettingsManager settings_{
        "rx_btle", app_settings::Mode::RX};

    uint8_t console_color{0};
    uint32_t prev_value{0};

    static constexpr uint8_t channel_number = 37;

    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(21), 4}};
    Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(21), 4}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    Button button_modem_setup{
        {screen_width - 12 * 8, UI_POS_Y(1), 96, 24},
        "Modem setup"};

    Console console{
        {0, 4 * 16, screen_width, screen_height - 80}};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::AFSKData,
        [this](Message* const p) {
            const auto message = static_cast<const AFSKDataMessage*>(p);
            this->on_data(message->value, message->is_data);
        }};
};

} /* namespace ui */

#endif /*__UI_BTLE_RX_H__*/
