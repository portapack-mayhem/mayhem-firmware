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

#include "ui_metronome.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui::external_app::metronome {

MetronomeView::MetronomeView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());  // proc_audio_beep baseband is external too

    add_children({
        &labels,
        &field_volume,
        &button_play_stop,
        &field_rythm_unaccent_time,
        &field_rythm_accent_time,
        &field_accent_beep_tune,
        &field_unaccent_beep_tune,
        &field_beep_flash_duration,
        &field_bpm,
        &progressbar,
    });

    field_bpm.set_value(120);
    field_rythm_accent_time.set_value(4);
    field_rythm_unaccent_time.set_value(4);
    field_accent_beep_tune.set_value(880);
    field_unaccent_beep_tune.set_value(440);
    field_beep_flash_duration.set_value(100);
    button_play_stop.on_select = [this]() {
        if (playing_) {
            stop_play();
        } else {
            play();
        }
    };

    field_volume.set_value(0);  // seems that a change is required to force update, so setting to 0 first
    field_volume.set_value(99);

    audio::set_rate(audio::Rate::Hz_48000);

    audio::output::start();
}

MetronomeView::~MetronomeView() {
    should_exit = true;
    if (thread) {
        chThdWait(thread);
        thread = nullptr;
    }
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void MetronomeView::focus() {
    field_bpm.focus();
}

void MetronomeView::stop_play() {
    if (playing_) {
        playing_ = false;
        button_play_stop.set_bitmap(&bitmap_icon_replay);
        baseband::request_beep_stop();
        progressbar.set_value(0);
        progressbar.set_style(Theme::getInstance()->fg_light);
    }
}

void MetronomeView::play() {
    if (!playing_) {
        playing_ = true;
        current_beat_ = 0;
        button_play_stop.set_bitmap(&bitmap_icon_sleep);

        if (!thread) {
            thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, MetronomeView::static_fn, this);
        }
    }
}

void MetronomeView::beep_accent_beat() {
    baseband::request_audio_beep(field_accent_beep_tune.value(), 48000, field_beep_flash_duration.value());
}

void MetronomeView::beep_unaccent_beat() {
    baseband::request_audio_beep(field_unaccent_beep_tune.value(), 48000, field_beep_flash_duration.value());
}

// TODO: draw the beat
// void MetronomeView::paint(Painter& painter) {
//     View::paint(painter);

// painter.fill_rectangle(
//     {visual_x, visual_y, visual_width, visual_height},
//     Theme::getInstance()->bg_darkest->background);

// if (playing_) {
//     const bool is_accent_beat = (current_beat_ % field_rythm_accent_time.value()) == 0;

// const Color beat_color = is_accent_beat ?
//     Theme::getInstance()->fg_red->foreground :
//     Theme::getInstance()->fg_green->foreground;

// painter.fill_rectangle(
//     {visual_x + visual_width/4,
//      visual_y + visual_height/4,
//      visual_width/2,
//      visual_height/2},
//     beat_color);
// }
// }

msg_t MetronomeView::static_fn(void* arg) {
    auto obj = static_cast<MetronomeView*>(arg);
    obj->run();
    return 0;
}

void MetronomeView::run() {
    while (!should_exit) {
        if (!playing_) {
            chThdSleepMilliseconds(100);
            continue;
        }

        uint32_t base_interval = (60 * 1000) / field_bpm.value();  // quarter note as 1 beat

        uint32_t beats_per_measure = field_rythm_unaccent_time.value();  // how many beates per bar
        progressbar.set_max(beats_per_measure);
        uint32_t beat_unit = field_rythm_accent_time.value();  // which note type (quarter, eighth, etc.) as 1 beat

        uint32_t actual_interval = (base_interval * 4) / beat_unit;  // e.g. when beat_unit==8 it's 1/2 of base_interval AKA eighths notes

        uint32_t beat_in_measure = current_beat_ % beats_per_measure;  // current beat in this bar (need to decide accent or unaccent)
        progressbar.set_value(beat_in_measure + 1);

        // accent beat is the first beat of this bar
        if (beat_in_measure == 0) {
            beep_accent_beat();
            progressbar.set_style(Theme::getInstance()->fg_red);
        } else {
            beep_unaccent_beat();
            progressbar.set_style(Theme::getInstance()->fg_green);
        }

        current_beat_++;
        chThdSleepMilliseconds(actual_interval);
    }
}

}  // namespace ui::external_app::metronome