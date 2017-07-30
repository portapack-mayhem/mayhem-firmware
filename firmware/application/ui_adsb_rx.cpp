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
#include "ui_geomap.hpp"

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

template<>
void RecentEntriesTable<ADSBRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	painter.draw_string(target_rect.location(), style, to_string_hex_array((uint8_t*)entry.raw_data, 10) + " " + entry.time);
}

void ADSBRxView::focus() {
	offset_field.focus();
	offset_field.set_value(13179);
}

ADSBRxView::~ADSBRxView() {
	receiver_model.disable();
	baseband::shutdown();
}

/*bool ADSBRxView::analyze(uint64_t offset) {
	Coord lcd_x = 0, lcd_y = 0;
	int8_t re, im;
	adsb_frame frame;
	int16_t file_data[128];		// 256 bytes / 2 IQ / 16 bits = 64 samples
	complex8_t iq_data[256];	// 256 samples
	uint64_t file_offset = 0;
	uint8_t data_put = 0, data_get = 0;
	int16_t f_re, f_im;
	uint32_t c;
	uint8_t level, bit, byte;
	Color mark_color;
	size_t preamble_count = 0, null_count = 0, bit_count = 0, sample_count = 0;
	bool decoding = false;
	float prev_mag = 0, mag;
	float threshold, threshold_low, threshold_high;
	std::string bits;
	std::string hex_str;
	bool confidence, first_in_window, last_in_window;
	std::pair<float, uint8_t> shifter[ADSB_PREAMBLE_LENGTH];
	
	iq_file.seek(offset * 2048);	// 256
	
	for (;;) {
		if (data_put == data_get) {
			auto result = iq_file.read(file_data, 256);
			if (!result.is_error()) {
				// Convert file's C16 to C8
				for (c = 0; c < (result.value() / 4); c++) {
					f_re = file_data[(c * 2) + 0] >> 5;		// >> 8 (<< 3 amp.)
					f_im = file_data[(c * 2) + 1] >> 5;
					iq_data[data_put] = { (int8_t)f_re, (int8_t)f_im };
					data_put++;
				}
				
				file_offset += result.value();
				
				if (file_offset >= 2048) {
					//text_debug_e.set("Read @ " + to_string_dec_uint(offset * 256 / 2000 / 4) + "ms    ");
					break;
				}
			} else {
				text_debug_a.set("Read error");
				return false;
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
			
			// 1 bit lasts 2 samples
			if (sample_count & 1) {
				if ((prev_mag < threshold_low) && (mag < threshold_low)) {
					// Both under window, silence.
					mark_color = Color::black();
					if (null_count > 3) {
						text_debug_b.set("Bits:" + bits.substr(0, 25));
						text_debug_c.set("Hex:" + hex_str.substr(0, 26));
						text_debug_d.set("DF=" + to_string_dec_uint(frame.get_DF()) + " ICAO=" + to_string_hex(frame.get_ICAO_address(), 6));
						if ((frame.get_DF() == 17) && (frame.get_msg_type() >= 1) && (frame.get_msg_type() <= 4)) {
							text_debug_a.set("Callsign:" + frame.get_callsign());
							return true;
						} else {
							text_debug_a.set("No ID data");
							return false;
						}
							
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
				
				bits.append(bit ? "1" : "0");			// DEBUG
				byte = bit | (byte << 1);
				bit_count++;
				if (!(bit_count & 7)) {
					// Got one byte
					hex_str += to_string_hex(byte, 2);	// DEBUG
					frame.push_byte(byte);
				}
			}
			sample_count++;
		} else {
			// Look for preamble
			mark_color = Color::white();
			
			// Shift
			for (c = 0; c < (ADSB_PREAMBLE_LENGTH - 1); c++)
				shifter[c] = shifter[c + 1];
			shifter[15] = std::make_pair(mag, level);
			
			// Compare
			for (c = 0; c < ADSB_PREAMBLE_LENGTH; c++) {
				if (shifter[c].second != adsb_preamble[c])
					break;
			}
				
			if (c == ADSB_PREAMBLE_LENGTH) {
				preamble_count++;
				if (preamble_count == 1) {
					// Try decoding the first frame found
					decoding = true;
					sample_count = 0;
					
					// Compute preamble pulses power to set thresholds
					threshold = (shifter[0].first + shifter[2].first + shifter[7].first + shifter[9].first) / 4;
					threshold_high = threshold * 1.414;		// +3dB
					threshold_low = threshold * 0.707;		// -3dB
				}
			}
		}
		
		prev_mag = mag;
		
		if (preamble_count) {
			if (lcd_y < 188) {
				mag *= 16;
				// Background
				display.fill_rectangle({lcd_x, 100 + lcd_y, 2, 32 - (int)mag}, decoding ? mark_color : Color::grey());
				// Bar
				display.fill_rectangle({lcd_x, 132 + lcd_y - (int)mag, 2, (int)mag}, Color::white());
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
	}
	
	return false;
}*/

void ADSBRxView::on_frame(const ADSBFrameMessage * message) {
	rtc::RTC datetime;
	std::string str_timestamp;
	auto frame = message->frame;
	auto& entry = ::on_packet(recent, frame.get_ICAO_address());
	
	rtcGetTime(&RTCD1, &datetime);
	str_timestamp = to_string_dec_uint(datetime.hour(), 2, '0') + ":" +
					to_string_dec_uint(datetime.minute(), 2, '0') + ":" +
					to_string_dec_uint(datetime.second(), 2, '0');
	entry.set_time(str_timestamp);
	entry.set_raw(frame.get_raw_data());
	recent_entries_view.set_dirty();
}

ADSBRxView::ADSBRxView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_adsb_rx);

	add_children({
		//&labels,
		&offset_field,
		//&button_ffw,
		&text_debug_a,
		&text_debug_b,
		&text_debug_c,
		&recent_entries_view
	});
	
	recent_entries_view.set_parent_rect({ 0, 64, 240, 224 });
	
	// File must be 16bit complex @ 2Msps !
	/*auto result = iq_file.open("ADSB.C16");
	if (result.is_valid()) {
		text_debug_a.set("Can't open file");
	}*/

	/*offset_field.on_change = [this, &nav](int32_t value) {
		// TODO
	};*/
	
	/*button_ffw.on_select = [this, &nav](Button&) {
		//nav.push<GeoMapView>(GeoMapView::Mode::SHOW);
		while (!analyze(f_offset)) {
			f_offset++;
		}
		offset_field.set_value(f_offset);
		f_offset++;
	};*/
	
	baseband::set_adsb();
	
	receiver_model.set_tuning_frequency(1090000000);
	receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(2000000);
	receiver_model.set_baseband_bandwidth(2500000);
	receiver_model.enable();
}

} /* namespace ui */
