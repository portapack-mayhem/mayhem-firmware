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

#include "ui_audio_test.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui {

AudioTestView::AudioTestView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());  // proc_audio_beep baseband is external too

    add_children({&labels,
                  &field_frequency,
                  &field_duration,
                  &field_volume,
                  &toggle_speaker});

    field_frequency.set_value(1000);
    field_frequency.on_change = [this](int32_t v) {
        (void)v;
        update_audio_beep();
    };

    field_duration.set_value(100);
    field_duration.on_change = [this](int32_t v) {
        (void)v;
        update_audio_beep();
    };

    toggle_speaker.on_change = [this](bool v) {
        beep = v;
        update_audio_beep();
    };

    field_volume.set_value(0);
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
    field_frequency.focus();
}

void AudioTestView::update_audio_beep() {
    if (beep)
        baseband::request_audio_beep(field_frequency.value(), field_duration.value());
    else
        baseband::request_beep_stop();
}

} /* namespace ui */
