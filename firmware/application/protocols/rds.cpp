/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "rds.hpp"

#include "portapack_shared_memory.hpp"

// RDS infos:
// One frame = X groups (as necessary)
// One group = 4 blocks = 4 * 26 bits
// One block = 26 bits (16 bits data + 10 bits checkword)
// Sent MSB first

namespace rds {

uint32_t make_block(uint32_t data, uint16_t offset) {
    uint16_t CRC = 0;
    uint8_t bit;
    
    for (uint8_t i = 0; i < 16; i++) {
        bit = (((data << i) & 0x8000) >> 15) ^ (CRC >> 9);
        if (bit) CRC ^= 0b0011011100;
        CRC = ((CRC << 1) | bit) & 0x3FF;
    }
    
    return (data << 10) | (CRC ^ offset);
}

// Boolean to binary
uint8_t b2b(const bool in) {
	if (in)
		return 1;
	else
		return 0;
}

// Type 0B groups are like 0A groups but without alternative frequency data
RDSGroup make_0B_group(const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool TA,
							const bool MS, const bool DI, const uint8_t C, const std::string chars) {
	RDSGroup group;
	
	group.block[0] = PI_code;
	group.block[1] = (0x0 << 12) | (1 << 11) | (b2b(TP) << 10) | ((PTY & 0x1F) << 5) | (b2b(TA) << 4) | (b2b(MS) << 3) | (b2b(DI) << 2) | (C & 3);
	group.block[2] = PI_code;
	group.block[3] = (chars[0] << 8) | chars[1];
	
	return group;
}

// For RadioText, up to 64 chars with 2A, 32 chars with 2B
RDSGroup make_2A_group(const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool AB,
							const uint8_t segment, const std::string chars) {
	RDSGroup group;
	
	group.block[0] = PI_code;
	group.block[1] = (0x2 << 12) | (0 << 11) | (b2b(TP) << 10) | ((PTY & 0x1F) << 5) | (b2b(AB) << 4) | (segment & 15);
	group.block[2] = (chars[0] << 8) | chars[1];
	group.block[3] = (chars[2] << 8) | chars[3];
	
	return group;
}

// Time and date - usually one message per minute - Month: 1~12 - Day: 1~31 - Hour/Minute: 0~59 - Local offset: -12/+12 from UTC
RDSGroup make_4A_group(const uint16_t PI_code, const bool TP, const uint8_t PTY,
							const uint16_t year, const uint8_t month, const uint8_t day,
							const uint8_t hour, const uint8_t minute, const int8_t local_offset) {
	RDSGroup group;
	uint32_t L = 0;
	uint32_t day_code;

	if ((month == 1) || (month == 2)) L = 1;
	
	day_code = 14956 + day + (uint32_t)((float)(year - 1900 - L) * 365.25) + uint16_t((float)((month + 1) + L * 12) * 30.6001);
	
	group.block[0] = PI_code;
	group.block[1] = (0x4 << 12) | (0 << 11) | (b2b(TP) << 10) | ((PTY & 0x1F) << 5) | ((day_code & 0x18000) >> 15);
	group.block[2] = ((day_code & 0x7FFF) << 1) | (hour >> 4);
	group.block[3] = ((hour & 15) << 12) | ((minute & 0x3F) << 6) | (local_offset & 0x3F);
	
	return group;
}

void gen_PSN(std::vector<RDSGroup>& frame, const std::string& psname, const RDS_flags * rds_flags) {
	uint8_t c;
	RDSGroup group;
	
	frame.clear();
	
	// 4 groups with 2 PSN characters in each
	for (c = 0; c < 4; c++) {
		group = make_0B_group(rds_flags->PI_code, rds_flags->TP, rds_flags->PTY, rds_flags->TA, rds_flags->MS, rds_flags->DI, c, psname.substr(c * 2, 2));
		group.block[0] = make_block(group.block[0], RDS_OFFSET_A);
		group.block[1] = make_block(group.block[1], RDS_OFFSET_B);
		group.block[2] = make_block(group.block[2], RDS_OFFSET_Cp);	// C' !
		group.block[3] = make_block(group.block[3], RDS_OFFSET_D);
		frame.emplace_back(group);
	}
}

void gen_RadioText(std::vector<RDSGroup>& frame, const std::string& text, const bool AB, const RDS_flags * rds_flags) {
	size_t c;
	//RDSGroup * groups_ptr;
	std::string radiotext_buffer = text;
	size_t rt_length, group_count;
	RDSGroup group;

	radiotext_buffer += 0x0D;
	rt_length = radiotext_buffer.length();
	rt_length = (rt_length + 3) & 0xFC;
	
	group_count = rt_length >> 2;	// 4 characters per group

	//groups_ptr = (RDSGroup*)chHeapAlloc(0, group_count * sizeof(RDSGroup));

	frame.clear();
	
	for (c = 0; c < group_count; c++) {
		group = make_2A_group(rds_flags->PI_code, rds_flags->TP, rds_flags->PTY, AB, c, radiotext_buffer.substr(c * 4, 4));
		group.block[0] = make_block(group.block[0], RDS_OFFSET_A);
		group.block[1] = make_block(group.block[1], RDS_OFFSET_B);
		group.block[2] = make_block(group.block[2], RDS_OFFSET_C);
		group.block[3] = make_block(group.block[3], RDS_OFFSET_D);
		frame.emplace_back(group);
	}
}

void gen_ClockTime(std::vector<RDSGroup>& frame, const RDS_flags * rds_flags,
						const uint16_t year, const uint8_t month, const uint8_t day,
						const uint8_t hour, const uint8_t minute, const int8_t local_offset) {
	RDSGroup group;
	
	group = make_4A_group(rds_flags->PI_code, rds_flags->TP, rds_flags->PTY, year, month, day, hour, minute, local_offset);
	
	// Generate checkbits for each block
	group.block[0] = make_block(group.block[0], RDS_OFFSET_A);
	group.block[1] = make_block(group.block[1], RDS_OFFSET_B);
	group.block[2] = make_block(group.block[2], RDS_OFFSET_C);
	group.block[3] = make_block(group.block[3], RDS_OFFSET_D);
	
	frame.clear();
	frame.emplace_back(group);		
}

} /* namespace rds */
