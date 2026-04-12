/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04 (EU extensions Phase 1 + Phase 2)
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "tpms_packet.hpp"

#include "crc.hpp"

#include <array>

namespace tpms {

Timestamp Packet::received_at() const {
    return packet_.timestamp();
}

FormattedSymbols Packet::symbols_formatted() const {
    return format_symbols(decoder_);
}

// ---------------------------------------------------------------------------
// NRZI helper
//
// Decodes n_bits of NRZI (Differential Manchester at raw bit rate) from
// packet_[] into bytes[].
//
//   NRZI convention: no transition = 1, transition = 0
//   (Standard Biphase-M / rtl_433 bitbuffer_differential_manchester_decode)
//
// prev_bit = last bit of the preamble pattern, used as reference for decoding
// the FIRST data bit. Set to the last bit of the matched preamble:
//   BMW Gen2/3 preamble 0xCCCD: last bit = 1
//   Porsche preamble 0x333320 (20 bits): last bit = 0
//
// Returns the number of bits actually decoded (may be < n_bits if packet ends).
// ---------------------------------------------------------------------------
size_t Packet::nrzi_decode(uint8_t* bytes, size_t n_bits, uint_fast8_t prev_bit) const {
    const size_t available = packet_.size();
    const size_t bits = (n_bits < available) ? n_bits : available;

    size_t bytes_needed = (n_bits + 7) / 8;
    for (size_t i = 0; i < bytes_needed; i++) bytes[i] = 0;

    for (size_t i = 0; i < bits; i++) {
        const uint_fast8_t cur = packet_[i];
        // No transition from prev → decoded 1; transition → decoded 0
        const uint_fast8_t decoded = (cur == prev_bit) ? 1 : 0;
        bytes[i / 8] = static_cast<uint8_t>((bytes[i / 8] << 1) | decoded);
        prev_bit = cur;
    }

    return bits;
}

// ===========================================================================
// FSK 19k2 multi-decoder (Phase 1 original + EU Phase 1 extensions)
//
// All these protocols share:
//   - FSK, 19200 bps, deviation ≈ 38400 Hz
//   - Preamble: 0x55 0x55 0x56 (captured as FSK_19k2_Schrader signal type)
//   - Captured payload: 160 raw bits → 80 Manchester-decoded bits (10 bytes)
//
// Order of attempts: FLM → Ford → Citroën/PSA → Renault
// ===========================================================================

Optional<Reading> Packet::reading_fsk_19k2_schrader() const {
    // 1. FLM (Continental/VDO/Freescale) — CRC-8 poly=0x01 init=0x00
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

    // 2–4. EU extensions
    auto r = reading_fsk_19k2_ford();
    if (r.is_valid()) return r;

    r = reading_fsk_19k2_citroen();
    if (r.is_valid()) return r;

    r = reading_fsk_19k2_renault();
    if (r.is_valid()) return r;

    return {};
}

// ---------------------------------------------------------------------------
// Ford TPMS — VDO/Continental S180084730Z
// Ford Fiesta, Focus, Kuga, Escape, Transit — 315.0 + 433.92 MHz
//
// Packet layout (8 decoded bytes after Manchester):
//   II II II II PP TT FF CC
//
//   I[0..3] = 32-bit sensor ID
//   P[4]    = pressure bits [7:0]
//   T[5]    = temperature: valid only if bit 7 == 0; (T & 0x7F) - 56 °C
//   F[6]    = flags (bit6=moving, bit5=pressure bit8, bit3=learn, bit2=normal)
//   C[7]    = checksum = SUM(b[0..6]) & 0xFF
//
// Pressure: psi_raw = ((F & 0x20) << 3) | P
//           kPa = psi_raw * 172 / 100   (≈ PSI/4 × 6.895)
//
// Reference: rtl_433/src/devices/tpms_ford.c
// ---------------------------------------------------------------------------
Optional<Reading> Packet::reading_fsk_19k2_ford() const {
    uint8_t b[8];
    for (size_t i = 0; i < 8; i++)
        b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));

    // Checksum: SUM(b[0..6]) == b[7]
    uint8_t sum = 0;
    for (size_t i = 0; i < 7; i++) sum += b[i];
    if (sum != b[7]) return {};

    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};

    // 9-bit pressure raw value (bit5 of flags = MSB)
    const int psi_raw = static_cast<int>(((b[6] & 0x20) << 3) | b[4]);
    const int pressure_kpa = psi_raw * 172 / 100;

    Optional<Temperature> temp{};
    if ((b[5] & 0x80) == 0) {
        temp = Temperature{static_cast<int>(b[5] & 0x7f) - 56};
    }

    return Reading{Reading::Type::Ford, id,
                   Pressure{pressure_kpa}, temp, Flags{b[6]}};
}

