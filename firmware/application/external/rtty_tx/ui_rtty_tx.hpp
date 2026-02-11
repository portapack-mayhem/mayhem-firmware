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

#ifndef __UI_RTTY_TX_H__
#define __UI_RTTY_TX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include "baudot.hpp"

using namespace ui;

namespace ui::external_app::rtty_tx {

class RttyTxView : public View {
   public:
    RttyTxView(NavigationView& nav);
    ~RttyTxView();
    void focus() override;
    std::string title() const override { return "RTTY"; };

   private:
    void on_tx_progress(bool done);
    void stop();
    void refresh_tones();

    NavigationView& nav_;
    RxRadioState radio_state_{};
    uint32_t baud_conf = 4545;  // default to 45.45 baud, which is common for RTTY.
    uint32_t shift = 170;       // 170 hz
    std::string message = "PORTAPACK";
    int32_t mark_tone = 0;  // for option
    int16_t mark = 0, space = 170;
    uint8_t stop_bits = 3;  // 2=1.0, 3=1.5, 4=2.0
    app_settings::SettingsManager settings_{
        "tx_rtty",
        app_settings::Mode::TX,
        {
            {"baud_conf"sv, &baud_conf},
            {"shift"sv, &shift},
            {"message"sv, &message},
            {"mark_tone"sv, &mark_tone},
        }};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Baud:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(14), UI_POS_Y(0)}, "Shift:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(15), UI_POS_Y(1)}, "Mark t:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "Message:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_baud{
        {UI_POS_X(7), UI_POS_Y(0)},
        5,
        {{"45", 4500},
         {"45.45", 4545},
         {"50", 5000},
         {"75", 7500},
         {"100", 10000},
         {"110", 11000},
         {"150", 15000},
         {"200", 20000}}};

    OptionsField options_shift{
        {UI_POS_X(21), UI_POS_Y(0)},
        5,
        {{"170", 170},
         {"85", 85},
         {"200", 200},
         {"450", 450},
         {"850", 850}}};

    Checkbox check_inverted{
        {UI_POS_X(0), UI_POS_Y(1)},
        10,
        "Inverted",
        true};

    OptionsField options_tone{
        {UI_POS_X(25), UI_POS_Y(1)},
        5,
        {{"CNT", -32000},
         {"0", 0},
         {"2125", 2125},
         {"-2125", -2125},
         {"1275", 1275},
         {"-1275", -1275}}};

    Text text_tones{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        "-"};

    Button btn_message{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_HEIGHT(2)},
        "Set message"};

    Text text_message{
        {UI_POS_X(0), UI_POS_Y(5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        ""};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        1000,
        0};

    RTTYDataMessage rtty_message{};
    BaudotCoder baudot_encoder{};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};
};

}  // namespace ui::external_app::rtty_tx

#endif /*__UI_RTTY_TX_H__*/