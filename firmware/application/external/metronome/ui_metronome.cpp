/*
 * copyleft 2024 zxkmm AKA zix aka sommermorgentraum
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
#include "ui_textentry.hpp"

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
        &button_enter_tap_tempo,
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

    button_enter_tap_tempo.on_select = [this](Button&) {
        auto tap_tempo_view = nav_.push<MetronomeTapTempoView>(field_bpm.value());

        tap_tempo_view->on_apply = [this](uint16_t bpm) {
            field_bpm.set_value(bpm);
        };
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

MetronomeTapTempoView::MetronomeTapTempoView(NavigationView& nav, uint16_t bpm)
    : nav_{nav},
      bpm_{bpm} {
    add_children({
        &button_input,
        &button_tap,
        &button_cancel,
        &button_apply,
    });

    bpm_when_entered_ = bpm;  // save for if user cancel

    // im aware that we have duplicated painter which means in this app, weo have two painter instances
    // here is the reason why this is necessary:
    // We need to draw the bpm big font once when enter, which would be at bad timing in constructor,
    // cuz it happened before the view is pushed to nav, which casued it actually didn't draw
    // which leads me have to override the paint func from father and draw inside of it.
    //
    // BUT I can't completely package the draw logic inside of the paint func,
    // cuz set_dirty has flaw and cause screen flicker during the char changes, if i just package there and use set_dirty()
    Painter painter_instance_2;

    button_input.on_select = [this](Button&) {
        input_buffer = to_string_dec_uint(bpm_);
        text_prompt(
            nav_,
            input_buffer,
            3,
            ENTER_KEYBOARD_MODE_DIGITS,
            [this](std::string& buffer) {
                if (buffer.empty()) {
                    return;
                }
                bpm_ = atoi(buffer.c_str());

                if (on_apply) {
                    on_apply(bpm_);
                }
            });
    };

    button_tap.on_select = [&](Button&) {
        on_tap(painter_instance_2);
    };

    button_apply.on_select = [this](Button&) {
        // it's dynamically applied in tap handler
        // the design allow user to hear changes before apply
        nav_.pop();
    };

    button_cancel.on_select = [this](Button&) {
        bpm_ = bpm_when_entered_;
        if (on_apply) {
            on_apply(bpm_);
        }
        nav_.pop();
    };
}

void MetronomeTapTempoView::focus() {
    button_tap.focus();
}

void MetronomeTapTempoView::paint(Painter& painter) {
    View::paint(painter);
    painter.draw_char({(0 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + bpm_ / 100, 4);
    painter.draw_char({(1 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + (bpm_ / 10) % 10, 4);
    painter.draw_char({(2 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + bpm_ % 10, 4);
}

/*
NB: i don't really know if the cpu clock is 1000Hz AKA 1ms per tick for chTimeNow()
    but it should be, refering to the stop watch app.
    and also i compared with my real metronome and it's very close
    so I assume it's 1ms per tick
*/
void MetronomeTapTempoView::on_tap(Painter& painter) {
    /*                                      ^ NB: this painter accepted from painter_instance_2*/
    systime_t current_time = chTimeNow();
    if (last_tap_time > 0) {
        uint32_t interval_ms = current_time - last_tap_time;

        if (interval_ms > 100) {
            uint16_t this_time_bpm = 60000 / interval_ms;

            if (this_time_bpm > 0 && this_time_bpm < 400) {
                bpms_deque.push_back(this_time_bpm);
                if (bpms_deque.size() > 4) {  // one bar length cuz most music tempo is quarter note as 1 beat
                    bpms_deque.pop_front();
                }

                // avg
                uint32_t sum = 0;
                for (auto& bpm : bpms_deque) {
                    sum += bpm;
                }
                bpm_ = sum / bpms_deque.size();

                if (on_apply) {
                    on_apply(bpm_);
                }

                painter.draw_char({(0 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + bpm_ / 100, 4);
                painter.draw_char({(1 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + (bpm_ / 10) % 10, 4);
                painter.draw_char({(2 * 16) * 4 + 2 * 16, 3 * 16}, *Theme::getInstance()->fg_light, '0' + bpm_ % 10, 4);
            }
        }
    }
    last_tap_time = current_time;
}

}  // namespace ui::external_app::metronome