/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __SONDE_PACKET_H__
#define __SONDE_PACKET_H__

#include <cstdint>
#include <cstddef>

#include "field_reader.hpp"
#include "baseband_packet.hpp"
#include "manchester.hpp"

namespace sonde {

class Packet {
public:
	enum class Type : uint32_t {
		Unknown = 0,
		M10 = 1,
	};

	Packet(
		const baseband::Packet& packet,
		const Type type
	) : packet_ { packet },
		decoder_ { packet_ },
		reader_ { decoder_ },
		type_ { type }
	{
	}

	size_t length() const;
	
	bool is_valid() const;

	Timestamp received_at() const;

	Type type() const;
	std::string serial_number() const;
	uint32_t GPS_altitude() const;
	float GPS_latitude() const;
	float GPS_longitude() const;
	std::string signature() const;
	uint32_t battery_voltage() const;

	FormattedSymbols symbols_formatted() const;

	bool crc_ok() const;

private:
	using Reader = FieldReader<BiphaseMDecoder, BitRemapNone>;

	const baseband::Packet packet_;
	const BiphaseMDecoder decoder_;
	const Reader reader_;
	const Type type_;

	bool crc_ok_M10() const;
};

} /* namespace sonde */

#endif/*__SONDE_PACKET_H__*/
