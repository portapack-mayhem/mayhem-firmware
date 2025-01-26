/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_view_wav.hpp"
#include "ui_fileman.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

void ViewWavView::update_scale(int32_t new_scale) {
    scale = new_scale;
    ns_per_pixel = (1000000000UL / wav_reader->sample_rate()) * scale;
    refresh_waveform();
    refresh_measurements();
}

void ViewWavView::refresh_waveform() {
    // NB: We can't read from the file to update the waveform when playback is in progress, so defer til playback done.
    // (This only happens if the user messes with position or scale fields while playback is occurring)
    if (playback_in_progress) {
        waveform_update_needed = true;
        return;
    }

    uint8_t bits_per_sample = wav_reader->bits_per_sample();

    for (size_t i = 0; i < 240; i++) {
        wav_reader->data_seek(position + (i * scale));
        if (bits_per_sample == 8) {
            uint8_t sample;
            wav_reader->read(&sample, 1);
            waveform_buffer[i] = (sample - 0x80) * 256;
        } else {
            wav_reader->read(&waveform_buffer[i], 2);
        }
    }

    waveform.set_dirty();

    // Window
    uint64_t w_start = (position * 240) / wav_reader->sample_count();
    uint64_t w_width = (scale * 240) / (wav_reader->sample_count() / 240);
    display.fill_rectangle({0, 10 * 16 + 1, 240, 16}, Theme::getInstance()->bg_darkest->background);
    display.fill_rectangle({(Coord)w_start, 21 * 8, (Dim)w_width + 1, 8}, Theme::getInstance()->bg_darkest->foreground);
    display.draw_line({0, 10 * 16 + 1}, {(Coord)w_start, 21 * 8}, Theme::getInstance()->bg_darkest->foreground);
    display.draw_line({239, 10 * 16 + 1}, {(Coord)(w_start + w_width), 21 * 8}, Theme::getInstance()->bg_darkest->foreground);
}

void ViewWavView::refresh_measurements() {
    uint64_t span_ns = ns_per_pixel * abs(field_cursor_b.value() - field_cursor_a.value());

    if (span_ns)
        text_delta.set(unit_auto_scale(span_ns, 0, 3) + "s (" + to_string_dec_uint(1000000000UL / span_ns) + "Hz)");
    else
        text_delta.set("0us ?Hz");
}

void ViewWavView::paint(Painter& painter) {
    // Waveform limits
    painter.draw_hline({0, 6 * 16 - 1}, 240, Theme::getInstance()->bg_medium->background);
    painter.draw_hline({0, 10 * 16}, 240, Theme::getInstance()->bg_medium->background);

    // Overall amplitude view, 0~127 to 0~255 color index
    for (size_t i = 0; i < 240; i++)
        painter.draw_vline({(Coord)i, 11 * 16}, 8, spectrum_rgb2_lut[amplitude_buffer[i] << 1]);
}

void ViewWavView::on_pos_time_changed() {
    position = (uint64_t)((field_pos_seconds.value() * 1000) + field_pos_milliseconds.value()) * wav_reader->sample_rate() / 1000;
    field_pos_milliseconds.set_range(0, ((uint32_t)field_pos_seconds.value() == wav_reader->ms_duration() / 1000) ? wav_reader->ms_duration() % 1000 : 999);
    if (!updating_position) {
        updating_position = true;  // prevent recursion
        field_pos_samples.set_value(position);
        updating_position = false;
    }
    refresh_waveform();
}

void ViewWavView::on_pos_sample_changed() {
    position = field_pos_samples.value();
    if (!updating_position) {
        updating_position = true;  // prevent recursion
        field_pos_seconds.set_value(field_pos_samples.value() / wav_reader->sample_rate());
        field_pos_milliseconds.set_value((field_pos_samples.value() * 1000ull / wav_reader->sample_rate()) % 1000);
        updating_position = false;
    }
    refresh_waveform();
}

void ViewWavView::load_wav(std::filesystem::path file_path) {
    uint32_t average;

    wav_file_path = file_path;

    text_filename.set(file_path.filename().string());
    auto ms_duration = wav_reader->ms_duration();
    text_duration.set(unit_auto_scale(ms_duration, 2, 3) + "s");

    wav_reader->rewind();

    text_samplerate.set(to_string_dec_uint(wav_reader->sample_rate()) + "Hz");
    text_bits_per_sample.set(to_string_dec_uint(wav_reader->bits_per_sample(), 2));
    text_title.set(wav_reader->title());

    // Fill amplitude buffer, world's worst downsampling
    uint64_t skip = wav_reader->sample_count() / (240 * subsampling_factor);
    uint8_t bits_per_sample = wav_reader->bits_per_sample();

    for (size_t i = 0; i < 240; i++) {
        average = 0;

        for (size_t s = 0; s < subsampling_factor; s++) {
            wav_reader->data_seek(((i * subsampling_factor) + s) * skip);
            if (bits_per_sample == 8) {
                uint8_t sample;
                wav_reader->read(&sample, 1);
                average += sample / 2;
            } else {
                int16_t sample;
                wav_reader->read(&sample, 2);
                average += (abs(sample) >> 8);
            }
        }

        amplitude_buffer[i] = average / subsampling_factor;
    }

    reset_controls();
    update_scale(1);
}

