/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AIS_PACKET_H__
#define __AIS_PACKET_H__

#include "baseband_packet.hpp"
#include "field_reader.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <cstddef>
#include <string>

namespace baseband {
namespace ais {

struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

using Latitude = int32_t;
using Longitude = int32_t;

using MMSI = uint32_t;

class Packet {
public:
	constexpr Packet(
		const rtc::RTC& received_at,
		const baseband::Packet& packet
	) : packet_ { packet },
		received_at_ { received_at },
		field_ { packet_ }
	{
	}

	size_t length() const;
	
	bool is_valid() const;

	rtc::RTC received_at() const;

	uint32_t message_id() const;
	MMSI user_id() const;
	MMSI source_id() const;

	uint32_t read(const size_t start_bit, const size_t length) const;

	std::string text(const size_t start_bit, const size_t character_count) const;

	DateTime datetime(const size_t start_bit) const;

	Latitude latitude(const size_t start_bit) const;
	Longitude longitude(const size_t start_bit) const;

	bool crc_ok() const;

private:
	using Reader = FieldReader<baseband::Packet, BitRemapByteReverse>;
	
	const baseband::Packet packet_;
	const rtc::RTC received_at_;
	const Reader field_;

	const size_t fcs_length = 16;

	size_t data_and_fcs_length() const;
	size_t data_length() const;

	bool length_valid() const;
};

} /* namespace ais */
} /* namespace baseband */

#endif/*__AIS_PACKET_H__*/
