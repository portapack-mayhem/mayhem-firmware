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

#ifndef __PROC_ADSBTX_H__
#define __PROC_ADSBTX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

class ADSBTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const p) override;

private:
	bool configured = false;
	
	BasebandThread baseband_thread { 4000000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	const complex8_t am_lut[4] = {
		{ 127, 0 },
		{ 0, 127 },
		{ -127, 0 },
		{ 0, -127 }		
	};
	
    uint32_t bit_pos { 0 };
    uint32_t cur_bit { 0 };
	uint32_t phase { 0 };
	
	TXProgressMessage txprogress_message { };
};

#endif
