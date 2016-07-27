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

#ifndef __PROC_FSK_LCR_H__
#define __PROC_FSK_LCR_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

class LCRFSKProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

private:
	BasebandThread baseband_thread { 2280000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	int8_t re, im;
	uint8_t s;
    uint8_t bit_pos = 0, byte_pos = 0;
    char cur_byte = 0;
    char ext_byte = 0;
    uint16_t gbyte;
    uint8_t cur_bit = 0;
    uint32_t sample_count;
	uint32_t aphase, phase, sphase;
	int32_t sample, sig, frq;
	TXDoneMessage message;
};

#endif
