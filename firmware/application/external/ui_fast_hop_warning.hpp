/*
 * Copyright (C) 2026
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

#ifndef __UI_FAST_HOP_WARNING_HPP__
#define __UI_FAST_HOP_WARNING_HPP__

#include <cstddef>
#include <utility>

#include "ch.h"
#include "ui_navigation.hpp"

namespace ui::external_app {

constexpr size_t fast_hop_warning_threshold_ms = 50;

template <typename StartTx>
void start_with_fast_hop_warning(
    NavigationView& nav,
    const size_t hop_interval_ms,
    StartTx&& start_tx) {
    // 50ms is safe for NXP MCU
    if (hop_interval_ms >= fast_hop_warning_threshold_ms) {
        start_tx();
        return;
    }

    nav.display_modal(
        "Warning",
        "Hopping interval is\nbelow 50ms.\n\n"
        "THIS WILL FREEZE\nTHE HACKRF.\n"
        "Press RESET button\nto stop.\n\n"
        "Are you sure?",
        YESNO,
        [start_tx = std::forward<StartTx>(start_tx)](bool choice) mutable {
            if (choice) {
                chThdSleepMilliseconds(50);
                start_tx();
            }
        },
        TRUE);
}

}  // namespace ui::external_app

#endif /* __UI_FAST_HOP_WARNING_HPP__ */
