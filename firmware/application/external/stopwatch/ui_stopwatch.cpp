/*
 * Copyright 2025 Mark Thompson
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

#include "ui_stopwatch.hpp"
#include "portapack.hpp"
#include "ch.h"

using namespace portapack;

namespace ui::external_app::stopwatch {

StopwatchView::StopwatchView(NavigationView& nav) {
    add_children({
        &labels,
        &button_run_stop,
        &button_reset_lap,
        &button_done,
        &big_display,
        &lap_display,
    });

    button_run_stop.on_select = [this](Button&) {
        if (running)
            stop();
        else
            run();
    };

    button_reset_lap.on_select = [this](Button&) {
        if (running)
            lap();
        else
            reset();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };
}

void StopwatchView::focus() {
    button_run_stop.focus();
}

void StopwatchView::run() {
    running = true;
    start_time = chTimeNow() - previously_elapsed;
    button_run_stop.set_text("STOP");
    button_reset_lap.set_text("LAP");
}

void StopwatchView::stop() {
    running = false;
    end_time = chTimeNow();
    previously_elapsed = end_time - start_time;
    button_run_stop.set_text("START");
    button_reset_lap.set_text("RESET");
}

void StopwatchView::reset() {
    lap_time = end_time = start_time = previously_elapsed = 0;
    big_display.set(0);
    lap_display.set(0);
}

void StopwatchView::lap() {
    lap_time = chTimeNow();
    lap_display.set((lap_time - start_time)*1000);
}

void StopwatchView::paint(Painter& painter) {
    (void)painter;
    if (running) {
        end_time = chTimeNow();
        big_display.set((end_time - start_time)*1000);
    }
}

void StopwatchView::frame_sync() {
    set_dirty();
}

}  // namespace ui::external_app::stopwatch