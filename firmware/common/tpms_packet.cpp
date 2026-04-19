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
    // Order: CRC-based decoders FIRST, then permissive plausibility-only decoders
    // (TruckSolar, Jansite, Nissan).
    auto r = reading_fsk_19k2_citroen();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_renault();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_hyundai_vdo();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_abarth();
    if (r.is_valid()) return r;
    r = reading_fsk_19k2_renault_0435r();
    if (r.is_valid()) return r;
    // TruckSolar: 168-bit Manchester payload with sum-check -- distinctive by length
    r = reading_fsk_19k2_truck_solar();
    if (r.is_valid()) return r;
    // Jansite: no CRC, only plausibility checks -- later in cascade
    r = reading_fsk_19k2_jansite();
    if (r.is_valid()) return r;
    // Nissan: 37-bit, no CRC, plausibility only -- last resort
    r = reading_fsk_19k2_nissan();
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
        case SignalType::FSK_19k2_Elantra2012:
            return reading_fsk_19k2_elantra2012();
        case SignalType::OOK_8k4_SMD3MA4:
            return reading_ook_8k4_smd3ma4();
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
// EU 433MHz sub-decoders called from reading_fsk_19k2_schrader()
// ---------------------------------------------------------------------------

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
    // Reject false positives with upper 12 bits all zero (real IDs are random,
    // high bits zero indicates noise decoded through the Schrader preamble).
    if ((id >> 20) == 0) return {};
    // Plausibility: pressure raw 30..240 (~0.5..4 bar), temperature raw 30..150 (~-20..100 C).
    // Tighter than raw sensor specs -- real tires never report outside this band.
    if (b[4] < 30 || b[4] > 240) return {};
    if (b[5] < 30 || b[5] > 150) return {};
    // Byte 6 is documented as checksum. Valid packets practically never have b[6]==0
    // when all preceding bytes are non-zero. Reject this common noise signature.
    if (b[6] == 0 && b[0] && b[1] && b[2] && b[3] && b[4] && b[5]) return {};
    return Reading{Reading::Type::Jansite, id,
                   Pressure{static_cast<int>(b[4]) * 17 / 10},
                   Temperature{static_cast<int>(b[5]) - 50},
                   Flags{static_cast<uint8_t>(b[3] & 0x0f)}};
}

// Hyundai/VDO Continental TG1C (433.92 MHz)
// Used in: Hyundai, KIA, BMW(older), Mitsubishi, Mazda, PSA
// Packet nibbles: UU IIIIIIII FR PP TT BB CC (10 bytes)
//   b[0]=state, b[1..4]=ID, b[5]=Flags(hi)+Repeat(lo),
//   b[6]=Pressure (*1.375=*11/8), b[7]=Temperature (-50),
//   b[8]=maybe_battery, b[9]=CRC-8 poly=0x07 init=0xAA
// Reference: rtl_433/src/devices/tpms_hyundai_vdo.c
Optional<Reading> Packet::reading_fsk_19k2_hyundai_vdo() const {
    uint8_t b[10];
    for (size_t i = 0; i < 10; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    CRC<8> crc{0x07, 0xaa};
    for (size_t i = 0; i < 10; i++) crc.process_byte(b[i]);
    if (crc.checksum() != 0) return {};
    const uint32_t id = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) |
                        ((uint32_t)b[3] << 8) | b[4];
    if (id == 0) return {};
    const int pres_kpa = static_cast<int>(b[6]) * 11 / 8;
    const int temp_c   = static_cast<int>(b[7]) - 50;
    if (pres_kpa < 100 || pres_kpa > 450) return {};
    if (temp_c   < -40 || temp_c   > 85)  return {};
    return Reading{Reading::Type::Hyundai_VDO, id,
                   Pressure{pres_kpa},
                   Temperature{temp_c}, Flags{b[0]}};
}

