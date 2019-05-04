/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2019 Furrtek
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

#ifndef __RFM69_H__
#define __RFM69_H__

#include <cstdint>
#include <vector>

#include "utility.hpp"

class RFM69 {
public:
    RFM69(const uint8_t num_preamble, const uint16_t sync_word, const bool CRC, const bool manchester)
        : num_preamble_(num_preamble), sync_word_(sync_word), CRC_(CRC), manchester_(manchester)
    { }
	//~RFM69();
	
	void set_sync_word(const uint16_t sync_word) {
		sync_word_ = sync_word;
	};
	void set_data_config(const bool CRC, const bool manchester) {
		CRC_ = CRC;
		manchester_ = manchester;
	};
	void set_num_preamble(const uint8_t num_preamble) {
		num_preamble_ = num_preamble;
	};
	
	uint32_t gen_frame(std::vector<uint8_t>& payload);

private:
	uint8_t num_preamble_ { 5 };
	uint16_t sync_word_ { 0x2DD4 };
	bool CRC_ { true };
	bool manchester_ { false };
};

#endif/*__RFM69_H__*/
