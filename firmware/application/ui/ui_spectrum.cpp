/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_spectrum.hpp"

#include "spectrum_color_lut.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "baseband_api.hpp"

#include "string_format.hpp"

#include <cmath>
#include <array>

namespace ui {
namespace spectrum {

/* AudioSpectrumView ******************************************************/

AudioSpectrumView::AudioSpectrumView(
    const Rect parent_rect)
    : View{parent_rect} {
    set_focusable(true);

    add_children({&labels,
                  &field_frequency,
                  &waveform});

    field_frequency.on_change = [this](int32_t) {
        set_dirty();
    };
    field_frequency.set_value(0);
}

void AudioSpectrumView::paint(Painter& painter) {
    const auto r = screen_rect();

    painter.fill_rectangle(r, Theme::getInstance()->bg_darkest->background);

    // if( !spectrum_sampling_rate ) return;

    // Cursor
    const Rect r_cursor{
        field_frequency.value() / (48000 / 240), r.bottom() - 32 - cursor_band_height,
        1, cursor_band_height};
    painter.fill_rectangle(
        r_cursor,
        Color::red());
}

void AudioSpectrumView::on_audio_spectrum(const AudioSpectrum* spectrum) {
    for (size_t i = 0; i < spectrum->db.size(); i++)
        audio_spectrum[i] = ((int16_t)spectrum->db[i] - 127) * 256;
    waveform.set_dirty();
}

/* FrequencyScale ********************************************************/

void FrequencyScale::on_show() {
    clear();
}

void FrequencyScale::set_spectrum_sampling_rate(const int new_sampling_rate) {
    if ((spectrum_sampling_rate != new_sampling_rate)) {
        spectrum_sampling_rate = new_sampling_rate;
        set_dirty();
    }
}

void FrequencyScale::set_channel_filter(
    const int low_frequency,
    const int high_frequency,
    const int transition) {
    if ((channel_filter_low_frequency != low_frequency) ||
        (channel_filter_high_frequency != high_frequency) ||
        (channel_filter_transition != transition)) {
        channel_filter_low_frequency = low_frequency;
        channel_filter_high_frequency = high_frequency;
        channel_filter_transition = transition;
        set_dirty();
    }
}

void FrequencyScale::paint(Painter& painter) {
    const auto r = screen_rect();

    clear_background(painter, r);

    if (!spectrum_sampling_rate) {
        // Can't draw without non-zero scale.
        return;
    }

    draw_filter_ranges(painter, r);
    draw_frequency_ticks(painter, r);

    if (_blink) {
        const Rect r_cursor{
            118 + cursor_position, r.bottom() - filter_band_height,
            5, filter_band_height};
        painter.fill_rectangle(
            r_cursor,
            Color::red());
    }
}

void FrequencyScale::clear() {
    spectrum_sampling_rate = 0;
    set_dirty();
}

void FrequencyScale::clear_background(Painter& painter, const Rect r) {
    painter.fill_rectangle(r, Theme::getInstance()->bg_darkest->background);
}

void FrequencyScale::draw_frequency_ticks(Painter& painter, const Rect r) {
    const auto x_center = r.width() / 2;

    const Rect tick{r.left() + x_center, r.top(), 1, r.height()};
    painter.fill_rectangle(tick, Theme::getInstance()->bg_darkest->foreground);

    constexpr int tick_count_max = 4;
    float rough_tick_interval = float(spectrum_sampling_rate) / tick_count_max;
    int magnitude = 1;
    int magnitude_n = 0;
    while (rough_tick_interval >= 10.0f) {
        rough_tick_interval /= 10;
        magnitude *= 10;
        magnitude_n += 1;
    }
    const int tick_interval = std::ceil(rough_tick_interval);

    auto tick_offset = tick_interval;
    while ((tick_offset * magnitude) < spectrum_sampling_rate / 2) {
        const Dim pixel_offset = tick_offset * magnitude * spectrum_bins / spectrum_sampling_rate;

        const std::string zero_pad =
            ((magnitude_n % 3) == 0) ? "" : ((magnitude_n % 3) == 1) ? "0"
                                                                     : "00";
        const std::string unit =
            (magnitude_n >= 6) ? "M" : (magnitude_n >= 3) ? "k"
                                                          : "";
        const std::string label = to_string_dec_uint(tick_offset) + zero_pad + unit;
        const auto label_width = style().font.size_of(label).width();

        const Coord offset_low = r.left() + x_center - pixel_offset;
        const Rect tick_low{offset_low, r.top(), 1, r.height()};
        painter.fill_rectangle(tick_low, Theme::getInstance()->bg_darkest->foreground);
        painter.draw_string({offset_low + 2, r.top()}, style(), label);

        const Coord offset_high = r.left() + x_center + pixel_offset;
        const Rect tick_high{offset_high, r.top(), 1, r.height()};
        painter.fill_rectangle(tick_high, Theme::getInstance()->bg_darkest->foreground);
        painter.draw_string({offset_high - 2 - label_width, r.top()}, style(), label);

        tick_offset += tick_interval;
    }
}

void FrequencyScale::draw_filter_ranges(Painter& painter, const Rect r) {
    if (channel_filter_low_frequency != channel_filter_high_frequency) {
        const auto x_center = r.width() / 2;

        const auto x_low = x_center + channel_filter_low_frequency * spectrum_bins / spectrum_sampling_rate;
        const auto x_high = x_center + channel_filter_high_frequency * spectrum_bins / spectrum_sampling_rate;

        if (channel_filter_transition) {
            const auto trans = channel_filter_transition * spectrum_bins / spectrum_sampling_rate;

            const Rect r_all{
                r.left() + x_low - trans, r.bottom() - filter_band_height,
                x_high - x_low + trans * 2, filter_band_height};
            painter.fill_rectangle(
                r_all,
                Color::yellow());
        }

        const Rect r_pass{
            r.left() + x_low, r.bottom() - filter_band_height,
            x_high - x_low, filter_band_height};
        painter.fill_rectangle(
            r_pass,
            Color::green());
    }
}

void FrequencyScale::on_focus() {
    _blink = true;
    on_tick_second();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };
}

void FrequencyScale::on_blur() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    _blink = false;
    set_dirty();
}