// ---------------------------------------------------------------------------
// Citroën/PSA TPMS — PSA Group
// Citroën, Peugeot, Fiat, Mitsubishi, VDO-types — 433.92 MHz
//
// Packet layout (10 decoded bytes after Manchester):
//   SS  II II II II  FR  PP TT BB  --
//
//   S[0]    = state byte (NOT covered by checksum)
//   I[1..4] = 32-bit sensor ID
//   F[5]>>4 = flags nibble; R[5]&0xF = repeat counter
//   P[6]    = pressure (b[6] ≠ 0); kPa = b[6] × 1.364 ≈ b[6] × 341/250
//   T[7]    = temperature (b[7] ≠ 0); °C = b[7] - 50
//   B[8]    = battery indicator
//   [9]     = implicit — checksum: XOR(b[1..9]) == 0
//
// Reference: rtl_433/src/devices/tpms_citroen.c
// ---------------------------------------------------------------------------
Optional<Reading> Packet::reading_fsk_19k2_citroen() const {
    uint8_t b[10];
    for (size_t i = 0; i < 10; i++)
        b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));

    if (b[6] == 0 || b[7] == 0) return {};

    uint8_t crc = 0;
    for (size_t i = 1; i < 10; i++) crc ^= b[i];
    if (crc != 0) return {};

    const uint32_t id = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) |
                        ((uint32_t)b[3] << 8) | b[4];
    if (id == 0) return {};

    const int pressure_kpa = static_cast<int>(b[6]) * 341 / 250;  // × 1.364
    const int temp_c = static_cast<int>(b[7]) - 50;

    return Reading{Reading::Type::Citroen_PSA, id,
                   Pressure{pressure_kpa}, Temperature{temp_c}, Flags{b[5]}};
}

// ---------------------------------------------------------------------------
// Renault TPMS — Renault Clio/Captur/Zoe, Dacia Sandero — 433.92 MHz
//
// Packet layout (9 decoded bytes after Manchester):
//   FF FF PP TT II II II ?? ?? CC
//
//   [0]    = flags + pressure MSB: bits[7:2]=flags, bits[1:0]=pressure[9:8]
//   [1]    = pressure LSB; kPa = ((b[0]&0x03)<<8 | b[1]) × 3/4  (0.75 kPa/step)
//   [2]    = temperature; °C = b[2] - 30
//   [3..5] = 24-bit sensor ID (little-endian: b[5]<<16 | b[4]<<8 | b[3])
//   [6..7] = unknown (often 0xFFFF; pressure-change indicator in Zoe)
//   [8]    = CRC-8, poly=0x07, init=0x00, over b[0..7]
//
// Reference: rtl_433/src/devices/tpms_renault.c
// ---------------------------------------------------------------------------
Optional<Reading> Packet::reading_fsk_19k2_renault() const {
    uint8_t b[9];
    for (size_t i = 0; i < 9; i++)
        b[i] = static_cast<uint8_t>(reader_.read(i * 8, 8));

    CRC<8> crc_check{0x07, 0x00};
    for (size_t i = 0; i < 8; i++) crc_check.process_byte(b[i]);
    if (crc_check.checksum() != b[8]) return {};

    const uint32_t id = ((uint32_t)b[5] << 16) | ((uint32_t)b[4] << 8) | b[3];
    if (id == 0) return {};

    const int pressure_raw = (static_cast<int>(b[0] & 0x03) << 8) | b[1];
    const int pressure_kpa = pressure_raw * 3 / 4;
    const int temp_c = static_cast<int>(b[2]) - 30;
    const Flags flags = b[0] >> 2;

    return Reading{Reading::Type::Renault, id,
                   Pressure{pressure_kpa}, Temperature{temp_c}, flags};
}

