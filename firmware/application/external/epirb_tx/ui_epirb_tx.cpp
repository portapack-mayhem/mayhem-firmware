/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#include "ui_epirb_tx.hpp"

#include "tonesets.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file_reader.hpp"
#include "file_path.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui::external_app::epirb_tx {

void EPIRBTXAppView::focus() {
    options_frame.focus();
}

EPIRBTXAppView::~EPIRBTXAppView() {
    // Restore bpsk fequency
    transmitter_model.set_target_frequency(bpsk_frequency);
    transmitter_model.disable();
    baseband::shutdown();
}

uint8_t EPIRBTXAppView::hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

uint8_t EPIRBTXAppView::hexToByte(char high, char low)
{
    return (hexval(high) << 4) | hexval(low);
}

void EPIRBTXAppView::on_timer() {
    if(loop)
    {
        if(checkbox_loop.value())
        {
            auto now = chTimeNow();
            std::string timeout = std::to_string((uint32_t)(field_delay.value() - ((now - last_frame_time)/1000)));
            if(timeout != text_timeout.get())
            {
                text_timeout.set(timeout);
            }
            if(now > (last_frame_time + (field_delay.value()*1000)))
            {
                start_tx();
            }
        }
        else
        {
            loop = false;
        }
    }
}

void EPIRBTXAppView::update_config() {
    if(epirb_tx_message.mode_bpsk)
    {   // Backup bpsk frequency
        bpsk_frequency = transmitter_model.target_frequency();
    }
    else
    {   // Restore bpsk frequency
        transmitter_model.set_target_frequency(bpsk_frequency);
    }
    epirb_tx_message.mode_bpsk = true;
    epirb_tx_message.pre_count = (500 * TONES_SAMPLERATE)/1000; // 500 ms
    epirb_tx_message.post_count = (100 * TONES_SAMPLERATE)/1000; // 100 ms
    Beacon& beacon = beacons[selected_beacon];
    text_description.set(beacon.description.substr(0,max_text_width_ext));
    text_description_end.set(beacon.description.size()>max_text_width_ext ? "-" + beacon.description.substr(max_text_width_ext,max_text_width_ext+max_text_width_ext-1) : "");
    text_frame.set(beacon.frame.substr(0,18));
    text_frame_end.set(beacon.frame.size()>18 ? beacon.frame.substr(18,36) : "");
    epirb_tx_message.data_len = std::min<size_t>((beacon.frame.size()/2),18);
    for(uint8_t i = 0 ; i < epirb_tx_message.data_len ; i++)
    {
        epirb_tx_message.data[i] = hexToByte(
            beacon.frame[2*i],
            beacon.frame[2*i + 1]);
    }
    baseband::set_epirb_tx_config(epirb_tx_message);
}

void EPIRBTXAppView::set_tx_button_state(bool active)
{
    button_tx.set_text(active ? "START" : "STOP");
    button_tx.set_style(active ? &style_tx_start : &style_tx_stop);
}


void EPIRBTXAppView::start_tx() {
    last_frame_time = chTimeNow();
    update_config();
    loop = checkbox_loop.value();
    transmitter_model.enable();
    tx_view.set_transmitting(true);
    set_tx_button_state(false);
    transmitting = true;
}

void EPIRBTXAppView::stop_tx() {
    loop = false;
    transmitter_model.disable();
    tx_view.set_transmitting(false);
    set_tx_button_state(true);
    transmitting = false;
}

void EPIRBTXAppView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;

    if (done) {
        if(checkbox_am.value())
        {   // BPSK frame sent, switch back to 121.5 signal
            epirb_tx_message.mode_bpsk = false;
            // Backup bpsk frequency 
            bpsk_frequency = transmitter_model.target_frequency();
            transmitter_model.set_target_frequency(am_frequency);
            baseband::set_epirb_tx_config(epirb_tx_message);
        }
        else
        {
            transmitter_model.disable();
            tx_view.set_transmitting(false);
            if(!loop)
            {
                set_tx_button_state(true);
                transmitting = false;  
            }
        }
    }
}

EPIRBTXAppView::EPIRBTXAppView(
    NavigationView& nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels,
                  &options_mode,
                  &options_frame,
                  &text_description,
                  &text_description_end,
                  &text_frame,
                  &text_frame_end,
                  &text_timeout,
                  &checkbox_loop,
                  &field_delay,
                  &button_tx,
                  &checkbox_am,
                  &field_am_frequency,
                  &tx_view});

    options_mode.on_change = [this](size_t index, OptionsField::value_t) {
        if(index == 0)
        {   // File mode

        }
        else
        {   // Manual mode

        }
        set_dirty();
    };

    field_am_frequency.set_value(am_frequency);
    field_am_frequency.on_change = [this](rf::Frequency freq) {
        am_frequency = freq;
    };

    load_beacons();  // Load available beacons from TXT files (or default).

    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t entries;

    for (const auto& beacon : beacons)
        entries.emplace_back(beacon.title, entries.size());

    options_frame.set_options(std::move(entries));
    options_frame.on_change = [this](size_t index, OptionsField::value_t) {
        selected_beacon = index;
        update_config();
        set_dirty();
    };
    options_frame.set_selected_index(selected_beacon);
    update_config();

    field_delay.set_value(30);
    checkbox_loop.set_value(true);

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_bandwidth_changed = [this]() {
        // we don't protect here with auto_update because other field of tx_view obj isn't protected too
        // to remains the design logic same

        update_config();
    };

    tx_view.on_start = [this]() {
        start_tx();
    };

    tx_view.on_stop = [this]() {
        stop_tx();
    };

    button_tx.on_select = [this](Button&) {
        if (!transmitting)
            start_tx();
        else
            stop_tx();
    };    
}

void EPIRBTXAppView::load_beacons() {
    File beacons_file;
    auto error = beacons_file.open(epirb_dir / u"BEACONS.TXT");
    beacons.clear();

    if (!error) {
        auto reader = FileLineReader(beacons_file);
        for (const auto& line : reader) {
            if (line.length() == 0 || line[0] == '#')
                continue;

            auto cols = split_string(line, ';');
            if (cols.size() != 3)
                continue;

            Beacon beacon{};
            beacon.title = trim(cols[0]);
            beacon.description = trim(cols[1]);
            // Make sure frame is not longer tha 18 bytes / 36 hex character
            beacon.frame = trim(cols[2]).substr(0,36);
            size_t size = beacon.frame.size();
            if (size <= 0)
                continue;  // Invalid line.
            beacons.emplace_back(std::move(beacon));
        }
    }
    if(beacons.empty())
    {   // No beacons file or empty flile: just add default beacon
        beacons.push_back(default_beacon);
    }
}


}  // namespace ui::external_app::epirb_tx