bool FrequencyScale::on_encoder(const EncoderEvent delta) {
    cursor_position += delta;

    cursor_position = std::min<int32_t>(cursor_position, 119);
    cursor_position = std::max<int32_t>(cursor_position, -120);

    set_dirty();

    return true;
}

bool FrequencyScale::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (on_select) {
            on_select((cursor_position * spectrum_sampling_rate) / 240);
            cursor_position = 0;
            return true;
        }
    }

    return false;
}

void FrequencyScale::on_tick_second() {
    set_dirty();
    _blink = !_blink;
}

/* WaterfallWidget *********************************************************/
// TODO: buffer and use "paint" instead of immediate drawing would help with
// preventing flicker from drawing. Would use more RAM however.

void WaterfallWidget::on_show() {
    clear();

    const auto screen_r = screen_rect();
    display.scroll_set_area(screen_r.top(), screen_r.bottom());
}

void WaterfallWidget::on_hide() {
    /* TODO: Clear region to eliminate brief flash of content at un-shifted
     * position?
     */
    display.scroll_disable();
}

void WaterfallWidget::on_channel_spectrum(
    const ChannelSpectrum& spectrum) {
    /* TODO: static_assert that message.spectrum.db.size() >= pixel_row.size() */

    std::array<Color, 240> pixel_row;
    for (size_t i = 0; i < 120; i++) {
        const auto pixel_color = spectrum_rgb3_lut[spectrum.db[256 - 120 + i]];
        pixel_row[i] = pixel_color;
    }

    for (size_t i = 120; i < 240; i++) {
        const auto pixel_color = spectrum_rgb3_lut[spectrum.db[i - 120]];
        pixel_row[i] = pixel_color;
    }

    const auto draw_y = display.scroll(1);

    display.draw_pixels(
        {{0, draw_y}, {pixel_row.size(), 1}},
        pixel_row);
}

void WaterfallWidget::clear() {
    display.fill_rectangle(
        screen_rect(),
        Color::black());
}

/* WaterfallView *******************************************************/

WaterfallView::WaterfallView(const bool cursor) {
    add_children({&waterfall_widget,
                  &frequency_scale});

    frequency_scale.set_focusable(cursor);

    // Making the event climb up all the way up to here kinda sucks
    frequency_scale.on_select = [this](int32_t offset) {
        if (on_select) on_select(offset);
    };
}

void WaterfallView::on_show() {
    start();
}

void WaterfallView::on_hide() {
    stop();
}

void WaterfallView::start() {
    if (!running_) {
        baseband::spectrum_streaming_start();
        running_ = true;
    }
}

void WaterfallView::stop() {
    if (running_) {
        baseband::spectrum_streaming_stop();
        running_ = false;
    }
}

