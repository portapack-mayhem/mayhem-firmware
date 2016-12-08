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

#include <cstring>
#include <string>
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
	bool TP;
	uint8_t PTY;
	bool TA;
	bool MS;
	bool DI;
};

uint32_t makeblock(uint32_t blockdata, uint16_t offset);
uint8_t b2b(const bool in);
void make_0B_group(uint32_t group[], const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool TA,
							const bool MS, const bool DI, const uint8_t C, const char * chars);
void make_2A_group(uint32_t group[], const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool AB,
							const uint8_t segment, const char * chars);
void make_4A_group(uint32_t blocks[], const uint16_t PI_code, const bool TP, const uint8_t PTY,
							const uint16_t year, const uint8_t month, const uint8_t day,
							const uint8_t hour, const uint8_t minute, const int8_t local_offset);

uint16_t gen_PSN(const char * psname, const RDS_flags * rds_flags);
uint16_t gen_RadioText(const char * text, const bool AB, const RDS_flags * rds_flags);
uint16_t gen_ClockTime(const RDS_flags * rds_flags,
						const uint16_t year, const uint8_t month, const uint8_t day,
						const uint8_t hour, const uint8_t minute, const int8_t local_offset);

} /* namespace rds */

#endif/*__RDS_H__*/
