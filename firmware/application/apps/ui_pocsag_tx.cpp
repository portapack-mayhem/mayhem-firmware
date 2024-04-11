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

#include "ui_pocsag_tx.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "ui_textentry.hpp"

#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace pocsag;

namespace ui {

void POCSAGTXView::focus() {
    field_address.focus();
}

POCSAGTXView::~POCSAGTXView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void POCSAGTXView::on_remote(const PocsagTosendMessage data) {
    // check if still sending or not
    size_t tmp = 0;
    if (data.baud == 1200) tmp = 1;
    if (data.baud == 2400) tmp = 2;
    options_bitrate.set_selected_index(tmp);
    tmp = 0;
    options_type.set_selected_index(data.type);
    if (data.function == 'B') tmp = 1;
    if (data.function == 'C') tmp = 2;
    if (data.function == 'D') tmp = 3;
    options_function.set_selected_index(tmp);
    options_phase.set_selected_index(data.phase == 'P' ? 0 : 1);
    field_address.set_value(data.addr);
    message = (char*)data.msg;
    text_message.set(message);
    options_bitrate.dirty();
    options_type.dirty();
    options_function.dirty();
    options_phase.dirty();
    field_address.dirty();
    text_message.dirty();
    tx_view.focus();
    start_tx();
}

void POCSAGTXView::on_tx_progress(const uint32_t progress, const bool done) {
    if (done) {
        transmitter_model.disable();
        progressbar.set_value(0);
        tx_view.set_transmitting(false);
    } else
        progressbar.set_value(progress);
}

bool POCSAGTXView::start_tx() {
    uint32_t total_frames, i, codeword, bi, address;
    pocsag::BitRate bitrate;
    std::vector<uint32_t> codewords;

    address = field_address.to_integer();
    if (address > 0x1FFFFFU) {
        nav_.display_modal("Bad address", "Address must be less\nthan 2097152.");
        return false;
    }

    MessageType type = (MessageType)options_type.selected_index_value();

    if (type == MessageType::NUMERIC_ONLY) {
        // Check for invalid characters
        if (message.find_first_not_of("0123456789SU -][") != std::string::npos) {
            nav_.display_modal("Bad message", "A numeric only message must\nonly contain:\n0123456789SU][- or space.");
            return false;
        }
    }
    MessageType phase = (MessageType)options_phase.selected_index_value();

    pocsag_encode(type, BCH_code, options_function.selected_index_value(), message, address, codewords);

    total_frames = codewords.size() / 2;

    progressbar.set_max(total_frames);

    transmitter_model.set_rf_amp(true);
    transmitter_model.set_tx_gain(40);
    // We left exactly same TX LPF settings as previous fw 1.7.4,  TX LPF= 1M75 (min). It is fine even in max FM dev. 150khz and 2400 bauds.
    transmitter_model.set_baseband_bandwidth(1'750'000);  // Min TX LPF . Pocsag is NBFM , using BW channel 25khz or 12khz
    transmitter_model.enable();

    uint8_t* data_ptr = shared_memory.bb_data.data;

    bi = 0;
    for (i = 0; i < codewords.size(); i++) {
        if (phase == 0)
            codeword = ~(codewords[i]);
        else
            codeword = codewords[i];

        data_ptr[bi++] = (codeword >> 24) & 0xFF;
        data_ptr[bi++] = (codeword >> 16) & 0xFF;
        data_ptr[bi++] = (codeword >> 8) & 0xFF;
        data_ptr[bi++] = codeword & 0xFF;
    }

    bitrate = pocsag_bitrates[options_bitrate.selected_index()];

    baseband::set_fsk_data(
        codewords.size() * 32,
        2280000 / bitrate,
        4500,
        64);

    return true;
}

void POCSAGTXView::paint(Painter&) {
    message = buffer;
    text_message.set(message);
}

void POCSAGTXView::on_set_text(NavigationView& nav) {
    text_prompt(nav, buffer, 30);
}

POCSAGTXView::POCSAGTXView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_fsktx);

    add_children({&labels,
                  &options_bitrate,
                  &field_address,
                  &options_type,
                  &options_function,
                  &options_phase,
                  &text_message,
                  &button_message,
                  &progressbar,
                  &tx_view});

    options_bitrate.set_selected_index(1);  // 1200bps
    options_type.set_selected_index(0);     // Address only

    field_address.set_value(persistent_memory::pocsag_last_address());

    options_type.on_change = [this](size_t, int32_t i) {
        if (i == 2)
            options_function.set_selected_index(3);
    };

    button_message.on_select = [this, &nav](Button&) {
        this->on_set_text(nav);
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (start_tx())
            tx_view.set_transmitting(true);
    };

    tx_view.on_stop = [this]() {
        tx_view.set_transmitting(false);
        transmitter_model.disable();
    };
}

} /* namespace ui */
