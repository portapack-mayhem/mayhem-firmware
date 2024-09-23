/*
 * Copyright (C) 2023 Bernd Herzog
 * Copyright (C) 2023 Mark Thompson
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

#include "ui_dfu_menu.hpp"
#include "portapack_shared_memory.hpp"
#include "performance_counter.hpp"
#include "portapack.hpp"

namespace ui {

DfuMenu::DfuMenu(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_head,
                  &labels,
                  &text_info_line_1,
                  &text_info_line_2,
                  &text_info_line_3,
                  &text_info_line_4,
                  &text_info_line_5,
                  &text_info_line_6,
                  &text_info_line_7,
                  &text_info_line_8,
                  &text_info_line_9,
                  &text_info_line_10});

    if (battery::BatteryManagement::isDetected()) {
        add_child(&voltage_label);
        add_child(&text_info_line_11);
    }
}

void DfuMenu::paint(Painter& painter) {
    auto utilisation = get_cpu_utilisation_in_percent();
    size_t m0_fragmented_free_space = 0;
    const auto m0_fragments = chHeapStatus(NULL, &m0_fragmented_free_space);

    auto lines = (battery::BatteryManagement::isDetected() ? 11 : 10) + 2;

    text_info_line_1.set(to_string_dec_uint(chCoreStatus(), 6));
    text_info_line_2.set(to_string_dec_uint(m0_fragmented_free_space, 6));
    text_info_line_3.set(to_string_dec_uint(m0_fragments, 6));
    text_info_line_4.set(to_string_dec_uint((uint32_t)get_free_stack_space(), 6));
    text_info_line_5.set(to_string_dec_uint(utilisation, 6));
    text_info_line_6.set(to_string_dec_uint(shared_memory.m4_heap_usage, 6));
    text_info_line_7.set(to_string_dec_uint(shared_memory.m4_stack_usage, 6));
    text_info_line_8.set(to_string_dec_uint(shared_memory.m4_performance_counter, 6));
    text_info_line_9.set(to_string_dec_uint(shared_memory.m4_buffer_missed, 6));
    text_info_line_10.set(to_string_dec_uint(chTimeNow() / 1000, 6));
    if (battery::BatteryManagement::isDetected()) {
        text_info_line_11.set(to_string_decimal_padding((float)battery::BatteryManagement::getVoltage() / 1000.0, 3, 6));
    }

    constexpr auto margin = 5;

    painter.fill_rectangle(
        {{6 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin},
         {15 * CHARACTER_WIDTH + margin * 2, lines * LINE_HEIGHT + margin * 2}},
        Theme::getInstance()->bg_darkest->background);

    painter.fill_rectangle(
        {{5 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin},
         {CHARACTER_WIDTH, lines * LINE_HEIGHT + margin * 2}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{21 * CHARACTER_WIDTH + margin, 3 * LINE_HEIGHT - margin},
         {CHARACTER_WIDTH, lines * LINE_HEIGHT + margin * 2}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{5 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin - 8},
         {17 * CHARACTER_WIDTH + margin * 2, 8}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{5 * CHARACTER_WIDTH - margin, (lines + 3) * LINE_HEIGHT + margin},
         {17 * CHARACTER_WIDTH + margin * 2, 8}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);
}

DfuMenu2::DfuMenu2(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_head,
                  &labels,
                  &text_info_line_1,
                  &text_info_line_2,
                  &text_info_line_3,
                  &text_info_line_4,
                  &text_info_line_5,
                  &text_info_line_6,
                  &text_info_line_7,
                  &text_info_line_8,
                  &text_info_line_9,
                  &text_info_line_10,
                  &text_info_line_11});
}

void DfuMenu2::paint(Painter& painter) {
    text_info_line_1.set(to_string_dec_uint(portapack::receiver_model.target_frequency(), 10));
    text_info_line_2.set(to_string_dec_uint(portapack::receiver_model.baseband_bandwidth(), 10));
    text_info_line_3.set(to_string_dec_uint(portapack::receiver_model.sampling_rate(), 10));
    text_info_line_4.set(to_string_dec_uint(((uint32_t)shared_memory.m4_performance_counter) * 100 / 127, 10));
    text_info_line_5.set(to_string_dec_uint((uint32_t)portapack::receiver_model.modulation(), 10));
    text_info_line_6.set(to_string_dec_uint(portapack::receiver_model.am_configuration(), 10));
    text_info_line_7.set(to_string_dec_uint(portapack::receiver_model.nbfm_configuration(), 10));
    text_info_line_8.set(to_string_dec_uint(portapack::receiver_model.wfm_configuration(), 10));
    text_info_line_9.set(to_string_dec_uint(portapack::transmitter_model.target_frequency(), 10));
    text_info_line_10.set(to_string_dec_uint(portapack::transmitter_model.baseband_bandwidth(), 10));
    text_info_line_11.set(to_string_dec_uint(portapack::transmitter_model.sampling_rate(), 10));

    constexpr auto margin = 5;
    constexpr auto lines = 11 + 2;

    painter.fill_rectangle(
        {{5 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin},
         {19 * CHARACTER_WIDTH + margin * 2, lines * LINE_HEIGHT + margin * 2}},
        Theme::getInstance()->bg_darkest->background);

    painter.fill_rectangle(
        {{4 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin},
         {CHARACTER_WIDTH, lines * LINE_HEIGHT + margin * 2}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{24 * CHARACTER_WIDTH + margin, 3 * LINE_HEIGHT - margin},
         {CHARACTER_WIDTH, lines * LINE_HEIGHT + margin * 2}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{4 * CHARACTER_WIDTH - margin, 3 * LINE_HEIGHT - margin - 8},
         {21 * CHARACTER_WIDTH + margin * 2, 8}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);

    painter.fill_rectangle(
        {{4 * CHARACTER_WIDTH - margin, (lines + 3) * LINE_HEIGHT + margin},
         {21 * CHARACTER_WIDTH + margin * 2, 8}},
        ui::Theme::getInstance()->fg_darkcyan->foreground);
}

} /* namespace ui */
