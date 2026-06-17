/*
 * Copyleft zxkmm (>) 2026
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

#include "ui_constellation.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_spectrum.hpp"

#include <algorithm>

using namespace portapack;

namespace ui::external_app::constellation {

namespace {
constexpr Color color_point = Color::green();
constexpr Color color_axis = Color::dark_grey();
}  // namespace

ConstellationWidget::ConstellationWidget(
    Rect parent_rect,
    const uint8_t* iq_data,
    size_t point_count)
    : Widget{parent_rect},
      iq_data_{iq_data},
      point_count_{point_count} {
    recompute_geometry();
    reset_cache();
}

void ConstellationWidget::set_parent_rect(const Rect new_parent_rect) {
    Widget::set_parent_rect(new_parent_rect);
    recompute_geometry();
    reset_cache();
}

void ConstellationWidget::on_show() {
    reset_cache();
    set_dirty();
}

void ConstellationWidget::reset_cache() {
    history_count_ = 0;
    history_head_ = 0;
    cur_count_ = 0;
    needs_clear_ = true;
}

void ConstellationWidget::recompute_geometry() {
    const auto r = screen_rect();
    plot_side_ = std::min<Dim>(r.width(), r.height());
    plot_left_ = r.left() + (r.width() - plot_side_) / 2;
    plot_top_ = r.top() + (r.height() - plot_side_) / 2;
    center_x_ = plot_left_ + plot_side_ / 2;
    center_y_ = plot_top_ + plot_side_ / 2;
}

void ConstellationWidget::set_persistence_frames(uint8_t frames) {
    const auto clamped = static_cast<uint8_t>(std::clamp<size_t>(
        frames,
        1,
        max_persistence_frames));
    if (clamped != persistence_frames_) {
        persistence_frames_ = clamped;
        reset_cache();
        set_dirty();
    }
}

Color ConstellationWidget::background_at(Coord x, Coord y) const {
    // Keep the static crosshair intact when erasing aged points.
    if (x == center_x_ || y == center_y_) {
        return color_axis;
    }
    return Theme::getInstance()->bg_darkest->background;
}

void ConstellationWidget::paint(Painter& painter) {
    const auto r = screen_rect();
    const auto background = Theme::getInstance()->bg_darkest->background;

    if (!r || !iq_data_ || !point_count_ || plot_side_ < 2) {
        if (needs_clear_) {
            painter.fill_rectangle_unrolled8(r, background);
            needs_clear_ = false;
        }
        return;
    }

    if (needs_clear_) {
        painter.fill_rectangle_unrolled8(r, background);
        // Static crosshair.
        painter.draw_vline({center_x_, plot_top_}, plot_side_, color_axis);
        painter.draw_hline({plot_left_, center_y_}, plot_side_, color_axis);
        needs_clear_ = false;
        history_count_ = 0;
        history_head_ = 0;
    }

    // Map this frame's I/Q pairs to pixels. I -> x (right), Q -> y (up).
    const int32_t span = plot_side_ - 1;
    const size_t n = std::min(point_count_, max_points);
    cur_count_ = n;
    for (size_t k = 0; k < n; k++) {
        const int32_t i_val = iq_data_[k * 2];
        const int32_t q_val = iq_data_[k * 2 + 1];
        cur_x_[k] = static_cast<Coord>(plot_left_ + (i_val * span) / 255);
        cur_y_[k] = static_cast<Coord>(plot_top_ + ((255 - q_val) * span) / 255);
    }

    // Draw the current frame.
    for (size_t k = 0; k < n; k++) {
        display.draw_pixel({cur_x_[k], cur_y_[k]}, color_point);
    }

    // Expire the oldest frame, erasing only pixels no longer lit by the current
    // frame or any other retained frame.
    if (history_count_ >= persistence_frames_) {
        const size_t expired_slot = history_head_;
        const size_t expired_len = hist_len_[expired_slot];

        for (size_t e = 0; e < expired_len; e++) {
            const Coord ex = hist_x_[expired_slot][e];
            const Coord ey = hist_y_[expired_slot][e];
            bool keep = false;

            for (size_t k = 0; k < cur_count_ && !keep; k++) {
                if (cur_x_[k] == ex && cur_y_[k] == ey) {
                    keep = true;
                }
            }

            for (size_t i = 1; i < history_count_ && !keep; i++) {
                const size_t slot = (history_head_ + i) % max_persistence_frames;
                const size_t len = hist_len_[slot];
                for (size_t j = 0; j < len; j++) {
                    if (hist_x_[slot][j] == ex && hist_y_[slot][j] == ey) {
                        keep = true;
                        break;
                    }
                }
            }

            if (!keep) {
                display.draw_pixel({ex, ey}, background_at(ex, ey));
            }
        }

        history_head_ = (history_head_ + 1) % max_persistence_frames;
        --history_count_;
    }

    // Store the current frame in the ring buffer.
    const size_t tail_slot = (history_head_ + history_count_) % max_persistence_frames;
    for (size_t k = 0; k < n; k++) {
        hist_x_[tail_slot][k] = cur_x_[k];
        hist_y_[tail_slot][k] = cur_y_[k];
    }
    hist_len_[tail_slot] = static_cast<uint8_t>(n);
    ++history_count_;
}

ConstellationView::ConstellationView(NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({
        &labels,
        &field_frequency,
        &field_frequency_step,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &options_sample_rate,
        &options_decimation,
        &options_persistence,
        &check_frequency,
        &check_phase,
        &plot,
    });

    field_frequency_step.set_by_value(receiver_model.frequency_step());
    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        field_frequency.set_step(v);
    };

    options_sample_rate.set_by_nearest_value(sampling_rate);
    sampling_rate = options_sample_rate.selected_index_value();
    options_sample_rate.on_change = [this](size_t, OptionsField::value_t v) {
        sampling_rate = v;
        apply_config();
    };

    options_decimation.set_by_nearest_value(decimation);
    decimation = options_decimation.selected_index_value();
    options_decimation.on_change = [this](size_t, OptionsField::value_t v) {
        decimation = v;
        apply_config();
    };

    options_persistence.set_by_nearest_value(persistence_frames);
    persistence_frames = options_persistence.selected_index_value();
    plot.set_persistence_frames(persistence_frames);
    options_persistence.on_change = [this](size_t, OptionsField::value_t v) {
        persistence_frames = static_cast<uint8_t>(v);
        plot.set_persistence_frames(persistence_frames);
    };

    check_frequency.set_value(correct_frequency != 0);
    check_frequency.on_select = [this](Checkbox&, bool v) {
        correct_frequency = v ? 1 : 0;
        apply_config();
    };

    check_phase.set_value(correct_phase != 0);
    check_phase.on_select = [this](Checkbox&, bool v) {
        correct_phase = v ? 1 : 0;
        apply_config();
    };

    receiver_model.set_squelch_level(0);
    receiver_model.enable();
    apply_config();
}

ConstellationView::~ConstellationView() {
    receiver_model.disable();
    baseband::shutdown();
}

void ConstellationView::focus() {
    field_frequency.focus();
}

void ConstellationView::on_show() {
    baseband::spectrum_streaming_start();
}

void ConstellationView::on_hide() {
    baseband::spectrum_streaming_stop();
}

void ConstellationView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    plot.set_parent_rect({0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height});
}

void ConstellationView::apply_config() {
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(sampling_rate));

    baseband::set_constellation(
        sampling_rate,
        decimation,
        correct_frequency != 0,
        correct_phase != 0);
}

void ConstellationView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    const size_t count = std::min(spectrum.db.size(), sizeof(iq_buffer));
    std::copy(spectrum.db.begin(), spectrum.db.begin() + count, iq_buffer);
    plot.set_dirty();
}

void ConstellationView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

}  // namespace ui::external_app::constellation
