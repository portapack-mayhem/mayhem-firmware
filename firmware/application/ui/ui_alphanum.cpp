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

#include "hackrf_hal.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "utility.hpp"

namespace ui {

AlphanumView::AlphanumView(
    NavigationView& nav,
    std::string& str,
    size_t max_length)
    : TextEntryView(nav, str, max_length) {
    size_t n;

    add_children({
        &button_shift,
        &labels,
        &field_raw,
        &text_raw_to_char,
        &button_delete,
        &button_mode,
    });

    const auto button_fn = [this](Button& button) {
        this->on_button(button);
    };

    button_shift.set_vertical_center(true);
    button_shift.on_select = [this]() {
        incr(shift_mode);

        // Disallow one-time shift in digits/symbols mode
        if ((mode == 1) && (shift_mode == ShiftMode::Shift))
            shift_mode = ShiftMode::ShiftLock;
        else if (shift_mode > ShiftMode::ShiftLock)
            shift_mode = ShiftMode::None;

        refresh_keys();
    };

    n = 0;
    for (auto& button : buttons) {
        button.id = n;
        button.on_highlight = [this](Button& button) {
            focused_button = button.id;
        };
        button.on_select = button_fn;
        button.set_parent_rect({static_cast<Coord>((n % 5) * (screen_width / 5)),
                                static_cast<Coord>((n / 5) * 38 + 24),
                                screen_width / 5, 38});
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

    field_raw.on_change = [this](auto) {
        text_raw_to_char.set(std::string{static_cast<char>(field_raw.value())});
    };
}

void AlphanumView::set_mode(uint32_t new_mode, ShiftMode new_shift_mode) {
    if (new_mode < std::size(key_sets))
        mode = new_mode;
    else
        mode = 0;

    shift_mode = new_shift_mode;
    refresh_keys();

    if (mode + 1 < std::size(key_sets))
        button_mode.set_text(key_sets[mode + 1].name);
    else
        button_mode.set_text(key_sets[0].name);
}

void AlphanumView::refresh_keys() {
    auto key_list = shift_mode == ShiftMode::None
                        ? key_sets[mode].normal
                        : key_sets[mode].shifted;

    size_t n = 0;
    for (auto& button : buttons) {
        if (n > strlen(key_list)) {
            button.set_text(std::string{'\0'});
        } else {
            button.set_text(std::string{key_list[n]});
        }

        n++;
    }

    switch (shift_mode) {
        case ShiftMode::None:
            button_shift.set_color(Theme::getInstance()->bg_dark->background);
            break;
        case ShiftMode::Shift:
            button_shift.set_color(Theme::getInstance()->bg_darkest->background);
            break;
        case ShiftMode::ShiftLock:
            button_shift.set_color(Theme::getInstance()->fg_blue->foreground);
            break;
    }
}

void AlphanumView::on_button(Button& button) {
    const auto c = button.text()[0];
    if (c != '\0') {
        char_add(c);
    }

    // TODO: Consolidate shift handling.
    if (shift_mode == ShiftMode::Shift) {
        shift_mode = ShiftMode::None;
        refresh_keys();
    }
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
