/*
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

#include "ui_audio_test.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui::external_app::audio_test {

AudioTestView::AudioTestView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());  // proc_audio_beep baseband is external too

    add_children({&labels,
                  &options_sample_rate,
                  &field_frequency,
                  &options_step,
                  &field_duration,
                  &field_volume,
                  &toggle_speaker});

    audio::set_rate(audio::Rate::Hz_24000);
    options_sample_rate.on_change = [this](size_t, int32_t v) {
        switch (v) {
            case 12000:
                audio::set_rate(audio::Rate::Hz_12000);
                break;
            case 24000:
                audio::set_rate(audio::Rate::Hz_24000);
                break;
            case 48000:
                audio::set_rate(audio::Rate::Hz_48000);
                break;
        }
        field_frequency.set_range(v / 128, v / 2);
        update_audio_beep();
    };
    options_sample_rate.set_selected_index(0, 1);

    field_frequency.on_change = [this](int32_t) {
        update_audio_beep();
    };
    field_frequency.set_value(1000);

    options_step.on_change = [this](size_t, int32_t v) {
        field_frequency.set_step(v);
    };
    options_step.set_by_value(100);

    field_duration.on_change = [this](int32_t v) {
        (void)v;
        update_audio_beep();
    };
    field_duration.set_value(100);

    toggle_speaker.on_change = [this](bool v) {
        beep = v;
        update_audio_beep();
    };

    field_volume.set_value(0);  // seems that a change is required to force update, so setting to 0 first
    field_volume.set_value(80);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
}

AudioTestView::~AudioTestView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void AudioTestView::focus() {
    toggle_speaker.focus();
}

void AudioTestView::update_audio_beep() {
    if (beep)
        baseband::request_audio_beep(field_frequency.value(), options_sample_rate.selected_index_value(), field_duration.value());
    else
        baseband::request_beep_stop();
}

} /* namespace ui::external_app::audio_test */
