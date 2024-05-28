/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __MORSE_TX_H__
#define __MORSE_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"
#include "morse.hpp"

#include <ch.h>

using namespace morse;

namespace ui {

class MorseView : public View {
   public:
    MorseView(NavigationView& nav);
    ~MorseView();

    MorseView(const MorseView&) = delete;
    MorseView(MorseView&&) = delete;
    MorseView& operator=(const MorseView&) = delete;
    MorseView& operator=(MorseView&&) = delete;

    void focus() override;
    void paint(Painter& painter) override;

    void on_tx_progress(const uint32_t progress, const bool done);
    void on_loop_progress(const uint32_t progress, const bool done);

    std::string title() const override { return "Morse TX"; };

    uint32_t time_unit_ms{0};
    size_t symbol_count{0};
    uint32_t loop{0};

   private:
    NavigationView& nav_;
    std::string message{};
    uint32_t time_units{0};

    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        1536000 /* sampling rate */
    };

    std::string buffer{"PORTAPACK"};
    bool mode_cw{true};
    bool foxhunt_mode{false};
    uint32_t foxhunt_code{1};
    uint32_t speed{15};
    uint32_t tone{700};
    app_settings::SettingsManager settings_{
        "tx_morse",
        app_settings::Mode::TX,
        {
            {"message"sv, &buffer},
            {"foxhunt"sv, &foxhunt_mode},
            {"foxhunt_code"sv, &foxhunt_code},
            {"speed"sv, &speed},
            {"tone"sv, &tone},
            {"mode_cw"sv, &mode_cw},
            {"loop"sv, &loop},
        }};

    bool start_tx();
    void update_tx_duration();
    void on_set_text(NavigationView& nav);
    void set_foxhunt(size_t i);

    Thread* ookthread{nullptr};
    Thread* loopthread{nullptr};
    bool run{false};

    Labels labels{
        {{4 * 8, 6 * 8}, "Speed:   wps", Theme::getInstance()->fg_light->foreground},
        {{4 * 8, 8 * 8}, "Tone:    Hz", Theme::getInstance()->fg_light->foreground},
        {{4 * 8, 10 * 8}, "Modulation:", Theme::getInstance()->fg_light->foreground},
        {{4 * 8, 12 * 8}, "Loop:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 25 * 8}, "TX will last", Theme::getInstance()->fg_light->foreground}};

    Checkbox checkbox_foxhunt{
        {4 * 8, 16},
        8,
        "Foxhunt:"};
    NumberField field_foxhunt{
        {17 * 8, 16 + 4},
        2,
        {1, 11},
        1,
        ' '};

    NumberField field_speed{
        {10 * 8, 6 * 8},
        3,
        {10, 45},
        1,
        ' '};

    NumberField field_tone{
        {9 * 8, 8 * 8},
        4,
        {100, 9999},
        20,
        ' '};

    OptionsField options_modulation{
        {15 * 8, 10 * 8},
        2,
        {{"CW", true},
         {"FM", false}}};

    OptionsField options_loop{
        {9 * 8, 12 * 8},
        6,
        {{"Off", 0},
         {"5 sec", 5},
         {"10 sec", 10},
         {"30 sec", 30},
         {"1 min", 60},
         {"3 min", 180},
         {"5 min", 300}}};

    Text text_tx_duration{
        {14 * 8, 25 * 8, 4 * 8, 16},
        "-"};

    Text text_message{
        {1 * 8, 15 * 8, 28 * 8, 16},
        ""};

    Button button_message{
        {1 * 8, 17 * 8, 12 * 8, 28},
        "Set message"};

    ProgressBar progressbar{
        {2 * 8, 28 * 8, 208, 16}};

    TransmitterView tx_view{
        16 * 16,
        10000,
        12};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

} /* namespace ui */

#endif /*__MORSE_TX_H__*/
