/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_AFSK_RX_H__
#define __UI_AFSK_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::afsk_rx {

class AFSKLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

class AFSKRxView : public View {
   public:
    AFSKRxView(NavigationView& nav);
    ~AFSKRxView();

    void focus() override;

    std::string title() const override { return "AFSK RX"; };

   private:
    void on_data(uint32_t value, bool is_data);

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_afsk", app_settings::Mode::RX};

    uint8_t console_color{0};
    uint32_t prev_value{0};
    std::string str_log{""};
    bool logging{false};

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

    AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y(0)}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    Checkbox check_log{
        {UI_POS_X(0), 1 * 16},
        3,
        LanguageHelper::currentMessages[LANG_LOG],
        false};

    Text text_debug{
        {UI_POS_X(0), 12 + 2 * 16, screen_width, 16},
        LanguageHelper::currentMessages[LANG_DEBUG]};

    Button button_modem_setup{
        {screen_width - 12 * 8, 1 * 16, 96, 24},
        LanguageHelper::currentMessages[LANG_MODEM_SETUP]};

    Console console{
        {0, 4 * 16, screen_width, screen_width}};

    void on_data_afsk(const AFSKDataMessage& message);

    std::unique_ptr<AFSKLogger> logger{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::AFSKData,
        [this](Message* const p) {
            const auto message = static_cast<const AFSKDataMessage*>(p);
            this->on_data(message->value, message->is_data);
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void on_freqchg(int64_t freq);
};

}  // namespace ui::external_app::afsk_rx

#endif /*__UI_AFSK_RX_H__*/
