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

#ifndef __MORSE_PRACTICE_H__
#define __MORSE_PRACTICE_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_language.hpp"
#include "ui_painter.hpp"
#include "ui_receiver.hpp"
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

namespace ui::external_app::morse_practice {

class MorsePracticeView : public ui::View {
   public:
    MorsePracticeView(ui::NavigationView& nav);
    ~MorsePracticeView();

    std::string title() { return "Morse P"; }
    void focus() override;
    void on_show() override;

   private:
    void onPress();
    void onRelease();
    void on_framesync();
    void writeCharToConsole(const std::string& ch, double confidence);
    bool tx_button_held();

    ui::NavigationView& nav_;
    MorseDecoder morse_decoder_{};

    int64_t start_time = 0;
    int64_t end_time = 0;

    ui::Button btn_tt{{UI_POS_X_CENTER(12), UI_POS_Y(3), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)}, "KEY"};
    ui::Text txt_last{{UI_POS_X(0), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    ui::Button btn_clear{{UI_POS_X(0), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(6), UI_POS_HEIGHT(1)}, "CLR"};
    ui::Console console_text{{UI_POS_X(0), UI_POS_Y(7), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(10)}};
    AudioVolumeField field_volume{{UI_POS_X_RIGHT(2), UI_POS_X(0)}};

    uint8_t last_color_id = 255;
    uint8_t color_id = 255;
    std::string arr_color[4] = {STR_COLOR_WHITE, STR_COLOR_RED, STR_COLOR_YELLOW, STR_COLOR_GREEN};

    bool button_touch = false;
    bool button_was_selected = false;
    bool decode_timeout_calc = false;

    MessageHandlerRegistration message_handler_framesync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) {
            (void)p;
            this->on_framesync();
        }};
};

}  // namespace ui::external_app::morse_practice

#endif  // __MORSE_PRACTICE_H__
