/*
 * Copyright 2025 Mark Thompson
 * copyleft 2025 zxkmm AKA zix aka sommermorgentraum
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

// clang-format off
// clang-format doesn't allow use as this way but it's not worth to have const var for this thing. too expensive

// minute
#define TOTAL_M1_POS {0 * 8 * 4, 3 * 16}
#define TOTAL_M2_POS {1 * 8 * 4, 3 * 16}

// sec
#define TOTAL_S1_POS {2 * 8 * 4 + 24, 3 * 16}
#define TOTAL_S2_POS {3 * 8 * 4 + 24, 3 * 16}

// ms
#define TOTAL_MS1_POS {4 * 8 * 4 + 7 * 8, 3 * 16 + 24}
#define TOTAL_MS2_POS {5 * 8 * 4 + 5 * 8, 3 * 16 + 24}
#define TOTAL_MS3_POS {6 * 8 * 4 + 3 * 8, 3 * 16 + 24}

#define LAP_Y 9 * 16

//lap min
#define LAP_M1_POS {0 * 8 * 4, LAP_Y}
#define LAP_M2_POS {1 * 8 * 4, LAP_Y}

//lap sec
#define LAP_S1_POS {2 * 8 * 4 + 24, LAP_Y}
#define LAP_S2_POS {3 * 8 * 4 + 24, LAP_Y}

//lap ms
#define LAP_MS1_POS {4 * 8 * 4 + 7 * 8, LAP_Y + 24}
#define LAP_MS2_POS {5 * 8 * 4 + 5 * 8, LAP_Y + 24}
#define LAP_MS3_POS {6 * 8 * 4 + 3 * 8, LAP_Y + 24}
// clang-format on

StopwatchView::StopwatchView(NavigationView& nav)
    : painter{} {
    add_children({
        &labels,
        &button_run_stop,
        &button_reset_lap,
        &button_done,
        &options_ms_display_level,
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

    options_ms_display_level.on_change = [&](size_t, ui::OptionsField::value_t) {
        clean_ms_display(options_ms_display_level.selected_index());
    };

    options_ms_display_level.set_selected_index(0);

    refresh_painting();
}

void StopwatchView::focus() {
    button_run_stop.focus();
}

void StopwatchView::run() {
    options_ms_display_level.hidden(true);
    // ^ this won't take efferts if don't set_dirty, but needed to let user can't change during ticking
    if (!paused) refresh_painting();
    paused = false;
    running = true;
    start_time = chTimeNow() - previously_elapsed;
    button_run_stop.set_text("STOP");
    button_reset_lap.set_text("LAP");
}

void StopwatchView::stop() {
    options_ms_display_level.hidden(false);
    running = false;
    paused = true;
    end_time = chTimeNow();
    previously_elapsed = end_time - start_time;
    button_run_stop.set_text("START");
    button_reset_lap.set_text("RESET");
}

void StopwatchView::reset() {
    lap_time = end_time = start_time = previously_elapsed = 0;
    for (uint8_t i = 0; i < 7; i++) {
        refresh_painting();
    }
    refresh_painting();
    set_dirty();
}

void StopwatchView::lap() {
    lap_time = chTimeNow();
}

void StopwatchView::paint(Painter&) {
}

void StopwatchView::refresh_painting() {
    // minute
    painter.draw_char(TOTAL_M1_POS, *Theme::getInstance()->fg_light, ' ', 4);
    painter.draw_char(TOTAL_M2_POS, *Theme::getInstance()->fg_light, ' ', 4);

    // sec
    painter.draw_char(TOTAL_S1_POS, *Theme::getInstance()->fg_light, ' ', 4);
    painter.draw_char(TOTAL_S2_POS, *Theme::getInstance()->fg_light, ' ', 4);

    // ms
    painter.draw_char(TOTAL_MS1_POS, *Theme::getInstance()->fg_light, ' ', 2);
    painter.draw_char(TOTAL_MS2_POS, *Theme::getInstance()->fg_light, ' ', 2);
    painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_light, ' ', 2);

    // lap min
    painter.draw_char(LAP_M1_POS, *Theme::getInstance()->fg_light, ' ', 4);
    painter.draw_char(LAP_M2_POS, *Theme::getInstance()->fg_light, ' ', 4);

    // lap sec
    painter.draw_char(LAP_S1_POS, *Theme::getInstance()->fg_light, ' ', 4);
    painter.draw_char(LAP_S2_POS, *Theme::getInstance()->fg_light, ' ', 4);

    // lap ms
    painter.draw_char(LAP_MS1_POS, *Theme::getInstance()->fg_light, ' ', 2);
    painter.draw_char(LAP_MS2_POS, *Theme::getInstance()->fg_light, ' ', 2);
    painter.draw_char(LAP_MS3_POS, *Theme::getInstance()->fg_light, ' ', 2);
}

/*when user seted it to display more, then display less later, this prevent the remain value exist on screen*/
void StopwatchView::clean_ms_display(uint8_t level) {
    level++;
    switch (level) {
        case 0:
            painter.draw_char(TOTAL_MS1_POS, *Theme::getInstance()->fg_light, ' ', 2);
            painter.draw_char(TOTAL_MS2_POS, *Theme::getInstance()->fg_light, ' ', 2);
            painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_light, ' ', 2);
            break;
        case 1:
            painter.draw_char(TOTAL_MS2_POS, *Theme::getInstance()->fg_light, ' ', 2);
            painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_light, ' ', 2);
            break;
        case 2:
            painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_light, ' ', 2);
            break;
    }
}

