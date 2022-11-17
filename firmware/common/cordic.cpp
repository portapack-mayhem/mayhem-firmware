/*
 * Copyright (C) 2022 Belousov Oleg
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

#include "cordic.hpp"

namespace dsp {

void cordic(int32_t theta, int32_t *s, int32_t *c) {
	uint8_t pi = 0;

	if (theta > CORDIC_HALF_PI) {
		theta -= CORDIC_PI;
		pi = 1;
	} else if (theta < -CORDIC_HALF_PI) {
		theta += CORDIC_PI;
		pi = 1;
	}

  	int32_t d, tx, ty, tz;
  	int32_t x = *c, y = *s, z = theta;
  
  	for (uint8_t k = 0; k < CORDIC_TAB; k++) {
    	d = (z >= 0) ? 0 : -1;

    	tx = x - (((y >> k) ^ d) - d);
    	ty = y + (((x >> k) ^ d) - d);
    	tz = z - ((cordic_tab[k] ^ d) - d);

    	x = tx;
    	y = ty;
    	z = tz;
  	}

	if (pi) {
 		x = -x; 
 		y = -y;
 	}

	*c = x; 
	*s = y;
}

}
