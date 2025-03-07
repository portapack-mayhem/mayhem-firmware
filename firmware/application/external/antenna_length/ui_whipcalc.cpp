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

#include "ui_whipcalc.hpp"

#include "ch.h"
#include "convert.hpp"
#include "event_m0.hpp"
#include "file_reader.hpp"
#include "portapack.hpp"
#include "file_path.hpp"

#include <cstring>

using namespace portapack;
using namespace ui;

namespace ui::external_app::antenna_length {

void WhipCalcView::focus() {
    field_frequency.focus();
}

void WhipCalcView::update_result() {
    double length, calclength, divider;
    console.clear(true);
    divider = ((double)options_type.selected_index_value() / 8.0);

    // Antenna lengths fields
    if (field_frequency.value() > 0) {
        // Metric
        length = (speed_of_light_mps / (double)field_frequency.value()) * divider;
        auto m = to_string_dec_int((int)length, 0);
        // auto cm = to_string_dec_int(int(length * 100.0) % 100, 2);
        // auto mm = to_string_dec_int(int(length * 1000.0) % 10, 1);
        calclength = get_decimals(length, 100);  // cm
        auto cm = to_string_dec_int(int(calclength), 0);
        auto mm = to_string_dec_int(int(get_decimals(calclength, 10, true)), 0);
        text_result_metric.set(m + "m " + cm + "." + mm + "cm");

        // Imperial
        calclength = (speed_of_light_fps / (double)field_frequency.value()) * divider;
        auto feet = to_string_dec_int(int(calclength), 0);
        calclength = get_decimals(calclength, 12);  // inches
        auto inch = to_string_dec_int(int(calclength), 0);
        auto inch_c = to_string_dec_int(int(get_decimals(calclength, 10, true)), 0);
        text_result_imperial.set(feet + "ft " + inch + "." + inch_c + "in");
    } else {
        text_result_metric.set("infinity+");
        text_result_imperial.set("infinity+");
        return;
    }

    uint8_t ant_count = 9;                      // Shown antennas counter
    length *= 1000;                             // Get length in mm needed to extend the antenna
    for (antenna_entry antenna : antenna_db) {  // go thru all antennas available
        uint16_t element, refined_quarter = 0;
        for (element = 0; element < antenna.elements.size(); element++) {
            if (length == antenna.elements[element])  // Exact element in length
            {
                element++;  // Real element is +1  (zero based vector)
                break;      // Done with this ant
            } else if (length < antenna.elements[element]) {
                double remain, this_element, quarter = 0;
                remain = length - antenna.elements[element - 1];                           // mm needed from this element to reach length
                this_element = antenna.elements[element] - antenna.elements[element - 1];  // total mm on this element
                quarter = (remain * 4) / this_element;                                     // havoc & portack ended on this int(quarter) resolution.
                if (quarter - int(quarter) > 0.5) {                                        // rounding gave a measure closer to next quarter
                    refined_quarter = int(quarter) + 1;
                    if (refined_quarter == 4) {  // rounding gave a measure closer to next element
                        refined_quarter = 0;
                        element++;
                    }
                } else {
                    refined_quarter = int(quarter);
                }
                break;  // Done with this ant
            }
        }
        /*if (!ant_count)
                {
                        console.write(" and more ...");
                        break;
                }*/
        console.write(antenna.label + ": " + to_string_dec_int(element, 1) + frac_str[refined_quarter] + " elements\n");
        ant_count--;  // For now, just showing all.
    }
}

WhipCalcView::WhipCalcView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  //&antennas_on_memory,
                  &field_frequency,
                  &options_type,
                  &text_result_metric,
                  &text_result_imperial,
                  &console,
                  &button_exit});

    // Try loading antennas from file.
    load_antenna_db();

    if (!antenna_db.size())
        add_default_antenna();

    // antennas_on_memory.set(to_string_dec_int(antenna_db.size(),0) + " antennas");	//tell user

    options_type.set_selected_index(2);  // Quarter wave
    options_type.on_change = [this](size_t, OptionsField::value_t) {
        this->update_result();
    };

    field_frequency.set_step(1000000);  // 1MHz step
    field_frequency.updated = [this](rf::Frequency) {
        update_result();
    };

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    update_result();
}

void WhipCalcView::load_antenna_db() {
    File antennas_file;
    auto error = antennas_file.open(whipcalc_dir / u"ANTENNAS.TXT");

    if (error)
        return;

    auto reader = FileLineReader(antennas_file);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;  // Empty or comment line.

        auto cols = split_string(line, ',');
        if (cols.size() < 2)
            continue;  // Line doesn't have enough columns.

        antenna_entry new_antenna{
            std::string{cols[0]}};

        // Add antenna elements.
        for (auto i = 1ul; i < cols.size(); ++i) {
            uint16_t length = 0;
            if (parse_int(cols[i], length)) {
                new_antenna.elements.push_back(length);
            }
        }

        if (!new_antenna.elements.empty())
            antenna_db.push_back(std::move(new_antenna));
    }
}

void WhipCalcView::add_default_antenna() {
    antenna_db.push_back({"ANT500", {185, 315, 450, 586, 724, 862}});  // store a default ant500
}
}  // namespace ui::external_app::antenna_length
