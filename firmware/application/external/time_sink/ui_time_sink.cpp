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

#include "ui_time_sink.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_spectrum.hpp"

#include <algorithm>

using namespace portapack;

namespace ui::external_app::time_sink {

TimeSinkWaveformWidget::TimeSinkWaveformWidget(
    Rect parent_rect,
    const int8_t* data,
    size_t length,
    Color color)
    : Widget{parent_rect},
      data_{data},
      length_{length},
      color_{color} {
    reset_cache();
}

void TimeSinkWaveformWidget::set_parent_rect(const Rect new_parent_rect) {
    Widget::set_parent_rect(new_parent_rect);
    reset_cache();
}

void TimeSinkWaveformWidget::on_show() {
    reset_cache();
    set_dirty();
}

void TimeSinkWaveformWidget::reset_cache() {
    history_count_ = 0;
    history_head_ = 0;
    needs_clear_ = true;
}

void TimeSinkWaveformWidget::set_persistence_frames(uint8_t frames) {
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

Coord TimeSinkWaveformWidget::sample_to_y(const Rect& r, int8_t sample) const {
    const int32_t y_center = r.top() + (r.height() / 2);
    const int32_t y_span = std::max<int32_t>(1, r.height() - 1);
    const int32_t y = y_center - (static_cast<int32_t>(sample) * y_span) / 256;
    return static_cast<Coord>(std::clamp<int32_t>(y, r.top(), r.bottom() - 1));
}

void TimeSinkWaveformWidget::paint(Painter& painter) {
    const auto r = screen_rect();
    const auto background = Theme::getInstance()->bg_darkest->background;

    if (!r || !data_ || !length_) {
        if (needs_clear_) {
            painter.fill_rectangle_unrolled8(r, background);
            needs_clear_ = false;
        }
        history_count_ = 0;
        history_head_ = 0;
        return;
    }

    const size_t columns = std::min<size_t>({
        length_,
        max_columns,
        static_cast<size_t>(r.width()),
    });
    if (!columns) {
        history_count_ = 0;
        history_head_ = 0;
        return;
    }

    if (needs_clear_) {
        painter.fill_rectangle_unrolled8(r, background);
        needs_clear_ = false;
        history_count_ = 0;
        history_head_ = 0;
    }

    for (size_t x = 0; x < columns; ++x) {
        const size_t src_index = (x * length_) / columns;
        current_y_[x] = sample_to_y(r, data_[src_index]);
    }

    for (size_t x = 0; x < columns; ++x) {
        display.draw_pixel(
            {static_cast<Coord>(r.left() + x), current_y_[x]},
            color_);
    }

    if (history_count_ >= persistence_frames_) {
        const size_t expired_slot = history_head_;

        for (size_t x = 0; x < columns; ++x) {
            const auto expired_y = sample_to_y(r, history_samples_[expired_slot][x]);
            bool keep = (expired_y == current_y_[x]);

            if (!keep) {
                for (size_t i = 1; i < history_count_; ++i) {
                    const size_t slot = (history_head_ + i) % max_persistence_frames;
                    if (sample_to_y(r, history_samples_[slot][x]) == expired_y) {
                        keep = true;
                        break;
                    }
                }
            }

            if (!keep) {
                display.draw_pixel(
                    {static_cast<Coord>(r.left() + x), expired_y},
                    background);
            }
        }

        history_head_ = (history_head_ + 1) % max_persistence_frames;
        --history_count_;
    }

    const size_t tail_slot = (history_head_ + history_count_) % max_persistence_frames;
    for (size_t x = 0; x < columns; ++x) {
        const size_t src_index = (x * length_) / columns;
        history_samples_[tail_slot][x] = data_[src_index];
    }
    ++history_count_;
}

TimeSinkView::TimeSinkView(NavigationView& nav)
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
        &field_trigger,
        &options_persistence,
        &options_trigger_mode,
        &field_trigger_level,
        &waveform,
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
        apply_spectrum_config();
    };

    field_trigger.set_value(trigger);
    field_trigger.on_change = [this](int32_t v) {
        trigger = static_cast<uint8_t>(v);
        apply_spectrum_config();
    };

    options_persistence.set_by_nearest_value(persistence_frames);
    persistence_frames = options_persistence.selected_index_value();
    waveform.set_persistence_frames(persistence_frames);
    options_persistence.on_change = [this](size_t, OptionsField::value_t v) {
        persistence_frames = static_cast<uint8_t>(v);
        waveform.set_persistence_frames(persistence_frames);
    };

