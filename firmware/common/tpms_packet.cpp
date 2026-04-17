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
            break;
    }
    // Try EU/World sub-decoders on the same FSK 19k2 path
    auto r = reading_fsk_19k2_ford();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_citroen();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_renault();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_jansite();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_solar_truck();
    if (r.is_valid()) return r;
    return {};
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

Optional<Reading> Packet::reading() const {
    switch (signal_type()) {
        case SignalType::FSK_19k2_Schrader:
            return reading_fsk_19k2_schrader();
        case SignalType::OOK_8k192_Schrader:
            return reading_ook_8k192_schrader();
        case SignalType::OOK_8k4_Schrader:
            return reading_ook_8k4_schrader();
        case SignalType::FSK_38k4_BMW_G45:
            return reading_fsk_38k4_bmw_g45();
        case SignalType::FSK_19k2_BMW_G23:
            return reading_fsk_19k2_bmw_g23();
        case SignalType::FSK_19k2_Porsche:
            return reading_fsk_19k2_porsche();
        case SignalType::FSK_19k2_Toyota:
            return reading_fsk_19k2_toyota();
        case SignalType::FSK_19k2_Elantra:
            return reading_fsk_19k2_elantra();
        case SignalType::FSK_19k2_JansiteSolar:
            return reading_fsk_19k2_jansite_solar();
        default:
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

// ---------------------------------------------------------------------------
// NRZI helper (used by BMW G2/3, Porsche, Toyota decoders)
// ---------------------------------------------------------------------------
size_t Packet::nrzi_decode(uint8_t* bytes, size_t n_bits, uint_fast8_t prev_bit) const {
    const size_t available = packet_.size();
    const size_t bits = (n_bits < available) ? n_bits : available;
    const size_t bytes_needed = (n_bits + 7) / 8;
    for (size_t i = 0; i < bytes_needed; i++) bytes[i] = 0;
    for (size_t i = 0; i < bits; i++) {
        const uint_fast8_t cur = packet_[i];
        const uint_fast8_t decoded = (cur == prev_bit) ? 1 : 0;
        bytes[i / 8] = static_cast<uint8_t>((bytes[i / 8] << 1) | decoded);
        prev_bit = cur;
    }
    return bits;
}

// ---------------------------------------------------------------------------
// EU 433MHz sub-decoders called from reading_fsk_19k2_schrader()
// ---------------------------------------------------------------------------

// Ford/VDO Continental S180084730Z (315 + 433.92 MHz)
// Packet: II II II II PP TT FF CC (8 bytes), checksum = SUM(b[0..6])
Optional<Reading> Packet::reading_fsk_19k2_ford() const {
    uint8_t b[8];
    for (size_t i = 0; i < 8; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    uint8_t sum = 0;
    for (size_t i = 0; i < 7; i++) sum += b[i];
    if (sum != b[7]) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int psi_raw = static_cast<int>(((b[6] & 0x20) << 3) | b[4]);
    Optional<Temperature> temp{};
    if ((b[5] & 0x80) == 0) temp = Temperature{static_cast<int>(b[5] & 0x7f) - 56};
    return Reading{Reading::Type::Ford, id, Pressure{psi_raw * 172 / 100}, temp, Flags{b[6]}};
}

// Citroen/PSA Group (433.92 MHz)
// Packet: SS II II II II FR PP TT BB -- (10 bytes), checksum = XOR(b[1..9]) == 0
Optional<Reading> Packet::reading_fsk_19k2_citroen() const {
    uint8_t b[10];
    for (size_t i = 0; i < 10; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    if (b[6] == 0 || b[7] == 0) return {};
    uint8_t crc = 0;
    for (size_t i = 1; i < 10; i++) crc ^= b[i];
    if (crc != 0) return {};
    const uint32_t id = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 8) | b[4];
    if (id == 0) return {};
    return Reading{Reading::Type::Citroen_PSA, id,
                   Pressure{static_cast<int>(b[6]) * 341 / 250},
                   Temperature{static_cast<int>(b[7]) - 50}, Flags{b[5]}};
}

// Renault/Dacia (433.92 MHz)
// Packet: FF PP TT II II II ?? ?? CC (9 bytes), CRC-8 poly=0x07 init=0x00
Optional<Reading> Packet::reading_fsk_19k2_renault() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    CRC<8> crc{0x07, 0x00};
    for (size_t i = 0; i < 8; i++) crc.process_byte(b[i]);
    if (crc.checksum() != b[8]) return {};
    const uint32_t id = ((uint32_t)b[5] << 16) | ((uint32_t)b[4] << 8) | b[3];
    if (id == 0) return {};
    const int pressure_raw = (static_cast<int>(b[0] & 0x03) << 8) | b[1];
    return Reading{Reading::Type::Renault, id,
                   Pressure{pressure_raw * 3 / 4},
                   Temperature{static_cast<int>(b[2]) - 30},
                   Flags{static_cast<uint8_t>(b[0] >> 2)}};
}

// Jansite TY02S (315 + 433.92 MHz) - inverted Manchester, no CRC
// Packet: II II II IS PP TT CC (7 bytes = 56 decoded bits)
Optional<Reading> Packet::reading_fsk_19k2_jansite() const {
    uint8_t b[7];
    for (size_t i = 0; i < 7; i++) b[i] = static_cast<uint8_t>(reader_inv_.read(i * 8, 8));
    const uint32_t id = ((uint32_t)b[0] << 20) | ((uint32_t)b[1] << 12) |
                        ((uint32_t)b[2] << 4) | (b[3] >> 4);
    if (id == 0) return {};
    // False positives show 0x0000XXXX pattern — upper bytes must be non-zero
    if ((id >> 16) == 0) return {};
    // Plausibility check: pressure 0-240 raw, temperature 10-175 raw
    if (b[4] > 240 || b[5] < 10 || b[5] > 175) return {};
    return Reading{Reading::Type::Jansite, id,
                   Pressure{static_cast<int>(b[4]) * 17 / 10},
                   Temperature{static_cast<int>(b[5]) - 50},
                   Flags{static_cast<uint8_t>(b[3] & 0x0f)}};
}

// Unbranded Solar TPMS for trucks (433.92 MHz) - inverted Manchester
// Packet: U(4) II II II II WW F PPP TT CC (9 bytes after 4-bit state)
// Checksum: XOR of all 9 bytes == 0
Optional<Reading> Packet::reading_fsk_19k2_solar_truck() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++) b[i] = static_cast<uint8_t>(reader_inv_.read(4 + i * 8, 8));
    uint8_t xr = 0;
    for (size_t i = 0; i < 9; i++) xr ^= b[i];
    if (xr != 0) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int pressure_kpa = ((b[5] & 0x0f) << 8) | b[6];
    return Reading{Reading::Type::SolarTruck, id,
                   Pressure{pressure_kpa},
                   Temperature{static_cast<int>(static_cast<int8_t>(b[7]))},
                   Flags{static_cast<uint8_t>(b[5] >> 4)}};
}

