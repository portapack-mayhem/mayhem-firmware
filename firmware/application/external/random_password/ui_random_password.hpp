/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 zxkmm
 * Copyright (C) 2024 HTotoo
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

#ifndef __UI_RANDOM_PASSWORD_H__
#define __UI_RANDOM_PASSWORD_H__

#define MAX_DIGITS 30

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
#include "sha512.h"

#include <deque>
#include <string>

using namespace ui;

namespace ui::external_app::random_password {

enum Method {
    RADIO_LCG_ROLL,
    RADIO_LCG_ROLL_HASH,
};

class RandomPasswordLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

class RandomPasswordView : public View {
   public:
    RandomPasswordView(NavigationView& nav);
    ~RandomPasswordView();

    void focus() override;

    std::string title() const override { return "R.passwd"; };

   private:
    unsigned int seed = 0;  // extern void srand (unsigned int __seed) __THROW;
    std::string password = "";

    std::deque<unsigned int> seeds_deque = {0};
    bool seeds_buffer_not_full = true;
    bool in_benchmark = false;
    bool flooding = false;
    bool logging = false;
    bool async_prev_val = false;
    std::string str_log{""};

    void on_data(uint32_t value, bool is_data);
    void clean_buffer();
    void new_password();
    std::string generate_log_line();
    void paint_password_hints();

    bool islower(char c);
    bool isupper(char c);
    bool isdigit(char c);

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_passgen", app_settings::Mode::RX};

    Labels labels{
        {{UI_POS_X_CENTER(30), UI_POS_Y(0)}, "------------seeds-------------", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_CENTER(30), 3 * 16}, "-----------password-----------", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 7 * 16 - 2}, "digits:", Theme::getInstance()->fg_light->foreground},
        {{screen_width / 2, 7 * 16 - 2}, "method:", Theme::getInstance()->fg_light->foreground},
    };

    RFAmpField field_rf_amp{
        {13 * 8, 1 * 16}};
    LNAGainField field_lna{
        {15 * 8, 1 * 16}};
    VGAGainField field_vga{
        {18 * 8, 1 * 16}};

    RSSI rssi{
        {UI_POS_X(21), 1 * 16 + 0, UI_POS_WIDTH_REMAINING(21), 4}};
    Channel channel{
        {UI_POS_X(21), 1 * 16 + 5, UI_POS_WIDTH_REMAINING(21), 4}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), 1 * 16},
        nav_};

    Button button_modem_setup{
        {screen_width - 12 * 8, 2 * 16 - 1, 96, 16 + 2},
        LanguageHelper::currentMessages[LANG_MODEM_SETUP]};

    Text text_seed{
        {0, 2 * 16, 10 * 8, 16},
        "0000000000"};

    ProgressBar progressbar{
        {10 * 8 + 2, 2 * 16, screen_width - 96 - (10 * 8 + 4) - 1, 16}};

    Text text_generated_passwd{
        {0, 4 * 16, screen_width, 16},
        "000000000000000000000000000000"};

    Text text_char_type_hints{
        {0, 5 * 16 + 4, screen_width, 16},
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"};

    Checkbox check_show_seeds{
        {17 * 8, 8 * 16},
        6,
        "Show seed"};

    Checkbox check_auto_send{
        {1 * 8, 8 * 16},
        20,
        "Auto send"};

    Checkbox check_punctuation{
        {17 * 8, 12 * 16},
        6,
        ".,-!?"};

    Checkbox check_allow_confusable_chars{
        {1 * 8, 10 * 16},
        20,
        "0 O o 1 l"};

    Checkbox check_digits{
        {1 * 8, 12 * 16},
        3,
        "123"};

    Checkbox check_latin_lower{
        {1 * 8, 14 * 16},
        3,
        "abc"};

    Checkbox check_latin_upper{
        {17 * 8, 14 * 16},
        3,
        "ABC"};

    Checkbox check_log{
        {17 * 8, 10 * 16},
        3,
        LanguageHelper::currentMessages[LANG_SAVE]};

    Button button_flood{
        {UI_POS_X(0), 15 * 16 + 18, screen_width / 2, 22},
        LanguageHelper::currentMessages[LANG_FLOOD]};

    Button button_send{
        {screen_width / 2 + 2, 15 * 16 + 18, screen_width / 2 - 2, 22},
        "Send pwd"};

    Button button_refresh{
        {UI_POS_X(0), 17 * 16 + 10, screen_width / 2, 22},
        "Generate"};

    Button button_show_qr{
        {screen_width / 2 + 2, 17 * 16 + 10, screen_width / 2 - 2, 22},
        LanguageHelper::currentMessages[LANG_SHOWQR]};

    NumberField field_digits{
        {0 + (sizeof("digits:") - 1) * 8, 7 * 16 - 2},
        2,
        {1, 30},
        1,
        ' '};

    OptionsField field_method{
        {(screen_width / 2) + (int)(sizeof("method:") - 1) * 8, 7 * 16 - 2},
        sizeof("R+L+R+H"),
        {{"R+L+R", Method::RADIO_LCG_ROLL},
         {"R+L+R+H", Method::RADIO_LCG_ROLL_HASH}}};

    void on_data_afsk(const AFSKDataMessage& message);

    std::unique_ptr<RandomPasswordLogger> logger{};

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
    void set_random_freq();
};

}  // namespace ui::external_app::random_password

#endif /*__UI_RANDOM_PASSWORD_H__*/
