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

#include "ui_spectrum_painter_text.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "cpld_update.hpp"
#include "bmp.hpp"
#include "baseband_api.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "file.hpp"
#include "portapack_persistent_memory.hpp"

namespace ui::external_app::spainter {

SpectrumInputTextView::SpectrumInputTextView(NavigationView& nav) {
    hidden(true);

    add_children({&text_message_0,
                  &text_message_1,
                  &text_message_2,
                  &text_message_3,
                  &text_message_4,
                  &text_message_5,
                  &text_message_6,
                  &text_message_7,
                  &text_message_8,
                  &text_message_9,
                  &button_message});

    button_message.on_select = [this, &nav](Button&) {
        this->on_set_text(nav);
    };
}

SpectrumInputTextView::~SpectrumInputTextView() {
}

void SpectrumInputTextView::on_set_text(NavigationView& nav) {
    text_prompt(nav, buffer, 300);
}

void SpectrumInputTextView::focus() {
    button_message.focus();
}

void SpectrumInputTextView::paint(Painter& painter) {
    message = buffer;
    for (uint32_t i = 0; i < text_message.size(); i++) {
        if (message.length() > i * 30)
            text_message[i]->set(message.substr(i * 30, 30));
        else
            text_message[i]->set("");
    }

    painter.fill_rectangle(
        {{0, 40}, {240, 204}},
        style().background);
}

constexpr uint32_t pixel_repeat = 32;
uint16_t SpectrumInputTextView::get_width() {
    return 16 * pixel_repeat;
}

uint16_t SpectrumInputTextView::get_height() {
    return this->message.length() * 8;
}

std::vector<uint8_t> SpectrumInputTextView::get_line(uint16_t y) {
    auto character_position = y / 8;
    auto character = this->message[character_position];
    auto glyph_data = ui::font::fixed_8x16.glyph(character).pixels();

    auto line_in_character = y % 8;
    std::vector<uint8_t> data(16 * pixel_repeat);

    for (uint32_t index = 0; index < 16; index++) {
        auto glyph_byte = index;
        auto glyph_bit = line_in_character;
        uint8_t glyph_pixel = (glyph_data[glyph_byte] & (1 << glyph_bit)) >> glyph_bit;

        for (uint32_t j = 0; j < pixel_repeat; j++)
            data[index * pixel_repeat + j] = glyph_pixel * 255;
    }

    return data;
}
}  // namespace ui::external_app::spainter
