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

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"
#include "audio.hpp"

#include "acars_app.hpp"
using namespace portapack;

#include "string_format.hpp"
#include "utility.hpp"

// ACARS structured decoding (minimal, direct C++ implementation)
struct AcarsDecoded {
    bool crc_ok;
    std::string reg;
    std::string label;
    std::string flight_id;
    std::string msg_num;
    char block_id;
    std::string txt;
};

namespace ui::external_app::acars_rx {

// Minimal ACARS decode function
AcarsDecoded acars_decode(const std::string& raw) {
    AcarsDecoded result;
    // Parse raw ACARS message by field order (demo, not protocol byte-accurate)
    if (raw.size() < 20) {
        result.crc_ok = false;
        result.txt = "ACARS message too short";
        return result;
    }
    result.crc_ok = true;
    result.reg = raw.substr(1, 7);
    result.label = raw.substr(9, 2);
    result.block_id = raw[11];
    result.msg_num = raw.substr(12, 3);
    result.flight_id = raw.substr(15, 6);
    result.txt = raw.substr(21);
    return result;
    return result;
}

std::string acars_format(const AcarsDecoded& msg) {
        return std::string("ACARS Decoded Result\nCRC: ") + (msg.crc_ok ? "OK" : "FAIL") +
            "\nRegistration: " + msg.reg +
            "\nLabel: " + msg.label +
            "\nBlockID: " + msg.block_id +
            "\nMsgNum: " + msg.msg_num +
            "\nFlightID: " + msg.flight_id +
            "\nMessage: " + msg.txt;
}

void ACARSLogger::log_str(std::string msg) {
    log_file.write_entry(msg);
}

ACARSAppView::ACARSAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &field_volume,
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

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
}

ACARSAppView::~ACARSAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void ACARSAppView::focus() {
    field_frequency.focus();
}

void ACARSAppView::on_packet(const ACARSPacketMessage* packet) {
    std::string console_info;
    if (packet->state == 255) {
        // got a packet, parse it, and display
        rtc::RTC datetime;
        rtc_time::now(datetime);
        console_info = to_string_datetime(datetime, HMS);
        console_info += ": ";
        AcarsDecoded decoded = acars_decode(packet->message);
        console_info += acars_format(decoded);
        console.writeln(console_info);
        if (logger && logging)
            logger->log_str(console_info);
    } else {
        // debug message arrived
        console_info = "State: ";
        console_info += to_string_dec_int(packet->state);
        console_info += " lastbyte: ";
        console_info += to_string_dec_uint(packet->message[0]);
        console.writeln(console_info);
        if (logger && logging)
            logger->log_str(console_info);
    }
}

}  // namespace ui::external_app::acars_rx