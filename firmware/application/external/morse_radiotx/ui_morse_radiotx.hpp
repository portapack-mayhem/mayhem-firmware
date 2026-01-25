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
#include <ch.h>

namespace ui::external_app::morse_radiotx {

class MorseRadiotxView : public ui::View {
   public:
    MorseRadiotxView(ui::NavigationView& nav);
    ~MorseRadiotxView();

    std::string title() const override { return "Morse Radio Tx"; }
    void focus() override;
    void on_show() override;
    void paint(Painter& painter) override;

   private:
    void on_set_text(NavigationView& nav);
    void onPress();
    void onRelease();
    void on_framesync();
    void writeCharToConsole(const std::string& ch, double confidence);
    bool tx_button_held();

    ui::NavigationView& nav_;
    MorseDecoder morse_decoder_{};
    TxRadioState radio_state_{};
    std::string msg_buffer{"PORTAPACK"};
    uint8_t current_mode{0};  // 0=AM, 1=FM, 2=DSB, 3=USB, 4=LSB
    uint8_t wpm{0};
    uint32_t tone{0};

    app_settings::SettingsManager settings_{
        "tx_morse_radio",
        app_settings::Mode::TX,
        {
            {"cmode"sv, &current_mode},
            {"tone"sv, &tone},
            {"wpm"sv, &wpm},
            {"message"sv, &msg_buffer},
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

    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_X(0)}};
    ui::OptionsField options_mode{
        {UI_POS_X(6), UI_POS_Y(1)},
        5,
        {{"AM", 0}, {"FM", 1}, {"DSB", 2}, {"USB", 3}, {"LSB", 4}}};
    NumberField tone_{{UI_POS_X(16), UI_POS_Y(1)}, 4, {400, 1400}, 10, ' ', true};
    NumberField wpm_{{UI_POS_X(16), UI_POS_Y(4)}, 4, {10, 45}, 1, ' ', true};

    ui::Text txt_msg{{UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "[" + msg_buffer + "] "};
    ui::Text msg_index{{UI_POS_X(1), UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, ""};
    ui::Button btn_message{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)}, "Set message"};
    Checkbox chk_trans{{UI_POS_X(14), UI_POS_Y(5)}, 13, "Manual trans.", true};
    ui::Text txt_last{{UI_POS_X(6), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    ui::Console console_text{{UI_POS_X(0), UI_POS_Y(8), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(7)}};
    ui::Button btn_clear{{UI_POS_X(0), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "CLR"};
    ui::Button btn_tt{{UI_POS_X_CENTER(12), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "KEY"};

    ui::Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Mode:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(11), UI_POS_Y(1)}, "Tone:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(21), UI_POS_Y(1)}, "Hz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(14), UI_POS_Y(4)}, "WPM:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Last:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(7)}, "Sent Message:", Theme::getInstance()->fg_light->foreground},
    };

    uint8_t last_color_id{255};
    uint8_t color_id{255};
    std::string arr_color[4] = {STR_COLOR_WHITE, STR_COLOR_RED, STR_COLOR_YELLOW, STR_COLOR_GREEN};

    bool button_touch{false};
    bool button_was_selected{false};
    bool decode_timeout_calc{false};
    bool transmit{false};

    int64_t start_time{0};
    int64_t end_time{0};
    int64_t transmit_time{0};
    std::string msg_indicator[27];

    MessageHandlerRegistration message_handler_framesync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) {
            (void)p;
            this->on_framesync();
        }};
};

}  // namespace ui::external_app::morse_radiotx

#endif  // __MORSE_RADIOTX_H__