void StopwatchView::resume_last() {
    // minute
    painter.draw_char(TOTAL_M1_POS, *Theme::getInstance()->fg_light, last_displayed[0], 4);
    painter.draw_char(TOTAL_M2_POS, *Theme::getInstance()->fg_light, last_displayed[1], 4);

    // sec
    painter.draw_char(TOTAL_S1_POS, *Theme::getInstance()->fg_light, last_displayed[2], 4);
    painter.draw_char(TOTAL_S2_POS, *Theme::getInstance()->fg_light, last_displayed[3], 4);

    // ms
    painter.draw_char(TOTAL_MS1_POS, *Theme::getInstance()->fg_light, last_displayed[4], 2);
    painter.draw_char(TOTAL_MS2_POS, *Theme::getInstance()->fg_light, last_displayed[5], 2);
    painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_light, last_displayed[6], 2);
}

/* NB:
 * Due to the flaw of dirty management, it's using work around, to reduce screen flickering:
 *
 * for example when xx:15:xxx turn to xx:16:xxx, it actually only paint 6, the 1 is old,
 * but not dirty, so we can see without flikering.
 *
 * So with these work around, it won't show false info, but bare in mind that it could be, if you add more things.
 */
void StopwatchView::frame_sync() {
    uint32_t elapsed_ticks = 0;

    if (running) {
        end_time = chTimeNow();
        elapsed_ticks = end_time - start_time;
    } else if (previously_elapsed > 0) {
        elapsed_ticks = previously_elapsed;
    }

    constexpr uint32_t TICKS_PER_SECOND = CH_FREQUENCY;
    constexpr uint32_t TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;

    uint32_t minutes = elapsed_ticks / TICKS_PER_MINUTE;
    uint32_t seconds = (elapsed_ticks % TICKS_PER_MINUTE) / TICKS_PER_SECOND;
    uint32_t milliseconds = ((elapsed_ticks % TICKS_PER_SECOND) * 1000) / TICKS_PER_SECOND;

    // minute
    if (last_displayed[0] != (minutes / 10) % 10)
        painter.draw_char(TOTAL_M1_POS, *Theme::getInstance()->fg_red, '0' + (minutes / 10) % 10, 4);
    if (last_displayed[1] != minutes % 10)
        painter.draw_char(TOTAL_M2_POS, *Theme::getInstance()->fg_red, '0' + minutes % 10, 4);

    // sec
    if (last_displayed[2] != (seconds / 10) % 10)
        painter.draw_char(TOTAL_S1_POS, *Theme::getInstance()->fg_green, '0' + (seconds / 10) % 10, 4);
    if (last_displayed[3] != seconds % 10)
        painter.draw_char(TOTAL_S2_POS, *Theme::getInstance()->fg_green, '0' + seconds % 10, 4);

    // ms
    /*   v place holder to aligh logic*/
    if ((true) && last_displayed[4] != (milliseconds / 100) % 10)
        painter.draw_char(TOTAL_MS1_POS, *Theme::getInstance()->fg_yellow, '0' + (milliseconds / 100) % 10, 2);
    if ((options_ms_display_level.selected_index() >= 1) && last_displayed[5] != (milliseconds / 10) % 10)
        painter.draw_char(TOTAL_MS2_POS, *Theme::getInstance()->fg_yellow, '0' + (milliseconds / 10) % 10, 2);
    if ((options_ms_display_level.selected_index() >= 2) && last_displayed[6] != milliseconds % 10)
        painter.draw_char(TOTAL_MS3_POS, *Theme::getInstance()->fg_yellow, '0' + milliseconds % 10, 2);

    // min
    last_displayed[0] = (minutes / 10) % 10;
    last_displayed[1] = minutes % 10;
    // sec
    last_displayed[2] = (seconds / 10) % 10;
    last_displayed[3] = seconds % 10;
    // ms
    last_displayed[4] = (milliseconds / 100) % 10;
    last_displayed[5] = (milliseconds / 10) % 10;
    last_displayed[6] = milliseconds % 10;

    if (lap_time > 0) {
        uint32_t lap_elapsed = lap_time - start_time;

        uint32_t lap_minutes = lap_elapsed / TICKS_PER_MINUTE;
        uint32_t lap_seconds = (lap_elapsed % TICKS_PER_MINUTE) / TICKS_PER_SECOND;
        uint32_t lap_milliseconds = ((lap_elapsed % TICKS_PER_SECOND) * 1000) / TICKS_PER_SECOND;

        // lap min
        if (lap_last_displayed[0] == (lap_minutes / 10) % 10) {
            painter.draw_char(LAP_M1_POS, *Theme::getInstance()->fg_light, '0' + (lap_minutes / 10) % 10, 4);
        }
        if (lap_last_displayed[1] == lap_minutes % 10) {
            painter.draw_char(LAP_M2_POS, *Theme::getInstance()->fg_light, '0' + lap_minutes % 10, 4);
        }

        // lap sec
        if (lap_last_displayed[2] == (lap_seconds / 10) % 10) {
            painter.draw_char(LAP_S1_POS, *Theme::getInstance()->fg_light, '0' + (lap_seconds / 10) % 10, 4);
        }
        if (lap_last_displayed[3] == lap_seconds % 10) {
            painter.draw_char(LAP_S2_POS, *Theme::getInstance()->fg_light, '0' + lap_seconds % 10, 4);
        }

        // lap ms
        if (lap_last_displayed[4] == (lap_milliseconds / 100) % 10) {
            painter.draw_char(LAP_MS1_POS, *Theme::getInstance()->fg_light, '0' + (lap_milliseconds / 100) % 10, 2);
        }
        if (lap_last_displayed[5] == (lap_milliseconds / 10) % 10) {
            painter.draw_char(LAP_MS2_POS, *Theme::getInstance()->fg_light, '0' + (lap_milliseconds / 10) % 10, 2);
        }
        if (lap_last_displayed[6] == lap_milliseconds % 10) {
            painter.draw_char(LAP_MS3_POS, *Theme::getInstance()->fg_light, '0' + lap_milliseconds % 10, 2);
        }

        // lp m
        lap_last_displayed[0] = (lap_minutes / 10) % 10;
        lap_last_displayed[1] = lap_minutes % 10;

        // lp s
        lap_last_displayed[2] = (lap_seconds / 10) % 10;
        lap_last_displayed[3] = lap_seconds % 10;

        // lp mss
        lap_last_displayed[4] = (lap_milliseconds / 100) % 10;
        lap_last_displayed[5] = (lap_milliseconds / 10) % 10;
        lap_last_displayed[6] = lap_milliseconds % 10;
    }
}

}  // namespace ui::external_app::stopwatch