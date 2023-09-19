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
#include "ui_fileman.hpp"

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

        painter.draw_vline(
            pos_lines + Point{(int)trim_region_.start, 0},
            height_lines,
            Color::dark_green());
        painter.draw_vline(
            pos_lines + Point{(int)trim_region_.end, 0},
            height_lines,
            Color::dark_red());
    }
}

void IQTrimView::focus() {
    field_path.focus();
}

void IQTrimView::refresh_ui() {
    field_path.set_text(path_.filename().string());
    text_range.set(to_string_dec_uint(trim_region_.start) + "-" + to_string_dec_uint(trim_region_.end));
    set_dirty();
}

bool IQTrimView::read_capture(const fs::path& path) {
    trim_region_ = trim_region{};

    File f;
    auto error = f.open(path);
    if (error) {
        return false;
    }

    // TODO:
    // C16
    // Small files
    // Ranges

    constexpr uint8_t max_amp = 255;
    constexpr uint16_t max_mag_squared = 32768;
    constexpr uint8_t min_threshold = 5;

    bool has_start = false;
    auto sample_interval = f.size() / amp_data_.size();
    complex8_t value{};

    // Interval to be multiple of the complex_t length.
    auto sample_drift = sample_interval % sizeof(value);
    sample_interval -= sample_drift;

    for (size_t i = 0; i < amp_data_.size(); ++i) {
        auto pos = i * sample_interval;
        f.seek(pos);
        auto result = f.read(&value, sizeof(value));

        if (!result)
            return false;

        auto real = value.real();
        auto imag = value.imag();
        auto maq_squared = (real * real) + (imag + imag);

        amp_data_[i] = (max_amp * maq_squared) / max_mag_squared;

        if (amp_data_[i] > min_threshold) {
            if (has_start) {
                trim_region_.end = i;
                trim_region_.end_byte = i * sample_interval;
            } else {
                has_start = true;
                trim_region_.start = i;
                trim_region_.start_byte = i * sample_interval;
            }
        }
    }

    return true;
}

}  // namespace ui