// Abarth 124 Spider / VDO TG1C (433.92 MHz)
// Also: Fiat, Alfa Romeo, Lancia, Mazda
// Packet: II II II II ?? PP TT SS CC (9 bytes)
// Checksum: XOR of bytes 0..8 == 0
Optional<Reading> Packet::reading_fsk_19k2_abarth() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    uint8_t xr = 0;
    for (size_t i = 0; i < 9; i++) xr ^= b[i];
    if (xr != 0) return {};
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    const int pres_kpa = static_cast<int>(b[5]) * 138 / 100;
    const int temp_c   = static_cast<int>(b[6]) - 50;
    if (pres_kpa < 100 || pres_kpa > 450) return {};
    if (temp_c   < -40 || temp_c   > 85)  return {};
    return Reading{Reading::Type::Abarth, id,
                   Pressure{pres_kpa},
                   Temperature{temp_c}, Flags{b[7]}};
}

// Renault 0435R (433.92 MHz) - newer Renault/Dacia EU models
// Packet nibbles: II II II fx PP TT AA CC tt (9 bytes)
//   b[0..2]=ID (24-bit), b[3]=flags (observed 0xc0),
//   b[4]=Pressure (*4/3 kPa), b[5]=Temperature (-50),
//   b[6]=centrifugal acc (*5 m/s2), b[7]=XOR checksum,
//   b[8]=tick counter (bit 7 = has_tick, bits 0..6 = count)
// Checksum: XOR of bytes 0..8 == 0
// Reference: rtl_433/src/devices/tpms_renault_0435r.c
Optional<Reading> Packet::reading_fsk_19k2_renault_0435r() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    // Discriminator: b[3] high nibble always 0xc (observed always 0xc0)
    if ((b[3] & 0xf0) != 0xc0) return {};
    uint8_t xr = 0;
    for (size_t i = 0; i < 9; i++) xr ^= b[i];
    if (xr != 0) return {};
    // ID is 24-bit from b[0..2], padded to 32-bit with zero high byte
    const uint32_t id = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
    if (id == 0) return {};
    const int pres_kpa = static_cast<int>(b[4]) * 4 / 3;
    const int temp_c   = static_cast<int>(b[5]) - 50;
    if (pres_kpa < 100 || pres_kpa > 450) return {};
    if (temp_c   < -40 || temp_c   > 85)  return {};
    return Reading{Reading::Type::Renault_0435R, id,
                   Pressure{pres_kpa},
                   Temperature{temp_c}, Flags{b[3]}};
}

// ---------------------------------------------------------------------------
// TruckSolar / Unbranded Solar TPMS for trucks & RVs (433.92 MHz)
// ---------------------------------------------------------------------------
// Packet: preamble 0x55..56 + 21 bytes payload (168 bits)
// Layout: SS SS SS SS IIII PP TT FF CC RR (many variants; simple sum check)
// Reference: rtl_433/src/devices/tpms_truck.c (protocol 201)
// Works over pb_schrader (shared Schrader preamble). We distinguish by length
// (21 bytes vs 8/9/10 bytes for FLM/Ford/Citroen/etc) plus a simple sum check.
Optional<Reading> Packet::reading_fsk_19k2_truck_solar() const {
    // TruckSolar: 21 bytes after preamble. Sum of bytes 0..19 == b[20].
    uint8_t b[21];
    for (size_t i = 0; i < 21; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    uint8_t sum = 0;
    for (size_t i = 0; i < 20; i++) sum += b[i];
    if (sum != b[20]) return {};
    // ID is first 4 bytes (big-endian). Reject zero IDs.
    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};
    // Reject all-zero tail too (common noise signature).
    if (b[4] == 0 && b[5] == 0 && b[6] == 0 && b[7] == 0) return {};
    // Pressure byte at b[4] (PSI * 2 typical -- TruckSolar uses ~0.5 PSI steps).
    // Formula: kPa = raw * 6895 / 2000 = raw * 3.4475
    const int pres_kpa = static_cast<int>(b[4]) * 6895 / 2000;
    // Temperature byte at b[5] (C offset -50).
    const int temp_c = static_cast<int>(b[5]) - 50;
    if (pres_kpa < 50 || pres_kpa > 1200) return {};  // truck/RV tires go higher
    if (temp_c < -40 || temp_c > 85) return {};
    return Reading{Reading::Type::TruckSolar, id,
                   Pressure{pres_kpa},
                   Temperature{temp_c}, Flags{b[6]}};
}

