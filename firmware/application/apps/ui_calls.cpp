/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_search.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

template<>
void RecentEntriesTable<SearchRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	std::string str_duration = "";
	
	if (entry.duration < 600)
		str_duration = to_string_dec_uint(entry.duration / 10) + "." + to_string_dec_uint(entry.duration % 10) + "s";
	else
		str_duration = to_string_dec_uint(entry.duration / 600) + "m" + to_string_dec_uint((entry.duration / 10) % 60) + "s";
		
	str_duration.resize(target_rect.width() / 8, ' ');
	
	painter.draw_string(target_rect.location(), style, to_string_short_freq(entry.frequency) + " " + entry.time + " " + str_duration);
}

void SearchView::focus() {
	field_frequency_min.focus();
}

SearchView::~SearchView() {
	receiver_model.disable();
	baseband::shutdown();
}

void SearchView::do_detection() {
	uint8_t power_max = 0;
	int32_t bin_max = -1;
	uint32_t slice_max = 0;
	uint32_t snap_value;
	uint8_t power;
	rtc::RTC datetime;
	std::string str_approx, str_timestamp;
	
	// Display spectrum
	bin_skip_acc = 0;
	pixel_index = 0;
	display.draw_pixels(
		{ { 0, 88 }, { (Dim)spectrum_row.size(), 1 } },
		spectrum_row
	);
	
	mean_power = mean_acc / (SEARCH_BIN_NB_NO_DC * slices_nb);
	mean_acc = 0;
	
	overall_power_max = 0;
	
	// Find max power over threshold for all slices
	for (size_t slice = 0; slice < slices_nb; slice++) {
		power = slices[slice].max_power;
		if (power > overall_power_max)
			overall_power_max = power;
		
		if ((power >= mean_power + power_threshold) && (power > power_max)) {
			power_max = power;
			bin_max = slices[slice].max_index;
			slice_max = slice;
		}
	}
	
	// Lock / release
	if ((bin_max >= last_bin - 2) && (bin_max <= last_bin + 2) && (bin_max > -1) && (slice_max == last_slice)) {
		
		// Staying around the same bin
		if (detect_timer >= DETECT_DELAY) {
			if ((bin_max != locked_bin) || (!locked)) {
				
				if (!locked) {
					resolved_frequency = slices[slice_max].center_frequency + (SEARCH_BIN_WIDTH * (bin_max - 128));
					
					if (check_snap.value()) {
						snap_value = options_snap.selected_index_value();
						resolved_frequency = round(resolved_frequency / snap_value) * snap_value;
					}
					
					// Check range
					if ((resolved_frequency >= f_min) && (resolved_frequency <= f_max)) {
						
						duration = 0;
						
						auto& entry = ::on_packet(recent, resolved_frequency);
						
						rtcGetTime(&RTCD1, &datetime);
						str_timestamp = to_string_dec_uint(datetime.hour(), 2, '0') + ":" +
										to_string_dec_uint(datetime.minute(), 2, '0') + ":" +
										to_string_dec_uint(datetime.second(), 2, '0');
						entry.set_time(str_timestamp);
						recent_entries_view.set_dirty();

						text_infos.set("Locked ! ");
						big_display.set_style(&style_locked);
						
						locked = true;
						locked_bin = bin_max;
						
						// TODO
						/*nav_.pop();
						receiver_model.disable();
						baseband::shutdown();
						nav_.pop();*/

						/*if (options_goto.selected_index() == 1)
							nav_.push<AnalogAudioView>(false);
						else if (options_goto.selected_index() == 2)
							nav_.push<POCSAGAppView>();
						*/
					} else
						text_infos.set("Out of range");
				}
				
				big_display.set(resolved_frequency);
			}
		}
		release_timer = 0;
	} else {
		detect_timer = 0;
		if (locked) {
			if (release_timer >= RELEASE_DELAY) {
				locked = false;
				
				auto& entry = ::on_packet(recent, resolved_frequency);
				entry.set_duration(duration);
				recent_entries_view.set_dirty();
				
				text_infos.set("Listening");
				big_display.set_style(&style_grey);
			}
		}
	}
	
	last_bin = bin_max;
	last_slice = slice_max;
	search_counter++;
	
	// Refresh red tick
	portapack::display.fill_rectangle({last_tick_pos, 90, 1, 6}, Color::black());
	if (bin_max > -1) {
		last_tick_pos = (Coord)(bin_max / slices_nb);
		portapack::display.fill_rectangle({last_tick_pos, 90, 1, 6}, Color::red());
	}
}

void SearchView::add_spectrum_pixel(Color color) {
	// Is avoiding floats really necessary ?
	bin_skip_acc += bin_skip_frac;
	if (bin_skip_acc < 0x10000) 
		return;
	
	bin_skip_acc -= 0x10000;
	
	if (pixel_index < 240)
		spectrum_row[pixel_index++] = color;
}

void SearchView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	uint8_t max_power = 0;
	int16_t max_bin = 0;
	uint8_t power;
	size_t bin;
	
	baseband::spectrum_streaming_stop();
	
	// Add pixels to spectrum display and find max power for this slice
	// Center 12 bins are ignored (DC spike is blanked)
	// Leftmost and rightmost 2 bins are ignored
	for (bin = 0; bin < 256; bin++) {

		if ((bin < 2) || (bin > 253) || ((bin >= 122) && (bin < 134))) {
			power = 0;
		} else {
			if (bin < 128)
				power = spectrum.db[128 + bin];
			else
				power = spectrum.db[bin - 128];
		}
		
		add_spectrum_pixel(spectrum_rgb3_lut[power]);
		
		mean_acc += power;
		if (power > max_power) {
			max_power = power;
			max_bin = bin;
		}
	}
	
	slices[slice_counter].max_power = max_power;
	slices[slice_counter].max_index = max_bin;
	
	if (slices_nb > 1) {
		// Slice sequence
		if (slice_counter >= slices_nb) {
			do_detection();
			slice_counter = 0;
		} else
			slice_counter++;
		receiver_model.set_tuning_frequency(slices[slice_counter].center_frequency);
		baseband::set_spectrum(SEARCH_SLICE_WIDTH, 31);	// Clear
	} else {
		// Unique slice
		do_detection();
	}
	
	baseband::spectrum_streaming_start();
}

