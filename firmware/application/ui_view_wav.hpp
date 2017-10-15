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

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "io_wave.hpp"

namespace ui {

class ViewWavView : public View {
public:
	ViewWavView(NavigationView& nav);

	void focus() override;
	void paint(Painter&) override;

	std::string title() const override { return "WAV viewer"; };

private:
	NavigationView& nav_;
	static constexpr uint32_t subsampling_factor = 8;
	
	void update_scale(int32_t new_scale);
	void refresh_waveform();
	void on_pos_changed();
	void load_wav(std::filesystem::path file_path);
	void reset_controls();
	
	const Color amplitude_colors[16] = {
		{ 0x00, 0x3F, 0xB0 },
		{ 0x00, 0x6D, 0xB5 },
		{ 0x00, 0x9C, 0xBA },
		{ 0x00, 0xBF, 0xB0 },
		{ 0x00, 0xC5, 0x86 },
		{ 0x00, 0xCA, 0x5A },
		{ 0x00, 0xCF, 0x2A },
		{ 0x06, 0xD4, 0x00 },
		{ 0x3A, 0xDA, 0x00 },
		{ 0x71, 0xDF, 0x00 },
		{ 0xAA, 0xE4, 0x00 },
		{ 0xE6, 0xE9, 0x00 },
		{ 0xEF, 0xB9, 0x00 },
		{ 0xF4, 0x83, 0x00 },
		{ 0xF9, 0x4B, 0x00 },
		{ 0xFF, 0x0F, 0x00 }
	};

	std::unique_ptr<WAVFileReader> wav_reader { };
	
	int8_t waveform_buffer[240] { };
	uint8_t amplitude_buffer[240] { };
	int32_t scale { 1 };
	uint64_t ns_per_pixel { };
	uint64_t position { };
	
	Labels labels {
		{ { 0 * 8, 0 * 16 }, "File:", Color::light_grey() },
		{ { 0 * 8, 1 * 16 }, "Samplerate:", Color::light_grey() },
		{ { 0 * 8, 2 * 16 }, "Title:", Color::light_grey() },
		{ { 0 * 8, 3 * 16 }, "Duration:", Color::light_grey() },
		{ { 0 * 8, 11 * 16 }, "Position:   s         Scale:", Color::light_grey() },
		{ { 0 * 8, 12 * 16 }, "Cursor A:", Color::dark_cyan() },
		{ { 0 * 8, 13 * 16 }, "Cursor B:", Color::dark_magenta() },
		{ { 0 * 8, 14 * 16 }, "Delta:", Color::light_grey() }
	};
	
	Text text_filename {
		{ 5 * 8, 0 * 16, 12 * 8, 16 },
		""
	};
	Text text_samplerate {
		{ 11 * 8, 1 * 16, 8 * 8, 16 },
		""
	};
	Text text_title {
		{ 6 * 8, 2 * 16, 18 * 8, 16 },
		""
	};
	Text text_duration {
		{ 9 * 8, 3 * 16, 18 * 8, 16 },
		""
	};
	Button button_open {
		{ 24 * 8, 8, 6 * 8, 2 * 16 },
		"Open"
	};

	Waveform waveform {
		{ 0, 5 * 16, 240, 64 },
		waveform_buffer,
		240,
		0,
		false,
		Color::white()
	};
	
	NumberField field_pos_seconds {
		{ 9 * 8, 11 * 16 },
		3,
		{ 0, 999 },
		1,
		' '
	};
	NumberField field_pos_samples {
		{ 14 * 8, 11 * 16 },
		6,
		{ 0, 999999 },
		1,
		'0'
	};
	NumberField field_scale {
		{ 28 * 8, 11 * 16 },
		2,
		{ 1, 40 },
		1,
		' '
	};
	
	NumberField field_cursor_a {
		{ 9 * 8, 12 * 16 },
		3,
		{ 0, 239 },
		1,
		' ',
		true
	};
	
	NumberField field_cursor_b {
		{ 9 * 8, 13 * 16 },
		3,
		{ 0, 239 },
		1,
		' ',
		true
	};
	
	Text text_delta {
		{ 6 * 8, 14 * 16, 30 * 8, 16 },
		"-"
	};
};

} /* namespace ui */
