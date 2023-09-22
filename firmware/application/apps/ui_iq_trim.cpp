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
        &button_trim,
    });

    field_path.on_select = [this](TextField&) {
        auto open_view = nav_.push<FileLoadView>(".C*");
        open_view->push_dir(u"CAPTURES");
        open_view->on_changed = [this](fs::path path) {
            path_ = std::move(path);
            profile_capture();
            refresh_ui();
        };
    };

    text_samples.set_style(&Styles::light_grey);
    text_max.set_style(&Styles::light_grey);

    field_start.on_change = [this](int32_t v) {
        mode_ = TrimMode::Range;
        if (field_end.value() < v)
            field_end.set_value(v, false);
        set_dirty();
    };

    field_end.on_change = [this](int32_t v) {
        mode_ = TrimMode::Range;
        if (field_start.value() > v)
            field_start.set_value(v, false);
        set_dirty();
    };

    field_cutoff.set_value(7);  // 7% of max is a good default.
    field_cutoff.on_change = [this](int32_t) {
        mode_ = TrimMode::Cutoff;
        compute_range();
        refresh_ui();
    };

    button_trim.on_select = [this](Button&) {
        if (trim_capture()) {
            profile_capture();
            refresh_ui();
        }
    };
}

void IQTrimView::paint(Painter& painter) {
    if (info_) {
        // Draw power buckets.
        for (size_t i = 0; i < power_buckets_.size(); ++i) {
            auto power = power_buckets_[i].power;
            uint8_t amp = 0;

            if (power > power_cutoff_ && info_->max_power > 0)
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
    text_max.set(to_string_dec_uint(info_->max_power));
    update_range_controls();
    set_dirty();
}

void IQTrimView::update_range_controls() {
    auto max_range = info_ ? info_->sample_count : 0;
    auto step = info_ ? info_->sample_count / screen_width : 0;

    field_start.set_range(0, max_range);
    field_start.set_step(step);
    field_end.set_range(0, max_range);
    field_end.set_step(step);
}

void IQTrimView::compute_range() {
    if (!info_)
        return;

    bool has_start = false;
    uint8_t start = 0;
    uint8_t end = 0;

    power_cutoff_ = (static_cast<uint64_t>(field_cutoff.value()) * info_->max_power) / 100;

    for (size_t i = 0; i < power_buckets_.size(); ++i) {
        auto power = power_buckets_[i].power;

        if (power > power_cutoff_) {
            if (has_start)
                end = i;
            else {
                start = i;
                end = i;
                has_start = true;
            }
        }
    }

    // NB: update_range_controls must have been called first.
    auto samples_per_bucket = info_->sample_count / power_buckets_.size();
    field_start.set_value(start * samples_per_bucket, false);
    field_end.set_value(end * samples_per_bucket, false);
}

void IQTrimView::profile_capture() {
    power_buckets_ = {};
    iq::PowerBuckets buckets{
        .p = power_buckets_.data(),
        .size = power_buckets_.size()};

    progress_ui.show_reading();
    info_ = iq::profile_capture(path_, buckets.size * 10, &buckets);
    progress_ui.clear();

    mode_ = TrimMode::Cutoff;
    update_range_controls();
    compute_range();
}

bool IQTrimView::trim_capture() {
    if (!info_) {
        nav_.display_modal("Error", "Open a file first.");
        return false;
    }

    bool trimmed = false;

    if (mode_ == TrimMode::Cutoff) {
        if (power_cutoff_ == 0 || power_cutoff_ > info_->max_power) {
            nav_.display_modal("Error", "Invalid power cutoff.");
            return false;
        }

        progress_ui.show_trimming();
        trimmed = iq::trim_capture_with_cutoff(path_, power_cutoff_, progress_ui.get_callback());
        progress_ui.clear();

    } else {
        iq::TrimRange trim_range{
            static_cast<uint32_t>(field_start.value()),
            static_cast<uint32_t>(field_end.value()),
            info_->sample_size};

        if (trim_range.start_sample >= trim_range.end_sample) {
            nav_.display_modal("Error", "Invalid trimming range.");
            return false;
        }

        progress_ui.show_trimming();
        trimmed = iq::trim_capture_with_range(path_, trim_range);
        progress_ui.clear();
    }

    if (!trimmed)
        nav_.display_modal("Error", "Trimming failed.");

    return trimmed;
}

}  // namespace ui
