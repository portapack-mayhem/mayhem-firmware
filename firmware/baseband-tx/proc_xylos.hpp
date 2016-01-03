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

#ifndef __PROC_XYLOS_H__
#define __PROC_XYLOS_H__

#include "baseband_processor.hpp"

#define CCIR_TONELENGTH 22800
#define PHASEV 294.34

class XylosProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override;

private:
	uint32_t ccir_phases[16] = {
								1981*PHASEV,
								1124*PHASEV,
								1197*PHASEV,
								1275*PHASEV,
								1358*PHASEV,
								1446*PHASEV,
								1540*PHASEV,
								1640*PHASEV,
								1747*PHASEV,
								1860*PHASEV,
								2400*PHASEV,
								930*PHASEV,
								2247*PHASEV,
								991*PHASEV,
								2110*PHASEV,
								1055*PHASEV
							};

	int8_t re, im;
	uint8_t s;
    uint8_t byte_pos = 0;
    uint8_t digit = 0;
    uint32_t sample_count = CCIR_TONELENGTH;
	uint32_t aphase, phase, sphase;
	int32_t sample, frq;
	TXDoneMessage message;
};

#endif
