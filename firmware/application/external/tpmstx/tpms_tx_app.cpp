/*
 * Copyright (C) 2026 Speedster04
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

#include "tpms_tx_app.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "spi_image.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "manchester.hpp"
#include "rtc_time.hpp"
#include "file_path.hpp"
#include "encoders.hpp"
#include "crc.hpp"

using namespace portapack;
using namespace tpms;

namespace ui::external_app::tpmstx {

void TPMSTXView::focus() {
    options_packet_type.focus();
}

void TPMSTXView::update_signal_type_from_packet() {
    // Auto-select signal type based on packet type
    switch (packet_type_) {
        case tpms::Reading::Type::Schrader:
            signal_type_ = tpms::SignalType::OOK_8k192_Schrader;
            break;
        case tpms::Reading::Type::FLM_64:
        case tpms::Reading::Type::FLM_72:
        case tpms::Reading::Type::FLM_80:
            signal_type_ = tpms::SignalType::FSK_19k2_Schrader;
            break;
        case tpms::Reading::Type::GMC_96:
            signal_type_ = tpms::SignalType::OOK_8k4_Schrader;
            break;
        // EU 433MHz (FSK 19k2 path)
        case tpms::Reading::Type::Ford:
        case tpms::Reading::Type::Citroen_PSA:
        case tpms::Reading::Type::Renault:
        case tpms::Reading::Type::Jansite:
        case tpms::Reading::Type::Hyundai_VDO:
        case tpms::Reading::Type::Abarth:
        case tpms::Reading::Type::Renault_0435R:
            signal_type_ = tpms::SignalType::FSK_19k2_Schrader;
            break;
        // EU 433MHz (new M4 paths)
        case tpms::Reading::Type::BMW_G45:
            signal_type_ = tpms::SignalType::FSK_38k4_BMW_G45;
            break;
        case tpms::Reading::Type::BMW_G23:
            signal_type_ = tpms::SignalType::FSK_19k2_BMW_G23;
            break;
        case tpms::Reading::Type::Porsche:
            signal_type_ = tpms::SignalType::FSK_19k2_Porsche;
            break;
        // World 315MHz
        case tpms::Reading::Type::Toyota:
            signal_type_ = tpms::SignalType::FSK_19k2_Toyota;
            break;
        case tpms::Reading::Type::JansiteSolar:
            signal_type_ = tpms::SignalType::FSK_19k2_JansiteSolar;
            break;
        default:
            signal_type_ = tpms::SignalType::OOK_8k192_Schrader;
            break;
    }
    // Note: Don't call switch_baseband() here - it's called when needed
}

void TPMSTXView::switch_baseband() {
    // Switch baseband image based on signal type
    // Must shutdown first to avoid BBRunning error
    baseband::shutdown();
    chThdSleepMilliseconds(100);

    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        baseband::run_image(portapack::spi_flash::image_tag_ook);
    } else {
        baseband::run_image(portapack::spi_flash::image_tag_fsktx);
    }

    chThdSleepMilliseconds(100);
}

void TPMSTXView::update_bar_display() {
    int bar10 = pressure_kpa_ / 10;
    text_bar_tx.set("(" + to_string_dec_int(bar10 / 10, 1) + "." +
                    to_string_dec_int(bar10 % 10, 1) + " BAR)");
}

void TPMSTXView::update_packet_display() {
    update_bar_display();
    // Only update status text - field values are already set by user interaction
    // or by explicit set_value calls when loading from file
    std::string status = "ID:" + to_string_hex(transponder_id_, 8);

    // Note protocol-specific limitations
    if (packet_type_ == tpms::Reading::Type::Schrader) {
        status = "ID:" + to_string_hex(transponder_id_ & 0x00FFFFFF, 6) + " (24bit)";
    }

    status += " " + to_string_dec_uint(pressure_kpa_) + "kPa";

    // Check for pressure overflow and warn user
    if (packet_type_ == tpms::Reading::Type::GMC_96) {
        if (pressure_kpa_ > 701) {
            status += " !MAX";  // GMC_96 max is 701 kPa (101.7 PSI)
        }
    } else {
        if (pressure_kpa_ > 340) {
            status += " !MAX";  // Schrader/FLM max is 340 kPa (49.3 PSI)
        }
    }

    // Temperature note for protocols that support it
    if (packet_type_ == tpms::Reading::Type::GMC_96 ||
        packet_type_ == tpms::Reading::Type::FLM_64 ||
        packet_type_ == tpms::Reading::Type::FLM_72 ||
        packet_type_ == tpms::Reading::Type::FLM_80) {
        status += " " + to_string_dec_int(temperature_c_) + "C";
    } else {
        status += " (no temp)";
    }

    text_status.set(status);
}

void TPMSTXView::update_field_visibility() {
    // Schrader: Show Func, Hide Temp (no temperature support)
    // GMC_96, FLM_xx: Hide Func, Show Temp (temperature supported, no func field)
    if (packet_type_ == tpms::Reading::Type::Schrader) {
        // Show Func field and label
        label_flags.hidden(false);
        field_flags.hidden(false);
        // Hide Temp field, label, and unit selector
        label_temperature.hidden(true);
        field_temperature.hidden(true);
        options_temperature.hidden(true);
        // Show 24-bit ID field, hide 32-bit ID field
        field_transponder_id_24.hidden(false);
        field_transponder_id_32.hidden(true);
    } else {
        // Hide Func field and label
        label_flags.hidden(true);
        field_flags.hidden(true);
        // Show Temp field, label, and unit selector
        label_temperature.hidden(false);
        field_temperature.hidden(false);
        options_temperature.hidden(false);
        // Show 32-bit ID field, hide 24-bit ID field
        field_transponder_id_24.hidden(true);
        field_transponder_id_32.hidden(false);
    }
    // Force screen refresh to clear hidden widgets
    set_dirty();
}

void TPMSTXView::on_pressure_unit_change() {
    // Convert displayed pressure to new unit and update field
    units::Pressure pressure(pressure_kpa_);
    int display_value = 0;

    if (format::pressure_unit == PRESSURE_UNIT_PSI) {
        display_value = pressure.psi();
    } else if (format::pressure_unit == PRESSURE_UNIT_BAR) {
        display_value = pressure.kilopascal() / 10;  // display as bar*10 (e.g. 23 = 2.3 BAR)
    } else {
        display_value = pressure.kilopascal();
    }

    field_pressure.set_value(display_value);
}

void TPMSTXView::on_temperature_unit_change() {
    // Convert displayed temperature to new unit and update field
    units::Temperature temperature(temperature_c_);
    int display_value = 0;

    if (format::temp_unit == TEMP_UNIT_FAHRENHEIT) {
        display_value = temperature.fahrenheit();
    } else {
        display_value = temperature.celsius();
    }

    field_temperature.set_value(display_value);
}

void TPMSTXView::encode_and_transmit() {
    if (!is_transmitting_) return;

    // Build TPMS packet based on signal type
    std::string binary_string;
    uint32_t symbol_rate;
    uint32_t sample_rate;

    // Set sample rate based on signal type to match transmitter config
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        sample_rate = 2000000;  // 2 MHz for OOK
    } else {
        sample_rate = 2280000;  // 2.28 MHz for FSK
    }

    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader) {
        // Schrader OOK 8k192 format (Type = Schrader)
        // Preamble: 11*2, 01*14, 11, 10
        // Data: 3 bits function code, 24 bits ID, 8 bits pressure, 2 bits checksum
        // Total: 37 Manchester symbols (74 bits after encoding)
        // NOTE: This protocol does NOT support temperature transmission
        // NOTE: Only lower 24 bits of ID are transmitted
        // NOTE: Function code (flags_) is stored as 3-bit value (0-7)
        //       RX will display it combined with checksum in upper nibble

        symbol_rate = 8192;

        // Preamble as expected by RX decoder
        binary_string = "1111";  // 11*2
        for (int i = 0; i < 14; i++) {
            binary_string += "01";
        }
        binary_string += "1110";  // 11, 10

        // Build data bits (pre-Manchester)
        uint64_t data = 0;

        // 3-bit flags (0-7, stored directly)
        uint8_t flags_3bit = flags_ & 0x07;
        data |= ((uint64_t)flags_3bit << 34);

        // 24-bit ID (only use lower 24 bits - protocol limitation)
        uint32_t id_24bit = transponder_id_ & 0x00FFFFFF;
        data |= ((uint64_t)id_24bit << 10);

        // 8-bit pressure (convert kPa to raw: kPa * 3/4)
        // Clamp to prevent overflow: max raw = 255, max kPa = 255 * 4/3 = 340 (49.3 PSI)
        uint16_t pressure_clamped = (pressure_kpa_ > 340) ? 340 : pressure_kpa_;
        uint8_t pressure_raw = (pressure_clamped * 3 / 4);
        data |= ((uint64_t)pressure_raw << 2);

        // Calculate 2-bit checksum: sum all 2-bit pairs, result & 3 must equal 3
        uint32_t checksum_calc = (data >> 36) & 1;  // First bit
        for (size_t i = 1; i < 37; i += 2) {
            checksum_calc += (data >> (37 - i - 2)) & 3;
        }
        uint8_t checksum_2bit = (3 - (checksum_calc & 3)) & 3;
        data |= checksum_2bit;

        // Manchester encode the 37 data bits
        for (int i = 36; i >= 0; i--) {
            if ((data >> i) & 1) {
                binary_string += "10";  // '1' = 10 in Manchester
            } else {
                binary_string += "01";  // '0' = 01 in Manchester
            }
        }

    } else if (signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        // GMC_96 OOK 8k4 format
        symbol_rate = 8400;

        // Preamble: 01*40
        for (int i = 0; i < 40; i++) {
            binary_string += "01";
        }

        // First nibble of System ID (0x4) - part of preamble pattern match
        // RX looks for pattern ending with: 01010101...01100101
        // The "01100101" = Manchester for 0x4, and is consumed by pattern matcher
        binary_string += "01";  // 0
        binary_string += "10";  // 1
        binary_string += "01";  // 0
        binary_string += "01";  // 0  (0100 = 0x4)

        // Remaining 20 bits of system ID (padding with zeros)
        // These are the first 20 bits of the 76-bit payload
        for (int i = 0; i < 20; i++) {
            binary_string += "01";  // All zeros for padding
        }

        // 32-bit ID (Manchester encoded)
        for (int i = 31; i >= 0; i--) {
            if ((transponder_id_ >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }

        // Pressure: kPa * 4/11
        // Clamp to prevent overflow: max raw = 255, max kPa = 255 * 11/4 = 701.25 (101.7 PSI)
        uint16_t pressure_clamped = (pressure_kpa_ > 701) ? 701 : pressure_kpa_;
        uint8_t pressure_gmc = (pressure_clamped * 4 / 11);
        for (int i = 7; i >= 0; i--) {
            if ((pressure_gmc >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }

        // Temperature: temp + 61
        uint8_t temp_gmc = (temperature_c_ + 61) & 0xFF;
        for (int i = 7; i >= 0; i--) {
            if ((temp_gmc >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }

        // Checksum: sum of all data bytes (system ID + ID + pressure + temp)
        // System ID byte 0: (0x4 << 4) | 0x0 = 0x40
        // System ID byte 1: 0x00
        // System ID byte 2: 0x00
        uint8_t checksum = 0x40;  // First byte of system ID
        checksum += 0x00;         // Second byte of system ID
        checksum += 0x00;         // Third byte of system ID

        // Add all 4 bytes of the ID
        checksum += (transponder_id_ >> 24) & 0xFF;
        checksum += (transponder_id_ >> 16) & 0xFF;
        checksum += (transponder_id_ >> 8) & 0xFF;
        checksum += transponder_id_ & 0xFF;

        // Add pressure and temperature
        checksum += pressure_gmc;
        checksum += temp_gmc;

        for (int i = 7; i >= 0; i--) {
            if ((checksum >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_Schrader &&
               (packet_type_ == tpms::Reading::Type::FLM_64 ||
                packet_type_ == tpms::Reading::Type::FLM_72 ||
                packet_type_ == tpms::Reading::Type::FLM_80)) {
        // FSK 19.2k for FLM variants
        // Data is Manchester encoded: each data bit becomes two FSK symbols
        // Preamble: 14 x '01' + '10' = 30 bits (matches RX pattern exactly)
        // Payload: 160 Manchester-encoded bits (80 data bits max)

        symbol_rate = 19200;

        // Build preamble: 14 pairs of "01" followed by "10"
        for (int i = 0; i < 14; i++) {
            binary_string += "01";
        }
        binary_string += "10";

        std::array<uint8_t, 20> data_bytes = {0};  // 160 bits = 20 bytes max
        size_t num_data_bits = 0;

        if (packet_type_ == tpms::Reading::Type::FLM_64) {
            // FLM_64: 64 bits (8 bytes)
            // Bytes 0-3: ID (32 bits)
            // Byte 4: Pressure (8 bits)
            // Byte 5: Temperature (8 bits, LSB 7 bits used)
            // Byte 6: Padding
            // Byte 7: Sum checksum of bytes 0-6 (low 8 bits)

            num_data_bits = 64;

            data_bytes[0] = (transponder_id_ >> 24) & 0xFF;
            data_bytes[1] = (transponder_id_ >> 16) & 0xFF;
            data_bytes[2] = (transponder_id_ >> 8) & 0xFF;
            data_bytes[3] = transponder_id_ & 0xFF;
            // Clamp pressure to prevent overflow: max 340 kPa (49.3 PSI)
            uint16_t pressure_clamped_flm64 = (pressure_kpa_ > 340) ? 340 : pressure_kpa_;
            data_bytes[4] = (pressure_clamped_flm64 * 3 / 4);
            data_bytes[5] = ((temperature_c_ + 56) & 0x7F);  // LSB 7 bits only
            data_bytes[6] = 0x00;                            // Padding

            // Addition checksum over bytes 0-6
            uint32_t checksum = 0;
            for (int i = 0; i < 7; i++) {
                checksum += data_bytes[i];
            }
            data_bytes[7] = checksum & 0xFF;

        } else if (packet_type_ == tpms::Reading::Type::FLM_72) {
            // FLM_72: 72 bits (9 bytes)
            // Bytes 0-3: ID (32 bits)
            // Byte 4: Padding
            // Byte 5: Pressure (8 bits)
            // Byte 6: Temperature (8 bits)
            // Bytes 7-8: CRC field - RX processes ALL 9 bytes and expects CRC result = 0

            num_data_bits = 72;

            data_bytes[0] = (transponder_id_ >> 24) & 0xFF;
            data_bytes[1] = (transponder_id_ >> 16) & 0xFF;
            data_bytes[2] = (transponder_id_ >> 8) & 0xFF;
            data_bytes[3] = transponder_id_ & 0xFF;
            data_bytes[4] = 0x00;  // Padding
            // Clamp pressure to prevent overflow: max 340 kPa (49.3 PSI)
            uint16_t pressure_clamped_flm = (pressure_kpa_ > 340) ? 340 : pressure_kpa_;
            data_bytes[5] = (pressure_clamped_flm * 3 / 4);
            data_bytes[6] = (temperature_c_ + 56) & 0xFF;
            data_bytes[7] = 0x00;  // Set to 0 initially

            // Calculate CRC over bytes 0-7, place result in byte 8
            // When RX calculates CRC over bytes 0-8, it should get residual 0
            CRC<8> crc{0x01, 0x00};
            for (int i = 0; i < 8; i++) {
                crc.process_byte(data_bytes[i]);
            }
            data_bytes[8] = crc.checksum() & 0xFF;

        } else if (packet_type_ == tpms::Reading::Type::FLM_80) {
            // FLM_80: 80 bits (10 bytes)
            // Byte 0: Padding
            // Bytes 1-4: ID (32 bits)
            // Byte 5: Padding
            // Byte 6: Pressure (8 bits)
            // Byte 7: Temperature (8 bits)
            // Bytes 8-9: CRC field - RX processes bytes 1-9 and expects CRC result = 0

            num_data_bits = 80;

            data_bytes[0] = 0x00;  // Padding (not included in CRC)
            data_bytes[1] = (transponder_id_ >> 24) & 0xFF;
            data_bytes[2] = (transponder_id_ >> 16) & 0xFF;
            data_bytes[3] = (transponder_id_ >> 8) & 0xFF;
            data_bytes[4] = transponder_id_ & 0xFF;
            data_bytes[5] = 0x00;  // Padding
            // Clamp pressure to prevent overflow: max 340 kPa (49.3 PSI)
            uint16_t pressure_clamped_flm80 = (pressure_kpa_ > 340) ? 340 : pressure_kpa_;
            data_bytes[6] = (pressure_clamped_flm80 * 3 / 4);
            data_bytes[7] = (temperature_c_ + 56) & 0xFF;
            data_bytes[8] = 0x00;  // Padding/Reserved
            data_bytes[9] = 0x00;  // CRC byte

            // Calculate CRC over bytes 1-8 (all bytes except 0 and 9)
            // When RX calculates CRC over bytes 1-9, it should get residual 0
            CRC<8> crc{0x01, 0x00};
            for (int i = 1; i <= 8; i++) {
                crc.process_byte(data_bytes[i]);
            }
            data_bytes[9] = crc.checksum() & 0xFF;
        }

        // Manchester-encode data bits for FSK
        // RX ManchesterDecoder with sense=0: '1' = "10", '0' = "01"
        size_t num_bytes = num_data_bits / 8;
        for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
            uint8_t byte_val = data_bytes[byte_idx];
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                if ((byte_val >> bit_idx) & 1) {
                    binary_string += "10";  // Manchester '1'
                } else {
                    binary_string += "01";  // Manchester '0'
                }
            }
        }

        // Pad payload to 160 bits with valid Manchester pairs ("01" = 0)
        // Using proper Manchester encoding maintains clock recovery sync
        while (binary_string.length() < 190) {  // 30 preamble + 160 payload
            binary_string += "01";              // Manchester-encoded zero
        }

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_Schrader &&
               (packet_type_ == tpms::Reading::Type::Ford ||
                packet_type_ == tpms::Reading::Type::Citroen_PSA ||
                packet_type_ == tpms::Reading::Type::Renault ||
                packet_type_ == tpms::Reading::Type::Jansite ||
                packet_type_ == tpms::Reading::Type::Hyundai_VDO ||
                packet_type_ == tpms::Reading::Type::Abarth ||
                packet_type_ == tpms::Reading::Type::Renault_0435R)) {
        // EU/World protocols sharing the FSK 19k2 preamble path
        symbol_rate = 19200;

        // Standard FSK preamble: 14 x "01" + "10"
        for (int i = 0; i < 14; i++) binary_string += "01";
        binary_string += "10";

        if (packet_type_ == tpms::Reading::Type::Ford) {
            // Ford/VDO Continental S180084730Z
            // Packet: II II II II PP TT FF CC (8 bytes), SUM checksum
            uint8_t b[8] = {};
            b[0] = (transponder_id_ >> 24) & 0xFF;
            b[1] = (transponder_id_ >> 16) & 0xFF;
            b[2] = (transponder_id_ >> 8) & 0xFF;
            b[3] = transponder_id_ & 0xFF;
            int psi_raw = static_cast<int>(pressure_kpa_) * 100 / 172;
            if (psi_raw > 511) psi_raw = 511;
            b[4] = psi_raw & 0xFF;
            b[5] = (temperature_c_ + 56) & 0x7F;
            b[6] = flags_ & 0xDF;
            if (psi_raw >= 256) b[6] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++) sum += b[i];
            b[7] = sum;
            for (size_t byte_idx = 0; byte_idx < 8; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Citroen_PSA) {
            // Citroen/PSA Group - XOR checksum
            // Packet: SS II II II II FR PP TT BB -- (10 bytes)
            uint8_t b[10] = {};
            b[0] = 0x00;
            b[1] = (transponder_id_ >> 24) & 0xFF;
            b[2] = (transponder_id_ >> 16) & 0xFF;
            b[3] = (transponder_id_ >> 8) & 0xFF;
            b[4] = transponder_id_ & 0xFF;
            b[5] = flags_;
            b[6] = static_cast<uint8_t>(pressure_kpa_ * 250 / 341);
            if (b[6] == 0) b[6] = 1;
            b[7] = static_cast<uint8_t>(temperature_c_ + 50);
            if (b[7] == 0) b[7] = 1;
            b[8] = 0x00;
            uint8_t crc = 0;
            for (int i = 1; i < 9; i++) crc ^= b[i];
            b[9] = crc;
            for (size_t byte_idx = 0; byte_idx < 10; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Renault) {
            // Renault/Dacia - CRC-8 poly=0x07 init=0x00
            // Packet: FF PP TT II II II ?? ?? CC (9 bytes)
            uint8_t b[9] = {};
            int pressure_raw = pressure_kpa_ * 4 / 3;
            if (pressure_raw > 1023) pressure_raw = 1023;
            b[0] = static_cast<uint8_t>((flags_ & 0x3F) << 2) | static_cast<uint8_t>((pressure_raw >> 8) & 0x03);
            b[1] = pressure_raw & 0xFF;
            b[2] = static_cast<uint8_t>(temperature_c_ + 30);
            b[3] = transponder_id_ & 0xFF;
            b[4] = (transponder_id_ >> 8) & 0xFF;
            b[5] = (transponder_id_ >> 16) & 0xFF;
            b[6] = 0xFF;
            b[7] = 0xFF;
            CRC<8> crc{0x07, 0x00};
            for (int i = 0; i < 8; i++) crc.process_byte(b[i]);
            b[8] = crc.checksum() & 0xFF;
            for (size_t byte_idx = 0; byte_idx < 9; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Hyundai_VDO) {
            // Hyundai/VDO Continental TG1C - CRC-8 poly=0x07 init=0xAA
            // Packet: UU II II II II PP TT BB ?? CC (10 bytes)
            uint8_t b[10] = {};
            b[0] = flags_;
            b[1] = (transponder_id_ >> 24) & 0xFF;
            b[2] = (transponder_id_ >> 16) & 0xFF;
            b[3] = (transponder_id_ >> 8) & 0xFF;
            b[4] = transponder_id_ & 0xFF;
            b[5] = static_cast<uint8_t>(pressure_kpa_ * 100 / 137);
            b[6] = static_cast<uint8_t>(temperature_c_ + 50);
            b[7] = 0x00;
            b[8] = 0x00;
            CRC<8> crc{0x07, 0xaa};
            for (int i = 0; i < 9; i++) crc.process_byte(b[i]);
            b[9] = crc.checksum() & 0xFF;
            for (size_t byte_idx = 0; byte_idx < 10; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Abarth) {
            // Abarth/Fiat/Alfa Romeo - XOR checksum
            // Packet: II II II II ?? PP TT SS CC (9 bytes)
            uint8_t b[9] = {};
            b[0] = (transponder_id_ >> 24) & 0xFF;
            b[1] = (transponder_id_ >> 16) & 0xFF;
            b[2] = (transponder_id_ >> 8) & 0xFF;
            b[3] = transponder_id_ & 0xFF;
            b[4] = 0x00;
            b[5] = static_cast<uint8_t>(pressure_kpa_ * 100 / 138);
            b[6] = static_cast<uint8_t>(temperature_c_ + 50);
            b[7] = flags_;
            uint8_t xr = 0;
            for (int i = 0; i < 8; i++) xr ^= b[i];
            b[8] = xr;
            for (size_t byte_idx = 0; byte_idx < 9; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Renault_0435R) {
            // Renault 0435R - newer Renault/Dacia/Nissan EU - XOR checksum
            // Packet: CC II II II II PP TT RR ?? (9 bytes), b[0]=0xc0
            uint8_t b[9] = {};
            b[0] = 0xc0;
            b[1] = (transponder_id_ >> 24) & 0xFF;
            b[2] = (transponder_id_ >> 16) & 0xFF;
            b[3] = (transponder_id_ >> 8) & 0xFF;
            b[4] = transponder_id_ & 0xFF;
            b[5] = static_cast<uint8_t>(pressure_kpa_ * 4 / 3);
            b[6] = static_cast<uint8_t>(temperature_c_ + 30);
            b[7] = 0x00;
            uint8_t xr2 = 0;
            for (int i = 0; i < 8; i++) xr2 ^= b[i];
            b[8] = xr2;
            for (size_t byte_idx = 0; byte_idx < 9; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "10" : "01";

        } else if (packet_type_ == tpms::Reading::Type::Jansite) {
            // Jansite TY02S - inverted Manchester, no CRC
            // Packet: II II II IS PP TT CC (7 bytes)
            uint8_t b[7] = {};
            uint32_t id28 = transponder_id_ & 0x0FFFFFFF;
            b[0] = (id28 >> 20) & 0xFF;
            b[1] = (id28 >> 12) & 0xFF;
            b[2] = (id28 >> 4) & 0xFF;
            b[3] = static_cast<uint8_t>((id28 << 4) & 0xF0) | (flags_ & 0x0F);
            b[4] = static_cast<uint8_t>(pressure_kpa_ * 10 / 17);
            b[5] = static_cast<uint8_t>(temperature_c_ + 50);
            b[6] = 0x00;
            for (size_t byte_idx = 0; byte_idx < 7; byte_idx++)
                for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                    binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "01" : "10";  // inverted
        }

        while (binary_string.length() < 190) binary_string += "01";

    } else if (signal_type_ == tpms::SignalType::FSK_38k4_BMW_G45) {
        // BMW Gen4/5 + Audi - inverted Manchester ~40kbps
        // Preamble: 0xAA59 (16 raw bits), CRC-8 poly=0x2F init=0xAA
        symbol_rate = 40000;
        binary_string = "1010101001011001";
        uint8_t b[11] = {};
        b[0] = flags_;
        b[1] = (transponder_id_ >> 24) & 0xFF;
        b[2] = (transponder_id_ >> 16) & 0xFF;
        b[3] = (transponder_id_ >> 8) & 0xFF;
        b[4] = transponder_id_ & 0xFF;
        b[5] = static_cast<uint8_t>(pressure_kpa_ * 20 / 49);
        b[6] = static_cast<uint8_t>(temperature_c_ + 52);
        b[7] = 0x01;
        b[8] = 0x00;
        b[9] = 0x00;
        CRC<8> crc{0x2f, 0xaa};
        for (int i = 0; i < 10; i++) crc.process_byte(b[i]);
        b[10] = crc.checksum() & 0xFF;
        for (size_t byte_idx = 0; byte_idx < 11; byte_idx++)
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "01" : "10";  // inverted

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_BMW_G23) {
        // BMW Gen2/3 - NRZI 19200 bps, preamble 0xCCCD (last bit=1)
        // CRC-16 poly=0x1021 init=0x0000
        symbol_rate = 19200;
        binary_string = "1100110011001101";
        uint_fast8_t prev = 1;
        uint8_t b[11] = {};
        b[0] = (transponder_id_ >> 24) & 0xFF;
        b[1] = (transponder_id_ >> 16) & 0xFF;
        b[2] = (transponder_id_ >> 8) & 0xFF;
        b[3] = transponder_id_ & 0xFF;
        b[4] = static_cast<uint8_t>(pressure_kpa_ * 2 / 5 + 43);
        b[5] = static_cast<uint8_t>(temperature_c_ + 40);
        b[6] = flags_;
        b[7] = 0x51;
        b[8] = 0x03;
        CRC<16> crc_bmw23{0x1021, 0x0000};
        for (int i = 0; i < 9; i++) crc_bmw23.process_byte(b[i]);
        uint16_t cs = crc_bmw23.checksum();
        b[9] = (cs >> 8) & 0xFF;
        b[10] = cs & 0xFF;
        for (size_t byte_idx = 0; byte_idx < 11; byte_idx++)
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                uint_fast8_t d = (b[byte_idx] >> bit_idx) & 1;
                uint_fast8_t out = (d == 1) ? prev : (1 - prev);
                binary_string += (char)('0' + out);
                prev = out;
            }

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_Porsche) {
        // Porsche 987 - NRZI 19200 bps, preamble 0x333320 top 20 bits (last bit=0)
        // CRC-16 poly=0x1021 init=0xFFFF
        symbol_rate = 19200;
        binary_string = "00110011001100110010";
        uint_fast8_t prev = 0;
        uint8_t b[10] = {};
        b[0] = (transponder_id_ >> 24) & 0xFF;
        b[1] = (transponder_id_ >> 16) & 0xFF;
        b[2] = (transponder_id_ >> 8) & 0xFF;
        b[3] = transponder_id_ & 0xFF;
        b[4] = static_cast<uint8_t>((pressure_kpa_ + 100) * 2 / 5);
        b[5] = static_cast<uint8_t>(temperature_c_ + 40);
        b[6] = flags_;
        b[7] = 0x02;
        CRC<16> crc_prs{0x1021, 0xffff};
        for (int i = 0; i < 8; i++) crc_prs.process_byte(b[i]);
        uint16_t cs = crc_prs.checksum();
        b[8] = (cs >> 8) & 0xFF;
        b[9] = cs & 0xFF;
        for (size_t byte_idx = 0; byte_idx < 10; byte_idx++)
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                uint_fast8_t d = (b[byte_idx] >> bit_idx) & 1;
                uint_fast8_t out = (d == 1) ? prev : (1 - prev);
                binary_string += (char)('0' + out);
                prev = out;
            }

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_Toyota) {
        // Toyota PMV-C210 - NRZI 19200 bps, preamble 0xa9e0 (12 bits, last bit=0)
        // CRC-8 poly=0x07 init=0x80, with inverted pressure cross-check
        symbol_rate = 19200;
        binary_string = "101010011110";
        uint_fast8_t prev = 0;
        uint8_t b[9] = {};
        b[0] = (transponder_id_ >> 24) & 0xFF;
        b[1] = (transponder_id_ >> 16) & 0xFF;
        b[2] = (transponder_id_ >> 8) & 0xFF;
        b[3] = transponder_id_ & 0xFF;
        int p8 = pressure_kpa_ * 400 / 689 + 28;
        if (p8 < 0) p8 = 0;
        if (p8 > 255) p8 = 255;
        int t8 = temperature_c_ + 40;
        if (t8 < 0) t8 = 0;
        if (t8 > 255) t8 = 255;
        b[4] = static_cast<uint8_t>((flags_ & 0x01) << 7) | static_cast<uint8_t>((p8 >> 1) & 0x7F);
        b[5] = static_cast<uint8_t>((p8 & 0x01) << 7) | static_cast<uint8_t>((t8 >> 1) & 0x7F);
        b[6] = static_cast<uint8_t>((t8 & 0x01) << 7);
        b[7] = static_cast<uint8_t>(p8 ^ 0xFF);
        CRC<8> crc_toy{0x07, 0x80};
        for (int i = 0; i < 8; i++) crc_toy.process_byte(b[i]);
        b[8] = crc_toy.checksum() & 0xFF;
        for (size_t byte_idx = 0; byte_idx < 9; byte_idx++)
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                uint_fast8_t d = (b[byte_idx] >> bit_idx) & 1;
                uint_fast8_t out = (d == 1) ? prev : (1 - prev);
                binary_string += (char)('0' + out);
                prev = out;
            }

    } else if (signal_type_ == tpms::SignalType::FSK_19k2_JansiteSolar) {
        // Jansite Solar Model - inverted Manchester 19200 bps
        // Preamble: 0xa6a65a (24 bits), CRC-16/BUYPASS poly=0x8005 init=0x0000
        // Packet: SS SS II II II 00 TT PP 00 CK CK (11 bytes)
        symbol_rate = 19200;
        binary_string = "101001101010011001011010";
        uint8_t b[11] = {};
        b[0] = 0xDD;
        b[1] = 0x33;
        b[2] = (transponder_id_ >> 16) & 0xFF;
        b[3] = (transponder_id_ >> 8) & 0xFF;
        b[4] = transponder_id_ & 0xFF;
        b[5] = flags_;
        b[6] = static_cast<uint8_t>(temperature_c_ + 55);
        b[7] = static_cast<uint8_t>(pressure_kpa_ * 5 / 8);
        b[8] = 0x00;
        CRC<16> crc_js{0x8005, 0x0000};
        for (int i = 2; i < 9; i++) crc_js.process_byte(b[i]);
        uint16_t cs = crc_js.checksum();
        b[9] = (cs >> 8) & 0xFF;
        b[10] = cs & 0xFF;
        for (size_t byte_idx = 0; byte_idx < 11; byte_idx++)
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
                binary_string += ((b[byte_idx] >> bit_idx) & 1) ? "01" : "10";  // inverted

    } else {
        text_status.set("Unknown signal type");
        stop_tx();
        return;
    }

    // Convert binary string to bitstream
    size_t bitstream_length = encoders::make_bitstream(binary_string);

    // Calculate samples per bit (round to nearest for best timing accuracy)
    uint32_t samples_per_bit = (sample_rate + symbol_rate / 2) / symbol_rate;

    // Update status to show we're sending
    text_status.set("TX: " + to_string_dec_uint(binary_string.length()) + " bits");

    // Send via appropriate baseband (OOK or FSK)
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        // OOK mode
        baseband::set_ook_data(
            bitstream_length,
            samples_per_bit,
            repeat_count_,
            pause_duration_);
    } else {
        // FSK mode: 19.2 kbaud (or ~40kbaud for BMW Gen4/5), 38.4 kHz shift
        baseband::set_fsk_data(
            bitstream_length,
            samples_per_bit,
            38400,  // 38.4 kHz shift
            256);   // Progress notice interval
    }
}

void TPMSTXView::handle_tx_complete() {
    // For FSK (FLM packets), handle repeats at application level
    // OOK repeats are handled by the baseband processor
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        // OOK repeats are handled by baseband, just stop here
        stop_tx();
    } else {
        // FSK: handle repeats at application level
        fsk_repeat_counter_++;

        if (fsk_repeat_counter_ < repeat_count_) {
            // More repeats needed - send the next one
            // Update progress bar
            progressbar.set_value(fsk_repeat_counter_);

            // Wait for FSK processor to fully complete and reset
            // This allows the shared memory bitstream to be safely rewritten
            chThdSleepMilliseconds(50);

            encode_and_transmit();
        } else {
            // All FSK repeats complete
            stop_tx();
        }
    }
}

void TPMSTXView::start_tx() {
    if (is_transmitting_)
        return;

    is_transmitting_ = true;

    progressbar.set_max(repeat_count_);
    progressbar.set_value(0);

    button_transmit.set_text("STOP TX");
    text_status.set("Transmitting...");

    // Initialize FSK repeat counter for application-level repeat handling
    fsk_repeat_counter_ = 0;

    // Switch to appropriate baseband before transmission
    switch_baseband();

    // Configure transmitter based on modulation type
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        // OOK configuration
        transmitter_model.set_sampling_rate(2000000);  // 2 MHz for OOK
        transmitter_model.set_baseband_bandwidth(1750000);
    } else {
        // FSK configuration
        transmitter_model.set_sampling_rate(2280000);  // 2.28 MHz for FSK
        transmitter_model.set_baseband_bandwidth(1750000);
    }

    transmitter_model.enable();

    // Start transmission
    encode_and_transmit();
}

void TPMSTXView::stop_tx() {
    if (!is_transmitting_)
        return;

    is_transmitting_ = false;
    transmitter_model.disable();

    button_transmit.set_text("START TX");
    text_status.set("Transmission complete");
    progressbar.set_value(0);
}

TPMSTXView::TPMSTXView(NavigationView& nav)
    : nav_{nav} {
    // Start with OOK baseband (will switch if needed)
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&labels,
                  &label_temperature,
                  &label_flags,
                  &options_frequency,
                  &options_packet_type,
                  &field_transponder_id_24,
                  &field_transponder_id_32,
                  &field_pressure,
                  &options_pressure,
                  &text_bar_tx,
                  &field_temperature,
                  &options_temperature,
                  &field_flags,
                  &field_repeat,
                  &button_load,
                  &button_save,
                  &tx_view,
                  &button_transmit,
                  &text_status,
                  &progressbar});

    // Set label styles to match other labels (light grey)
    label_temperature.set_style(Theme::getInstance()->fg_light);
    label_flags.set_style(Theme::getInstance()->fg_light);

    // Initialize values
    options_frequency.set_by_value(transmitter_model.target_frequency());
    options_packet_type.set_selected_index(0);
    field_repeat.set_value(repeat_count_);

    // Set initial signal type based on packet type
    update_signal_type_from_packet();

    // Initialize unit selection
    options_pressure.set_by_value(format::pressure_unit);
    options_temperature.set_by_value(format::temp_unit);

    // Initialize field values from default member variables
    // For pressure and temperature, use unit conversion to display in selected units
    field_transponder_id_24.set_value(transponder_id_ & 0x00FFFFFF);
    field_transponder_id_32.set_value(transponder_id_);
    on_pressure_unit_change();
    on_temperature_unit_change();
    field_flags.set_value(flags_);

    update_field_visibility();
    update_packet_display();

    // Event handlers
    options_frequency.on_change = [this](size_t, OptionsField::value_t v) {
        transmitter_model.set_target_frequency(v);
    };

    options_packet_type.on_change = [this](size_t, int32_t value) {
        packet_type_ = static_cast<tpms::Reading::Type>(value);
        update_signal_type_from_packet();

        // Sync field values when switching packet types
        if (packet_type_ == tpms::Reading::Type::Schrader) {
            // Switching to Schrader: copy from 32-bit field to 24-bit field (masked)
            transponder_id_ = field_transponder_id_32.to_integer() & 0x00FFFFFF;
            field_transponder_id_24.set_value(transponder_id_);
        } else {
            // Switching from Schrader: copy from 24-bit field to 32-bit field
            transponder_id_ = field_transponder_id_24.to_integer();
            field_transponder_id_32.set_value(transponder_id_);
        }

        update_field_visibility();
        update_packet_display();
    };

    options_pressure.on_change = [this](size_t, int32_t i) {
        format::pressure_unit = (uint8_t)i;
        on_pressure_unit_change();
    };

    options_temperature.on_change = [this](size_t, int32_t i) {
        format::temp_unit = (uint8_t)i;
        on_temperature_unit_change();
    };

    field_transponder_id_24.on_change = [this](SymField&) {
        transponder_id_ = field_transponder_id_24.to_integer() & 0x00FFFFFF;
        update_packet_display();
    };

    field_transponder_id_32.on_change = [this](SymField&) {
        transponder_id_ = field_transponder_id_32.to_integer();
        update_packet_display();
    };

    field_pressure.on_change = [this](int32_t value) {
        // Convert from displayed unit to kPa for internal storage
        if (format::pressure_unit == PRESSURE_UNIT_PSI) {
            pressure_kpa_ = value * 6895 / 1000;
        } else if (format::pressure_unit == PRESSURE_UNIT_BAR) {
            pressure_kpa_ = value * 10;  // value is bar*10 (e.g. 23 = 2.3 BAR = 230 kPa)
        } else {
            pressure_kpa_ = value;
        }
        update_packet_display();
    };

    field_temperature.on_change = [this](int32_t value) {
        // Convert from displayed unit to Celsius for internal storage
        if (format::temp_unit == TEMP_UNIT_FAHRENHEIT) {
            temperature_c_ = (value - 32) * 5 / 9;
        } else {
            temperature_c_ = value;
        }
        update_packet_display();
    };

    field_flags.on_change = [this](int32_t value) {
        flags_ = value & 0x07;
        update_packet_display();
    };

    field_repeat.on_change = [this](int32_t value) {
        repeat_count_ = value;
    };

    button_transmit.on_select = [this](Button&) {
        if (is_transmitting_) {
            stop_tx();
        } else {
            start_tx();
        }
    };

    button_load.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path file_path) {
            // Load TPMS packet from file
            File f;
            auto error = f.open(file_path);
            if (!error.is_valid()) {
                char buffer[512];
                auto result = f.read(buffer, sizeof(buffer) - 1);
                if (result.is_ok()) {
                    buffer[result.value()] = 0;

                    // Parse the file format (key=value format, one per line)
                    std::string content(buffer);
                    size_t pos = 0;

                    auto parse_line = [&content, &pos](const std::string& key) -> std::string {
                        size_t key_pos = content.find(key + "=", pos);
                        if (key_pos != std::string::npos) {
                            size_t value_start = key_pos + key.length() + 1;
                            size_t value_end = content.find('\n', value_start);
                            if (value_end == std::string::npos) value_end = content.length();
                            return content.substr(value_start, value_end - value_start);
                        }
                        return "";
                    };

                    std::string type_str = parse_line("Type");
                    std::string id_str = parse_line("ID");
                    std::string pressure_str = parse_line("Pressure");
                    std::string temp_str = parse_line("Temperature");
                    std::string flags_str = parse_line("Flags");
                    std::string signal_type_str = parse_line("SignalType");

                    if (!type_str.empty()) {
                        int type = std::stoi(type_str);
                        if (type >= static_cast<int>(tpms::Reading::Type::FLM_64) &&
                            type <= static_cast<int>(tpms::Reading::Type::GMC_96)) {
                            packet_type_ = static_cast<tpms::Reading::Type>(type);
                            options_packet_type.set_by_value(type);
                        }
                    }

                    if (!id_str.empty()) {
                        transponder_id_ = std::stoul(id_str, nullptr, 16);
                        field_transponder_id_24.set_value(transponder_id_ & 0x00FFFFFF);
                        field_transponder_id_32.set_value(transponder_id_);
                    }

                    if (!pressure_str.empty()) {
                        pressure_kpa_ = std::stoi(pressure_str);
                        on_pressure_unit_change();
                    }

                    if (!temp_str.empty()) {
                        temperature_c_ = std::stoi(temp_str);
                        field_temperature.set_value(temperature_c_);
                    }

                    if (!flags_str.empty()) {
                        uint8_t loaded_flags = std::stoul(flags_str, nullptr, 16);
                        // Handle old format files: extract bits 6-4 if value > 7
                        flags_ = (loaded_flags > 7) ? ((loaded_flags >> 4) & 0x07) : (loaded_flags & 0x07);
                        field_flags.set_value(flags_);
                    }

                    // Load signal type if available, otherwise auto-select based on packet type
                    if (!signal_type_str.empty()) {
                        int signal_type = std::stoi(signal_type_str);
                        if (signal_type >= 1 && signal_type <= 3) {
                            signal_type_ = static_cast<tpms::SignalType>(signal_type);
                        } else {
                            // Fall back to auto-detect
                            update_signal_type_from_packet();
                        }
                    } else {
                        // Fall back to auto-detect for backwards compatibility
                        update_signal_type_from_packet();
                    }

                    update_packet_display();
                    update_field_visibility();
                    text_status.set("Loaded: " + file_path.filename().string());
                } else {
                    text_status.set("Error reading file");
                }
            } else {
                text_status.set("Error opening file");
            }
        };
    };

    button_save.on_select = [this](Button&) {
        // Save current TPMS packet to file
        auto timestamp = to_string_timestamp(rtc_time::now());
        std::string file_name = "TPMS_" + timestamp + ".TXT";
        ensure_directory(tpms_dir);
        auto file_path = tpms_dir / file_name;

        File f;
        auto error = f.create(file_path);
        if (!error.is_valid()) {
            std::string content = "Type=" + to_string_dec_uint(toUType(packet_type_), 1) + "\n";
            content += "ID=" + to_string_hex(transponder_id_, 8) + "\n";
            content += "Pressure=" + to_string_dec_uint(pressure_kpa_) + "\n";
            content += "Temperature=" + to_string_dec_int(temperature_c_) + "\n";
            content += "Flags=" + to_string_hex(flags_ & 0x07, 1) + "\n";

            f.write(content.c_str(), content.length());
            text_status.set("Saved: " + file_name);
        } else {
            text_status.set("Error saving file");
        }
    };
}

TPMSTXView::~TPMSTXView() {
    stop_tx();
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::tpmstx
