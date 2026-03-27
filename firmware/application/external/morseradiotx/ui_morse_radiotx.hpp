/*
 * Copyright (C) 2026 Pezsma
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

#ifndef __MORSE_RADIOTX_H__
#define __MORSE_RADIOTX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_language.hpp"
#include "ui_painter.hpp"
#include "ui_freq_field.hpp"
#include "ui_transmitter.hpp"
#include "ui_textentry.hpp"
#include "string_format.hpp"
#include "morsedecoder.hpp"
#include "irq_controls.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "external_app.hpp"
#include "string_format.hpp"
#include <ch.h>
#include <hal.h>

namespace ui::external_app::morseradiotx {

class MorseRadiotxView : public ui::View {
   public:
    MorseRadiotxView(ui::NavigationView& nav);
    ~MorseRadiotxView();

    MorseRadiotxView(const MorseRadiotxView&) = delete;
    MorseRadiotxView(MorseRadiotxView&&) = delete;
    MorseRadiotxView& operator=(const MorseRadiotxView&) = delete;
    MorseRadiotxView& operator=(MorseRadiotxView&&) = delete;

    std::string title() const override { return "Morse Tx"; }
    void focus() override;
    void on_show() override;
    void transmit_morse_message();

   private:
    struct MorseTimings {
        uint32_t dot_ms;
        uint32_t dash_ms;
        uint32_t symbol_gap;
        uint32_t char_gap;
        uint32_t word_gap;
    };

    MorseTimings current_timings{};

    std::string msg_indicator{""};
    MorseTimings calculate_morse_timings(uint32_t wpm);
    void onPress();
    void onRelease();
    void on_framesync();
    void writeCharToConsole(const std::string& ch, double confidence);
    void ui_toggle();
    bool tx_button_held();
    void ptt_button_visibility(bool hidden);

    ui::NavigationView& nav_;
    MorseDecoder morse_decoder_{};
    TxRadioState radio_state_{};
    Thread* tx_thread{nullptr};

    std::string msg_buffer{"PORTAPACK"};
    uint8_t current_mode{0};  // 0=AM, 1=FM, 2=DSB, 3=USB, 4=LSB
    uint8_t wpm{20};
    uint32_t tone{700};
    float band{5.8};
    std::string call_sign{""};

    app_settings::SettingsManager settings_{
        "tx_morseradio",
        app_settings::Mode::TX,
        {
            {"cmode"sv, &current_mode},
            {"audtone"sv, &tone},
            {"wpm"sv, &wpm},
            {"message"sv, &msg_buffer},
            {"bandwith"sv, &band},
            {"call_sign"sv, &call_sign},
        }};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        10000,
        0, false};

    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_Y_BOTTOM(5)}};
    ui::OptionsField options_mode{
        {UI_POS_X(5), UI_POS_Y(0)},
        3,
        {{"AM", 0}, {"FM", 1}, {"DSB", 2}, {"USB", 3}, {"LSB", 4}}};
    NumberField tone_{{UI_POS_X(14), UI_POS_Y(0)}, 4, {400, 1400}, 10, ' ', true};
    NumberField wpm_{{UI_POS_X(25), UI_POS_Y(0)}, 2, {10, 45}, 1, ' ', true};
    NumberField wait_time{{UI_POS_X(10), UI_POS_Y(4)}, 3, {1, 99}, 1, ' ', true};  // wait between tx-es in loop mode (sec)
    FloatField bandwidth{{UI_POS_X(6), UI_POS_Y(5)}, 4, {1.0, 16.0}, 0.1, ' ', true, 1};

    ProgressBar progressbar{
        {UI_POS_X(0), UI_POS_Y(6), screen_width, 16}};

    ui::Text txt_msg{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "[" + msg_buffer + "] "};
    ui::Button btn_message{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)}, "Message"};
    ui::Button btn_calls{{UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)}, (call_sign.empty()) ? "call sign?" : call_sign};
    Checkbox chk_trans{{UI_POS_X(14), UI_POS_Y(2)}, 13, "Manual trans.", true};
    Checkbox chk_callsgn{{UI_POS_X(14), UI_POS_Y(3)}, 9, "Call sign", true};
    Checkbox chk_loop{{UI_POS_X(14), UI_POS_Y(4)}, 4, "loop", true};
    ui::Text txt_last{{UI_POS_X(10), UI_POS_Y(7), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)}, ""};
    ui::Console console_text{{UI_POS_X(0), UI_POS_Y(9), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(14)}};
    ui::Button btn_clear{{UI_POS_X(0), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "CLR"};
    ui::Button btn_ptt{{UI_POS_X_CENTER(12), UI_POS_Y_BOTTOM(7), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "PTT"};

    ui::Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Mode:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(0)}, "Tone:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(18), UI_POS_Y(0)}, "Hz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(21), UI_POS_Y(0)}, "WPM:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(5)}, "BandW:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(10), UI_POS_Y(5)}, "kHz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(4)}, "Wait time: ", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(7)}, "Last seq:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "Sent Message:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(7), UI_POS_Y_BOTTOM(5)}, "Vol.:", Theme::getInstance()->fg_light->foreground},
    };

    uint8_t last_color_id{255};
    uint8_t color_id{255};
    std::string arr_color[4] = {STR_COLOR_WHITE, STR_COLOR_RED, STR_COLOR_YELLOW, STR_COLOR_GREEN};

    bool button_touch{false};
    bool button_was_selected{false};
    bool decode_timeout_calc{false};
    bool transmit{false};
    bool thread_running = false;

    uint8_t send_indicator{5};
    int64_t start_time{0};
    int64_t end_time{0};
    int64_t transmit_time{0};
    SwitchesState initial_switch_config_{};

    MessageHandlerRegistration message_handler_framesync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) {
            (void)p;
            this->on_framesync();
        }};
};

}  // namespace ui::external_app::morseradiotx

#endif  // __MORSE_RADIOTX_H__
