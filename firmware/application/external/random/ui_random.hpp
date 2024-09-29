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
#include "ui_qrcode.hpp"
#include "usb_serial_asyncmsg.hpp"


using namespace ui;

namespace ui::external_app::random {

class RandomLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

class RandomView : public View {
   public:
    RandomView(NavigationView& nav);
    ~RandomView();

    void focus() override;

    std::string title() const override { return "random"; };

   private:
    unsigned int seed = 0;  // extern void srand (unsigned int __seed) __THROW;
    std::string password = "";

    void on_data(uint32_t value, bool is_data);
    void new_password();
    std::string generate_log_line();

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_afsk", app_settings::Mode::RX};

    uint8_t console_color{0};
    uint32_t prev_value{0};
    std::string str_log{""};
    bool logging{false};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};
    Channel channel{
        {21 * 8, 5, 6 * 8, 4}};



    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    Checkbox check_log{
        {0 * 8, 1 * 16},
        3,
        "save gened pw",
        false};

    Text text_debug{
        {0 * 8, 12 + 2 * 16, screen_width, 16},
        LanguageHelper::currentMessages[LANG_DEBUG]};

    Button button_modem_setup{
        {screen_width - 12 * 8, 1 * 16, 96, 24},
        "AFSK modem"};

    Console console{
        {0, 3 * 16, 1, 16}};
    Text text_seed{
        {0, 4 * 16, 240, 16},
        "test seed"
    };

    Labels labels{
        {{5 * 8, 9 * 16}, "digits:", Theme::getInstance()->fg_light->foreground}};

    Text text_generated_passwd{
        {0, 5 * 16, screen_width, 28},
        "000000000000000000000000000000"};

    Text text_char_type_hints{
        {0, 6 * 16, screen_width, 28},
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"};

    Checkbox check_digits{
        {3 * 8, 13 * 16},
        3,
        "123"};

    Checkbox check_punctuation{
        {20 * 8, 13 * 16},
        6,
        ".,-!?"};

    Checkbox check_latin_lower{
        {3 * 8, 15 * 16},
        3,
        "abc"};

    Checkbox check_latin_upper{
        {20 * 8, 15 * 16},
        3,
        "ABC"};

    Checkbox check_allow_confusable_chars{
        {3 * 8, 11 * 16},
        20,
        "Include 0 O o 1 l"};

    Button button_refresh{
        {0 * 8, 17 * 16 + 10, screen_width / 2, 24},
        "refresh"};

    Button button_show_qr{
        {screen_width / 2, 17 * 16 + 10, screen_width / 2, 24},
        "show QR"};

    NumberField field_digits{
        {24 * 8, 9 * 16},
        2,
        {1, 30},
        1,
        ' '};

    void on_data_afsk(const AFSKDataMessage& message);

    std::unique_ptr<RandomLogger> logger{};

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

    bool seed_protect_helper();

    void on_freqchg(int64_t freq);
    void set_random_freq();
};

}  // namespace ui::external_app::random

#endif /*__UI_AFSK_RX_H__*/