// ===========================================================================
// Phase 2: BMW Gen4/5 — inverted Manchester, ~40 kbps
//
// BMW Gen4/5, Audi Pressure Alert, HUF/Beru Gen5, Continental, Schrader/Sensata
// 433.92 MHz — FSK PCM, 25µs symbols (~40000 bps raw), Manchester encoded
//
// Important: rtl_433 INVERTS the Manchester-decoded bytes before field extraction.
// In Mayhem this is handled by using ManchesterDecoder with sense=1 (decoder_inv_),
// which reads the SECOND bit of each Manchester pair and thus gives the complement
// of the default sense=0 convention. This matches rtl_433's post-invert output.
//
// Packet layout after inverted Manchester decode:
//   BMW (11 bytes): MM II II II II PP TT F1 F2 F3 CC
//   Audi alert (8 bytes): MM II II II II PP TT CC
//
//   MM    = brand ID (0x03=HUF/Beru, 0x23=Schrader/Sensata, 0x80=Continental,
//                     0x00=Audi Pressure Alert, 0x88=Audi)
//   II    = 32-bit sensor ID
//   PP    = pressure × 2.45 kPa  (0..255 → 0..624.75 kPa)
//   TT    = temperature - 52 °C
//   F1,F2,F3 = warning flags (BMW only; F3 = nominal pressure × 0.0245 for HUF)
//   CC    = CRC-8, poly=0x2F, init=0xAA, over all bytes including CC (residue=0)
//
// Preamble (M4 side): 0xAA59 (16 raw bits), captured payload: 176 raw bits
// M4 signal: FSK_38k4_BMW_G45
//
// Reference: rtl_433/src/devices/tpms_bmw.c (Bruno OCTAU / ProfBoc75)
// ===========================================================================
Optional<Reading> Packet::reading_fsk_38k4_bmw_g45() const {
    // Read up to 11 decoded bytes using inverted Manchester (sense=1)
    uint8_t b[11];
    for (size_t i = 0; i < 11; i++)
        b[i] = static_cast<uint8_t>(reader_inv_.read(i * 8, 8));

    // Determine packet length: try BMW (11 bytes) then Audi (8 bytes)
    // CRC-8 poly=0x2F init=0xAA residue check: CRC over all N bytes == 0
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

    // PP × 2.45 kPa — integer: b[5] * 245 / 100
    const int pressure_kpa = static_cast<int>(b[5]) * 245 / 100;

    // TT - 52 °C
    const int temp_c = static_cast<int>(b[6]) - 52;

    // Store brand_id in flags (identifies sensor manufacturer)
    // 0x03=HUF/Beru Gen5, 0x23=Schrader/Sensata, 0x80=Continental,
    // 0x00=Audi Pressure Alert, 0x88=Audi
    const Flags flags = b[0];

    return Reading{Reading::Type::BMW_G45, id,
                   Pressure{pressure_kpa}, Temperature{temp_c}, flags};
}

// ===========================================================================
// Phase 2: BMW Gen2/Gen3 — NRZI encoding, 19200 bps
//
// BMW Gen2 (9+1 bytes) and Gen3 (10+1 bytes) TPMS sensors.
// 433.92 MHz — FSK PCM, 52µs symbols (19200 bps), NRZI encoded.
//
// NOTE: "Differential Manchester" in rtl_433 = NRZI (1 raw bit → 1 decoded bit
// via transition comparison, no Manchester pair doubling).
// NRZI: no transition from previous bit = 1; transition = 0.
//
// Preamble (M4 side): 0xCCCD (16 raw bits); last preamble bit = 1.
// M4 captures 90 raw NRZI bits after preamble (enough for 11 decoded bytes).
//
// Packet layout after NRZI decode:
//   Gen3 (11 bytes): II II II II PP TT F1 F2 F3 CK CK
//   Gen2 (10 bytes): II II II II PP TT F1 F2 CK CK
//
//   I[0..3] = 32-bit sensor ID
//   PP      = pressure: kPa = (PP - 43) × 2.5
//   TT      = temperature: °C = TT - 40
//   F1,F2   = flags (battery, pressure warning?)
//   F3      = additional flags (Gen3 only)
//   CK CK   = CRC-16, poly=0x1021, init=0x0000, over all bytes incl CK (residue=0)
//
// Gen2 vs Gen3 is distinguished by CRC check at 10 vs 11 bytes.
//
// Reference: rtl_433/src/devices/tpms_bmw_g3.c (Bruno OCTAU / @Billymazze)
// ===========================================================================
Optional<Reading> Packet::reading_fsk_19k2_bmw_g23() const {
    // NRZI decode — preamble 0xCCCD: last bit = 1
    uint8_t b[11]{};
    const size_t decoded_bits = nrzi_decode(b, 11 * 8, /*prev_bit=*/1);

    if (decoded_bits < 10 * 8) return {};  // Need at least 10 bytes

    // Try Gen3 (11 bytes) first, then Gen2 (10 bytes)
    uint8_t len = 0;

    if (decoded_bits >= 11 * 8) {
        CRC<16> crc{0x1021, 0x0000};
        for (size_t i = 0; i < 11; i++) crc.process_byte(b[i]);
        if (crc.checksum() == 0) len = 11;
    }

    if (len == 0) {
        CRC<16> crc{0x1021, 0x0000};
        for (size_t i = 0; i < 10; i++) crc.process_byte(b[i]);
        if (crc.checksum() == 0) len = 10;
    }

    if (len == 0) return {};

    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};

    // PP: kPa = (PP - 43) × 2.5  → integer: (b[4] - 43) * 5 / 2
    const int pressure_kpa = (static_cast<int>(b[4]) - 43) * 5 / 2;

    // TT: °C = TT - 40
    const int temp_c = static_cast<int>(b[5]) - 40;

    // F1 (b[6]) — battery/pressure flags; use as primary flags byte
    const Flags flags = b[6];

    return Reading{Reading::Type::BMW_G23, id,
                   Pressure{pressure_kpa}, Temperature{temp_c}, flags};
}

