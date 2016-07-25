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

#ifndef __ERT_PACKET_H__
#define __ERT_PACKET_H__

#include <cstdint>
#include <cstddef>

#include "field_reader.hpp"
#include "baseband_packet.hpp"
#include "manchester.hpp"

namespace ert {

using ID = uint32_t;
using Consumption = uint32_t;
using CommodityType = uint32_t;

constexpr ID invalid_id = 0;
constexpr CommodityType invalid_commodity_type = -1;
constexpr Consumption invalid_consumption = 0;

class Packet {
public:
	enum class Type : uint32_t {
		Unknown = 0,
		IDM = 1,
		SCM = 2,
	};

	Packet(
		const Type type,
		const baseband::Packet& packet
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
	ID id() const;
	CommodityType commodity_type() const;
	Consumption consumption() const;

	FormattedSymbols symbols_formatted() const;

	bool crc_ok() const;

private:
	using Reader = FieldReader<ManchesterDecoder, BitRemapNone>;

	const baseband::Packet packet_;
	const ManchesterDecoder decoder_;
	const Reader reader_;
	const Type type_;

	bool crc_ok_idm() const;
	bool crc_ok_scm() const;
};

} /* namespace ert */

#endif/*__ERT_PACKET_H__*/
