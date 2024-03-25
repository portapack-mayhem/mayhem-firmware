/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "acars_app.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace acars;

#include "string_format.hpp"
#include "utility.hpp"

void ACARSLogger::log_raw_data(const acars::Packet& packet, const uint32_t frequency) {
    (void)frequency;
    std::string entry{};  //= "Raw: F:" + to_string_dec_uint(frequency) + "Hz ";
    entry.reserve(256);

    // Raw hex dump of all the bytes
    // for (size_t c = 0; c < packet.length(); c += 32)
    //	entry += to_string_hex(packet.read(c, 32), 8) + " ";

    for (size_t c = 0; c < 256; c += 32)
        entry += to_string_bin(packet.read(c, 32), 32);

    log_file.write_entry(packet.received_at(), entry);
}

/*void ACARSLogger::log_decoded(
        const acars::Packet& packet,
        const std::string text) {

        log_file.write_entry(packet.timestamp(), text);
}*/

namespace ui {

ACARSAppView::ACARSAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_acars);

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &check_log,
                  &console});

    receiver_model.enable();

    check_log.set_value(logging);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
    };

    logger = std::make_unique<ACARSLogger>();
    if (logger)
        logger->append(logs_dir / u"ACARS.TXT");
}

ACARSAppView::~ACARSAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void ACARSAppView::focus() {
    field_frequency.focus();
}

void ACARSAppView::on_packet(const acars::Packet& packet) {
    std::string console_info;

    /*if (!packet.is_valid()) {
                console_info = to_string_datetime(packet.received_at(), HMS);
                console_info += " INVALID";

                console.writeln(console_info);
        } else {
                console_info = to_string_datetime(packet.received_at(), HMS);
                console_info += ":" + to_string_bin(packet.read(0, 32), 32);
                //console_info += " REG:" + packet.registration_number();

                console.writeln(console_info);
        }*/

    packet_counter++;
    if (packet_counter % 10 == 0)
        console.writeln("Block #" + to_string_dec_uint(packet_counter));

    // Log raw data whatever it contains
    if (logger && logging)
        logger->log_raw_data(packet, receiver_model.target_frequency());
}

} /* namespace ui */