void ViewWavView::reset_controls() {
    field_scale.set_value(1);
    field_pos_seconds.set_value(0);
    field_pos_samples.set_value(0);
    field_cursor_a.set_value(0);
    field_cursor_b.set_value(0);
    field_pos_seconds.set_range(0, wav_reader->ms_duration() / 1000);
    field_pos_milliseconds.set_range(0, (wav_reader->ms_duration() < 1000) ? wav_reader->ms_duration() % 1000 : 999);
    field_pos_samples.set_range(0, wav_reader->sample_count() - 1);
    field_scale.set_range(1, std::min(99999ul, wav_reader->sample_count() / 240));
}

bool ViewWavView::is_active() {
    return (bool)replay_thread;
}

void ViewWavView::stop() {
    if (is_active())
        replay_thread.reset();

    audio::output::stop();

    button_play.set_bitmap(&bitmap_play);
    ready_signal = false;
}

void ViewWavView::handle_replay_thread_done(const uint32_t return_code) {
    (void)return_code;

    stop();
    progressbar.set_value(0);

    if (return_code == ReplayThread::READ_ERROR)
        file_error();

    // Playback complete - now it's safe to update waveform view
    playback_in_progress = false;
    if (waveform_update_needed) {
        waveform_update_needed = false;
        refresh_waveform();
    }
}

void ViewWavView::set_ready() {
    ready_signal = true;
}

void ViewWavView::file_error() {
    nav_.display_modal("Error", "File read error.");
}

void ViewWavView::start_playback() {
    uint32_t sample_rate;
    uint8_t bits_per_sample;

    auto reader = std::make_unique<WAVFileReader>();

    stop();

    if (!reader->open(wav_file_path)) {
        file_error();
        return;
    }

    playback_in_progress = true;

    button_play.set_bitmap(&bitmap_stop);

    sample_rate = reader->sample_rate();
    bits_per_sample = reader->bits_per_sample();

    progressbar.set_max(reader->sample_count());

    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        read_size, buffer_count,
        &ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });

    baseband::set_audiotx_config(
        1536000 / 20,     // Rate of sending progress updates
        0,                // Transmit BW = 0 = not transmitting
        0,                // Gain - unused
        8,                // shift_bits_s16, default 8 bits - unused
        bits_per_sample,  // bits_per_sample
        0,                // tone key disabled
        false,            // AM
        false,            // DSB
        false,            // USB
        false             // LSB
    );
    baseband::set_sample_rate(sample_rate);
    transmitter_model.set_sampling_rate(1536000);

    audio::output::start();
}

void ViewWavView::on_playback_progress(const uint32_t progress) {
    progressbar.set_value(progress);
    field_pos_samples.set_value(progress);
}

ViewWavView::ViewWavView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
    wav_reader = std::make_unique<WAVFileReader>();

    add_children({&labels,
                  &text_filename,
                  &text_samplerate,
                  &text_title,
                  &text_duration,
                  &text_bits_per_sample,
                  &button_open,
                  &button_play,
                  &waveform,
                  &progressbar,
                  &field_pos_seconds,
                  &field_pos_milliseconds,
                  &field_pos_samples,
                  &field_scale,
                  &field_cursor_a,
                  &field_cursor_b,
                  &text_delta,
                  &field_volume});

    reset_controls();

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".WAV");
        open_view->on_changed = [this, &nav](std::filesystem::path file_path) {
            // Can't show new dialogs in an on_changed handler, so use continuation.
            nav.set_on_pop([this, &nav, file_path]() {
                if (!wav_reader->open(file_path)) {
                    file_error();
                    return;
                }
                if ((wav_reader->channels() != 1) || ((wav_reader->bits_per_sample() != 8) && (wav_reader->bits_per_sample() != 16))) {
                    nav_.display_modal("Error", "Wrong format.\nWav viewer only accepts\n8 or 16-bit mono files.");
                    return;
                }
                load_wav(file_path);
                field_pos_seconds.focus();
            });
        };
    };

    field_volume.set_value(field_volume.value());

    button_play.on_select = [this, &nav](ImageButton&) {
        if (this->is_active())
            stop();
        else
            start_playback();
    };

    field_scale.on_change = [this](int32_t value) {
        update_scale(value);
    };
    field_pos_seconds.on_change = [this](int32_t) {
        on_pos_time_changed();
    };
    field_pos_milliseconds.on_change = [this](int32_t) {
        on_pos_time_changed();
    };
    field_pos_samples.on_change = [this](int32_t) {
        on_pos_sample_changed();
    };

    field_cursor_a.on_change = [this](int32_t v) {
        waveform.set_cursor(0, v);
        refresh_measurements();
    };
    field_cursor_b.on_change = [this](int32_t v) {
        waveform.set_cursor(1, v);
        refresh_measurements();
    };
}

void ViewWavView::focus() {
    button_open.focus();
}

ViewWavView::~ViewWavView() {
    stop();
    baseband::shutdown();
}

} /* namespace ui */
