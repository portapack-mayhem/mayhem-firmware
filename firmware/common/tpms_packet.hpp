/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04 (EU extensions: Ford, Citroen/PSA, Renault,
 *                                  BMW Gen4/5, BMW Gen2/3, Porsche)
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __TPMS_PACKET_H__
#define __TPMS_PACKET_H__

#include <cstdint>
#include <cstddef>

#include "optional.hpp"

#include "units.hpp"
using units::Pressure;
using units::Temperature;

#include "baseband_packet.hpp"
#include "manchester.hpp"
#include "field_reader.hpp"

namespace tpms {

using Flags = uint8_t;

// ---------------------------------------------------------------------------
// SignalType — identifies the M4 demodulation path that produced the packet.
//
// Phase 1 (original):
//   FSK_19k2_Schrader  — 19200 bps FSK, std Manchester, preamble 0x55 0x56
//   OOK_8k192_Schrader — OOK 8192 bps
//   OOK_8k4_Schrader   — OOK 8400 bps
//
// Phase 2 (proc_tpms_eu only):
//   FSK_38k4_BMW_G45   — ~40000 bps FSK, inverted Manchester, preamble 0xAA59
//                        (BMW Gen4/5, Audi Pressure Alert, HUF/Beru, Continental,
//                         Schrader/Sensata variants)
//   FSK_19k2_BMW_G23   — 19200 bps FSK, NRZI (Differential Manchester raw rate),
//                        preamble 0xCCCD (BMW Gen2/3)
//   FSK_19k2_Porsche   — 19200 bps FSK, NRZI, preamble 0x333320 (20 bits)
//                        (Porsche 987 Boxster/Cayman Typ 987)
// ---------------------------------------------------------------------------
enum SignalType {
    FSK_19k2_Schrader = 1,
    OOK_8k192_Schrader = 2,
    OOK_8k4_Schrader = 3,
    FSK_38k4_BMW_G45 = 4,  // proc_tpms_eu only
    FSK_19k2_BMW_G23 = 5,  // proc_tpms_eu only
    FSK_19k2_Porsche = 6,  // proc_tpms_eu only
};

class TransponderID {
   public:
    constexpr TransponderID()
        : id_{0} {
    }

    constexpr TransponderID(
        const uint32_t id)
        : id_{id} {
    }

    constexpr uint32_t value() const {
        return id_;
    }

    constexpr bool operator==(const TransponderID& other) const {
        return id_ == other.id_;
    }

   private:
    uint32_t id_;
};

class Reading {
   public:
    enum Type {
        // --- Phase 1 (original) ---
        None = 0,
        FLM_64 = 1,      // Continental/VDO/Freescale — 64-bit variant
        FLM_72 = 2,       // Continental/VDO/Freescale — 72-bit variant
        FLM_80 = 3,       // Continental/VDO/Freescale — 80-bit variant
        Schrader = 4,     // Schrader OOK 8192 bps
        GMC_96 = 5,       // Schrader GMC variant OOK 8400 bps

        // --- Phase 1 EU extensions (same M4 path as Schrader/FLM) ---
        // All use FSK_19k2_Schrader signal type, same preamble 0x55 0x56
        Ford = 6,        // VDO/Continental S180084730Z — 315.0 + 433.92 MHz
                         // Ford Fiesta/Focus/Kuga/Escape/Transit
        Citroen_PSA = 7, // PSA Group — Citroën, Peugeot, Fiat, Mitsubishi — 433.92 MHz
        Renault = 8,     // Renault Clio/Captur/Zoe, Dacia — 433.92 MHz

        // --- Phase 2 EU extensions (proc_tpms_eu M4 required) ---
        BMW_G45 = 9,     // BMW Gen4/5, Audi Pressure Alert, HUF/Beru, Continental/Schrader
                         // FSK_38k4_BMW_G45 signal — 433.92 MHz
                         // brand_id in flags: 0x03=HUF/Beru, 0x23=Schrader, 0x80=Continental,
                         //                    0x00/0x88=Audi Pressure Alert
        BMW_G23 = 10,    // BMW Gen2 and Gen3 — FSK_19k2_BMW_G23 signal — 433.92 MHz
        Porsche = 11,    // Porsche 987 Boxster/Cayman Typ 987 — FSK_19k2_Porsche — 433.92 MHz
    };

