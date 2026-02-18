/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
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

#include "tpms_packet.hpp"

#include "crc.hpp"

namespace tpms {

Timestamp Packet::received_at() const {
    return packet_.timestamp();
}

FormattedSymbols Packet::symbols_formatted() const {
    return format_symbols(decoder_);
}

Optional<Reading> Packet::reading_fsk_19k2_schrader() const {
    const auto length = crc_valid_length();

    switch (length) {
        case 64:
            return Reading{
                Reading::Type::FLM_64,
                (uint32_t)reader_.read(0, 32),
                Pressure{static_cast<int>(reader_.read(32, 8)) * 4 / 3},
                Temperature{static_cast<int>(reader_.read(40, 8) & 0x7f) - 56}};

        case 72:
            return Reading{
                Reading::Type::FLM_72,
                (uint32_t)reader_.read(0, 32),
                Pressure{static_cast<int>(reader_.read(40, 8)) * 4 / 3},
                Temperature{static_cast<int>(reader_.read(48, 8)) - 56}};

        case 80:
            return Reading{
                Reading::Type::FLM_80,
                (uint32_t)reader_.read(8, 32),
                Pressure{static_cast<int>(reader_.read(48, 8)) * 4 / 3},
                Temperature{static_cast<int>(reader_.read(56, 8)) - 56}};

        default:
            return {};
    }
}

Optional<Reading> Packet::reading_ook_8k192_schrader() const {
    /*
     * Preamble: 11*2, 01*14, 11, 10
     * Function code: 3 Manchester symbols
     * ID: 24 Manchester symbols (one variant seen with 21 symbols?)
     * Pressure: 8 Manchester symbols
     * Checksum: 2 Manchester symbols (2 LSBs of sum incl this field == 3)
     */
    const auto flags = reader_.read(0, 3);
    const auto checksum = reader_.read(35, 2);

    uint32_t checksum_calculated = reader_.read(0, 1);
    for (size_t i = 1; i < 37; i += 2) {
        checksum_calculated += reader_.read(i, 2);
    }

    if ((checksum_calculated & 3) == 3) {
        return Reading{
            Reading::Type::Schrader,
            (uint32_t)reader_.read(3, 24),
            Pressure{static_cast<int>(reader_.read(27, 8)) * 4 / 3},
            {},
            Flags{static_cast<Flags>((flags << 4) | checksum)}};
    } else {
        return {};
    }
}

Optional<Reading> Packet::reading_ook_8k4_schrader() const {
    /*
     * Preamble: 01*40
     * System ID: 01100101, ??*20 (not really sure what this data is)
     * ID: 32 Manchester symbols
     * Value: 8 Manchester symbols (pressure)
     * Value: 8 Manchester symbols (temperature)
     * Checksum: 8 Manchester symbols (uint8_t sum of bytes starting with system ID)
     */
    /* NOTE: First four bits of packet are consumed in preamble detection.
     * Those bits assumed to be 0b0100", which may not be entirely true...
     */
    constexpr uint8_t first_nibble = 0x4;
    // const auto system_id = (first_nibble << 20) | reader_.read(0, 20);
    const auto id = reader_.read(20, 32);
    const auto value_0 = reader_.read(52, 8);
    const auto value_1 = reader_.read(60, 8);
    const auto checksum = reader_.read(68, 8);

    uint8_t checksum_calculated = (first_nibble << 4) | reader_.read(0, 4);
    for (size_t i = 4; i < 68; i += 8) {
        checksum_calculated += reader_.read(i, 8);
    }

    if (checksum_calculated == checksum) {
        return Reading{
            Reading::Type::GMC_96,
            (uint32_t)id,
            Pressure{static_cast<int>(value_0) * 11 / 4},
            Temperature{static_cast<int>(value_1) - 61}};
    } else {
        return {};
    }
}

Optional<Reading> Packet::reading_toyota() const {
    /*
     * Toyota TPMS - FSK Differential Manchester
     * 72 bits data with CRC-8
     * Pressure in (raw * 0.25 - 7) PSI
     * Temperature in (raw - 40) C
     */
    const auto length = crc_valid_length();
    if (length != 72) {
        return {};
    }

    const auto id = reader_.read(0, 32);
    const auto status = (reader_.read(32, 1) << 7) | reader_.read(39, 7);
    const auto pressure_raw = (reader_.read(33, 7) << 1) | reader_.read(40, 1);
    const auto temp_raw = (reader_.read(41, 7) << 1) | reader_.read(48, 1);
    const auto pressure2 = reader_.read(56, 8) ^ 0xff;

    // Verify pressure consistency
    if (pressure_raw != pressure2) {
        return {};
    }

    return Reading{
        Reading::Type::Toyota,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 0.25 - 7.0) * 7},  // Convert to kPa
        Temperature{static_cast<int>(temp_raw - 40)}};
}

