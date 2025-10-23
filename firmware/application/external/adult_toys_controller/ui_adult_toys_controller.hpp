/*
 * Copyright (C) 2025 Pezsma
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

// Code from https://github.com/Next-Flip/Momentum-Apps/blob/89d493daa1ea0cb86906fa0042c41dd4edc22a6a/ble_spam/protocols/lovespouse.c#L93
// Thanks for the work of the original creators!

#ifndef __UI_ADULT_TOYS_H__
#define __UI_ADULT_TOYS_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::adult_toys_controller {

enum PKT_TYPE {
    PKT_TYPE_INVALID_TYPE,
    PKT_TYPE_RAW,
    PKT_TYPE_DISCOVERY,
    PKT_TYPE_IBEACON,
    PKT_TYPE_ADV_IND,
    PKT_TYPE_ADV_DIRECT_IND,
    PKT_TYPE_ADV_NONCONN_IND,
    PKT_TYPE_ADV_SCAN_IND,
    PKT_TYPE_SCAN_REQ,
    PKT_TYPE_SCAN_RSP,
    PKT_TYPE_CONNECT_REQ,
    PKT_TYPE_LL_DATA,
    PKT_TYPE_LL_CONNECTION_UPDATE_REQ,
    PKT_TYPE_LL_CHANNEL_MAP_REQ,
    PKT_TYPE_LL_TERMINATE_IND,
    PKT_TYPE_LL_ENC_REQ,
    PKT_TYPE_LL_ENC_RSP,
    PKT_TYPE_LL_START_ENC_REQ,
    PKT_TYPE_LL_START_ENC_RSP,
    PKT_TYPE_LL_UNKNOWN_RSP,
    PKT_TYPE_LL_FEATURE_REQ,
    PKT_TYPE_LL_FEATURE_RSP,
    PKT_TYPE_LL_PAUSE_ENC_REQ,
    PKT_TYPE_LL_PAUSE_ENC_RSP,
    PKT_TYPE_LL_VERSION_IND,
    PKT_TYPE_LL_REJECT_IND,
    PKT_TYPE_NUM_PKT_TYPE
};

class AdultToysView : public ui::View {
   public:
    AdultToysView(NavigationView& nav);
    ~AdultToysView();

    void focus() override;
    void on_show() override;

    std::string title() const override {
        return "Adult Toys";
    };

    struct LovespouseMode {
        uint32_t value;
        std::string name;
    };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */
    };

    TxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    TransmitterView2 tx_view{
        {UI_POS_X(11), UI_POS_Y(0)},
        /*short_ui*/ true};

    app_settings::SettingsManager settings_{
        "Adult Toys", app_settings::Mode::TX};

    OptionsField options_target{
        {UI_POS_X(6), UI_POS_Y(1)},
        11,
        {{"LoveSpouse", 1}}};

    Button btn_adult{{UI_POS_X(0), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(3)}, "adult"};

    Button btn_child{{UI_POS_X_RIGHT(10), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(3)}, "child"};

    Button button_on{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)},
        LanguageHelper::currentMessages[LANG_START]};

    Button btn_mode_decrease{
        {UI_POS_X(0), UI_POS_Y(13), UI_POS_WIDTH(3), UI_POS_HEIGHT(2)},
        "<-"};

    Button btn_mode_increase{
        {UI_POS_X_RIGHT(3), UI_POS_Y(13), UI_POS_WIDTH(3), UI_POS_HEIGHT(2)},
        "->"};

    Text txt_selected_mode{{UI_POS_X_CENTER(22), UI_POS_Y(13.5), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}, ""};

    Button btn_pcknum_inc{
        {UI_POS_X(7), UI_POS_Y(9), UI_POS_WIDTH(2), UI_POS_HEIGHT(1.5)},
        "+"};

    Button btn_pcknum_dec{
        {UI_POS_X(0), UI_POS_Y(9), UI_POS_WIDTH(2), UI_POS_HEIGHT(1.5)},
        "-"};

    NumberField message_num{{UI_POS_X(3), UI_POS_Y(9.2)}, 3, {0, 999}, 1, ' ', true};

    Checkbox chk_mode_rnd{{UI_POS_X_RIGHT(15), UI_POS_Y(3)}, 12, "Random Msg.", false};
    Checkbox chk_mode_play{{UI_POS_X_RIGHT(15), UI_POS_Y(5)}, 10, "Play Mode", false};
    Checkbox chk_mode_stop{{UI_POS_X_RIGHT(15), UI_POS_Y(7)}, 10, "Stop Mode", false};
    Checkbox chk_mode_ncnt{{UI_POS_X_RIGHT(15), UI_POS_Y(9)}, 7, "N Msg.", false};
    Checkbox chk_mode_infinite{{UI_POS_X_RIGHT(15), UI_POS_Y(11)}, 10, "Inf. Msg.", false};

    Text screen_message_1{{0, 0, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "The application is intended"};
    Text screen_message_2{{0, 25, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "for controlling Love Spouse"};
    Text screen_message_3{{0, 50, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "auxiliary tools/devices."};
    Text screen_message_4{{0, 75, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "It is only for use by adults."};
    Text screen_message_5{{0, 100, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Are you an adult?"};

    Text screen_message_6{{0, 140, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "To use the application, "};
    Text screen_message_7{{0, 165, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "I assume full responsibility"};
    Text screen_message_8{{0, 190, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "for the \"I do not assume"};
    Text screen_message_9{{0, 215, UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "responsibility\"."};
    Text screen_message_10{{UI_POS_X_RIGHT(16), 230, UI_POS_WIDTH(16), UI_POS_HEIGHT(1)}, "(The developer)"};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Type:", Theme::getInstance()->fg_light->foreground}};

    bool button_onstate{false};
    bool play_running{false};
    bool stop_running{false};
    bool random_running{false};
    bool n_message_running{false};
    bool inf_message_running{false};
    bool start_screen{false};

    uint8_t target = 1;  // lovespouse

    char mac[13] = "010203040506";
    uint8_t channel_number = 37;
    char advertisementData[63] = {"03032CFE06162CFED5A59E020AB4\0"};

    PKT_TYPE pduType = {PKT_TYPE_DISCOVERY};

    uint8_t toy_packet = 0;
    uint16_t n_message_max = 999;
    uint16_t n_message_set = 0;
    uint16_t n_message_current = 0;
    static const size_t max_plays = 27;
    static const size_t max_stops = 3;

    uint16_t counter = 599;

    void createPacket(bool regenerate);
    void start();
    void stop();
    void reset();
    void printCurrentModes();
    void on_tx_progress(const bool done);
    void hidden_program(bool hide);
    void randomizeMac();
    void randomChn();
    uint8_t randomize(uint8_t max);
    uint64_t get_freq_by_channel_number(uint8_t channel_number);

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};

    LovespouseMode plays[max_plays] = {
        {0xE49C6C, "Classic 1"},
        {0xE7075E, "Classic 2"},
        {0xE68E4F, "Classic 3"},
        {0xE1313B, "Classic 4"},
        {0xE0B82A, "Classic 5"},
        {0xE32318, "Classic 6"},
        {0xE2AA09, "Classic 7"},
        {0xED5DF1, "Classic 8"},
        {0xECD4E0, "Classic 9"},
        {0xD41F5D, "Independent 1-1"},
        {0xD7846F, "Independent 1-2"},
        {0xD60D7E, "Independent 1-3"},
        {0xD1B20A, "Independent 1-4"},
        {0xD0B31B, "Independent 1-5"},
        {0xD3A029, "Independent 1-6"},
        {0xD22938, "Independent 1-7"},
        {0xDDDEC0, "Independent 1-8"},
        {0xDC57D1, "Independent 1-9"},
        {0xA4982E, "Independent 2-1"},
        {0xA7031C, "Independent 2-2"},
        {0xA68A0D, "Independent 2-3"},
        {0xA13579, "Independent 2-4"},
        {0xA0BC68, "Independent 2-5"},
        {0xA3275A, "Independent 2-6"},
        {0xA2AE4B, "Independent 2-7"},
        {0xAD59B3, "Independent 2-8"},
        {0xACD0A2, "Independent 2-9"},
    };

    LovespouseMode stops[max_stops] = {
        {0xE5157D, "Classic Stop"},
        {0xD5964C, "Independent 1 Stop"},
        {0xA5113F, "Independent 2 Stop"},
    };
};
};  // namespace ui::external_app::adult_toys_controller

#endif  // ui_adult_toys_controller_hpp