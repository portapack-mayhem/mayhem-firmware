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

#include "pocsag_app.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "utility.hpp"

using namespace portapack;
using namespace pocsag;

void POCSAGLogger::log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz " +
                        to_string_dec_uint(packet.bitrate()) + " Codewords:";

    // Raw hex dump of all the codewords
    for (size_t c = 0; c < 16; c++)
        entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(packet.timestamp(), entry);
}

void POCSAGLogger::log_decoded(
    const pocsag::POCSAGPacket& packet,
    const std::string text) {
    log_file.write_entry(packet.timestamp(), text);
}

namespace ui {

POCSAGSettingsView::POCSAGSettingsView(
    NavigationView& nav,
    POCSAGSettings& settings)
    : settings_{settings} {
    add_children(
        {
            &check_log,
            &check_small_font,
            &check_ignore,
            &sym_ignore,
            &button_save,
        });

    check_log.set_value(settings_.enable_logging);
    check_small_font.set_value(settings_.enable_small_font);
    check_ignore.set_value(settings_.enable_ignore);

    button_save.on_select = [this, &nav](Button&) {
        settings_.enable_logging = check_log.value();
        settings_.enable_small_font = check_small_font.value();
        settings_.enable_ignore = check_ignore.value();

        nav.pop();
    };
}

POCSAGAppView::POCSAGAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_pocsag);

    add_children(
        {&rssi,
         //&channel,
         &audio,
         &field_rf_amp,
         &field_lna,
         &field_vga,
         &field_frequency,
         &field_volume,
         &button_config,
         &console});

    if (!app_settings_.loaded())
        field_frequency.set_value(initial_target_frequency);

    // TODO: why allocated?
    logger = std::make_unique<POCSAGLogger>();
    if (logger)
        logger->append(LOG_ROOT_DIR "/POCSAG.TXT");

    button_config.on_select = [this](Button&) {
        nav_.push<POCSAGSettingsView>(settings_);
        nav_.set_on_pop([this]() { refresh_ui(); });
    };

    refresh_ui();
    receiver_model.enable();
    audio::output::start();
    baseband::set_pocsag();
}

void POCSAGAppView::focus() {
    field_frequency.focus();
}

POCSAGAppView::~POCSAGAppView() {
    audio::output::stop();

    // Save settings.
    // persistent_memory::set_pocsag_ignore_address(sym_ignore.value_dec_u32());
    // enable_logging = check_log.value();

    receiver_model.disable();
    baseband::shutdown();
}

void POCSAGAppView::refresh_ui() {
    console.set_style(
        settings_.enable_small_font
            ? &Styles::white_small
            : &Styles::white);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage* message) {
    std::string alphanum_text = "";

    if (message->packet.flag() != NORMAL)
        console.writeln("\n\x1B\x0CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
    else {
        pocsag_decode_batch(message->packet, &pocsag_state);

        // if ((ignore()) && (pocsag_state.address == sym_ignore.value_dec_u32())) {
        //     // Ignore (inform, but no log)
        //     // console.write("\n\x1B\x03" + to_string_time(message->packet.timestamp()) +
        //     //			" Ignored address " + to_string_dec_uint(pocsag_state.address));
        //     return;
        // }
        // // Too many errors for reliable decode
        // if ((ignore()) && (pocsag_state.errors >= 3)) {
        //     return;
        // }

        std::string console_info;
        const uint32_t roundVal = 50;
        const uint32_t bitrate = roundVal * ((message->packet.bitrate() + (roundVal / 2)) / roundVal);
        console_info = "\n" + to_string_datetime(message->packet.timestamp(), HM);
        console_info += " " + to_string_dec_uint(bitrate);
        console_info += " ADDR:" + to_string_dec_uint(pocsag_state.address);
        console_info += " F" + to_string_dec_uint(pocsag_state.function);

        // Store last received address for POCSAG TX
        persistent_memory::set_pocsag_last_address(pocsag_state.address);

        if (pocsag_state.out_type == ADDRESS) {
            // Address only

            console.write(console_info);

            if (logger && logging()) {
                logger->log_decoded(
                    message->packet, to_string_dec_uint(pocsag_state.address) +
                                         " F" + to_string_dec_uint(pocsag_state.function) +
                                         " Address only");
            }

            last_address = pocsag_state.address;
        } else if (pocsag_state.out_type == MESSAGE) {
            if (pocsag_state.address != last_address) {
                // New message
                console.writeln(console_info);
                console.write(pocsag_state.output);

                last_address = pocsag_state.address;
            } else {
                // Message continues...
                console.write(pocsag_state.output);
            }

            if (logger && logging())
                logger->log_decoded(
                    message->packet, to_string_dec_uint(pocsag_state.address) +
                                         " F" + to_string_dec_uint(pocsag_state.function) +
                                         " Alpha: " + pocsag_state.output);
        }
    }

    // TODO: make setting.
    // Log raw data whatever it contains
    /*if (logger && logging())
        logger->log_raw_data(message->packet, receiver_model.target_frequency());*/
}

void POCSAGAppView::on_stats(const POCSAGStatsMessage* stats) {
}

} /* namespace ui */
