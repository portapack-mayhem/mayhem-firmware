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

FormattedSymbols Packet::symbols_formatted() const {
	return format_symbols(decoder_);
}

Optional<Reading> Packet::reading_fsk_19k2_schrader() const {
	const auto length = crc_valid_length();

	switch(length) {
	case 64:
		return Reading {
			Reading::Type::FLM_64,
			(uint32_t)reader_.read(0, 32),
			Pressure { static_cast<int>(reader_.read(32, 8)) * 4 / 3 },
			Temperature { static_cast<int>(reader_.read(40, 8) & 0x7f) - 56 }
		};

	case 72:
		return Reading {
			Reading::Type::FLM_72,
			(uint32_t)reader_.read(0, 32),
			Pressure { static_cast<int>(reader_.read(40, 8)) * 4 / 3 },
			Temperature { static_cast<int>(reader_.read(48, 8)) - 56 }
		};

	case 80:
		return Reading {
			Reading::Type::FLM_80,
			(uint32_t)reader_.read(8, 32),
			Pressure { static_cast<int>(reader_.read(48, 8)) * 4 / 3 },
			Temperature { static_cast<int>(reader_.read(56, 8)) - 56 }
		};

	default:
		return { };
	}
}

Optional<Reading> Packet::reading_ook_8k192_schrader() const {
	/*
	 * Preamble: 11*2, 01*14, 11, 10
	 * Function code: 3 Manchester symbols
	 * ID: 24 Manchester symbols (one variant seen with 21 symbols?)
	 * Pressure: 8 Manchester symbols
	 * Checksum: 2 Manchester symbols (2 LSBs of sum incl this field == 3)
	 */
	const auto flags = reader_.read(0, 3);
	const auto checksum = reader_.read(35, 2);
	
	uint32_t checksum_calculated = reader_.read(0, 1);
	for(size_t i=1; i<37; i+=2) {
		checksum_calculated += reader_.read(i, 2);
	}

	if( (checksum_calculated & 3) == 3 ) {
		return Reading {
			Reading::Type::Schrader,
			(uint32_t)reader_.read(3, 24),
			Pressure { static_cast<int>(reader_.read(27, 8)) * 4 / 3 },
			{ },
			Flags { static_cast<Flags>((flags << 4) | checksum) }
		};
	} else {
		return { };
	}
}

Optional<Reading> Packet::reading_ook_8k4_schrader() const {
	/*
	 * Preamble: 01*40
	 * System ID: 01100101, ??*20 (not really sure what this data is)
	 * ID: 32 Manchester symbols
	 * Value: 8 Manchester symbols (temperature?)
	 * Value: 8 Manchester symbols (pressure?)
	 * Checksum: 8 Manchester symbols (uint8_t sum of bytes starting with system ID)
	 */
	/* NOTE: First four bits of packet are consumed in preamble detection.
	 * Those bits assumed to be 0b0100", which may not be entirely true...
	 */
	constexpr uint8_t first_nibble = 0x4;
	// const auto system_id = (first_nibble << 20) | reader_.read(0, 20);
	const auto id = reader_.read(20, 32);
	const auto value_0 = reader_.read(52, 8);
	const auto value_1 = reader_.read(60, 8);
	const auto checksum = reader_.read(68, 8);

	uint8_t checksum_calculated = (first_nibble << 4) | reader_.read(0, 4);
	for(size_t i=4; i<68; i+=8) {
		checksum_calculated += reader_.read(i, 8);
	}

	if( checksum_calculated == checksum ) {
		return Reading {
			Reading::Type::GMC_96,
			(uint32_t)id,
			Pressure { static_cast<int>(value_1) * 4 / 3 },
			Temperature { static_cast<int>(value_0) - 56 }
		};
	} else {
		return { };
	}
}

Optional<Reading> Packet::reading() const {
	switch( signal_type() ) {
	case SignalType::FSK_19k2_Schrader:		return reading_fsk_19k2_schrader();
	case SignalType::OOK_8k192_Schrader:	return reading_ook_8k192_schrader();
	case SignalType::OOK_8k4_Schrader:		return reading_ook_8k4_schrader();
	default:								return { };
	}
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
