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

namespace ui::external_app::acars_rx {

// ACARS frame field layout (0-based byte offsets):
//   [0]          SOH framing byte   (skipped)
//   [1..7]       Aircraft registration (7 chars)
//   [8]          STX framing byte   (skipped)
//   [9..10]      Label              (2 chars)
//   [11]         Block ID           (1 char)
//   [12..14]     Message number     (3 chars)
//   [15..20]     Flight ID          (6 chars)
//   [21..N-3]    Free-text payload  (variable)
//   [N-2..N-1]   CRC-16/CCITT      (2 bytes, MSB first, NOT part of payload)
//
// Minimum valid frame: 21-byte fixed header + 2 CRC bytes = 23 bytes.
// (A zero-length payload is legal per spec; we require at least 23 bytes.)
static constexpr std::string::size_type kAcarsCrcLen = 2;
static constexpr std::string::size_type kAcarsHeaderLen = 21;
static constexpr std::string::size_type kAcarsMinLen = kAcarsHeaderLen + kAcarsCrcLen;

// CRC-16/CCITT: poly 0x1021, init 0x0000, no reflection, no final XOR.
// This is the variant used by ACARS (same as XMODEM CRC).
static uint16_t acars_crc16(const std::string& data, std::string::size_type len) {
    uint16_t crc = 0x0000;
    for (std::string::size_type i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(static_cast<uint8_t>(data[i])) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000)
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            else
                crc = static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

AcarsDecoded acars_decode(const std::string& raw) {
    AcarsDecoded result;
    if (raw.size() < kAcarsMinLen) {
        result.txt = "ACARS message too short (" + std::to_string(raw.size()) +
                     " bytes, need " + std::to_string(kAcarsMinLen) + ")";
        return result;
    }

    // Verify CRC: computed over everything except the last 2 bytes,
    // then compared against those 2 bytes (MSB first).
    const std::string::size_type payload_len = raw.size() - kAcarsCrcLen;
    const uint16_t computed = acars_crc16(raw, payload_len);
    const uint16_t received = (static_cast<uint16_t>(static_cast<uint8_t>(raw[raw.size() - 2])) << 8) |
                              static_cast<uint16_t>(static_cast<uint8_t>(raw[raw.size() - 1]));
    result.crc_ok = (computed == received);

    result.reg = raw.substr(1, 7);
    result.label = raw.substr(9, 2);
    result.block_id = raw[11];
    result.msg_num = raw.substr(12, 3);
    result.flight_id = raw.substr(15, 6);
    // Payload sits between end of fixed header and the 2 trailing CRC bytes.
    if (payload_len > kAcarsHeaderLen)
        result.txt = raw.substr(kAcarsHeaderLen, payload_len - kAcarsHeaderLen);
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
        std::string message{packet->message, packet->message + packet->msg_len};
        AcarsDecoded decoded = acars_decode(message);
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