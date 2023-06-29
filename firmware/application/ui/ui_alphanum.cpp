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

#include "ui_alphanum.hpp"

#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>

namespace ui {

AlphanumView::AlphanumView(
    NavigationView& nav,
    std::string& str,
    size_t max_length)
    : TextEntryView(nav, str, max_length) {
    size_t n;

    add_children({
        &labels,
        &field_raw,
        &text_raw_to_char,
        &button_delete,
        &button_mode,
    });

    const auto button_fn = [this](Button& button) {
        this->on_button(button);
    };

    n = 0;
    for (auto& button : buttons) {
        button.id = n;
        button.on_highlight = [this](Button& button) {
            focused_button = button.id;
        };
        button.on_select = button_fn;
        button.set_parent_rect({static_cast<Coord>((n % 5) * (240 / 5)),
                                static_cast<Coord>((n / 5) * 38 + 24),
                                240 / 5, 38});
        add_child(&button);
        n++;
    }

    set_mode(mode);

    button_mode.on_select = [this](Button&) {
        set_mode(mode + 1);
    };

    button_delete.on_select = [this](Button&) {
        char_delete();
    };

    field_raw.set_value('0');
    field_raw.on_select = [this](NumberField&) {
        char_add(field_raw.value());
    };

    // make text_raw_to_char widget display the char value from field_raw
    field_raw.on_change = [this](auto) {
        text_raw_to_char.set(std::string{static_cast<char>(field_raw.value())});
    };
}

void AlphanumView::set_mode(const uint32_t new_mode) {
    size_t n = 0;

    if (new_mode < 3)
        mode = new_mode;
    else
        mode = 0;

    const char* key_list = key_sets[mode].second;

    for (auto& button : buttons) {
        const std::string label{
            key_list[n]};
        button.set_text(label);
        n++;
    }

    if (mode < 2)
        button_mode.set_text(key_sets[mode + 1].first);
    else
        button_mode.set_text(key_sets[0].first);
}

void AlphanumView::on_button(Button& button) {
    const auto c = button.text()[0];
    char_add(c);
}

bool AlphanumView::on_encoder(const EncoderEvent delta) {
    focused_button += delta;
    if (focused_button < 0) {
        focused_button = buttons.size() - 1;
    } else if (focused_button >= (int16_t)buttons.size()) {
        focused_button = 0;
    }
    buttons[focused_button].focus();
    return true;
}

}  // namespace ui