// ---------------------------------------------------------------------------
// EU 433MHz new M4 signal path decoders
// ---------------------------------------------------------------------------

// BMW Gen4/5 + Audi (433.92 MHz) - inverted Manchester ~40kbps
// Packet: MM II II II II PP TT [F1 F2 F3] CC (11 bytes BMW / 8 bytes Audi)
// CRC-8 poly=0x2F init=0xAA (residue check over all N bytes == 0)
Optional<Reading> Packet::reading_fsk_38k4_bmw_g45() const {
    uint8_t b[11];
    for (size_t i = 0; i < 11; i++) b[i] = static_cast<uint8_t>(reader_inv_.read(i * 8, 8));
    uint8_t len = 0;
    {
        CRC<8> crc{0x2f, 0xaa};
        for (size_t i = 0; i < 11; i++) crc.process_byte(b[i]);
        if (crc.checksum() == 0) len = 11;
    }
    if (len == 0) {
        CRC<8> crc{0x2f, 0xaa};
        for (size_t i = 0; i < 8; i++) crc.process_byte(b[i]);
        if (crc.checksum() == 0) len = 8;
    }
    if (len == 0) return {};
    const uint32_t id = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) |
                        ((uint32_t)b[3] << 8) | b[4];
    if (id == 0) return {};
    const int pres_kpa_g45 = static_cast<int>(b[5]) * 245 / 100;
    const int temp_c_g45   = static_cast<int>(b[6]) - 52;
    if (pres_kpa_g45 < 100 || pres_kpa_g45 > 450) return {};
    if (temp_c_g45   < -40 || temp_c_g45   > 85)  return {};
    return Reading{Reading::Type::BMW_G45, id,
                   Pressure{pres_kpa_g45},
                   Temperature{temp_c_g45}, Flags{b[0]}};
}

// BMW Gen2/3 (433.92 MHz) - NRZI 19200 bps, preamble 0xCCCD (last bit=1)
// Packet Gen3: II II II II PP TT F1 F2 F3 CK CK (11 bytes)
// CRC-16 poly=0x1021 init=0x0000 (residue check == 0)
Optional<Reading> Packet::reading_fsk_19k2_bmw_g23() const {
    uint8_t b[11]{};
    const size_t decoded = nrzi_decode(b, 11 * 8, 1);
    if (decoded < 10 * 8) return {};
    uint8_t len = 0;
    if (decoded >= 11 * 8) {
        CRC<16> c{0x1021, 0x0000};
        for (size_t i = 0; i < 11; i++) c.process_byte(b[i]);
        if (c.checksum() == 0) len = 11;
    }
    if (len == 0) {
        CRC<16> c{0x1021, 0x0000};
        for (size_t i = 0; i < 10; i++) c.process_byte(b[i]);
        if (c.checksum() == 0) len = 10;
    }
    if (len == 0) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int pres_kpa_g23 = (static_cast<int>(b[4]) - 43) * 5 / 2;
    const int temp_c_g23   = static_cast<int>(b[5]) - 40;
    if (pres_kpa_g23 < 100 || pres_kpa_g23 > 450) return {};
    if (temp_c_g23   < -40 || temp_c_g23   > 85)  return {};
    return Reading{Reading::Type::BMW_G23, id,
                   Pressure{pres_kpa_g23},
                   Temperature{temp_c_g23}, Flags{b[6]}};
}

