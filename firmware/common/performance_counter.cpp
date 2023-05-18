/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "performance_counter.hpp"
#include "ch.h"

uint8_t get_cpu_utilisation_in_percent() {
    static systime_t last_time = 0;
    static systime_t last_idle_ticks = 0;

    auto now = chTimeNow();
    auto idle_ticks = chThdGetTicks(chSysGetIdleThread());

    if (last_time == 0) {
        last_time = now;
        last_idle_ticks = idle_ticks;

        return 0;
    }

    int32_t time_elapsed = now - last_time;
    int32_t idle_elapsed = idle_ticks - last_idle_ticks;

    int32_t working_ticks = time_elapsed - idle_elapsed;

    if (working_ticks < 0)
        working_ticks = 0;

    auto utilisation = working_ticks * 100 / time_elapsed;

    last_time = now;
    last_idle_ticks = idle_ticks;

    if (utilisation > 100) {
        return 100;
    }

    return (uint8_t)utilisation;
}