void WaterfallView::show_audio_spectrum_view(const bool show) {
    if ((audio_spectrum_view && show) || (!audio_spectrum_view && !show)) return;

    if (show) {
        audio_spectrum_view = std::make_unique<AudioSpectrumView>(audio_spectrum_view_rect);
        add_child(audio_spectrum_view.get());
        update_widgets_rect();
    } else {
        audio_spectrum_update = false;
        remove_child(audio_spectrum_view.get());
        audio_spectrum_view.reset();
        update_widgets_rect();
    }
}

void WaterfallView::update_widgets_rect() {
    if (audio_spectrum_view) {
        frequency_scale.set_parent_rect({0, audio_spectrum_height, screen_rect().width(), scale_height});
        waterfall_widget.set_parent_rect(waterfall_reduced_rect);
    } else {
        frequency_scale.set_parent_rect({0, 0, screen_rect().width(), scale_height});
        waterfall_widget.set_parent_rect(waterfall_normal_rect);
    }
    waterfall_widget.on_show();
}

void WaterfallView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    waterfall_normal_rect = {0, scale_height, new_parent_rect.width(), new_parent_rect.height() - scale_height};
    waterfall_reduced_rect = {0, audio_spectrum_height + scale_height, new_parent_rect.width(), new_parent_rect.height() - scale_height - audio_spectrum_height};

    update_widgets_rect();
}

void WaterfallView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    waterfall_widget.on_channel_spectrum(spectrum);
    sampling_rate = spectrum.sampling_rate;
    frequency_scale.set_spectrum_sampling_rate(sampling_rate);
    frequency_scale.set_channel_filter(
        spectrum.channel_filter_low_frequency,
        spectrum.channel_filter_high_frequency,
        spectrum.channel_filter_transition);
}

void WaterfallView::on_audio_spectrum() {
    audio_spectrum_view->on_audio_spectrum(audio_spectrum_data);
}

} /* namespace spectrum */

uint32_t filter_bandwidth_for_sampling_rate(int32_t sampling_rate) {
    switch (sampling_rate) {   // Use the var fs (sampling_rate) to set up BPF aprox < fs_max / 2 by Nyquist theorem.
        case 0 ... 3'500'000:  // BW Captured range BW (<=250K) : fs = 8x250k = 2000k, 16x150k = 2400k, 16x100k=1600k,
                               // 32x75k = 2400k, 32x50k=1600, 32x32k=1024, 64x25k = 1600k, 64x16k = 1024k, 64x12k5 = 800k.
            return 1'750'000;  // Minimum BPF MAX2837 for all those lower BW options.

        case 4'000'000 ... 7'000'000:  // OVS x8, BW capture range (500k...750kHz max) fs_max = 8 x 750k = 6Mhz
                                       // BW 500k...750kHz, ex. 500kHz (fs = 8 x BW = 4Mhz), BW 600kHz (fs = 4,8Mhz), BW 750 kHz (fs = 6Mhz).
            return 2'500'000;          // In some IC, MAX2837 appears as 2250000, but both work similarly.

        case 7'000'001 ... 10'000'000:  // OVS x8 and x4, BW capture 1Mhz fs = 8 x 1Mhz = 8Mhz. (1Mhz showed slightly higher noise background).
            return 3'500'000;           // some low SD cards, if not showing avg. writing speed >4MB/sec, they will produce sammples drop at REC with 1MB and C16 format.

        case 12'000'000 ... 14'000'000:  // OVS x4, BW capture 3Mhz, fs = 4 x 3Mhz = 12Mhz
                                         // Good BPF, good matching, we have some periodical M4 % samples drop.
            return 5'000'000;

        case 16'000'000:  // OVS x4, BW capture 4Mhz, fs = 4 x 4Mhz = 16Mhz
                          // Good BPF, good matching, we have some periodical M4 % samples drop.
            return 5'500'000;

        case 18'000'000:  // OVS x4, BW capture 4,5Mhz, fs = 4 x 4,5Mhz = 18Mhz
                          // Good BPF, good matching, we have some periodical M4 % samples drop.
            return 6'000'000;

        case 20'000'000:  // OVS x4, BW capture 5Mhz, fs = 4 x 5Mhz = 20Mhz
                          // Good BPF, good matching, we have some periodical M4 % samples drop.
            return 7'000'000;

        default:  // BW capture 5,5Mhz, fs = 4 x 5,5Mhz = 22Mhz max ADC sampling and others.
                  // We tested also 9Mhz FPB slightly too much noise floor, better at 8Mhz.
            return 8'000'000;
    }
}

} /* namespace ui */
