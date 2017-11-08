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

#include "test_packet.hpp"
#include "string_format.hpp"

namespace testapp {

size_t Packet::length() const {
	return decoder_.symbols_count();
}

bool Packet::is_valid() const {
	return true;
}

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

FormattedSymbols Packet::symbols_formatted() const {
	return format_symbols(decoder_);
}

uint32_t Packet::value() const {
	return (reader_.read(10 * 8, 6) << 8) | reader_.read(9 * 8, 8);
}

uint32_t Packet::alt() const {
	return reader_.read(1 * 8, 12);
}

} /* namespace testapp */
