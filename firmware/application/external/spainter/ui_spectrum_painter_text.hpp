/*
 * Copyright (C) 2023 Bernd Herzog
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

#ifndef __UI_SPECTRUM_PAINTER_TEXT_H
#define __UI_SPECTRUM_PAINTER_TEXT_H

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
#include "message.hpp"

namespace ui::external_app::spainter {

class SpectrumInputTextView : public View {
   public:
    SpectrumInputTextView(NavigationView& nav);
    ~SpectrumInputTextView();

    void focus() override;
    void paint(Painter&) override;

    uint16_t get_width();
    uint16_t get_height();
    std::vector<uint8_t> get_line(uint16_t);

   private:
    std::string buffer{"PORTAPACK"};
    std::string message{};
    void on_set_text(NavigationView& nav);

    Text text_message_0{
        {UI_POS_X(0), UI_POS_Y(0), screen_width, 16},
        ""};

    Text text_message_1{
        {UI_POS_X(0), 1 * 16, screen_width, 16},
        ""};

    Text text_message_2{
        {UI_POS_X(0), 2 * 16, screen_width, 16},
        ""};

    Text text_message_3{
        {UI_POS_X(0), 3 * 16, screen_width, 16},
        ""};

    Text text_message_4{
        {UI_POS_X(0), 4 * 16, screen_width, 16},
        ""};

    Text text_message_5{
        {UI_POS_X(0), 5 * 16, screen_width, 16},
        ""};

    Text text_message_6{
        {UI_POS_X(0), 6 * 16, screen_width, 16},
        ""};

    Text text_message_7{
        {UI_POS_X(0), 7 * 16, screen_width, 16},
        ""};

    Text text_message_8{
        {UI_POS_X(0), 8 * 16, screen_width, 16},
        ""};

    Text text_message_9{
        {UI_POS_X(0), 9 * 16, screen_width, 16},
        ""};

    std::array<Text*, 10> text_message{{
        &text_message_0,
        &text_message_1,
        &text_message_2,
        &text_message_3,
        &text_message_4,
        &text_message_5,
        &text_message_6,
        &text_message_7,
        &text_message_8,
        &text_message_9,
    }};

    Button button_message{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(10), screen_width, 28},
        "Set message"};
};

}  // namespace ui::external_app::spainter

#endif
