/*
 * copyleft 2024 sommermorgentraum
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

#include "ui_tuner.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui::external_app::tuner {

TunerView::TunerView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());  // proc_audio_beep baseband is external too

    add_children({
        &labels,
        &field_volume,
        &options_instrument,
        &options_note,
        &button_play_stop,
        &text_note_frequency,
        &text_note_octave_shift,
    });

    audio::set_rate(audio::Rate::Hz_24000);

    options_instrument.on_change = [this](size_t, int32_t value) {
        const Instrument* selected_instrument = nullptr;

        switch (value) {
            case 0:
                selected_instrument = &guitar;
                break;
            case 1:
                selected_instrument = &violin;
                break;
            case 2:
                selected_instrument = &pitch_fork;
                break;
        }

        if (selected_instrument) {
            update_notes_for_instrument(*selected_instrument);
        }

        update_current_note();
    };

    options_note.on_change = [this](size_t, int32_t index) {
        const Instrument* current_instrument = nullptr;
        switch (options_instrument.selected_index_value()) {
            case 0:
                current_instrument = &guitar;
                break;
            case 1:
                current_instrument = &violin;
                break;
            case 2:
                current_instrument = &pitch_fork;
                break;
        }

        if (current_instrument) {
            auto it = current_instrument->notes.begin();
            std::advance(it, index);
            if (it != current_instrument->notes.end()) {
                current_note_frequency = it->second.frequency;
                current_note_sample_rate = it->second.sample_rate;
                current_note_octave_shift = it->second.octave_shift;
            }
        }

        update_current_note();
    };

    button_play_stop.on_select = [this]() {
        if (current_note_playing) {
            stop_play();
        } else {
            play_change_note();
        }
    };

    options_instrument.set_selected_index(0);
    update_notes_for_instrument(guitar);
    update_current_note();

    field_volume.set_value(0);  // seems that a change is required to force update, so setting to 0 first
    field_volume.set_value(99);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
}

TunerView::~TunerView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void TunerView::focus() {
    options_instrument.focus();
}

void TunerView::update_notes_for_instrument(const Instrument& instrument) {
    std::vector<OptionsField::option_t> note_options;
    size_t index = 0;

    for (const auto& note_pair : instrument.notes) {
        note_options.emplace_back(OptionsField::option_t{
            note_pair.first,
            index++});
    }

    options_note.set_options(note_options);
    if (!note_options.empty()) {
        options_note.set_selected_index(0);
    }
}

void TunerView::update_current_note() {
    const Instrument* current_instrument = nullptr;

    switch (options_instrument.selected_index_value()) {
        case 0:
            current_instrument = &guitar;
            break;
        case 1:
            current_instrument = &violin;
            break;
        case 2:
            current_instrument = &pitch_fork;
            break;
    }

    if (current_instrument) {
        std::string note_name = options_note.selected_index_name();

        // map
        auto note_it = current_instrument->notes.find(note_name);
        if (note_it != current_instrument->notes.end()) {
            current_note_frequency = note_it->second.frequency;
            current_note_sample_rate = note_it->second.sample_rate;
            current_note_octave_shift = note_it->second.octave_shift;

            text_note_frequency.set(std::to_string(current_note_frequency));
            text_note_octave_shift.set(std::to_string(current_note_octave_shift));

            set_dirty();

            if (current_note_playing) {
                play_change_note();
            }
        }
    }
}

void TunerView::stop_play() {
    baseband::request_beep_stop();
    current_note_playing = false;
    button_play_stop.set_bitmap(&bitmap_icon_replay);
}

void TunerView::play_change_note() {
    if (current_note_playing) {
        baseband::request_beep_stop();
        audio::set_rate(current_note_sample_rate);
        baseband::request_audio_beep(current_note_frequency, protected_sample_rate(current_note_sample_rate), 0);
    } else {
        audio::set_rate(current_note_sample_rate);
        baseband::request_audio_beep(current_note_frequency, protected_sample_rate(current_note_sample_rate), 0);
    }
    current_note_playing = true;
    button_play_stop.set_bitmap(&bitmap_icon_sleep);
    set_dirty();
}

uint32_t TunerView::protected_sample_rate(audio::Rate r) {
    switch (r) {
        case audio::Rate::Hz_12000:
            return 12000;
        case audio::Rate::Hz_24000:
            return 24000;
        case audio::Rate::Hz_48000:
            return 48000;
        default:
            return 24000;
    }
}

void TunerView::paint(Painter& painter) {
    View::paint(painter);

    painter.fill_rectangle(
        {screen_width / 4, 8 * 16, screen_width / 2, 6 * 16},
        Theme::getInstance()->bg_darkest->background);

    if (!current_note_playing) return;

    painter.fill_rectangle(
        {screen_width / 4, 10 * 16, 2, 2 * 16},
        Theme::getInstance()->fg_light->foreground);

    painter.fill_rectangle(
        {screen_width / 4, 12 * 16, screen_width / 4 * 2 + 2, 2},
        Theme::getInstance()->fg_light->foreground);

    painter.fill_rectangle(
        {(screen_width / 4) * 3, 10 * 16, 2, 2 * 16},
        Theme::getInstance()->fg_light->foreground);

    painter.draw_string(
        {screen_width / 4 - 2 * 8, 8 * 16},
        (current_note_octave_shift == 0) ? *Theme::getInstance()->bg_darkest : *Theme::getInstance()->fg_red,
        std::to_string(current_note_frequency));

    int16_t real_frequency = current_note_frequency;
    if (current_note_octave_shift > 0) {
        for (int i = 0; i < current_note_octave_shift; i++) {
            real_frequency *= 2;
        }
    } else if (current_note_octave_shift < 0) {
        for (int i = 0; i > current_note_octave_shift; i--) {
            real_frequency /= 2;
        }
    }
    painter.draw_string({(screen_width / 4) * 3 - 2 * 8, 8 * 16},
                        (current_note_octave_shift == 0) ? *Theme::getInstance()->bg_darkest : *Theme::getInstance()->fg_red,
                        std::to_string(real_frequency));

    painter.draw_string({screen_width / 2 - 3 * 16, 13 * 16},
                        (current_note_octave_shift == 0) ? *Theme::getInstance()->bg_darkest : *Theme::getInstance()->fg_red,
                        std::to_string(current_note_octave_shift) + " * 8ev");
}

}  // namespace ui::external_app::tuner
