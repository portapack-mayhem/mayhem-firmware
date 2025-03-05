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

#include "ui_toast.hpp"
#include "ui_alphanum.hpp"

using namespace portapack;

namespace ui {

void toast(
    NavigationView& nav,
    std::string& str,
    size_t max_length,
    std::function<void(std::string&)> on_done) {
    text_prompt(nav, str, str.length(), max_length, on_done);
}

void toast(
    NavigationView& nav,
    std::string& str,
    uint32_t cursor_pos,
    size_t max_length,
    std::function<void(std::string&)> on_done) {
    auto te_view = nav.push<AlphanumView>(str, max_length);
    te_view->set_cursor(cursor_pos);
    te_view->on_changed = [on_done](std::string& value) {
        if (on_done)
            on_done(value);
    };
}

/* ToastView ***********************************************************/

void ToastView::on_frame_sync() {
    if(current_cycle <= target_cycle) {
        current_cycle++;
        return;
    } else {
        set_dirty();
    }
}

ToastView::ToastView(
    NavigationView& nav,
    std::string& str,
    size_t cycle) {

    painter.draw_string({0,0}, *Theme::getInstance()->bg_darkest, str);

}

ToastView::~ToastView() {
    set_dirty();

} /* namespace ui */
