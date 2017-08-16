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

#include <string>
#include <vector>
#include "ch.h"

#ifndef __RDS_H__
#define __RDS_H__

namespace rds {
	
#define RDS_OFFSET_A	0b0011111100
#define RDS_OFFSET_B	0b0110011000
#define RDS_OFFSET_C	0b0101101000
#define RDS_OFFSET_Cp	0b1101010000
#define RDS_OFFSET_D	0b0110110100

struct RDS_flags {
	uint16_t PI_code;
	uint8_t PTY;
	uint8_t DI;
	bool TP;
	bool TA;
	bool MS;
};

struct RDSGroup {
	uint32_t block[4];
};

uint32_t make_block(uint32_t blockdata, uint16_t offset);
uint8_t b2b(const bool in);

RDSGroup make_0B_group(const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool TA,
							const bool MS, const bool DI, const uint8_t C, const std::string chars);
RDSGroup make_2A_group(const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool AB,
							const uint8_t segment, const std::string chars);
RDSGroup make_4A_group(const uint16_t PI_code, const bool TP, const uint8_t PTY,
							const uint16_t year, const uint8_t month, const uint8_t day,
							const uint8_t hour, const uint8_t minute, const int8_t local_offset);

void gen_PSN(std::vector<RDSGroup>& frame, const std::string& psname, const RDS_flags * rds_flags);
void gen_RadioText(std::vector<RDSGroup>& frame, const std::string& text, const bool AB, const RDS_flags * rds_flags);
void gen_ClockTime(std::vector<RDSGroup>& frame, const RDS_flags * rds_flags,
						const uint16_t year, const uint8_t month, const uint8_t day,
						const uint8_t hour, const uint8_t minute, const int8_t local_offset);

} /* namespace rds */

#endif/*__RDS_H__*/
