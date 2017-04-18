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

#include "afsk.hpp"

#include "portapack_persistent_memory.hpp"

namespace afsk {

void generate_data(const std::string & in_message, char * out_data) {
	const afsk_formats_t * format_def;
	uint8_t pm, pp, bit, cp, cur_byte, new_byte;
	uint16_t dp;
	
	format_def = &afsk_formats[portapack::persistent_memory::afsk_format()];
	
	if (format_def->parity == ODD)
		pm = 1; 	// Odd parity
	else
		pm = 0; 	// Even parity

	if (format_def->data_bits == 7) {
		if (!format_def->use_LUT) {
			for (dp = 0; dp < in_message.length(); dp++) {
				pp = pm;
				new_byte = 0;
				cur_byte = in_message[dp];
				for (cp = 0; cp < 7; cp++) {
					bit = (cur_byte >> cp) & 1;
					pp += bit;
					new_byte |= (bit << (7 - cp));
				}
				if (format_def->parity != NONE) new_byte |= (pp & 1);
				out_data[dp] = new_byte;
			}
			out_data[dp++] = 0;
			out_data[dp] = 0;
		} else {
			for (dp = 0; dp < in_message.length(); dp++) {
				pp = pm;
				
				cur_byte = in_message[dp];
				
				for (cp = 0; cp < 8; cp++)
					if ((cur_byte >> cp) & 1) pp++;

				out_data[dp * 2] = cur_byte;
				out_data[(dp * 2) + 1] = 0xFE;
				if (format_def->parity != NONE) out_data[(dp * 2) + 1] |= (pp & 1);
			}
			out_data[dp * 2] = 0;
			out_data[(dp * 2) + 1] = 0;
		}
	} else {
		/*
		for (dp = 0; dp < strlen(in_message); dp++) {
			pp = pm;
			
			// Do not apply LUT on checksum (last byte) ?
			if (dp != strlen(in_message) - 1)
				cur_byte = alt_lookup[(uint8_t)in_message[dp] & 0x7F];
			else
				cur_byte = in_message[dp];
			
			for (cp = 0; cp < 8; cp++)
				if ((cur_byte >> cp) & 1) pp++;

			out_data[dp * 2] = cur_byte;
			out_data[(dp * 2) + 1] = 0xFE | (pp & 1);
		}
		out_data[dp * 2] = 0;
		out_data[(dp * 2) + 1] = 0;
		*/
	}

	/*
		// MSB first
		for (dp = 0; dp < strlen(lcr_message); dp++) {
			pp = pm;
			cur_byte = lcr_message[dp];
			for (cp = 0; cp < 7; cp++)
				if ((cur_byte >> cp) & 1) pp++;
			lcr_message_data[dp] = (cur_byte << 1) | (pp & 1);
		}
	}*/
}

} /* namespace afsk */
