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

    button_trim.on_select = [this, &nav](Button&) {
        if (!path_.empty() && trim_range_.file_size > 0) {
            progress_ui.show_trimming();
            TrimFile(path_, trim_range_);
            read_capture(path_);
            refresh_ui();
        } else {
            nav.display_modal("Error", "Select a file first.");
        }
    };
}

void IQTrimView::paint(Painter& painter) {
    if (!path_.empty()) {
        // Draw power buckets.
        for (size_t i = 0; i < power_buckets_.size(); ++i) {
            auto amp = power_buckets_[i].power;
            painter.draw_vline(
                pos_lines + Point{(int)i, 0},
                height_lines,
                Color(amp, amp, amp));
        }

        // Draw trim range edges.
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
    power_buckets_ = {};
    PowerBuckets buckets{
        .p = power_buckets_.data(),
        .size = power_buckets_.size()};

    progress_ui.show_reading();
    auto range = ComputeTrimRange(path, amp_threshold, &buckets, progress_ui.get_callback());
    progress_ui.clear();

    if (range) {
        trim_range_ = *range;
        return true;
    } else {
        trim_range_ = {};
        return false;
    }
}

}  // namespace ui