Optional<Reading> Packet::reading_ford() const {
    /*
     * Ford TPMS - FSK Manchester
     * 64 bits with simple checksum
     * Pressure in (raw * 0.25) PSI
     * Temperature in (raw - 56) C
     */
    const auto id = reader_.read(0, 32);
    const auto pressure_raw = ((reader_.read(48, 1) << 8) | reader_.read(32, 8));
    const auto temp_raw = reader_.read(40, 8);
    const auto flags = reader_.read(48, 8);
    const auto checksum = reader_.read(56, 8);

    // Verify checksum
    uint8_t sum = 0;
    for (size_t i = 0; i < 7; i++) {
        sum += reader_.read(i * 8, 8);
    }
    if ((sum & 0xff) != checksum) {
        return {};
    }

    // Temperature valid if bit 7 is not set
    Optional<Temperature> temperature{};
    if ((temp_raw & 0x80) == 0) {
        temperature = Temperature{static_cast<int>(temp_raw & 0x7f) - 56};
    }

    return Reading{
        Reading::Type::Ford,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 0.25 * 7)},  // Convert to kPa
        temperature,
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_citroen() const {
    /*
     * Citroen TPMS - FSK Manchester
     * 80 bits with XOR checksum
     * Pressure in (raw * 1.364) kPa
     * Temperature in (raw - 50) C
     */
    const auto state = reader_.read(0, 8);
    const auto id = reader_.read(8, 32);
    const auto flags = reader_.read(40, 4);
    const auto repeat = reader_.read(44, 4);
    const auto pressure_raw = reader_.read(48, 8);
    const auto temp_raw = reader_.read(56, 8);
    const auto battery = reader_.read(64, 8);
    const auto checksum = reader_.read(72, 8);

    // Sanity checks
    if (pressure_raw == 0 || temp_raw == 0) {
        return {};
    }

    // Verify XOR checksum (bytes 1-9 XOR = 0)
    uint8_t xor_sum = 0;
    for (size_t i = 1; i < 10; i++) {
        xor_sum ^= reader_.read(i * 8, 8);
    }
    if (xor_sum != 0) {
        return {};
    }

    return Reading{
        Reading::Type::Citroen,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 1.364)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_renault() const {
    /*
     * Renault TPMS - FSK Manchester
     * 72 bits with CRC-8
     * Pressure in (raw * 0.75) kPa
     * Temperature in (raw - 30) C
     */
    const auto flags = reader_.read(0, 6);
    const auto pressure_raw = (reader_.read(6, 2) << 8) | reader_.read(8, 8);
    const auto temp_raw = reader_.read(16, 8);
    const auto id = reader_.read(24, 24);  // Little-endian in original
    const auto unknown = reader_.read(48, 16);
    const auto crc = reader_.read(64, 8);

    // Verify CRC-8
    uint8_t bytes[8];
    for (size_t i = 0; i < 8; i++) {
        bytes[i] = reader_.read(i * 8, 8);
    }

    CRC<8> crc_calc{0x07, 0x00};
    for (size_t i = 0; i < 8; i++) {
        crc_calc.process_byte(bytes[i]);
    }

    if (crc_calc.checksum() != crc) {
        return {};
    }

    return Reading{
        Reading::Type::Renault,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 0.75)},
        Temperature{static_cast<int>(temp_raw - 30)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_hyundai_vdo() const {
    /*
     * Hyundai VDO TPMS - FSK Manchester
     * 80 bits with CRC-8
     * Pressure in (raw * 1.375) kPa
     * Temperature in (raw - 50) C
     */
    const auto state = reader_.read(0, 8);
    const auto id = reader_.read(8, 32);
    const auto flags = reader_.read(40, 4);
    const auto repeat = reader_.read(44, 4);
    const auto pressure_raw = reader_.read(48, 8);
    const auto temp_raw = reader_.read(56, 8);
    const auto battery = reader_.read(64, 8);
    const auto crc = reader_.read(72, 8);

    // Verify CRC-8 with poly 0x07 init 0xaa
    uint8_t bytes[9];
    for (size_t i = 0; i < 9; i++) {
        bytes[i] = reader_.read(i * 8, 8);
    }

    CRC<8> crc_calc{0x07, 0xaa};
    for (size_t i = 0; i < 9; i++) {
        crc_calc.process_byte(bytes[i]);
    }

    if (crc_calc.checksum() != crc) {
        return {};
    }

    return Reading{
        Reading::Type::Hyundai_VDO,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 1.375)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_nissan() const {
    /*
     * Nissan TPMS - FSK Manchester
     * 37 bits
     * Pressure in (raw / 4.0) PSI
     */
    const auto mode = reader_.read(0, 3);
    const auto id = ((reader_.read(3, 5) << 19) |
                     (reader_.read(8, 8) << 11) |
                     (reader_.read(16, 8) << 3) |
                     reader_.read(24, 3));
    const auto pressure_raw = ((reader_.read(27, 5) << 3) | reader_.read(32, 3));

    return Reading{
        Reading::Type::Nissan,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw / 4.0 * 7)},  // Convert PSI to kPa
        {},
        Flags{static_cast<Flags>(mode)}};
}

Optional<Reading> Packet::reading_abarth124() const {
    /*
     * Abarth 124 Spider / VDO TG1C - FSK Manchester
     * 72 bits with XOR checksum
     * Pressure in (raw * 1.38) kPa
     * Temperature in (raw - 50) C
     */
    const auto id = reader_.read(0, 32);
    const auto flags = reader_.read(32, 8);
    const auto pressure_raw = reader_.read(40, 8);
    const auto temp_raw = reader_.read(48, 8);
    const auto status = reader_.read(56, 8);
    const auto checksum = reader_.read(64, 8);

    // Verify XOR checksum
    uint8_t xor_sum = 0;
    for (size_t i = 0; i < 9; i++) {
        xor_sum ^= reader_.read(i * 8, 8);
    }
    if (xor_sum != 0) {
        return {};
    }

    return Reading{
        Reading::Type::Abarth124,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 1.38)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_jansite() const {
    /*
     * Jansite TPMS - FSK Manchester
     * 56 bits (no reliable checksum)
     * Pressure in (raw * 1.7) kPa
     * Temperature in (raw - 50) C
     */
    const auto id = ((reader_.read(0, 8) << 20) |
                     (reader_.read(8, 8) << 12) |
                     (reader_.read(16, 8) << 4) |
                     (reader_.read(24, 4)));
    const auto flags = reader_.read(28, 4);
    const auto pressure_raw = reader_.read(32, 8);
    const auto temp_raw = reader_.read(40, 8);

    return Reading{
        Reading::Type::Jansite,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 1.7)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_jansite_solar() const {
    /*
     * Jansite Solar TPMS - FSK Manchester
     * 88 bits with CRC-16
     * Pressure in (raw * 1.6) kPa
     * Temperature in (raw - 55) C
     */
    const auto sync = reader_.read(0, 16);
    if (sync != 0xdd33) {
        return {};
    }

    const auto id = ((reader_.read(16, 8) << 16) |
                     (reader_.read(24, 8) << 8) |
                     reader_.read(32, 8));
    const auto flags = reader_.read(40, 8);
    const auto temp_raw = reader_.read(48, 8);
    const auto pressure_raw = reader_.read(56, 8);
    const auto unknown = reader_.read(64, 8);
    const auto crc = reader_.read(72, 16);

    // Verify CRC-16/BUYPASS
    uint8_t bytes[9];
    for (size_t i = 0; i < 9; i++) {
        bytes[i] = reader_.read(i * 8 + 16, 8);  // Skip sync word
    }

    uint16_t crc_calc = 0;
    for (size_t i = 0; i < 7; i++) {  // CRC over first 7 bytes
        crc_calc = crc_calc ^ (bytes[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc_calc & 0x8000) {
                crc_calc = (crc_calc << 1) ^ 0x8005;
            } else {
                crc_calc = crc_calc << 1;
            }
        }
    }

    if (crc_calc != crc) {
        return {};
    }

    return Reading{
        Reading::Type::Jansite_Solar,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw * 1.6)},
        Temperature{static_cast<int>(temp_raw - 55)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_kia() const {
    /*
     * Kia TPMS - FSK Manchester
     * 138 bits (after 16-bit preamble) with CRC-8
     * Pressure in (raw / 5.0) PSI
     * Temperature in (raw - 50) C
     */
    const auto unknown1 = reader_.read(0, 4);
    const auto pressure_raw = ((reader_.read(4, 4) << 4) | reader_.read(8, 4));
    const auto temp_raw = ((reader_.read(12, 4) << 4) | reader_.read(16, 4));
    const auto id = ((reader_.read(20, 4) << 28) |
                     (reader_.read(24, 8) << 20) |
                     (reader_.read(32, 8) << 12) |
                     (reader_.read(40, 8) << 4) |
                     reader_.read(48, 4));
    const auto unknown2 = ((reader_.read(52, 4) << 8) | reader_.read(56, 8));
    const auto crc = reader_.read(64, 8) & 0xF8;  // Last 3 bits are padding

    // Verify CRC-8
    uint8_t bytes[8];
    for (size_t i = 0; i < 8; i++) {
        bytes[i] = reader_.read(i * 8, 8);
    }
    bytes[8] = crc;

    CRC<8> crc_calc{0x07, 0x76};
    for (size_t i = 0; i < 8; i++) {
        crc_calc.process_byte(bytes[i]);
    }

    if (crc_calc.checksum() != crc) {
        return {};
    }

    return Reading{
        Reading::Type::Kia,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw / 5.0 * 7)},  // Convert PSI to kPa
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(unknown1)}};
}

Optional<Reading> Packet::reading_elantra2012() const {
    /*
     * Elantra 2012 / TRW - FSK Manchester
     * 64 bits with CRC-8
     * Pressure in (raw + 60) kPa
     * Temperature in (raw - 50) C
     */
    const auto pressure_raw = reader_.read(0, 8);
    const auto temp_raw = reader_.read(8, 8);
    const auto id = reader_.read(16, 32);
    const auto flags = reader_.read(48, 8);
    const auto crc = reader_.read(56, 8);

    // Verify CRC-8
    uint8_t bytes[7];
    for (size_t i = 0; i < 7; i++) {
        bytes[i] = reader_.read(i * 8, 8);
    }

    CRC<8> crc_calc{0x07, 0x00};
    for (size_t i = 0; i < 7; i++) {
        crc_calc.process_byte(bytes[i]);
    }

    if (crc_calc.checksum() != crc) {
        return {};
    }

    return Reading{
        Reading::Type::Elantra2012,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw + 60)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_pmv107j() const {
    /*
     * PMV-107J (Toyota) - FSK Differential Manchester
     * 66 bits with CRC-8
     * Pressure in (raw - 40) * 2.48 kPa
     * Temperature in (raw - 40) C
     */
    // This uses differential Manchester which requires special handling
    // For now, return empty as it needs different decoder
    return {};
}

Optional<Reading> Packet::reading_renault_0435r() const {
    /*
     * Renault 0435R - FSK Manchester
     * 72 bits with XOR checksum
     * Pressure in (raw / 0.75) kPa
     * Temperature in (raw - 50) C
     */
    const auto id = ((reader_.read(0, 8) << 16) |
                     (reader_.read(8, 8) << 8) |
                     reader_.read(16, 8));
    const auto flags = reader_.read(24, 8);
    const auto pressure_raw = reader_.read(32, 8);
    const auto temp_raw = reader_.read(40, 8);
    const auto accel = reader_.read(48, 8);
    const auto checksum = reader_.read(56, 8);
    const auto tick = reader_.read(64, 8);

    // Verify XOR checksum
    uint8_t xor_sum = 0;
    for (size_t i = 0; i < 9; i++) {
        xor_sum ^= reader_.read(i * 8, 8);
    }
    if (xor_sum != 0) {
        return {};
    }

    // Sanity check tick
    const auto has_tick = (tick >> 7) & 1;
    const auto tick_val = tick & 0x7F;
    if (tick != 0 && (!has_tick || tick_val > 30)) {
        return {};
    }

    return Reading{
        Reading::Type::Renault_0435R,
        (uint32_t)id,
        Pressure{static_cast<int>(pressure_raw / 0.75)},
        Temperature{static_cast<int>(temp_raw - 50)},
        Flags{static_cast<Flags>(flags)}};
}

Optional<Reading> Packet::reading_ave() const {
    /*
     * AVE TPMS - FSK Differential Manchester
     * 64 bits with CRC-8 (poly 0x31, init 0xff)
     * Pressure varies by mode (default: raw * 2.352 kPa)
     * Temperature in (raw - 50) C
     */
    // This uses differential Manchester which requires special handling
    // For now, return empty as it needs different decoder
    return {};
}

Optional<Reading> Packet::reading() const {
    switch (signal_type()) {
        case SignalType::FSK_19k2_Schrader:
            return reading_fsk_19k2_schrader();
        case SignalType::OOK_8k192_Schrader:
            return reading_ook_8k192_schrader();
        case SignalType::OOK_8k4_Schrader:
            return reading_ook_8k4_schrader();
        default:
            // Try all new decoders
            {
                auto result = reading_toyota();
                if (result.is_valid()) return result;

                result = reading_ford();
                if (result.is_valid()) return result;

                result = reading_citroen();
                if (result.is_valid()) return result;

                result = reading_renault();
                if (result.is_valid()) return result;

                result = reading_hyundai_vdo();
                if (result.is_valid()) return result;

                result = reading_nissan();
                if (result.is_valid()) return result;

                result = reading_abarth124();
                if (result.is_valid()) return result;

                result = reading_jansite();
                if (result.is_valid()) return result;

                result = reading_jansite_solar();
                if (result.is_valid()) return result;

                result = reading_kia();
                if (result.is_valid()) return result;

                result = reading_elantra2012();
                if (result.is_valid()) return result;

                result = reading_pmv107j();
                if (result.is_valid()) return result;

                result = reading_renault_0435r();
                if (result.is_valid()) return result;

                result = reading_ave();
                if (result.is_valid()) return result;
            }
            return {};
    }
}

size_t Packet::crc_valid_length() const {
    constexpr uint32_t checksum_bytes = 0b1111111;
    constexpr uint32_t crc_72_bytes = 0b111111111;
    constexpr uint32_t crc_80_bytes = 0b1111111110;

    std::array<uint8_t, 10> bytes;
    for (size_t i = 0; i < bytes.size(); i++) {
        bytes[i] = reader_.read(i * 8, 8);
    }

    uint32_t checksum = 0;
    CRC<8> crc_72{0x01, 0x00};
    CRC<8> crc_80{0x01, 0x00};

    for (size_t i = 0; i < bytes.size(); i++) {
        const uint32_t byte_mask = 1 << i;
        const auto byte = bytes[i];

        if (checksum_bytes & byte_mask) {
            checksum += byte;
        }
        if (crc_72_bytes & byte_mask) {
            crc_72.process_byte(byte);
        }
        if (crc_80_bytes & byte_mask) {
            crc_80.process_byte(byte);
        }
    }

    if (crc_80.checksum() == 0) {
        return 80;
    } else if (crc_72.checksum() == 0) {
        return 72;
    } else if ((checksum & 0xff) == bytes[7]) {
        return 64;
    } else {
        return 0;
    }
}

} /* namespace tpms */