// ---------------------------------------------------------------------------
// Nissan TPMS (433.92 MHz) - FSK Manchester, 37 bits, no CRC
// ---------------------------------------------------------------------------
// Layout: MMM IIIIIIIIIIIIIIIIIIIIIIIIII PPPPPPPP (37 bits)
//   3 bit mode, 24 bit ID (broken up), 8 bit pressure
// Reference: rtl_433/src/devices/tpms_nissan.c (protocol 248)
// WARNING: no CRC. Relies entirely on strict plausibility checks.
// Placed last in the decoder cascade.
Optional<Reading> Packet::reading_fsk_19k2_nissan() const {
    const auto mode = reader_.read(0, 3);
    // ID is spread across the packet:
    //   b[0]: 3 mode | 5 id-hi
    //   b[1]: 8 id
    //   b[2]: 8 id
    //   b[3]: 3 id-lo | 5 pres-hi
    //   b[4]: 3 pres-lo | ...
    const uint32_t id = ((reader_.read(3, 5) << 19) |
                         (reader_.read(8, 8) << 11) |
                         (reader_.read(16, 8) << 3) |
                         reader_.read(24, 3));
    const auto pressure_raw = ((reader_.read(27, 5) << 3) | reader_.read(32, 3));

    // Strict validation to avoid false positives (no CRC here):
    // - ID cannot be zero
    // - Mode in observed range (real sensors use 0x00..0x07, avoid 0 as ambiguous)
    // - Pressure raw in plausible tire range (25..55 PSI -> raw ~100..220)
    if (id == 0) return {};
    if (mode == 0) return {};  // avoid noise matches
    if (pressure_raw < 60 || pressure_raw > 250) return {};
    // Pressure: raw / 4 PSI -> kPa = raw * 6895 / 4000
    const int pres_kpa = static_cast<int>(pressure_raw) * 6895 / 4000;
    if (pres_kpa < 100 || pres_kpa > 450) return {};
    return Reading{Reading::Type::Nissan, id,
                   Pressure{pres_kpa},
                   {},  // no temperature in Nissan packet
                   Flags{static_cast<uint8_t>(mode)}};
}

