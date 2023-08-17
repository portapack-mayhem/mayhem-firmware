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

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace pocsag;

#include "string_format.hpp"
#include "utility.hpp"
#include "audio.hpp"

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

POCSAGAppView::POCSAGAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_pocsag);

    add_children({&rssi,
                  &channel,
                  &audio,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &check_log,
                  &field_volume,
                  &check_ignore,
                  &sym_ignore,
                  &console});

    if (!settings_.loaded())
        field_frequency.set_value(initial_target_frequency);

    check_log.set_value(ui_settings_[enable_logging]->as_bool());

    receiver_model.enable();

    // TODO: app setting instead?
    uint32_t ignore_address;
    ignore_address = persistent_memory::pocsag_ignore_address();

    // TODO: is this common enough for a helper?
    for (size_t c = 0; c < 7; c++) {
        sym_ignore.set_sym(6 - c, ignore_address % 10);
        ignore_address /= 10;
    }

    logger = std::make_unique<POCSAGLogger>();
    if (logger)
        logger->append(LOG_ROOT_DIR "/POCSAG.TXT");

    audio::output::start();
    baseband::set_pocsag();
}

void POCSAGAppView::focus() {
    field_frequency.focus();
}

POCSAGAppView::~POCSAGAppView() {
    audio::output::stop();

    // Save settings.
    persistent_memory::set_pocsag_ignore_address(sym_ignore.value_dec_u32());
    ui_settings_[enable_logging]->set(check_log.value());

    receiver_model.disable();
    baseband::shutdown();
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage* message) {
    std::string alphanum_text = "";

    if (message->packet.flag() != NORMAL)
        console.writeln("\n\x1B\x0CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
    else {
        pocsag_decode_batch(message->packet, &pocsag_state);

        if ((ignore()) && (pocsag_state.address == sym_ignore.value_dec_u32())) {
            // Ignore (inform, but no log)
            // console.write("\n\x1B\x03" + to_string_time(message->packet.timestamp()) +
            //			" Ignored address " + to_string_dec_uint(pocsag_state.address));
            return;
        }
        // Too many errors for reliable decode
        if ((ignore()) && (pocsag_state.errors >= 3)) {
            return;
        }

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

} /* namespace ui */