void SearchView::on_show() {
	baseband::spectrum_streaming_start();
}

void SearchView::on_hide() {
	baseband::spectrum_streaming_stop();
}

void SearchView::on_range_changed() {
	rf::Frequency slices_span, center_frequency;
	int64_t offset;
	size_t slice;

	f_min = field_frequency_min.value();
	f_max = field_frequency_max.value();
	search_span = abs(f_max - f_min);
	
	if (search_span > SEARCH_SLICE_WIDTH) {
		// ex: 100M~115M (15M span):
		// slices_nb = (115M-100M)/2.5M = 6
		slices_nb = (search_span + SEARCH_SLICE_WIDTH - 1) / SEARCH_SLICE_WIDTH;
		if (slices_nb > 32) {
			text_slices.set("!!");
			slices_nb = 32;
		} else {
			text_slices.set(to_string_dec_uint(slices_nb, 2, ' '));
		}
		// slices_span = 6 * 2.5M = 15M
		slices_span = slices_nb * SEARCH_SLICE_WIDTH;
		// offset = 0 + 2.5/2 = 1.25M
		offset = ((search_span - slices_span) / 2) + (SEARCH_SLICE_WIDTH / 2);
		// slice_start = 100M + 1.25M = 101.25M
		center_frequency = std::min(f_min, f_max) + offset;
		
		for (slice = 0; slice < slices_nb; slice++) {
			slices[slice].center_frequency = center_frequency;
			center_frequency += SEARCH_SLICE_WIDTH;
		}
	} else {
		slices[0].center_frequency = (f_max + f_min) / 2;
		receiver_model.set_tuning_frequency(slices[0].center_frequency);

		slices_nb = 1;
		text_slices.set(" 1");
	}
	
	bin_skip_frac = 0xF000 / slices_nb;

	slice_counter = 0;
}

void SearchView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void SearchView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void SearchView::do_timers() {
	
	if (timing_div >= 60) {
		// ~1Hz
		
		timing_div = 0;
		
		// Update scan rate
		text_rate.set(to_string_dec_uint(search_counter, 3));
		search_counter = 0;
	}
	
	if (timing_div % 12 == 0) {
		// ~5Hz
		
		// Update power levels
		text_mean.set(to_string_dec_uint(mean_power, 3));
		
		vu_max.set_value(overall_power_max);
		vu_max.set_mark(mean_power + power_threshold);
	}
	
	if (timing_div % 6 == 0) {
		// ~10Hz
		
		// Update timing indicator
		if (locked) {
			progress_timers.set_max(RELEASE_DELAY);
			progress_timers.set_value(RELEASE_DELAY - release_timer);
		} else {
			progress_timers.set_max(DETECT_DELAY);
			progress_timers.set_value(detect_timer);
		}
		
		// Increment timers
		if (detect_timer < DETECT_DELAY) detect_timer++;
		if (release_timer < RELEASE_DELAY) release_timer++;
		
		if (locked) duration++;
	}

	timing_div++;
}

SearchView::SearchView(
	NavigationView& nav
) : nav_ (nav)
{
	baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
	
	add_children({
		&labels,
		&field_frequency_min,
		&field_frequency_max,
		&field_lna,
		&field_vga,
		&field_threshold,
		&text_mean,
		&text_slices,
		&text_rate,
		&text_infos,
		&vu_max,
		&progress_timers,
		&check_snap,
		&options_snap,
		&big_display,
		&recent_entries_view
	});
	
	baseband::set_spectrum(SEARCH_SLICE_WIDTH, 31);
	
	recent_entries_view.set_parent_rect({ 0, 28 * 8, 240, 12 * 8 });
	recent_entries_view.on_select = [this, &nav](const SearchRecentEntry& entry) {
		nav.push<FrequencyKeypadView>(entry.frequency);
	};
	
	text_mean.set_style(&style_grey);
	text_slices.set_style(&style_grey);
	text_rate.set_style(&style_grey);
	progress_timers.set_style(&style_grey);
	big_display.set_style(&style_grey);
	
	check_snap.set_value(true);
	options_snap.set_selected_index(1);		// 12.5kHz
	
	field_threshold.set_value(80);
	field_threshold.on_change = [this](int32_t value) {
		power_threshold = value;
	};

	field_frequency_min.set_value(receiver_model.tuning_frequency() - 1000000);
	field_frequency_min.set_step(100000);
	field_frequency_min.on_change = [this](rf::Frequency) {
		this->on_range_changed();
	};
	field_frequency_min.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->field_frequency_min.set_value(f);
		};
	};
	
	field_frequency_max.set_value(receiver_model.tuning_frequency() + 1000000);
	field_frequency_max.set_step(100000);
	field_frequency_max.on_change = [this](rf::Frequency) {
		this->on_range_changed();
	};
	field_frequency_max.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->field_frequency_max.set_value(f);
		};
	};
	
	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};
	
	progress_timers.set_max(DETECT_DELAY);
	
	on_range_changed();

	receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(SEARCH_SLICE_WIDTH);
	receiver_model.set_baseband_bandwidth(2500000);
	receiver_model.enable();
}

} /* namespace ui */
