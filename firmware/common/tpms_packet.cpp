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

#include "tpms_packet.hpp"

#include "crc.hpp"

namespace tpms {

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

ManchesterFormatted Packet::symbols_formatted() const {
	return format_manchester(decoder_);
}

Optional<Reading> Packet::reading(const SignalType signal_type) const {
	if( signal_type == SignalType::FLM ) {
		const auto length = crc_valid_length();

		switch(length) {
		case 64:
			return Reading {
				Reading::Type::FLM_64,
				reader_.read(0, 32),
				Pressure { static_cast<int>(reader_.read(32, 8)) * 4 / 3 },
				Temperature { static_cast<int>(reader_.read(40, 8) & 0x7f) - 50 }
			};

		case 72:
			return Reading {
				Reading::Type::FLM_72,
				reader_.read(0, 32),
				Pressure { static_cast<int>(reader_.read(40, 8)) * 4 / 3 },
				Temperature { static_cast<int>(reader_.read(48, 8)) - 50 }
			};

		case 80:
			return Reading {
				Reading::Type::FLM_80,
				reader_.read(8, 32),
				Pressure { static_cast<int>(reader_.read(48, 8)) * 4 / 3 },
				Temperature { static_cast<int>(reader_.read(56, 8)) - 50 }
			};

		default:
			return { };
		}
	}

	if( signal_type == SignalType::Subaru ) {
		return Reading {
			Reading::Type::SUB_35,
			reader_.read(3, 25),
			Pressure { static_cast<int>(reader_.read(28, 8)) }
		};
	}

	if( signal_type == SignalType::GMC ) {
		return Reading {
			Reading::Type::GMC_96,
			reader_.read(20, 32),
			Pressure { static_cast<int>(reader_.read(52, 8)) }
		};
	}

	return { };
}

size_t Packet::crc_valid_length() const {
	constexpr uint32_t checksum_bytes = 0b1111111;
	constexpr uint32_t crc_72_bytes = 0b111111111;
	constexpr uint32_t crc_80_bytes = 0b1111111110;

	std::array<uint8_t, 10> bytes;
	for(size_t i=0; i<bytes.size(); i++) {
		bytes[i] = reader_.read(i * 8, 8);
	}

	uint32_t checksum = 0;
	CRC<8> crc_72 { 0x01, 0x00 };
	CRC<8> crc_80 { 0x01, 0x00 };

	for(size_t i=0; i<bytes.size(); i++) {
		const uint32_t byte_mask = 1 << i;
		const auto byte = bytes[i];

		if( checksum_bytes & byte_mask ) {
			checksum += byte;
		}
		if( crc_72_bytes & byte_mask ) {
			crc_72.process_byte(byte);
		}
		if( crc_80_bytes & byte_mask ) {
			crc_80.process_byte(byte);
		}
	}

	if( crc_80.checksum() == 0 ) {
		return 80;
	} else if( crc_72.checksum() == 0 ) {
		return 72;
	} else if( (checksum & 0xff) == bytes[7] ) {
		return 64;
	} else {
		return 0;
	}
}

} /* namespace tpms */