// ===========================================================================
// Phase 2: Porsche Boxster/Cayman Typ 987 — NRZI encoding, 19200 bps
//
// 433.92 MHz — FSK PCM, 52µs symbols (19200 bps), NRZI encoded.
// Observed on 2nd-gen (Typ 987) Porsche Boxster and Cayman.
//
// NOTE: Same NRZI encoding as BMW Gen2/3 (rtl_433 bitbuffer_differential_manchester_decode).
//
// Preamble (M4 side): 0x33 0x33 0x20 top 20 bits = 0b00110011001100110010
//                     last preamble bit = 0.
// M4 captures 80 raw NRZI bits after preamble (10 decoded bytes).
//
// Packet layout after NRZI decode (10 bytes):
//   II II II II PP TT SS SS CK CK
//
//   I[0..3] = 32-bit sensor ID
//   PP      = pressure: kPa = PP × 2.5 - 100  (scale=2.5, offset=100; min seen=41=0kPa)
//   TT      = temperature: °C = TT - 40
//   SS SS   = status word (b[6]<<8 | b[7])
//   CK CK   = CRC-16, poly=0x1021, init=0xFFFF, over all 10 bytes (residue=0)
//
// Reference: rtl_433/src/devices/tpms_porsche.c (Christian W. Zuckschwerdt)
// ===========================================================================
Optional<Reading> Packet::reading_fsk_19k2_porsche() const {
    // NRZI decode — preamble 0x333320 (20 bits): last bit = 0
    uint8_t b[10]{};
    const size_t decoded_bits = nrzi_decode(b, 10 * 8, /*prev_bit=*/0);

    if (decoded_bits < 10 * 8) return {};

    // CRC-16 poly=0x1021 init=0xFFFF residue check
    {
        CRC<16> crc{0x1021, 0xffff};
        for (size_t i = 0; i < 10; i++) crc.process_byte(b[i]);
        if (crc.checksum() != 0) return {};
    }

    const uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                        ((uint32_t)b[2] << 8) | b[3];
    if (id == 0) return {};

    // PP × 2.5 - 100 kPa; min valid PP = 41 (= 0 kPa)
    const int pressure_kpa = static_cast<int>(b[4]) * 5 / 2 - 100;

    // TT - 40 °C
    const int temp_c = static_cast<int>(b[5]) - 40;

    // Status word (high byte) as flags
    const Flags flags = b[6];

    return Reading{Reading::Type::Porsche, id,
                   Pressure{(pressure_kpa > 0) ? pressure_kpa : 0}, Temperature{temp_c}, flags};
}

// ===========================================================================
// OOK decoders (unchanged from original)
// ===========================================================================

Optional<Reading> Packet::reading_ook_8k192_schrader() const {
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
    constexpr uint8_t first_nibble = 0x4;
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

// ===========================================================================
// reading() — dispatch by signal type
// ===========================================================================
Optional<Reading> Packet::reading() const {
    switch (signal_type()) {
        case SignalType::FSK_19k2_Schrader:
            return reading_fsk_19k2_schrader();
        case SignalType::OOK_8k192_Schrader:
            return reading_ook_8k192_schrader();
        case SignalType::OOK_8k4_Schrader:
            return reading_ook_8k4_schrader();
        // Phase 2 — proc_tpms_eu only:
        case SignalType::FSK_38k4_BMW_G45:
            return reading_fsk_38k4_bmw_g45();
        case SignalType::FSK_19k2_BMW_G23:
            return reading_fsk_19k2_bmw_g23();
        case SignalType::FSK_19k2_Porsche:
            return reading_fsk_19k2_porsche();
        default:
            return {};
    }
}

// ===========================================================================
// crc_valid_length() — FLM/Schrader CRC helper (unchanged)
// ===========================================================================
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

        if (checksum_bytes & byte_mask) checksum += byte;
        if (crc_72_bytes & byte_mask) crc_72.process_byte(byte);
        if (crc_80_bytes & byte_mask) crc_80.process_byte(byte);
    }

    if (crc_80.checksum() == 0) return 80;
    else if (crc_72.checksum() == 0) return 72;
    else if ((checksum & 0xff) == bytes[7]) return 64;
    else return 0;
}

} /* namespace tpms */
