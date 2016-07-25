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

#include "ert_packet.hpp"

#include "crc.hpp"

namespace ert {

size_t Packet::length() const {
	return decoder_.symbols_count();
}

bool Packet::is_valid() const {
	return true;
}

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

Packet::Type Packet::type() const {
	return type_;
}

ID Packet::id() const {
	if( type() == Type::SCM ) {
		const auto msb = reader_.read(0, 2);
		const auto lsb = reader_.read(35, 24);
		return (msb << 24) | lsb;
	}
	if( type() == Type::IDM ) {
		return reader_.read(5 * 8, 32);
	}
	return invalid_id;
}

Consumption Packet::consumption() const {
	if( type() == Type::SCM ) {
		return reader_.read(11, 24);
	}
	if( type() == Type::IDM ) {
		return reader_.read(25 * 8, 32);
	}
	return invalid_consumption;
}

CommodityType Packet::commodity_type() const {
	if( type() == Type::SCM ) {
		return reader_.read(5, 4);
	}
	if( type() == Type::IDM ) {
		return reader_.read(4 * 8 + 4, 4);
	}
	return invalid_commodity_type;
}

FormattedSymbols Packet::symbols_formatted() const {
	return format_symbols(decoder_);
}

bool Packet::crc_ok() const {
	switch(type()) {
	case Type::SCM:	return crc_ok_scm();
	case Type::IDM:	return crc_ok_idm();
	default:		return false;
	}
}

bool Packet::crc_ok_scm() const {
	CRC<16> ert_bch { 0x6f63 };
	size_t start_bit = 5;
	ert_bch.process_byte(reader_.read(0, start_bit));
	for(size_t i=start_bit; i<length(); i+=8) {
		ert_bch.process_byte(reader_.read(i, 8));
	}
	return ert_bch.checksum() == 0x0000;
}

bool Packet::crc_ok_idm() const {
	CRC<16> ert_crc_ccitt { 0x1021, 0xffff, 0x1d0f };
	for(size_t i=0; i<length(); i+=8) {
		ert_crc_ccitt.process_byte(reader_.read(i, 8));
	}
	return ert_crc_ccitt.checksum() == 0x0000;
}

} /* namespace ert */
