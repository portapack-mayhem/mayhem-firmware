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
#include "binder.hpp"
#include "ui_geomap.hpp"

#include <cstring>
#include <stdio.h>

#include "beacon.hpp"

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

static uint8_t hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static uint8_t hexToByte(char high, char low)
{
    return (hexval(high) << 4) | hexval(low);
}

std::string EPIRBTXAppView::frame_to_hex_string(bool start)
{
    return beacon_to_hex_string(epirb_tx_message.data,start);
}

void EPIRBTXAppView::generate_frame(BeaconParams params)
{
    epirb_tx_message.data_len = generate_beacon(epirb_tx_message.data,params);
}

void EPIRBTXAppView::on_timer() {
    if(loop)
    {
        if(loop_enabled)
        {
            auto now = chTimeNow();
            std::string timeout = std::to_string((uint32_t)(delay - ((now - last_frame_time)/1000)));
            if(timeout != text_timeout.get())
            {
                text_timeout.set(timeout);
            }
            if(now > (last_frame_time + (delay*1000)))
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

void EPIRBTXAppView::update_frame(bool updateConfig) {
    if(mode_file)
    {
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
    }
    else
    {
        generate_frame(beacon_params);
        text_frame.set(frame_to_hex_string(true));
        text_frame_end.set(frame_to_hex_string(false));
    }
    if(updateConfig && send_on_change && loop)
    {   // Need to update config / send new beacon
        if(am_enabled)
        {   // Already transmitting => update config
            last_frame_time = chTimeNow();
            update_config();
        }
        else
        {   // Not yet transmitting => start tx
            start_tx();
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
    loop = loop_enabled;
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

void EPIRBTXAppView::update_location(bool updateLocatorField)
{
    locator = beacon_params.location.locator;
    if(updateLocatorField) text_field_beacon_locator.set_text(beacon_params.location.locator);
    text_beacon_latitude_value.set(to_latitude_string(beacon_params.location));
    text_beacon_longitude_value.set(to_longitude_string(beacon_params.location));
}

void EPIRBTXAppView::update_mode()
{
    text_beacon.hidden(!mode_file);
    text_description_label.hidden(!mode_file);
    options_frame.hidden(!mode_file);
    text_description.hidden(!mode_file);
    text_description_end.hidden(!mode_file);
    text_beacon_type.hidden(mode_file);
    text_beacon_country.hidden(mode_file);
    checkbox_beacon_internal.hidden(mode_file);
    text_beacon_locator.hidden(mode_file);
    text_beacon_latitude.hidden(mode_file);
    text_beacon_latitude_value.hidden(mode_file);
    text_beacon_longitude.hidden(mode_file);
    text_beacon_longitude_value.hidden(mode_file);
    button_mangps.hidden(mode_file);
    options_beacon_type.hidden(mode_file);
    options_beacon_protocol.hidden(mode_file);
    options_beacon_country.hidden(mode_file);
    text_field_beacon_locator.hidden(mode_file);
}

EPIRBTXAppView::EPIRBTXAppView(
    NavigationView& nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels,
                  &options_mode,
                  &text_beacon,
                  &text_description_label,
                  &text_beacon_type,
                  &options_beacon_type,
                  &options_beacon_protocol,
                  &text_beacon_country,
                  &options_beacon_country,
                  &checkbox_beacon_internal,
                  &text_beacon_locator,
                  &text_beacon_latitude,
                  &text_beacon_latitude_value,
                  &text_beacon_longitude,
                  &text_beacon_longitude_value,
                  &button_mangps,
                  &text_field_beacon_locator,
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
                  &checkbox_send_on_change,
                  &tx_view});

    text_beacon.set_style(Theme::getInstance()->fg_light);
    text_description_label.set_style(Theme::getInstance()->fg_light);
    text_beacon_type.set_style(Theme::getInstance()->fg_light);
    text_beacon_country.set_style(Theme::getInstance()->fg_light);
    text_beacon_locator.set_style(Theme::getInstance()->fg_light);
    text_beacon_latitude.set_style(Theme::getInstance()->fg_light);
    text_beacon_longitude.set_style(Theme::getInstance()->fg_light);

    // Restore settings
    checkbox_am.set_value(am_enabled);
    checkbox_loop.set_value(loop_enabled);
    checkbox_send_on_change.set_value(send_on_change);
    options_mode.set_by_value(!mode_file);
    transmitter_model.set_target_frequency(bpsk_frequency);
    field_am_frequency.set_value(am_frequency);
    field_delay.set_value(delay);
    options_beacon_type.set_by_value(beacon_type);
    options_beacon_protocol.set_by_value(beacon_protocol);
    options_beacon_country.set_by_value(beacon_country);
    checkbox_beacon_internal.set_value(beacon_internal);
    beacon_params.type = (BeaconType)beacon_type;
    beacon_params.has_121_5 = am_enabled;
    beacon_params.location.locator = locator;
    beacon_params.is_internal = beacon_internal;
    init_from_locator(beacon_params.location);
    update_mode();
    update_location();

    options_mode.on_change = [this](size_t index, OptionsField::value_t) {
        mode_file = (index == 0);
        update_mode();
        update_frame();
        set_dirty();
    };

    options_beacon_type.on_change = [this](size_t index, OptionsField::value_t) {
        beacon_params.type = (BeaconType)index;
        beacon_type = index;
        update_frame();
        set_dirty();
    };

    options_beacon_protocol.on_change = [this](size_t index, OptionsField::value_t) {
        beacon_params.protocol = (BeaconProtocol)index;
        beacon_protocol = index;
        update_frame();
        set_dirty();
    };


    options_beacon_country.on_change = [this](size_t, OptionsField::value_t v) {
        beacon_params.country = v;
        beacon_country = v;
        update_frame();
        set_dirty();
    };

    checkbox_beacon_internal.on_select = [this](Checkbox&, bool v) {
        beacon_internal = v;
        beacon_params.is_internal = v;
        update_frame();
        set_dirty();
    };

    bind(text_field_beacon_locator, locator, nav, [this](std::string value) {
        beacon_params.location.locator = value;
        init_from_locator(beacon_params.location);
        update_location(false);
        update_frame();
        set_dirty();
    });

    button_mangps.on_select = [this, &nav](Button&) {
        nav.push<GeoMapView>(
            0,
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::HIDDEN,
            beacon_params.location.latitude,
            beacon_params.location.longitude,
            [this](int32_t, float lat, float lon, int32_t) {
                beacon_params.location.latitude = lat;
                beacon_params.location.longitude = lon;
                init_from_decimal(beacon_params.location);
                // Update locator field
                update_location();
                update_frame();
                set_dirty();
            });
    };

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
    options_frame.set_selected_index(selected_beacon);
    options_frame.on_change = [this](size_t index, OptionsField::value_t) {
        selected_beacon = index;
        update_frame();
        set_dirty();
    };
    update_frame(false);

    checkbox_loop.on_select = [this](Checkbox&, bool v) {
        loop_enabled = v;
    };

    field_delay.on_change = [this](int32_t v) {
        delay = v;
    };

    checkbox_am.on_select = [this](Checkbox&, bool v) {
        beacon_params.has_121_5 = v;
        am_enabled = v;
        if(!mode_file) update_frame(false);
    };

    // AM frequency field edit
    field_am_frequency.on_edit = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(field_am_frequency.value());
        new_view->on_changed = [this](rf::Frequency f) {
            field_am_frequency.set_value(f);
            update_config();
        };
    };

    checkbox_send_on_change.on_select = [this](Checkbox&, bool v) {
        send_on_change = v;
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
            bpsk_frequency = f;
        };
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