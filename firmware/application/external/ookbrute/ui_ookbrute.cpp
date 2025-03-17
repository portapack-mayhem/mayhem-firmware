/*
 * Copyright (C) 2024 HTotoo
 * copyleft Whiterose of the Dark Army
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

#include "ui_ookbrute.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_textentry.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::ookbrute {

void OOKBruteView::focus() {
    button_startstop.focus();
}

OOKBruteView::OOKBruteView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &button_startstop,
        &button_input_stop_position,
        &button_input_start_position,
        &labels,
        &field_frequency,
        &tx_view,
        &options_atkmode,
        &field_start,
        &field_stop,
    });

    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            is_running = false;
            stop();
        } else {
            is_running = true;
            start();
        }
    };

    options_atkmode.on_change = [this](size_t, int32_t i) {
        update_start_stop(i);
        validate_start_stop();
    };
    field_start.on_change = [this](int32_t) {
        validate_start_stop();
    };
    field_stop.on_change = [this](int32_t) {
        validate_start_stop();
    };
    button_input_start_position.on_select = [this](Button&) {
        stop();  // prevent mess count var up

        text_input_buffer = to_string_dec_uint(field_start.value());
        if (text_input_buffer == "0") {
            text_input_buffer = "";
        }

        text_prompt(
            nav_,
            text_input_buffer,
            8,  // currently longest is princeton
            [this](std::string& buffer) {
                field_start.set_value(atoi(buffer.c_str()));
                validate_start_stop();
            });
    };
    button_input_stop_position.on_select = [this](Button&) {
        stop();  // prevent mess count var up

        text_input_buffer = to_string_dec_uint(field_stop.value());
        if (text_input_buffer == "0") {
            text_input_buffer = "";
        }

        text_prompt(
            nav_,
            text_input_buffer,
            8,  // currently longest is princeton
            [this](std::string& buffer) {
                field_stop.set_value(atoi(buffer.c_str()));
                validate_start_stop();
            });
    };
    update_start_stop(0);
}

void OOKBruteView::update_start_stop(uint32_t proto) {
    uint8_t bits = 12;
    switch (proto) {
        default:
        case 0:
        case 2:
        case 4:
            bits = 12;
            break;
        case 1:
        case 3:
        case 5:
            bits = 24;
            break;
    }
    uint32_t max = (1 << bits) - 1;
    field_start.set_range(0, max);
    field_stop.set_range(0, max);
    field_start.set_value(0);
    field_stop.set_value(max);
}

void OOKBruteView::validate_start_stop() {
    if (field_start.value() > field_stop.value()) {
        field_start.set_value(field_stop.value());
    }
    if (field_stop.value() < field_start.value()) {
        field_stop.set_value(field_start.value());
    }
}

void OOKBruteView::generate_packet() {
    uint32_t protocol = options_atkmode.selected_index_value();
    uint8_t byte = 0;
    size_t bitstream_length = 0;
    uint8_t* bitstream = shared_memory.bb_data.data;  // max 512 in size
    uint32_t samples_per_bit = 0;                     // OOK_SAMPLERATE * bit_duration_in_sec
    std::string dataFormat = "";
    std::string zero = "";
    std::string one = "";
    uint16_t databits = 0;
    uint16_t repeat = 1;
    uint16_t pause_sym = 0;
    if (protocol == 0) {  // came 12
        samples_per_bit = OOK_SAMPLERATE / ((3 * 1000) / 1);
        dataFormat = "0000000000000000000000000000000000001CCCCCCCCCCCC0000";  // 36 0 preamble +start bit + data
        databits = 12;
        zero = "011";
        one = "001";
        repeat = 2;
        pause_sym = 0;
    }

    if (protocol == 1) {  // came24
        samples_per_bit = OOK_SAMPLERATE / ((3 * 1000) / 1);
        dataFormat = "0000000000000000000000000000000000001CCCCCCCCCCCCCCCCCCCCCCCC0000";  // 36 0 preamble +start bit + data
        databits = 24;
        zero = "011";
        one = "001";
        repeat = 2;
        pause_sym = 0;
    }
    if (protocol == 2) {  // nice12
        samples_per_bit = OOK_SAMPLERATE * (680.0 / 1000000.0);
        dataFormat = "000000000000000000000000000000000000000001CCCCCCCCCCCC0000";  // 36 0 preamble +start bit + data
        databits = 12;
        zero = "011";
        one = "001";
        repeat = 2;
        pause_sym = 0;
    }
    if (protocol == 3) {  // nice24
        samples_per_bit = OOK_SAMPLERATE * (680.0 / 1000000.0);
        dataFormat = "000000000000000000000000000000000000000001CCCCCCCCCCCCCCCCCCCCCCCC0000";  // 36 0 preamble +start bit + data
        databits = 24;
        zero = "011";
        one = "001";
        repeat = 2;
        pause_sym = 0;
    }
    if (protocol == 4) {  // holtek_ht12
        samples_per_bit = OOK_SAMPLERATE * (390.0 / 1000000.0);
        dataFormat = "0000000000000000000000000000000000001CCCCCCCCCCCC00000000000";  // 36 0 preamble +start bit + data.
        databits = 12;
        zero = "011";
        one = "001";
        repeat = 2;
        pause_sym = 0;
    }
    if (protocol == 5) {                                                                      // princeton24
        samples_per_bit = OOK_SAMPLERATE * (450.0 / 1000000.0);                               // long = 3*
        dataFormat = "000000000000000000000000000000000000CCCCCCCCCCCCCCCCCCCCCCCC10000000";  //
        databits = 24;
        zero = "1000";
        one = "1110";
        repeat = 6;
        pause_sym = 0;
    }

    std::string fragments = "";  // storage

    uint16_t cdb = 0;            // current data bit
    for (auto c : dataFormat) {  // generate fragments from template
        if (c == '0') fragments += '0';
        if (c == '1') fragments += '1';
        if (c == 'C') {
            if (counter & (1 << (databits - cdb - 1))) {
                fragments += one;
            } else {
                fragments += zero;
            }
            cdb++;
        }
    }

    // create bitstream
    for (auto c : fragments) {
        byte <<= 1;
        if (c != '0') byte |= 1;
        if ((bitstream_length & 7) == 7)
            bitstream[bitstream_length >> 3] = byte;
        bitstream_length++;
    }

    // Finish last byte if needed
    size_t padding = 8 - (bitstream_length & 7);
    if (padding != 8) {
        byte <<= padding;
        bitstream[(bitstream_length + padding - 1) >> 3] = byte;
        padding++;
    }

    // send bitstream
    baseband::set_ook_data(
        bitstream_length,
        samples_per_bit,
        repeat,
        pause_sym,
        0);
}

void OOKBruteView::stop() {
    transmitter_model.disable();
    baseband::shutdown();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_START]);
}

void OOKBruteView::start() {
    counter = field_start.value();
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    transmitter_model.enable();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_STOP]);
    generate_packet();
}

void OOKBruteView::on_tx_progress(const bool done) {
    if (done) {
        if (is_running) {
            counter++;
            field_start.set_value(counter);
            if (counter > (uint32_t)field_stop.value()) {
                stop();
            } else {
                generate_packet();
            }
        }
    }
}

OOKBruteView::~OOKBruteView() {
    is_running = false;
    stop();
}

}  // namespace ui::external_app::ookbrute

/*

https://web.archive.org/web/20230331125843/https://phreakerclub.com/447


*/
