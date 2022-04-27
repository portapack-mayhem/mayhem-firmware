/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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
using units::Temperature;
using units::Pressure;

#include "baseband_packet.hpp"
#include "manchester.hpp"
#include "field_reader.hpp"

namespace tpms {

using Flags = uint8_t;

enum SignalType {
	FSK_19k2_Schrader = 1,
	OOK_8k192_Schrader = 2,
	OOK_8k4_Schrader = 3,
};

class TransponderID {
public:
	constexpr TransponderID(
	) : id_ { 0 }
	{
	}

	constexpr TransponderID(
		const uint32_t id
	) : id_ { id }
	{
	}

	constexpr uint32_t value() const {
		return id_;
	}

private:
	uint32_t id_;
};

class Reading {
public:
	enum Type {
		None = 0,
		FLM_64 = 1,
		FLM_72 = 2,
		FLM_80 = 3,
		Schrader = 4,
		GMC_96 = 5,
	};

	constexpr Reading(
	) : type_ { Type::None }
	{
	}
	
	constexpr Reading(
		Type type,
		TransponderID id
	) : type_ { type },
		id_ { id }
	{
	}
	
	constexpr Reading(
		Type type,
		TransponderID id,
		Optional<Pressure> pressure = { },
		Optional<Temperature> temperature = { },
		Optional<Flags> flags = { }
	) : type_ { type },
		id_ { id },
		pressure_ { pressure },
		temperature_ { temperature },
		flags_ { flags }
	{
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
	Type type_ { Type::None };
	TransponderID id_ { 0 };
	Optional<Pressure> pressure_ { };
	Optional<Temperature> temperature_ { };
	Optional<Flags> flags_ { };
};

class Packet {
public:
	constexpr Packet(
		const baseband::Packet& packet,
		const SignalType signal_type
	) : packet_ { packet },
		signal_type_ { signal_type },
		decoder_ { packet_, 0 },
		reader_ { decoder_ }
	{
	}

	SignalType signal_type() const { return signal_type_; }
	Timestamp received_at() const;

	FormattedSymbols symbols_formatted() const;

	Optional<Reading> reading() const;

private:
	using Reader = FieldReader<ManchesterDecoder, BitRemapNone>;

	const baseband::Packet packet_;
	const SignalType signal_type_;
	const ManchesterDecoder decoder_;

	const Reader reader_;

	Optional<Reading> reading_fsk_19k2_schrader() const;
	Optional<Reading> reading_ook_8k192_schrader() const;
	Optional<Reading> reading_ook_8k4_schrader() const;

	size_t crc_valid_length() const;
};

} /* namespace tpms */

#endif/*__TPMS_PACKET_H__*/