    options_trigger_mode.set_by_nearest_value(trigger_mode);
    trigger_mode = static_cast<uint8_t>(options_trigger_mode.selected_index_value());
    options_trigger_mode.on_change = [this](size_t, OptionsField::value_t v) {
        trigger_mode = static_cast<uint8_t>(v);
        trigger_lock_valid = false;
    };

    field_trigger_level.set_value(trigger_level);
    field_trigger_level.on_change = [this](int32_t v) {
        trigger_level = v;
        trigger_lock_valid = false;
    };

    receiver_model.set_squelch_level(0);
    receiver_model.enable();
    apply_spectrum_config();
}

TimeSinkView::~TimeSinkView() {
    receiver_model.disable();
    baseband::shutdown();
}

void TimeSinkView::focus() {
    field_frequency.focus();
}

void TimeSinkView::on_show() {
    baseband::spectrum_streaming_start();
}

void TimeSinkView::on_hide() {
    baseband::spectrum_streaming_stop();
}

void TimeSinkView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    waveform.set_parent_rect({0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height});
}

void TimeSinkView::apply_spectrum_config() {
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(sampling_rate));

    baseband::set_time_sink(
        sampling_rate,
        trigger);
}

size_t TimeSinkView::find_stable_trigger_index(const ChannelSpectrum& spectrum) {
    if (spectrum.db.size() < 2) {
        return 0;
    }

    TriggerMode mode = TriggerMode::Rising;
    switch (trigger_mode) {
        case static_cast<uint8_t>(TriggerMode::Off):
            mode = TriggerMode::Off;
            break;
        case static_cast<uint8_t>(TriggerMode::Rising):
            mode = TriggerMode::Rising;
            break;
        case static_cast<uint8_t>(TriggerMode::Falling):
            mode = TriggerMode::Falling;
            break;
        default:
            break;
    }

    if (mode == TriggerMode::Off) {
        trigger_lock_valid = false;
        return 0;
    }

    constexpr int32_t hysteresis = 2;
    const int32_t threshold = std::clamp<int32_t>(
        128 + trigger_level,
        hysteresis,
        255 - hysteresis);
    const size_t center = spectrum.db.size() / 2;
    const size_t reference_index = trigger_lock_valid ? trigger_lock_index : center;

    bool found = false;
    size_t best_index = 0;
    size_t best_distance = spectrum.db.size();
    const auto circular_distance = [count = spectrum.db.size()](size_t a, size_t b) -> size_t {
        const size_t linear =
            (a > b) ? (a - b) : (b - a);
        return std::min(linear, count - linear);
    };

    for (size_t i = 1; i < spectrum.db.size(); ++i) {
        const int32_t prev = spectrum.db[i - 1];
        const int32_t curr = spectrum.db[i];

        bool crossing = false;
        if (mode == TriggerMode::Rising) {
            crossing =
                (prev <= (threshold - hysteresis)) &&
                (curr >= (threshold + hysteresis));
        } else {
            crossing =
                (prev >= (threshold + hysteresis)) &&
                (curr <= (threshold - hysteresis));
        }

        if (!crossing) {
            continue;
        }

        const size_t distance = circular_distance(i, reference_index);
        if (!found || (distance < best_distance)) {
            found = true;
            best_index = i;
            best_distance = distance;
        }
    }

    if (found) {
        trigger_lock_index = best_index;
        trigger_lock_valid = true;
        return best_index;
    }

    return trigger_lock_valid ? trigger_lock_index : 0;
}

void TimeSinkView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    const size_t trigger_index = find_stable_trigger_index(spectrum);
    const size_t source_count = spectrum.db.size();
    const size_t window_size = source_count;

    for (size_t x = 0; x < waveform_points; x++) {
        const size_t offset = (x * window_size) / waveform_points;
        const size_t src_index =
            (trigger_index + offset) % source_count;
        const int32_t centered = static_cast<int32_t>(spectrum.db[src_index]) - 128;
        waveform_buffer[x] = static_cast<int8_t>(std::clamp<int32_t>(centered, -128, 127));
    }

    waveform.set_dirty();
}

void TimeSinkView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

}  // namespace ui::external_app::time_sink