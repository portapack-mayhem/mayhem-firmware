/*
 * Copyright (C) 2023 Kyle Reed
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

#include "ui_iq_trim.hpp"

#include "complex.hpp"
#include "portapack.hpp"
#include "ui_fileman.hpp"
#include "ui_styles.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

IQTrimView::IQTrimView(NavigationView& nav) {
    add_children({
        &labels,
        &field_path,
        &text_range,
        &button_trim,
    });

    field_path.on_select = [this, &nav](TextField&) {
        auto open_view = nav.push<FileLoadView>(".C*");
        open_view->push_dir(u"CAPTURES");
        open_view->on_changed = [this](fs::path path) {
            read_capture(path);
            path_ = std::move(path);
            refresh_ui();
        };
    };

    button_trim.on_select = [this](Button&) {
        if (!path_.empty() && trim_range_.file_size > 0) {
            TrimFile(path_, trim_range_);
            read_capture(path_);
            refresh_ui();
        }
    };
}

void IQTrimView::paint(Painter& painter) {
    painter.fill_rectangle({pos_lines, {screen_width, height_lines}}, Color::dark_grey());

    if (!path_.empty()) {
        for (size_t i = 0; i < amp_data_.size(); ++i) {
            auto amp = amp_data_[i];
            painter.draw_vline(
                pos_lines + Point{(int)i, 0},
                height_lines,
                Color(amp, amp, amp));
        }

        int start_x = screen_width * trim_range_.start / trim_range_.file_size;
        int end_x = screen_width * trim_range_.end / trim_range_.file_size;

        painter.draw_vline(
            pos_lines + Point{start_x, 0},
            height_lines,
            Color::dark_green());
        painter.draw_vline(
            pos_lines + Point{end_x, 0},
            height_lines,
            Color::dark_red());
    }
}

void IQTrimView::focus() {
    field_path.focus();
}

void IQTrimView::refresh_ui() {
    field_path.set_text(path_.filename().string());
    text_range.set(to_string_dec_uint(trim_range_.start) + "-" + to_string_dec_uint(trim_range_.end));
    set_dirty();
}

bool IQTrimView::read_capture(const fs::path& path) {
    auto on_progress = [](uint8_t p) {
        Painter painter;
        auto width = p * screen_width / 100;
        painter.fill_rectangle({0, 4 * 16, screen_width, 3 * 16}, Color::black());
        painter.draw_string({6 * 8, 5 * 16}, Styles::yellow, "Reading Capture...");
        painter.draw_hline({0, 6 * 16}, width, Color::yellow());
    };

    auto range = ComputeTrimRange(path, amp_threshold, on_progress);

    if (range) {
        trim_range_ = *range;
        return true;
    } else {
        trim_range_ = {};
        return false;
    }
}

}  // namespace ui