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

#include "cpld_update.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

#include "ui_about.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include <math.h>
#include <cstring>

using namespace lpc43xx;
using namespace portapack;

namespace ui {

// This is pretty much WaterfallView but in the opposite direction
CreditsWidget::CreditsWidget(
    Rect parent_rect)
    : Widget{parent_rect} {
}

void CreditsWidget::paint(Painter&) {
}

void CreditsWidget::on_show() {
    clear();

    const auto screen_r = screen_rect();
    display.scroll_set_area(screen_r.top(), screen_r.bottom());
}

void CreditsWidget::on_hide() {
    display.scroll_disable();
}

void CreditsWidget::new_row(
    const std::array<Color, 240>& pixel_row) {
    // Glitch be here (see comment in main.cpp)
    const auto draw_y = display.scroll(-1);

    display.draw_pixels(
        {{0, draw_y - 1}, {240, 1}},
        pixel_row);
}

void CreditsWidget::clear() {
    display.fill_rectangle(
        screen_rect(),
        Theme::getInstance()->bg_darkest->background);
}

void AboutView::update() {
    size_t i = 0;
    std::array<Color, 240> pixel_row;

    slow_down++;
    if (slow_down % 3 < 2) return;

    if (!timer) {
        if (loop) {
            credits_index = 0;
            loop = false;
        }

        text = credits[credits_index].text;
        timer = credits[credits_index].delay;
        start_pos = credits[credits_index].start_pos;

        if (timer < 0) {
            timer = 240;
            loop = true;
        } else
            timer += 16;

        render_line = 0;
        credits_index++;
    } else
        timer--;

    if (render_line < 16) {
        for (const auto c : text) {
            const auto glyph = style().font.glyph(c);

            const size_t start = (glyph.size().width() / 8) * render_line;
            for (Dim c = 0; c < glyph.size().width(); c++) {
                const auto pixel = glyph.pixels()[start + (c >> 3)] & (1U << (c & 0x7));
                pixel_row[start_pos + i + c] = pixel ? Theme::getInstance()->bg_darkest->foreground : Theme::getInstance()->bg_darkest->background;
            }

            const auto advance = glyph.advance();
            i += advance.x();
        }
        render_line++;
    }

    credits_display.new_row(pixel_row);
}

AboutView::AboutView(
    NavigationView& nav) {
    add_children({&credits_display,
                  &button_ok});

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };
}

void AboutView::focus() {
    button_ok.focus();
}

} /* namespace ui */
