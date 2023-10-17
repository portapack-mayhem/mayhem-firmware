/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_modemsetup.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;

namespace ui {

void ModemSetupView::focus() {
    field_baudrate.focus();
}

ModemSetupView::ModemSetupView(
    NavigationView& nav) {
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t modem_options;

    add_children({
        &labels,
        &field_baudrate,
        &field_mark,
        &field_space,
        &field_repeat,
        &options_modem,
        &button_set_modem,
        &sym_format_data,
        &sym_format_parity,
        &sym_format_stop,
        &sym_format_msb,
        &button_save,
    });

    // Only list AFSK modems for now
    for (size_t i = 0; i < MODEM_DEF_COUNT; i++) {
        if (modem_defs[i].modulation == AFSK)
            modem_options.emplace_back(std::make_pair(modem_defs[i].name, i));
    }
    options_modem.set_options(modem_options);
    options_modem.set_selected_index(0);

    sym_format_data.set_offset(0, persistent_memory::serial_format().data_bits - 6);
    sym_format_parity.set_offset(0, persistent_memory::serial_format().parity);
    sym_format_stop.set_offset(0, persistent_memory::serial_format().stop_bits);
    sym_format_msb.set_offset(0, persistent_memory::serial_format().bit_order);

    field_mark.set_value(persistent_memory::afsk_mark_freq());
    field_space.set_value(persistent_memory::afsk_space_freq());
    field_repeat.set_value(persistent_memory::modem_repeat());

    field_baudrate.set_value(persistent_memory::modem_baudrate());

    button_set_modem.on_select = [this, &nav](Button&) {
        size_t modem_def_index = options_modem.selected_index();

        field_mark.set_value(modem_defs[modem_def_index].mark_freq);
        field_space.set_value(modem_defs[modem_def_index].space_freq);
        field_baudrate.set_value(modem_defs[modem_def_index].baudrate);
    };

    button_save.on_select = [this, &nav](Button&) {
        persistent_memory::set_afsk_mark(field_mark.value());
        persistent_memory::set_afsk_space(field_space.value());

        persistent_memory::set_modem_baudrate(field_baudrate.value());
        persistent_memory::set_modem_repeat(field_repeat.value());

        serial_format_t serial_format{};
        serial_format.data_bits = sym_format_data.get_offset(0) + 6;
        serial_format.parity = (parity_enum)sym_format_parity.get_offset(0);
        serial_format.stop_bits = sym_format_stop.get_offset(0);
        serial_format.bit_order = (order_enum)sym_format_msb.get_offset(0);

        persistent_memory::set_serial_format(serial_format);

        nav.pop();
    };
}

} /* namespace ui */
