/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_adsb_rx.hpp"
#include "ui_alphanum.hpp"

#include "adsb.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace adsb;
using namespace portapack;

namespace ui {

void ADSBRxView::focus() {
	offset_field.focus();
	offset_field.set_value(13179);
}

ADSBRxView::~ADSBRxView() {
	//transmitter_model.disable();
	//baseband::shutdown();
}

void ADSBRxView::analyze(uint64_t offset) {
	size_t c;
	Coord lcd_x = 0, lcd_y = 0;
	int16_t file_data[128];		// 256 bytes / 2 IQ / 16 bits = 64 samples
	complex8_t iq_data[256];	// 256 samples
	uint64_t file_offset = 0;
	uint8_t data_put = 0, data_get = 0;
	int16_t f_re, f_im;
	int8_t re, im;
	uint8_t level, bit, byte;
	Color mark_color;
	size_t preamble_count = 0, null_count = 0, bit_count = 0, sample_count = 0;
	bool decoding = false;
	float prev_mag, mag;
	float threshold, threshold_low, threshold_high;
	std::string bits;
	std::string hex_str;
	bool confidence, first_in_window, last_in_window;
	uint8_t frame_index;
	uint8_t adsb_frame[256];
	
	// Use vectors and std::rotate !
	float shifter[16];
	uint8_t shifter_bits[16];
	
	iq_file.seek(offset * 256);
	
	for (;;) {
		if (data_put == data_get) {
			auto result = iq_file.read(file_data, 256);
			if (!result.is_error()) {
				// Convert file's C16 to C8
				for (c = 0; c < (result.value() / 4); c++) {
					f_re = file_data[(c * 2) + 0] >> 5;
					f_im = file_data[(c * 2) + 1] >> 5;
					iq_data[data_put] = { f_re, f_im };
					data_put++;
				}
				
				file_offset += result.value();
				
				if (file_offset >= 4096) {
					text_debug_e.set("Read @ " + to_string_dec_uint(offset * 256 / 2000 / 4) + "ms    ");
					break;
				}
			} else {
				text_debug_a.set("Read error");
				return;
			}
		}
		
		re = iq_data[data_get].real();
		im = iq_data[data_get].imag();
		mag = __builtin_sqrtf((re * re) + (im * im)) * k;
		data_get++;
		
		// Only used for preamble detection and visualisation
		level = (mag < 0.3) ? 0 :		// Blank weak signals
					(mag > prev_mag) ? 1 : 0;
		
		if (decoding) {
			// Decode
			mark_color = Color::grey();
			if (sample_count & 1) {
				if ((prev_mag < threshold_low) && (mag < threshold_low)) {
					// Both under window, silence.
					mark_color = Color::black();
					if (null_count > 3) {
						text_debug_b.set("Bits:" + bits.substr(0, 25));
						text_debug_c.set("Hex:" + hex_str.substr(0, 26));
						text_debug_d.set("DF=" + to_string_dec_uint(adsb_frame[0] >> 3) + " ICAO=" + to_string_hex((adsb_frame[1] << 16) + (adsb_frame[2] << 8) + adsb_frame[3], 6));
						decoding = false;
					} else
						null_count++;
						
					confidence = false;
					if (prev_mag > mag)
						bit = 1;
					else
						bit = 0;
						
					mark_color = bit ? Color::dark_red() : Color::dark_green();
					
				} else {
					
					null_count = 0;
				
					first_in_window = ((prev_mag >= threshold_low) && (prev_mag <= threshold_high));
					last_in_window = ((mag >= threshold_low) && (mag <= threshold_high));
					
					if ((first_in_window && !last_in_window) || (!first_in_window && last_in_window)) {
						confidence = true;
						if (prev_mag > mag)
							bit = 1;
						else
							bit = 0;
					} else {
						confidence = false;
						if (prev_mag > mag)
							bit = 1;
						else
							bit = 0;
					}
				
					mark_color = bit ? Color::red() : Color::green();
				}
				
				bits.append(bit ? "1" : "0");
				byte = bit | (byte << 1);
				bit_count++;
				if (!(bit_count & 7)) {
					// Got one byte
					hex_str += to_string_hex(byte, 2);
					adsb_frame[frame_index++] = byte;
				}
			}
			sample_count++;
		} else {
			// Look for preamble
			mark_color = Color::white();
			
			// TODO: Stop using 2 separate shift registers for float and bit...
			for (c = 0; c < 15; c++)
				shifter[c] = shifter[c + 1];
			shifter[15] = mag;
			
			for (c = 0; c < 15; c++)
				shifter_bits[c] = shifter_bits[c + 1];
			shifter_bits[15] = level;
			
			if (!memcmp(shifter_bits, adsb_preamble, 16)) {
				preamble_count++;
				if (preamble_count == 1) {
					// Try decoding the first frame found
					decoding = true;
					sample_count = 0;
					frame_index = 0;
					
					// Compute preamble pulses power to set thresholds
					threshold = (shifter[0] + shifter[2] + shifter[7] + shifter[9]) / 4;
					threshold_high = threshold * 1.414;		// +3dB
					threshold_low = threshold * 0.707;		// -3dB
				}
			}
		}
		
		prev_mag = mag;
		
		/*if (!lcd_y && !lcd_x) {
			for (c = 0; c < 16; c++)
				display.fill_rectangle({c * 4, 300, 4, 16}, shifter[c] ? Color::white() : Color::blue());
		}*/
		
		if (lcd_y < 188) {
			mag *= 16;
			// Background
			display.fill_rectangle({lcd_x, 100 + lcd_y, 2, 32 - mag}, decoding ? mark_color : Color::grey());
			// Bar
			display.fill_rectangle({lcd_x, 132 + lcd_y - mag, 2, mag}, Color::white());
			// Level
			display.fill_rectangle({lcd_x, 132 + lcd_y, 2, 4}, decoding ? ((sample_count & 1) ? Color::white() : Color::light_grey()) : (level ? Color::white() : Color::dark_blue()));
			if (lcd_x == 238) {
				lcd_x = 0;
				lcd_y += 40;
			} else {
				lcd_x += 2;
			}
		}
	}
	
	text_debug_a.set("Found " + to_string_dec_uint(preamble_count) + " preambles   ");
}

ADSBRxView::ADSBRxView(NavigationView& nav) {
	uint32_t c;
	
	//baseband::run_image(portapack::spi_flash::image_tag_adsb_rx);

	add_children({
		&labels,
		&offset_field,
		&button_ffw,
		&text_debug_a,
		&text_debug_b,
		&text_debug_c,
		&text_debug_d,
		&text_debug_e
	});
	
	// File must be 16bit complex @ 2Msps !
	
	auto result = iq_file.open("ADSB.C16");
	if (result.is_valid()) {
		text_debug_a.set("Can't open file");
	}
	
	offset_field.on_change = [this, &nav](int32_t value) {
		analyze(value);
	};
	
	button_ffw.on_select = [this, &nav](Button&) {
		offset_field.set_value(offset_field.value() + 1562);
	};
}

} /* namespace ui */