// Porsche 987 Boxster/Cayman (433.92 MHz) - NRZI 19200 bps, preamble 0x333320 (last bit=0)
// Packet: II II II II PP TT SS SS CK CK (10 bytes)
// CRC-16 poly=0x1021 init=0xFFFF (residue check == 0)
Optional<Reading> Packet::reading_fsk_19k2_porsche() const {
    uint8_t b[10]{};
    if (nrzi_decode(b, 10 * 8, 0) < 10 * 8) return {};
    CRC<16> c{0x1021, 0xffff};
    for (size_t i = 0; i < 10; i++) c.process_byte(b[i]);
    if (c.checksum() != 0) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int p = static_cast<int>(b[4]) * 5 / 2 - 100;
    return Reading{Reading::Type::Porsche, id,
                   Pressure{p > 0 ? p : 0}, Temperature{static_cast<int>(b[5]) - 40}, Flags{b[6]}};
}

// ---------------------------------------------------------------------------
// World 315MHz new M4 signal path decoders
// ---------------------------------------------------------------------------

// Toyota PMV-C210 (315 MHz) - NRZI 19200 bps, preamble 0xa9e0 (12 bits, last bit=0)
// Packet: II II II II x PP TT x iP CC (9 bytes, bitfields cross byte boundaries)
// CRC-8 poly=0x07 init=0x80, with inverted pressure cross-check
Optional<Reading> Packet::reading_fsk_19k2_toyota() const {
    uint8_t b[9]{};
    if (nrzi_decode(b, 9 * 8, 0) < 9 * 8) return {};
    const int pressure8 = (static_cast<int>(b[4] & 0x7f) << 1) | (b[5] >> 7);
    const int inv_pres = static_cast<int>(b[7] ^ 0xff);
    if (pressure8 != inv_pres) return {};
    CRC<8> crc{0x07, 0x80};
    for (size_t i = 0; i < 8; i++) crc.process_byte(b[i]);
    if (crc.checksum() != b[8]) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int pressure_kpa = (pressure8 * 1724 - 48300) / 1000;
    const int temp8 = (static_cast<int>(b[5] & 0x7f) << 1) | (b[6] >> 7);
    return Reading{Reading::Type::Toyota, id,
                   Pressure{pressure_kpa > 0 ? pressure_kpa : 0},
                   Temperature{temp8 - 40},
                   Flags{static_cast<uint8_t>(b[4] >> 7)}};
}

// Hyundai Elantra / Honda Civic TRW GQ4-44T (315 MHz) - standard Manchester
// Preamble 0x7155 (16 bits), CRC-8 poly=0x07 init=0x00
// Packet: PP TT ID ID ID ID FF CC (8 bytes)
Optional<Reading> Packet::reading_fsk_19k2_elantra() const {
    uint8_t b[8];
    for (size_t i = 0; i < 8; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    CRC<8> c{0x07, 0x00};
    for (size_t i = 0; i < 8; i++) c.process_byte(b[i]);
    if (c.checksum() != 0) return {};
    const uint32_t id = ((uint32_t)b[2] << 24) | ((uint32_t)b[3] << 16) |
                        ((uint32_t)b[4] << 8) | b[5];
    if (id == 0) return {};
    const int pres_kpa_e = static_cast<int>(b[0]) + 60;
    const int temp_c_e   = static_cast<int>(b[1]) - 50;
    if (pres_kpa_e < 100 || pres_kpa_e > 450) return {};
    if (temp_c_e   < -40 || temp_c_e   > 85)  return {};
    return Reading{Reading::Type::Elantra, id,
                   Pressure{pres_kpa_e},
                   Temperature{temp_c_e}, Flags{b[6]}};
}

// Jansite Solar Model (433.92 MHz) - inverted Manchester 19200 bps
// Preamble 0xa6a65a (24 bits), CRC-16/BUYPASS poly=0x8005 init=0x0000
// Packet: SS SS II II II 00 TT PP 00 CK CK (11 bytes)
Optional<Reading> Packet::reading_fsk_19k2_jansite_solar() const {
    uint8_t b[11];
    for (size_t i = 0; i < 11; i++) b[i] = static_cast<uint8_t>(reader_inv_.read(i * 8, 8));
    if (((b[0] << 8) | b[1]) != 0xdd33) return {};
    CRC<16> crc{0x8005, 0x0000};
    for (size_t i = 2; i < 9; i++) crc.process_byte(b[i]);
    const uint32_t expected = static_cast<uint32_t>((static_cast<uint16_t>(b[9]) << 8) | b[10]);
    if (crc.checksum() != expected) return {};
    const uint32_t id = ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 8) | b[4];
    if (id == 0) return {};
    return Reading{Reading::Type::JansiteSolar, id,
                   Pressure{static_cast<int>(b[7]) * 8 / 5},
                   Temperature{static_cast<int>(b[6]) - 55}, Flags{b[5]}};
}

} /* namespace tpms */
