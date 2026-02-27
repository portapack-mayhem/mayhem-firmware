/*
 * copyleft spammingdramaqueen
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

/*
 * Opera Cake is an antenna-switching add-on board for HackRF One.
 * It uses a PCA9557 I/O expander (8-bit output port) at I2C address 0x18.
 *
 * Supported modes:
 *   Manual    -- user directly chooses Port A0 and Port B0 connections.
 *   Frequency -- four frequency bands are mapped to ports A1-A4; the
 *                antenna is switched automatically whenever the receiver
 *                retunes (mirroring HackRF firmware behaviour where
 *                operacake_set_range() is called from tuning.c on every
 *                frequency change).  A4 is the catch-all fallback for
 *                unmatched frequencies.  B mirrors A.
 *
 * Time-based switching is NOT supported: the required timer GPIO pins
 * conflict with PortaPack hardware.
 */

#include "ui_opera_cake.hpp"
#include "opera_cake_app_boot.hpp"
#include "portapack.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::opera_cake {

// ---- Constructor -------------------------------------------------------

OperaCakeView::OperaCakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &text_status,
        &options_mode,
        &field_min_a1,
        &field_max_a1,
        &field_min_a2,
        &field_max_a2,
        &field_min_a3,
        &field_max_a3,
        &field_min_a4,
        &field_max_a4,
        &options_port_a,
        &options_port_b,
        &button_apply,
        &button_rescan,
        &text_result,
    });

    // Clamp loaded settings to valid ranges
    if (setting_mode > 1) setting_mode = 0;
    if (setting_port_a > 3) setting_port_a = 0;
    if (setting_port_b > 3) setting_port_b = 0;

    // Apply loaded settings to widgets
    options_mode.set_selected_index(setting_mode, false);
    options_port_a.set_selected_index(setting_port_a, false);
    options_port_b.set_selected_index(setting_port_b, false);

    field_min_a1.set_value(static_cast<int32_t>(setting_min_a1), false);
    field_max_a1.set_value(static_cast<int32_t>(setting_max_a1), false);
    field_min_a2.set_value(static_cast<int32_t>(setting_min_a2), false);
    field_max_a2.set_value(static_cast<int32_t>(setting_max_a2), false);
    field_min_a3.set_value(static_cast<int32_t>(setting_min_a3), false);
    field_max_a3.set_value(static_cast<int32_t>(setting_max_a3), false);
    field_min_a4.set_value(static_cast<int32_t>(setting_min_a4), false);
    field_max_a4.set_value(static_cast<int32_t>(setting_max_a4), false);

    detect_board();

    // Wire up callbacks to keep settings in sync with widget state
    options_mode.on_change = [this](size_t idx, int32_t) {
        setting_mode = static_cast<uint8_t>(idx);
    };
    options_port_a.on_change = [this](size_t idx, int32_t) {
        setting_port_a = static_cast<uint8_t>(idx);
    };
    options_port_b.on_change = [this](size_t idx, int32_t) {
        setting_port_b = static_cast<uint8_t>(idx);
    };

    field_min_a1.on_change = [this](int32_t v) { setting_min_a1 = static_cast<uint32_t>(v); };
    field_max_a1.on_change = [this](int32_t v) { setting_max_a1 = static_cast<uint32_t>(v); };
    field_min_a2.on_change = [this](int32_t v) { setting_min_a2 = static_cast<uint32_t>(v); };
    field_max_a2.on_change = [this](int32_t v) { setting_max_a2 = static_cast<uint32_t>(v); };
    field_min_a3.on_change = [this](int32_t v) { setting_min_a3 = static_cast<uint32_t>(v); };
    field_max_a3.on_change = [this](int32_t v) { setting_max_a3 = static_cast<uint32_t>(v); };
    field_min_a4.on_change = [this](int32_t v) { setting_min_a4 = static_cast<uint32_t>(v); };
    field_max_a4.on_change = [this](int32_t v) { setting_max_a4 = static_cast<uint32_t>(v); };

    button_apply.on_select = [this](Button&) {
        if (setting_mode == 0)
            apply_manual();
        else
            apply_frequency();
    };

    button_rescan.on_select = [this](Button&) {
        text_result.set("");
        detect_board();
    };
}

// ---- Board detection ---------------------------------------------------

void OperaCakeView::detect_board() {
    // Re-probe the board (it may have been connected after boot).
    // update_config will probe and apply the current mode/ranges.
    ::opera_cake::FreqRanges ranges;
    build_ranges(ranges);
    board_detected_ = ::opera_cake::update_config(
        setting_mode, setting_port_a, setting_port_b, ranges);
    text_status.set(board_detected_ ? "Found at 0x18" : "Not detected");
}

// ---- Build ranges from UI settings -------------------------------------

void OperaCakeView::build_ranges(::opera_cake::FreqRanges& ranges) {
    ranges.mins[0] = static_cast<uint16_t>(setting_min_a1);
    ranges.maxs[0] = static_cast<uint16_t>(setting_max_a1);
    ranges.mins[1] = static_cast<uint16_t>(setting_min_a2);
    ranges.maxs[1] = static_cast<uint16_t>(setting_max_a2);
    ranges.mins[2] = static_cast<uint16_t>(setting_min_a3);
    ranges.maxs[2] = static_cast<uint16_t>(setting_max_a3);
    ranges.mins[3] = static_cast<uint16_t>(setting_min_a4);
    ranges.maxs[3] = static_cast<uint16_t>(setting_max_a4);
}

// ---- Manual mode -------------------------------------------------------

void OperaCakeView::apply_manual() {
    ::opera_cake::FreqRanges ranges;
    build_ranges(ranges);
    if (::opera_cake::update_config(0, setting_port_a, setting_port_b, ranges))
        text_result.set("Applied OK");
    else
        text_result.set("Err: board not found");
}

// ---- Frequency mode ----------------------------------------------------

void OperaCakeView::apply_frequency() {
    ::opera_cake::FreqRanges ranges;
    build_ranges(ranges);
    if (::opera_cake::update_config(1, setting_port_a, setting_port_b, ranges)) {
        text_result.set("Auto-switch active");
    } else {
        text_result.set("Err: board not found");
    }
}

// ---- Focus -------------------------------------------------------------

void OperaCakeView::focus() {
    options_mode.focus();
}

}  // namespace ui::external_app::opera_cake
