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
namespace pmem = portapack::persistent_memory;

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
        {&check_log,
         &check_log_raw,
         &check_small_font,
         &check_ignore,
         &field_ignore,
         &button_save});

    check_log.set_value(settings_.enable_logging);
    check_log_raw.set_value(settings_.enable_raw_log);
    check_small_font.set_value(settings_.enable_small_font);
    check_ignore.set_value(settings_.enable_ignore);
    field_ignore.set_value(settings_.address_to_ignore);

    button_save.on_select = [this, &nav](Button&) {
        settings_.enable_logging = check_log.value();
        settings_.enable_raw_log = check_log_raw.value();
        settings_.enable_small_font = check_small_font.value();
        settings_.enable_ignore = check_ignore.value();
        settings_.address_to_ignore = field_ignore.value();

        nav.pop();
    };
}

POCSAGAppView::POCSAGAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_pocsag);

    add_children(
        {&rssi,
         &audio,
         &field_rf_amp,
         &field_lna,
         &field_vga,
         &field_frequency,
         &field_volume,
         &image_status,
         &button_ignore_last,
         &button_config,
         &console});

    // No app settings, use fallbacks.
    if (!app_settings_.loaded()) {
        field_frequency.set_value(initial_target_frequency);
        settings_.address_to_ignore = pmem::pocsag_ignore_address();
        settings_.enable_ignore = settings_.address_to_ignore > 0;
    }

    logger.append(LOG_ROOT_DIR "/POCSAG.TXT");

    button_ignore_last.on_select = [this](Button&) {
        settings_.enable_ignore = true;
        settings_.address_to_ignore = last_address;
    };

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
    receiver_model.disable();
    baseband::shutdown();

    // Save pmem settings. TODO: Even needed anymore?
    pmem::set_pocsag_ignore_address(settings_.address_to_ignore);
    pmem::set_pocsag_last_address(pocsag_state.address);  // For POCSAG TX.
}

void POCSAGAppView::refresh_ui() {
    console.set_style(
        settings_.enable_small_font
            ? &Styles::white_small
            : &Styles::white);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage* message) {
    packet_toggle = !packet_toggle;
    image_status.set_foreground(packet_toggle
                                    ? Color::dark_grey()
                                    : Color::white());

    const uint32_t roundVal = 50;
    const uint32_t bitrate_rounded = roundVal * ((message->packet.bitrate() + (roundVal / 2)) / roundVal);
    auto bitrate = to_string_dec_uint(bitrate_rounded);
    auto timestamp = to_string_datetime(message->packet.timestamp(), HM);
    auto prefix = timestamp + " " + bitrate;

    if (logging_raw())
        logger.log_raw_data(message->packet, receiver_model.target_frequency());

    if (message->packet.flag() != NORMAL)
        console.writeln("\n\x1B\x04" + prefix + " CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
    else {
        pocsag_decode_batch(message->packet, &pocsag_state);

        // Too many errors for reliable decode.
        if (pocsag_state.errors >= 3) {
            console.write("\n\x1B\x0D" + prefix + " Too many decode errors.");
            return;
        }

        // Ignored address.
        if (ignore() && pocsag_state.address == settings_.address_to_ignore) {
            console.write("\n\x1B\x03" + prefix + " Ignored: " + to_string_dec_uint(pocsag_state.address));
            return;
        }

        std::string console_info = "\n" + prefix;
        console_info += " #" + to_string_dec_uint(pocsag_state.address);
        console_info += " F" + to_string_dec_uint(pocsag_state.function);

        if (pocsag_state.out_type == ADDRESS) {
            last_address = pocsag_state.address;
            console.write(console_info);

            if (logging()) {
                logger.log_decoded(
                    message->packet,
                    to_string_dec_uint(pocsag_state.address) +
                        " F" + to_string_dec_uint(pocsag_state.function) +
                        " Address only");
            }

        } else if (pocsag_state.out_type == MESSAGE) {
            if (pocsag_state.address != last_address) {
                // New message
                last_address = pocsag_state.address;
                console.writeln(console_info);
                console.write(pocsag_state.output);
            } else {
                // Message continues...
                console.write(pocsag_state.output);
            }

            if (logging()) {
                logger.log_decoded(
                    message->packet,
                    to_string_dec_uint(pocsag_state.address) +
                        " F" + to_string_dec_uint(pocsag_state.function) +
                        " > " + pocsag_state.output);
            }
        }
    }
}

void POCSAGAppView::on_stats(const POCSAGStatsMessage*) {
}

} /* namespace ui */