    constexpr Reading()
        : type_{Type::None} {
    }

    constexpr Reading(
        Type type,
        TransponderID id)
        : type_{type},
          id_{id} {
    }

    constexpr Reading(
        Type type,
        TransponderID id,
        Optional<Pressure> pressure = {},
        Optional<Temperature> temperature = {},
        Optional<Flags> flags = {})
        : type_{type},
          id_{id},
          pressure_{pressure},
          temperature_{temperature},
          flags_{flags} {
    }

    Type type() const { return type_; }
    TransponderID id() const { return id_; }
    Optional<Pressure> pressure() const { return pressure_; }
    Optional<Temperature> temperature() const { return temperature_; }
    Optional<Flags> flags() const { return flags_; }

   private:
    Type type_{Type::None};
    TransponderID id_{0};
    Optional<Pressure> pressure_{};
    Optional<Temperature> temperature_{};
    Optional<Flags> flags_{};
};

class Packet {
   public:
    constexpr Packet(
        const baseband::Packet& packet,
        const SignalType signal_type)
        : packet_{packet},
          signal_type_{signal_type},
          decoder_{packet_, 0},      // Standard Manchester (sense=0)
          decoder_inv_{packet_, 1},  // Inverted Manchester (sense=1) — BMW Gen4/5
          reader_{decoder_},
          reader_inv_{decoder_inv_} {
    }

    SignalType signal_type() const { return signal_type_; }
    Timestamp received_at() const;

    FormattedSymbols symbols_formatted() const;

    Optional<Reading> reading() const;

   private:
    using Reader    = FieldReader<ManchesterDecoder, BitRemapNone>;
    using ReaderInv = FieldReader<ManchesterDecoder, BitRemapNone>;

    const baseband::Packet  packet_;
    const SignalType        signal_type_;
    const ManchesterDecoder decoder_;      // sense=0: FLM, Schrader, Ford, Citroen, Renault
    const ManchesterDecoder decoder_inv_;  // sense=1: BMW Gen4/5 (inverted convention)
    const Reader            reader_;
    const ReaderInv         reader_inv_;

    // --- Phase 1 decoders (FSK_19k2_Schrader signal path) ---
    Optional<Reading> reading_fsk_19k2_schrader() const;
    Optional<Reading> reading_ook_8k192_schrader() const;
    Optional<Reading> reading_ook_8k4_schrader() const;

    // Phase 1 EU sub-decoders (called from reading_fsk_19k2_schrader):
    Optional<Reading> reading_fsk_19k2_ford() const;
    Optional<Reading> reading_fsk_19k2_citroen() const;
    Optional<Reading> reading_fsk_19k2_renault() const;

    // --- Phase 2 decoders (proc_tpms_eu signal paths) ---
    Optional<Reading> reading_fsk_38k4_bmw_g45() const;  // inverted Manchester
    Optional<Reading> reading_fsk_19k2_bmw_g23() const;  // NRZI raw decode
    Optional<Reading> reading_fsk_19k2_porsche() const;  // NRZI raw decode

    // Helpers
    size_t crc_valid_length() const;

    // NRZI (Differential Manchester) decoder helper
    // Decodes `n_bits` from packet_ into bytes[], using prev_bit as initial reference.
    // prev_bit = last bit of preamble (ensures correct decode of first data bit).
    // Returns number of bits decoded (may be < n_bits if packet is too short).
    size_t nrzi_decode(uint8_t* bytes, size_t n_bits, uint_fast8_t prev_bit) const;
};

} /* namespace tpms */

#endif /*__TPMS_PACKET_H__*/