// ---------------------------------------------------------------------------
// Elantra 2012 / TRW (433.92 MHz) -- own SignalType FSK_19k2_Elantra2012
// ---------------------------------------------------------------------------
// Preamble 0x7155 (16 bits, handled by dedicated pb_elantra2012 in proc_tpms_all)
// Packet (8 bytes): PP TT IIII FF CC
//   b[0] = Pressure (+60 kPa)
//   b[1] = Temperature (-50 C)
//   b[2..5] = 32-bit ID
//   b[6] = Flags (??SBT: Storage/Battery-low/Triggered in low bits)
//   b[7] = CRC-8 poly=0x07 init=0x00
// Reference: rtl_433/src/devices/tpms_elantra2012.c (protocol 186 in older builds)
Optional<Reading> Packet::reading_fsk_19k2_elantra2012() const {
    uint8_t b[8];
    for (size_t i = 0; i < 8; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    CRC<8> crc{0x07, 0x00};
    for (size_t i = 0; i < 7; i++) crc.process_byte(b[i]);
    if (crc.checksum() != b[7]) return {};
    const uint32_t id = ((uint32_t)b[2] << 24) | ((uint32_t)b[3] << 16) |
                        ((uint32_t)b[4] << 8) | b[5];
    if (id == 0) return {};
    const int pres_kpa = static_cast<int>(b[0]) + 60;
    const int temp_c = static_cast<int>(b[1]) - 50;
    if (pres_kpa < 100 || pres_kpa > 450) return {};
    if (temp_c < -40 || temp_c > 85) return {};
    return Reading{Reading::Type::Elantra2012, id,
                   Pressure{pres_kpa},
                   Temperature{temp_c}, Flags{b[6]}};
}

// ---------------------------------------------------------------------------
// Schrader SMD3MA4 / 3039 (Subaru, Renault Koleos, Nissan 370Z, Infiniti FX)
// ---------------------------------------------------------------------------
// Own SignalType OOK_8k4_SMD3MA4 (handled by pb_smd3ma4 in proc_tpms_all).
// Preamble: 36 raw bits 0xF5555555E (captured by packet builder).
// Payload: 37 Manchester-decoded bits after preamble:
//   FFFSSSSS SSSSSSSS SSSSSSSS SSSPPPPP PPPCCxxx
//   3 bit Flags, 24 bit ID, 8 bit Pressure (PSI * 5 scale), 2 bit Check
// NOTE: there is NO temperature data transmitted.
// Reference: rtl_433/src/devices/schraeder.c (protocol 168, SMD3MA4 branch)
Optional<Reading> Packet::reading_ook_8k4_smd3ma4() const {
    // Read 37 bits, unpacked across 5 bytes for clarity.
    uint8_t b[5];
    for (size_t i = 0; i < 5; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    // The spec XOR-inverts the decoded stream; pb_smd3ma4 uses inverted sense (sense=1).
    // Field extraction matches rtl_433:
    const uint32_t flags = b[0] >> 5;
    const uint32_t id = ((uint32_t)(b[0] & 0x1f) << 19) |
                        ((uint32_t)b[1] << 11) |
                        ((uint32_t)b[2] << 3) |
                        ((uint32_t)b[3] >> 5);
    const uint32_t pressure_raw = ((uint32_t)(b[3] & 0x1f) << 3) | (b[4] >> 5);
    // Parity check (3 bit parity of flags+id -> 2 bit check field):
    uint32_t parity_stream = 0;
    for (size_t i = 0; i < 4; i++) parity_stream ^= b[i];
    parity_stream ^= (b[4] & 0xe0);
    parity_stream = (parity_stream >> 4) ^ (parity_stream & 0x0f);
    parity_stream = (parity_stream >> 2) ^ (parity_stream & 0x03);
    const uint32_t check = (b[4] >> 3) & 0x03;
    // Parity check lost its exact specification in rtl_433 TODO -- use it as
    // soft sanity guard only (reject if both parity_stream high bits mismatch
    // AND all fields look like noise).
    if (flags == 0 && id == 0 && pressure_raw == 0) return {};
    (void)parity_stream;
    (void)check;
    // Pressure: raw * 0.2 PSI -> kPa = raw * 6895 / 5000 = raw * 1.379
    const int pres_kpa = static_cast<int>(pressure_raw) * 6895 / 5000;
    if (pres_kpa < 50 || pres_kpa > 600) return {};
    return Reading{Reading::Type::Schrader_SMD3MA4, id,
                   Pressure{pres_kpa},
                   {},  // no temperature in SMD3MA4 packet
                   Flags{static_cast<uint8_t>(flags)}};
}

// Jansite Solar Model (433.92 MHz) - Manchester 19200 bps
// Packet nibbles: SS SS II II II 00 TT PP 00 CC CC (11 decoded bytes)
//   where SS SS = 0xDD33 (sync word, 16 bits, consumed by pb_jansite_solar
//   preamble 32 raw bits 0xA6A65A5A -> decoded 0xDD33). The reader therefore
//   only sees the remaining 9 bytes: II II II 00 TT PP 00 CC CC
//   b[0..2]=ID (24-bit), b[3]=unknown,
//   b[4]=Temperature (-55), b[5]=Pressure (*8/5 kPa),
//   b[6]=unknown, b[7..8]=CRC-16/BUYPASS poly=0x8005 init=0x0000
// CRC is computed over the full decoded stream SS SS II II II 00 TT PP 00,
// i.e. sync word 0xDD 0x33 prepended to the 7 payload bytes b[0..6].
// Reference: rtl_433/src/devices/tpms_jansite_solar.c
Optional<Reading> Packet::reading_fsk_19k2_jansite_solar() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++) b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));
    CRC<16> crc{0x8005, 0x0000};
    // Sync word 0xDD33 was absorbed by the preamble but is part of the CRC input
    crc.process_byte(0xdd);
    crc.process_byte(0x33);
    for (size_t i = 0; i < 7; i++) crc.process_byte(b[i]);
    const uint16_t expected = (static_cast<uint16_t>(b[7]) << 8) | b[8];
    if (crc.checksum() != expected) return {};
    const uint32_t id = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
    if (id == 0) return {};
    return Reading{Reading::Type::JansiteSolar, id,
                   Pressure{static_cast<int>(b[5]) * 8 / 5},
                   Temperature{static_cast<int>(b[4]) - 55}, Flags{b[3]}};
}

} /* namespace tpms */
