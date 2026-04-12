/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04 (EU Phase 1+2, World Phase 3)
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
// SignalType — M4 demodulation path that produced the packet.
//
// Phase 1 (original — proc_tpms PTPM):
//   FSK_19k2_Schrader, OOK_8k192_Schrader, OOK_8k4_Schrader
//
// Phase 2 EU (proc_tpms_eu PTPE):
//   FSK_38k4_BMW_G45  — ~40kbps Manchester inverted, preamble 0xAA59
//   FSK_19k2_BMW_G23  — 19200 bps NRZI, preamble 0xCCCD
//   FSK_19k2_Porsche  — 19200 bps NRZI, preamble 0x333320 (20 bits)
//
// Phase 3 World (proc_tpms_world PTPW):
//   FSK_19k2_Toyota        — 19200 bps NRZI, preamble 0xa9e0 (12 bits)
//   FSK_19k2_Elantra       — ~20000 bps std Manchester, preamble 0x7155 (16 bits)
//   FSK_19k2_JansiteSolar  — 19200 bps inv Manchester, preamble 0xa6a65a (24 bits)
// ---------------------------------------------------------------------------
enum SignalType {
    // Phase 1 — original
    FSK_19k2_Schrader = 1,
    OOK_8k192_Schrader = 2,
    OOK_8k4_Schrader = 3,
    // Phase 2 — EU (proc_tpms_eu)
    FSK_38k4_BMW_G45 = 4,
    FSK_19k2_BMW_G23 = 5,
    FSK_19k2_Porsche = 6,
    // Phase 3 — World (proc_tpms_world)
    FSK_19k2_Toyota = 7,
    FSK_19k2_Elantra = 8,
    FSK_19k2_JansiteSolar = 9,
};

class TransponderID {
   public:
    constexpr TransponderID() : id_{0} {}
    constexpr TransponderID(const uint32_t id) : id_{id} {}
    constexpr uint32_t value() const { return id_; }
    constexpr bool operator==(const TransponderID& other) const { return id_ == other.id_; }

   private:
    uint32_t id_;
};

class Reading {
   public:
    enum Type {
        // Phase 1 — original
        None = 0,
        FLM_64 = 1,
        FLM_72 = 2,
        FLM_80 = 3,
        Schrader = 4,
        GMC_96 = 5,
        // Phase 1 EU extensions (FSK_19k2_Schrader path)
        Ford = 6,        // VDO/Continental S180084730Z — 315 + 433.92 MHz
        Citroen_PSA = 7, // PSA Group — 433.92 MHz
        Renault = 8,     // Renault/Dacia — 433.92 MHz
        // Phase 2 EU (proc_tpms_eu)
        BMW_G45 = 9,     // BMW Gen4/5, Audi, HUF, Continental, Schrader/Sensata
        BMW_G23 = 10,    // BMW Gen2/3
        Porsche = 11,    // Porsche 987 Boxster/Cayman
        // Phase 3 World (proc_tpms_world + FSK_19k2_Schrader path)
        Toyota = 12,     // Toyota PMV-C210 — 315 MHz NRZI (Pacific Industries)
        Elantra = 13,    // Hyundai Elantra / Honda Civic TRW GQ4-44T — 315 MHz
        Jansite = 14,    // Jansite TY02S — 315 + 433.92 MHz (no CRC)
        SolarTruck = 15, // Unbranded Solar TPMS for trucks — 433.92 MHz
        JansiteSolar = 16, // Jansite Solar Model — 433.92 MHz
    };

    constexpr Reading() : type_{Type::None} {}
    constexpr Reading(Type type, TransponderID id) : type_{type}, id_{id} {}
    constexpr Reading(
        Type type, TransponderID id,
        Optional<Pressure> pressure = {},
        Optional<Temperature> temperature = {},
        Optional<Flags> flags = {})
        : type_{type}, id_{id}, pressure_{pressure},
          temperature_{temperature}, flags_{flags} {}

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
    constexpr Packet(const baseband::Packet& packet, const SignalType signal_type)
        : packet_{packet},
          signal_type_{signal_type},
          decoder_{packet_, 0},
          decoder_inv_{packet_, 1},
          reader_{decoder_},
          reader_inv_{decoder_inv_} {}

    SignalType signal_type() const { return signal_type_; }
    Timestamp received_at() const;
    FormattedSymbols symbols_formatted() const;
    Optional<Reading> reading() const;

   private:
    using Reader    = FieldReader<ManchesterDecoder, BitRemapNone>;
    using ReaderInv = FieldReader<ManchesterDecoder, BitRemapNone>;

    const baseband::Packet  packet_;
    const SignalType        signal_type_;
    const ManchesterDecoder decoder_;      // sense=0: FLM, Schrader, Ford, Citroen, Renault, Elantra
    const ManchesterDecoder decoder_inv_;  // sense=1: BMW G45, Jansite, Solar Truck, Jansite Solar
    const Reader            reader_;
    const ReaderInv         reader_inv_;

    // Phase 1 original
    Optional<Reading> reading_fsk_19k2_schrader() const;
    Optional<Reading> reading_ook_8k192_schrader() const;
    Optional<Reading> reading_ook_8k4_schrader() const;

    // Phase 1 EU sub-decoders (called from reading_fsk_19k2_schrader)
    Optional<Reading> reading_fsk_19k2_ford() const;
    Optional<Reading> reading_fsk_19k2_citroen() const;
    Optional<Reading> reading_fsk_19k2_renault() const;

    // Phase 1 World sub-decoders (called from reading_fsk_19k2_schrader)
    Optional<Reading> reading_fsk_19k2_jansite() const;
    Optional<Reading> reading_fsk_19k2_solar_truck() const;

    // Phase 2 EU (proc_tpms_eu signal paths)
    Optional<Reading> reading_fsk_38k4_bmw_g45() const;
    Optional<Reading> reading_fsk_19k2_bmw_g23() const;
    Optional<Reading> reading_fsk_19k2_porsche() const;

    // Phase 3 World (proc_tpms_world signal paths)
    Optional<Reading> reading_fsk_19k2_toyota() const;
    Optional<Reading> reading_fsk_19k2_elantra() const;
    Optional<Reading> reading_fsk_19k2_jansite_solar() const;

    // Helpers
    size_t crc_valid_length() const;
    size_t nrzi_decode(uint8_t* bytes, size_t n_bits, uint_fast8_t prev_bit) const;
};

} /* namespace tpms */

#endif /*__TPMS_PACKET_H__*/
