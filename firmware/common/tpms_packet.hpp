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

enum SignalType {
    // Original Mayhem signal paths
    FSK_19k2_Schrader = 1,
    OOK_8k192_Schrader = 2,
    OOK_8k4_Schrader = 3,
    // Extension signal paths. Slot 4 is unused (was FSK_19k2_Elantra2012,
    // removed -- ghost signals; was briefly OOK_8k192_EG53MA4, removed --
    // bit-stream collision with pb_ook_8k192. EG53MA4 will be a separate
    // standalone app in the future.).
    OOK_8k4_SMD3MA4 = 5,
    // Slots 6, 7 reserved for future protocols.
    FSK_19k2_JansiteSolar = 9,
};

inline constexpr const char* signal_type_name(const SignalType signal_type) {
    switch (signal_type) {
        case FSK_19k2_Schrader:
            return "FSK 19200 Schrader";
        case OOK_8k192_Schrader:
            return "OOK 8192 Schrader";
        case OOK_8k4_Schrader:
            return "OOK 8400 Schrader";
        case OOK_8k4_SMD3MA4:
            return "OOK 8400 SMD3MA4";
        case FSK_19k2_JansiteSolar:
            return "FSK 19200 JanSolar";
        default:
            return "- - - -";
    }
}

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
        // Original
        None = 0,
        FLM_64 = 1,
        FLM_72 = 2,
        FLM_80 = 3,
        Schrader = 4,
        GMC_96 = 5,
        // EU 433MHz (FSK_19k2_Schrader path -- slot 6 recycled from ex-Ford)
        TruckSolar = 6,
        Citroen_PSA = 7,
        Renault = 8,
        // Slot 9 was Elantra2012 (removed) and briefly EG53MA4 (removed --
        // collides with Mayhem-original Schrader on the OOK 8k192 path).
        // EG53MA4 will be a standalone app in the future.
        Schrader_SMD3MA4 = 10,  // own SignalType OOK_8k4_SMD3MA4 (5)
        // Slots 11 (ex-Nissan, removed), 12 (ex-Toyota) intentionally left
        // unused. Slot 14 was Jansite TY02S (removed due to too-weak
        // validation -- no CRC -- causing ghost signals).
        JansiteSolar = 16,
        // EU 433MHz (FSK_19k2_Schrader path) - second batch
        Hyundai_VDO = 17,
        Abarth = 18,
        Renault_0435R = 19,
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

    Type type() const {
        return type_;
    }

    TransponderID id() const {
        return id_;
    }

    Optional<Pressure> pressure() const {
        return pressure_;
    }

    Optional<Temperature> temperature() const {
        return temperature_;
    }

    Optional<Flags> flags() const {
        return flags_;
    }

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
          decoder_{packet_, 0},
          decoder_inv_{packet_, 1},
          reader_{decoder_},
          reader_inv_{decoder_inv_} {
    }

    SignalType signal_type() const { return signal_type_; }
    Timestamp received_at() const;

    FormattedSymbols symbols_formatted() const;

    Optional<Reading> reading() const;

   private:
    using Reader = FieldReader<ManchesterDecoder, BitRemapNone>;

    const baseband::Packet packet_;
    const SignalType signal_type_;
    const ManchesterDecoder decoder_;      // sense=0: standard Manchester
    const ManchesterDecoder decoder_inv_;  // sense=1: inverted Manchester

    const Reader reader_;
    const Reader reader_inv_;

    // Original decoders
    Optional<Reading> reading_fsk_19k2_schrader() const;
    Optional<Reading> reading_ook_8k192_schrader() const;
    Optional<Reading> reading_ook_8k4_schrader() const;

    // EU 433MHz sub-decoders (called from reading_fsk_19k2_schrader)
    Optional<Reading> reading_fsk_19k2_citroen() const;
    Optional<Reading> reading_fsk_19k2_renault() const;
    Optional<Reading> reading_fsk_19k2_hyundai_vdo() const;
    Optional<Reading> reading_fsk_19k2_abarth() const;
    Optional<Reading> reading_fsk_19k2_renault_0435r() const;
    Optional<Reading> reading_fsk_19k2_truck_solar() const;

    // Extension signal paths (own SignalType, own PacketBuilder)
    Optional<Reading> reading_ook_8k4_smd3ma4() const;
    Optional<Reading> reading_fsk_19k2_jansite_solar() const;

    size_t crc_valid_length() const;
};

} /* namespace tpms */

#endif /*__TPMS_PACKET_H__*/
