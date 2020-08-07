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

#include "ui_view_wav.hpp"
#include "ui_fileman.hpp"

using namespace portapack;

#include "string_format.hpp"

namespace ui {

void ViewWavView::update_scale(int32_t new_scale) {
	scale = new_scale;
	ns_per_pixel = (1000000000UL / wav_reader->sample_rate()) * scale;
	refresh_waveform();
	refresh_measurements();
}

void ViewWavView::refresh_waveform() {
	for (size_t i = 0; i < 240; i++) {
		wav_reader->data_seek(position + (i * scale));
		wav_reader->read(&waveform_buffer[i], sizeof(int16_t));
	}
	
	waveform.set_dirty();
	
	// Window
	uint64_t w_start = (position * 240) / wav_reader->sample_count();
	uint64_t w_width = (scale * 240) / (wav_reader->sample_count() / 240);
	display.fill_rectangle({ 0, 10 * 16 + 1, 240, 16 }, Color::black());
	display.fill_rectangle({ (Coord)w_start, 21 * 8, (Dim)w_width + 1, 8 }, Color::white());
	display.draw_line({ 0, 10 * 16 + 1 }, { (Coord)w_start, 21 * 8 }, Color::white());
	display.draw_line({ 239, 10 * 16 + 1 }, { (Coord)(w_start + w_width), 21 * 8 }, Color::white());
}

void ViewWavView::refresh_measurements() {
	uint64_t span_ns = ns_per_pixel * abs(field_cursor_b.value() - field_cursor_a.value());
	
	if (span_ns)
		text_delta.set(unit_auto_scale(span_ns, 0, 3) + "s (" + to_string_dec_uint(1000000000UL / span_ns) + "Hz)");
	else
		text_delta.set("0us ?Hz");
}

void ViewWavView::paint(Painter& painter) {
	// Waveform limits
	painter.draw_hline({ 0, 6 * 16 - 1 }, 240, Color::grey());
	painter.draw_hline({ 0, 10 * 16 }, 240, Color::grey());
	
	// Overall amplitude view, 0~127 to 0~255 color index
	for (size_t i = 0; i < 240; i++)
		painter.draw_vline({ (Coord)i, 11 * 16 }, 8, spectrum_rgb2_lut[amplitude_buffer[i] << 1]);
}

void ViewWavView::on_pos_changed() {
	position = (field_pos_seconds.value() * wav_reader->sample_rate()) + field_pos_samples.value();
	refresh_waveform();
}

void ViewWavView::load_wav(std::filesystem::path file_path) {
	int16_t sample;
	uint32_t average;

	
	text_filename.set(file_path.filename().string());
	auto ms_duration = wav_reader->ms_duration();
	text_duration.set(unit_auto_scale(ms_duration, 2, 3) + "s");
	
	wav_reader->rewind();
	
	text_samplerate.set(to_string_dec_uint(wav_reader->sample_rate()) + "Hz");
	text_title.set(wav_reader->title());
	
	// Fill amplitude buffer, world's worst downsampling
	uint64_t skip = wav_reader->sample_count() / (240 * subsampling_factor);
	
	for (size_t i = 0; i < 240; i++) {
		average = 0;
		
		for (size_t s = 0; s < subsampling_factor; s++) {
			wav_reader->data_seek(((i * subsampling_factor) + s) * skip);
			wav_reader->read(&sample, 2);
			average += (abs(sample) >> 8);
		}
		
		amplitude_buffer[i] = average / subsampling_factor;
	}
	
	reset_controls();
	update_scale(1);
}

void ViewWavView::reset_controls() {
	field_scale.set_value(1);
	field_pos_seconds.set_value(0);
	field_pos_samples.set_value(0);
	field_cursor_a.set_value(0);
	field_cursor_b.set_value(0);
}
	
ViewWavView::ViewWavView(
	NavigationView& nav
) : nav_(nav)
{
	wav_reader = std::make_unique<WAVFileReader>();
	
	add_children({
		&labels,
		&text_filename,
		&text_samplerate,
		&text_title,
		&text_duration,
		&button_open,
		&waveform,
		&field_pos_seconds,
		&field_pos_samples,
		&field_scale,
		&field_cursor_a,
		&field_cursor_b,
		&text_delta
	});
	reset_controls();
	button_open.on_select = [this, &nav](Button&) {
		auto open_view = nav.push<FileLoadView>(".WAV");
		open_view->on_changed = [this](std::filesystem::path file_path) {
			if (!wav_reader->open(file_path)) {
		nav_.display_modal("Error", "Couldn't open file.", INFO, nullptr);
		return;
	}
	if ((wav_reader->channels() != 1) || (wav_reader->bits_per_sample() != 16)) {
		nav_.display_modal("Error", "Wrong format.\nWav viewer only accepts\n16-bit mono files.", INFO, nullptr);
		return;
	}
			load_wav(file_path);
			field_pos_seconds.focus();
		};
	};
	
	field_scale.on_change = [this](int32_t value) {
		update_scale(value);
	};
	field_pos_seconds.on_change = [this](int32_t) {
		on_pos_changed();
	};
	field_pos_samples.on_change = [this](int32_t) {
		on_pos_changed();
	};
	
	field_cursor_a.on_change = [this](int32_t v) {
		waveform.set_cursor(0, v);
		refresh_measurements();
	};
	field_cursor_b.on_change = [this](int32_t v) {
		waveform.set_cursor(1, v);
		refresh_measurements();
	};
	
}

void ViewWavView::focus() {
	button_open.focus();
}

} /* namespace ui */
