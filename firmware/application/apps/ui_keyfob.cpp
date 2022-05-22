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

#include "ui_keyfob.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

uint8_t KeyfobView::subaru_get_checksum() {
	uint8_t checksum = 0;
	
	for (size_t i = 0; i < 9; i++) {
		checksum ^= (frame[i] & 0x0F);			// 00 11 22 33 44 55 66 77 88 9-
		checksum ^= ((frame[i] >> 4) & 0x0F);
	}
	
	checksum ^= ((frame[9] >> 4) & 0x0F);
	checksum++;
	checksum &= 0x0F;
	
	return checksum;
}

bool KeyfobView::subaru_is_valid() {
	if (frame[0] != 0x55)
		return false;
	
	if (subaru_get_checksum() != (frame[9] & 0x0F))
		return false;
	
	return true;
}

uint16_t KeyfobView::subaru_get_code() {
	// 77777777 88888888 9999
	return (frame[7] << 12) | (frame[8] << 4) | (frame[9] >> 4);
}

void KeyfobView::subaru_set_code(const uint16_t code) {
	frame[7] = (code >> 12) & 0xFF;
	frame[8] = (code >> 4) & 0xFF;
	frame[9] &= 0x0F;
	frame[9] |= (code & 0x0F) << 4;
}

int32_t KeyfobView::subaru_get_command() {
	uint32_t command_a = frame[5] & 0x0F;
	uint32_t command_b = frame[6] & 0x0F;
	
	if (command_a != command_b)
		return -1;
	
	return command_a;
}

void KeyfobView::subaru_set_command(const uint32_t command) {
	frame[5] &= 0xF0;
	frame[5] |= command;
	frame[6] &= 0xF0;
	frame[6] |= command;
}

void KeyfobView::generate_payload(size_t& bitstream_length) {
	for (size_t i = 0; i < (10 * 8); i++) {
		if (frame[i >> 3] & (1 << (7 - (i & 7))))
			bitstream_append(bitstream_length, 2, 0b10);
		else
			bitstream_append(bitstream_length, 2, 0b01);
	}
}

size_t KeyfobView::generate_frame() {
	size_t bitstream_length = 0;
	uint64_t payload;
	
	// Symfield word to frame
	payload = field_payload_a.value_hex_u64();
	for (size_t i = 0; i < 5; i++) {
		frame[4 - i] = payload & 0xFF;
		payload >>= 8;
	}
	payload = field_payload_b.value_hex_u64();
	for (size_t i = 0; i < 5; i++) {
		frame[9 - i] = payload & 0xFF;
		payload >>= 8;
	}

	// Recompute checksum
	frame[9] = (frame[9] & 0xF0) | subaru_get_checksum();
	update_symfields();
	
	// Preamble: 128x 01
	for (size_t i = 0; i < 128; i++)
		bitstream_append(bitstream_length, 2, 0b01);

	// Space: 4x 0
	bitstream_append(bitstream_length, 4, 0b0000);

	// Payload
	generate_payload(bitstream_length);

	// Space: 8x 0
	bitstream_append(bitstream_length, 8, 0b00000000);

	// Payload again
	generate_payload(bitstream_length);
	
	return bitstream_length;
}

void KeyfobView::focus() {
	options_make.focus();
}

KeyfobView::~KeyfobView() {
	// save app settings
	settings.save("tx_keyfob", &app_settings);

	transmitter_model.disable();
	baseband::shutdown();
}

void KeyfobView::update_progress(const uint32_t progress) {
	text_status.set("Repeat #" + to_string_dec_uint(progress));
}

void KeyfobView::on_tx_progress(const uint32_t progress, const bool done) {
	if (!done) {
		// Repeating...
		update_progress(progress + 1);
		progressbar.set_value(progress);
	} else {
		transmitter_model.disable();
		text_status.set("Done");
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	}
}

void KeyfobView::on_make_change(size_t index) {
	(void)index;
}

// DEBUG
void KeyfobView::update_symfields() {
	for (size_t i = 0; i < 5; i++) {
		field_payload_a.set_sym(i << 1, frame[i] >> 4);
		field_payload_a.set_sym((i << 1) + 1, frame[i] & 0x0F);
	}
	for (size_t i = 0; i < 5; i++) {
		field_payload_b.set_sym(i << 1, frame[5 + i] >> 4);
		field_payload_b.set_sym((i << 1) + 1, frame[5 + i] & 0x0F);
	}
}

void KeyfobView::on_command_change(uint32_t value) {
	subaru_set_command(value);
	update_symfields();
}

void KeyfobView::start_tx() {
	progressbar.set_max(repeats - 1);
	
	update_progress(1);
	
	size_t bitstream_length = generate_frame();

	transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_ook_data(
		bitstream_length,
		subaru_samples_per_bit,
		repeats,
		200		// Pause symbols
	);
}

KeyfobView::KeyfobView(
	NavigationView& nav
) : nav_ { nav }
{
	baseband::run_image(portapack::spi_flash::image_tag_ook);

	add_children({
		&labels,
		&options_make,
		&options_command,
		&field_payload_a,
		&field_payload_b,
		&text_status,
		&progressbar,
		&tx_view
	});
	
	// load app settings
	auto rc = settings.load("tx_keyfob", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	frame[0] = 0x55;
	update_symfields();
	
	options_make.on_change = [this](size_t index, int32_t) {
		on_make_change(index);
	};
	
	options_command.on_change = [this](size_t, int32_t value) {
		on_command_change(value);
	};
	
	options_make.set_selected_index(0);
	
	transmitter_model.set_tuning_frequency(433920000);	// Fixed 433.92MHz
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
	};
}

} /* namespace ui */
