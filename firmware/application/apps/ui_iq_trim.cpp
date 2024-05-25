/*
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
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
#include "file_path.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

IQTrimView::IQTrimView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &field_path,
        &field_start,
        &field_end,
        &text_samples,
        &text_max,
        &field_cutoff,
        &field_amplify,
        &button_trim,
    });

    field_path.on_select = [this](TextField&) {
        auto open_view = nav_.push<FileLoadView>(".C*");
        open_view->push_dir(captures_dir);
        open_view->on_changed = [this](fs::path path) {
            open_file(path);
        };
    };

    text_samples.set_style(Theme::getInstance()->fg_light);
    text_max.set_style(Theme::getInstance()->fg_light);

    field_start.on_change = [this](int32_t v) {
        if (field_end.value() < v)
            field_end.set_value(v, false);
        set_dirty();
    };

    field_end.on_change = [this](int32_t v) {
        if (field_start.value() > v)
            field_start.set_value(v, false);
        set_dirty();
    };

    field_cutoff.set_value(7);  // 7% of max is a good default.
    field_cutoff.on_change = [this](int32_t) {
        compute_range();
        refresh_ui();
    };

    field_amplify.set_value(1);  // 1X is default (no amplification)
    field_amplify.on_change = [this](int32_t) {
        refresh_ui();
    };

    button_trim.on_select = [this](Button&) {
        if (trim_capture()) {
            profile_capture();
            compute_range();
            refresh_ui();
        }
    };
}

IQTrimView::IQTrimView(NavigationView& nav, const fs::path& path)
    : IQTrimView(nav) {
    open_file(path);
}

void IQTrimView::open_file(const std::filesystem::path& path) {
    path_ = std::move(path);
    profile_capture();
    compute_range();
    refresh_ui();
}

void IQTrimView::paint(Painter& painter) {
    if (info_) {
        uint32_t power_cutoff = field_cutoff.value() * static_cast<uint64_t>(info_->max_power) / 100;

        // Draw power buckets.
        for (size_t i = 0; i < power_buckets_.size(); ++i) {
            auto power = power_buckets_[i].power;
            uint8_t amp = 0;

            if (power > power_cutoff && info_->max_power > 0)
                amp = (255ULL * power) / info_->max_power;

            painter.draw_vline(
                pos_lines + Point{(int)i, 0},
                height_lines,
                Color(amp, amp, amp));
        }

        // Draw trim range edges.
        int start_x = screen_width * field_start.value() / info_->sample_count;
        int end_x = screen_width * field_end.value() / info_->sample_count;

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
    text_samples.set(to_string_dec_uint(info_->sample_count));

    // show max power after amplification applied
    uint64_t power_amp = field_amplify.value() * field_amplify.value() * field_amplify.value() * field_amplify.value();
    text_max.set(to_string_dec_uint(info_->max_power * power_amp));

    // show max power in red if amplification is too high, causing clipping
    uint32_t clipping_limit = (fs::capture_file_sample_size(path_) == sizeof(complex8_t)) ? 0x80 : 0x8000;
    if ((field_amplify.value() * info_->max_iq) > clipping_limit)
        text_max.set_style(Theme::getInstance()->fg_red);
    else
        text_max.set_style(Theme::getInstance()->fg_light);

    set_dirty();
}

void IQTrimView::update_range_controls(iq::TrimRange trim_range) {
    auto max_range = info_ ? info_->sample_count : 0;
    auto step = info_ ? info_->sample_count / screen_width : 0;

    field_start.set_range(0, max_range);
    field_start.set_step(step);
    field_end.set_range(0, max_range);
    field_end.set_step(step);

    field_start.set_value(trim_range.start_sample, false);
    field_end.set_value(trim_range.end_sample, false);
}

void IQTrimView::profile_capture() {
    power_buckets_ = {};
    iq::PowerBuckets buckets{
        .p = power_buckets_.data(),
        .size = power_buckets_.size()};

    progress_ui.show_reading();
    info_ = iq::profile_capture(path_, buckets);
    progress_ui.clear();
}

void IQTrimView::compute_range() {
    if (!info_)
        return;

    iq::PowerBuckets buckets{
        .p = power_buckets_.data(),
        .size = power_buckets_.size()};
    auto trim_range = iq::compute_trim_range(*info_, buckets, field_cutoff.value());

    update_range_controls(trim_range);
}

bool IQTrimView::trim_capture() {
    if (!info_) {
        nav_.display_modal("Error", "Open a file first.");
        return false;
    }

    bool trimmed = false;
    iq::TrimRange trim_range{
        static_cast<uint32_t>(field_start.value()),
        static_cast<uint32_t>(field_end.value()),
        info_->sample_size};

    if (trim_range.start_sample >= trim_range.end_sample) {
        nav_.display_modal("Error", "Invalid trimming range.");
        return false;
    }

    progress_ui.show_trimming();
    trimmed = iq::trim_capture_with_range(path_, trim_range, progress_ui.get_callback(), field_amplify.value());
    progress_ui.clear();

    if (!trimmed)
        nav_.display_modal("Error", "Trimming failed.");

    return trimmed;
}

}  // namespace ui
