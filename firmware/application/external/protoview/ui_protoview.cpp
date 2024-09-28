/*
 * Copyright (C) 2024 HTotoo, zxkmm
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

#include "ui_protoview.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::protoview {

void ProtoView::focus() {
    field_frequency.focus();
}

ProtoView::ProtoView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &label_zoom,
                  &label_shift,
                  &options_zoom,
                  &number_shift,
                  &button_reset,
                  &button_pause,
                  &waveform,
                  &waveform2,
                  &waveform3,
                  &waveform4});

    field_frequency.set_step(100);
    options_zoom.on_change = [this](size_t, int32_t v) {
        zoom = v;
        draw();
        draw2();
    };
    number_shift.on_change = [this](int32_t value) {
        waveform_shift = value;
        draw();
        draw2();
    };
    button_reset.on_select = [this](Button&) {
        reset();
    };
    button_pause.on_select = [this](Button&) {
        set_pause(!paused);
    };
    set_pause(false);  // need to use this to default hide shift functionality
    baseband::set_subghzd_config(0, receiver_model.sampling_rate());
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    receiver_model.enable();
}

void ProtoView::reset() {
    cnt = 0;
    set_pause(false);
    number_shift.set_value(0);
    waveform_shift = 0;
    for (uint16_t i = 0; i < MAXSIGNALBUFFER; i++) time_buffer[i] = 0;
    needCntReset = false;
    draw();
    draw2();
}

void ProtoView::on_timer() {
    timercnt++;
    if ((timercnt % 90) == 0) {
        if (datacnt == 0) {
            needCntReset = true;
        }
        datacnt = 0;
    }
}

void ProtoView::draw2() {
    if (drawcnt < MAXDRAWCNTPERWF) {
        waveform.set_length(drawcnt);
        waveform2.set_length(0);
        waveform3.set_length(0);
        waveform4.set_length(0);
    } else if (drawcnt < MAXDRAWCNTPERWF * 2) {
        waveform.set_length(MAXDRAWCNTPERWF);
        waveform2.set_length(drawcnt - MAXDRAWCNTPERWF);
        waveform3.set_length(0);
        waveform4.set_length(0);
    } else if (drawcnt < MAXDRAWCNTPERWF * 3) {
        waveform.set_length(MAXDRAWCNTPERWF);
        waveform2.set_length(MAXDRAWCNTPERWF);
        waveform3.set_length(drawcnt - MAXDRAWCNTPERWF * 2);
        waveform4.set_length(0);
    } else {
        waveform.set_length(MAXDRAWCNTPERWF);
        waveform2.set_length(MAXDRAWCNTPERWF);
        waveform3.set_length(MAXDRAWCNTPERWF);
        waveform4.set_length(drawcnt - MAXDRAWCNTPERWF * 3);
    }
    waveform.set_dirty();
    waveform2.set_dirty();
    waveform3.set_dirty();
    waveform4.set_dirty();
}

void ProtoView::draw() {
    uint32_t remain = 0;
    int32_t lmax = 0;
    bool lmaxstate = false;
    bool state = false;
    drawcnt = 0;
    for (uint16_t i = 0; i < MAXDRAWCNT; i++) waveform_buffer[i] = 0;  // reset

    // add empty data for padding (so you can shift left/negative)
    if (waveform_shift < 0) {
        for (int32_t i = 0; (i < -1 * waveform_shift) && drawcnt < MAXDRAWCNT;  // this is for shift negative (move to right)
             ++i) {
            waveform_buffer[drawcnt++] = 0;
        }
    }
    uint16_t skipped = 0;
    uint16_t to_skip = ((waveform_shift > 0) ? waveform_shift : 0);  // when >0 it'll skip some to move left
    for (uint16_t i = 0;
         i < MAXSIGNALBUFFER && drawcnt < MAXDRAWCNT;  // prevent out of ranging
         ++i) {
        state = time_buffer[i] >= 0;
        int32_t timeabs = state ? time_buffer[i] : -1 * time_buffer[i];
        int32_t timesize = timeabs / zoom;
        if (timesize == 0) {
            remain += timeabs;
            if (lmax < timeabs) {
                lmax = timeabs;
                lmaxstate = state;
            }
            if (remain / zoom > 0) {
                timesize = remain / zoom;
                state = lmaxstate;
            } else {
                continue;
            }
        }
        remain = 0;
        lmax = 0;
        for (int32_t ii = 0; ii < timesize && drawcnt < MAXDRAWCNT; ++ii) {
            if (skipped < to_skip) {
                skipped++;
            } else {
                waveform_buffer[drawcnt++] = state;
            }
        }
    }
}

void ProtoView::add_time(int32_t time) {
    if (cnt >= MAXSIGNALBUFFER) cnt = 0;
    time_buffer[cnt++] = time;
}

void ProtoView::on_data(const ProtoViewDataMessage* message) {
    if (paused) return;
    // filter out invalid ones.
    uint16_t start = 0;
    uint16_t stop = 0;
    bool has_valid = false;
    for (uint16_t i = 0; i <= message->maxptr; ++i) {
        if (message->times[i] >= 30000 || message->times[i] <= -30000) {
            if (!has_valid) {
                start = i;
            }
        } else {
            has_valid = true;
            stop = i;
        }
    }
    if (!has_valid) return;  // no valid data arrived
    // if (needCntReset) reset();   //todo implement auto reset

    datacnt++;

    // valid data, redraw
    for (uint16_t i = start; i <= stop; i++) {
        add_time(message->times[i]);
    }

    draw();
    draw2();
}

void ProtoView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void ProtoView::set_pause(bool pause) {
    paused = pause;
    if (pause) {
        label_shift.hidden(false);
        number_shift.hidden(false);
        button_pause.set_text(LanguageHelper::currentMessages[LANG_RESUME]);
    } else {
        label_shift.hidden(true);
        number_shift.hidden(true);
        button_pause.set_text(LanguageHelper::currentMessages[LANG_PAUSE]);
    }
    set_dirty();
}

ProtoView::~ProtoView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::protoview
