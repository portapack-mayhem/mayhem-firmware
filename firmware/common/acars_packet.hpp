/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __ACARS_PACKET_H__
#define __ACARS_PACKET_H__

#include "baseband_packet.hpp"
#include "field_reader.hpp"

#include <cstdint>
#include <cstddef>
#include <string>

namespace acars {

class Packet {
public:
	constexpr Packet(
		const baseband::Packet& packet
	) : packet_ { packet },
		field_ { packet_ }
	{
	}

	size_t length() const;
	
	bool is_valid() const;

	Timestamp received_at() const;

	uint8_t block_id() const;
	std::string registration_number() const;

	uint32_t read(const size_t start_bit, const size_t length) const;
	//std::string text(const size_t start_bit, const size_t character_count) const;

	bool crc_ok() const;

private:
	using Reader = FieldReader<baseband::Packet, BitRemapByteReverse>;
	using CRCReader = FieldReader<baseband::Packet, BitRemapNone>;
	
	const baseband::Packet packet_;
	const Reader field_;

	const size_t fcs_length = 16;

	size_t data_and_fcs_length() const;
	size_t data_length() const;

	bool length_valid() const;
};

} /* namespace acars */

#endif/*__ACARS_PACKET_H__*/
