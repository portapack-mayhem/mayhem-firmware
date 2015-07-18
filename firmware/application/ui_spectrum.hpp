/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_SPECTRUM_H__
#define __UI_SPECTRUM_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "spectrum_color_lut.hpp"

#include "lcd_ili9341.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>

namespace ui {
namespace spectrum {

class FrequencyScale : public Widget {
public:
	void set_spectrum_sampling_rate(const uint32_t new_sampling_rate, const size_t new_spectrum_bins) {
		if( (spectrum_sampling_rate != new_sampling_rate) ||
			(spectrum_bins != new_spectrum_bins) ) {
			spectrum_sampling_rate = new_sampling_rate;
			spectrum_bins = new_spectrum_bins;
			set_dirty();
		}
	}

	void set_channel_filter(
		const uint32_t pass_frequency,
		const uint32_t stop_frequency
	) {
		if( (channel_filter_pass_frequency != pass_frequency) ||
			(channel_filter_stop_frequency != stop_frequency) ) {
			channel_filter_pass_frequency = pass_frequency;
			channel_filter_stop_frequency = stop_frequency;
			set_dirty();
		}
	}

	void paint(Painter& painter) override {
		if( !spectrum_sampling_rate || !spectrum_bins ) {
			// Can't draw without non-zero scale.
			return;
		}

		const auto r = screen_rect();
		const auto x_center = r.width() / 2;

		/*
		const auto text = to_string_dec_int(bandwidth, 6);

		painter.draw_string(
			r.pos,
			style(),
			text
		);
		*/

		if( channel_filter_pass_frequency ) {
			const auto pass_offset = channel_filter_pass_frequency * spectrum_bins / spectrum_sampling_rate;
			const auto stop_offset = channel_filter_stop_frequency * spectrum_bins / spectrum_sampling_rate;

			const auto pass_x_lo = x_center - pass_offset;
			const auto pass_x_hi = x_center + pass_offset;

			if( channel_filter_stop_frequency ) {
				const auto stop_x_lo = x_center - stop_offset;
				const auto stop_x_hi = x_center + stop_offset;

				const Rect r_stop_lo {
					static_cast<Coord>(r.left() + stop_x_lo), r.top(),
					static_cast<Dim>(pass_x_lo - stop_x_lo), r.height()
				};
				painter.fill_rectangle(
					r_stop_lo,
					Color::yellow()
				);

				const Rect r_stop_hi {
					static_cast<Coord>(r.left() + pass_x_hi), r.top(),
					static_cast<Dim>(stop_x_hi - pass_x_hi), r.height()
				};
				painter.fill_rectangle(
					r_stop_hi,
					Color::yellow()
				);
			}

			const Rect r_pass {
				static_cast<Coord>(r.left() + pass_x_lo), r.top(),
				static_cast<Dim>(pass_x_hi - pass_x_lo), r.height()
			};
			painter.fill_rectangle(
				r_pass,
				Color::green()
			);
		}

		const Rect tick {
			static_cast<Coord>(r.left() + x_center), r.top(),
			1, r.height()
		};
		painter.fill_rectangle(tick, Color::white());

		const Rect bottom_separator {
			r.left(), static_cast<Coord>(r.bottom() - 1),
			r.width(), 1
		};
		painter.fill_rectangle(bottom_separator, Color::white());
	}

private:
	uint32_t spectrum_sampling_rate { 0 };
	size_t spectrum_bins { 0 };
	uint32_t channel_filter_pass_frequency { 0 };
	uint32_t channel_filter_stop_frequency { 0 };
};

class WaterfallView : public Widget {
public:
	void on_show() override {
		const auto screen_r = screen_rect();
		context().display.scroll_set_area(screen_r.top(), screen_r.bottom());
	}

	void on_hide() override {
		/* TODO: Clear region to eliminate brief flash of content at un-shifted
		 * position?
		 */
		context().display.scroll_disable();
	}

	void paint(Painter& painter) override {
		// Do nothing.
		(void)painter;
	}

	void on_channel_spectrum(
		const ChannelSpectrum& spectrum
	) {
		/* TODO: static_assert that message.spectrum.db.size() >= pixel_row.size() */
		const auto& db = *spectrum.db;

		std::array<Color, 240> pixel_row;
		for(size_t i=0; i<120; i++) {
			const auto pixel_color = spectrum_rgb3_lut[db[256 - 120 + i]];
			pixel_row[i] = pixel_color;
		}

		for(size_t i=120; i<240; i++) {
			const auto pixel_color = spectrum_rgb3_lut[db[i - 120]];
			pixel_row[i] = pixel_color;
		}

		auto& display = context().display;
		const auto draw_y = display.scroll(1);

		display.draw_pixels(
			{ { 0, draw_y }, { pixel_row.size(), 1 } },
			pixel_row
		);
	}
};

class WaterfallWidget : public View {
public:
	WaterfallWidget() {
		add_children({
			&waterfall_view,
			&frequency_scale,
		});
	}

	void on_show() override {
		context().message_map[Message::ID::ChannelSpectrum] = [this](const Message* const p) {
			this->on_channel_spectrum(reinterpret_cast<const ChannelSpectrumMessage*>(p)->spectrum);
		};
	}

	void on_hide() override {
		context().message_map[Message::ID::ChannelSpectrum] = nullptr;
	}

	void set_parent_rect(const Rect new_parent_rect) override {
		constexpr Dim scale_height = 16;

		View::set_parent_rect(new_parent_rect);
		frequency_scale.set_parent_rect({ 0, 0, new_parent_rect.width(), scale_height });
		waterfall_view.set_parent_rect({
			0, scale_height,
			new_parent_rect.width(),
			static_cast<Dim>(new_parent_rect.height() - scale_height)
		});
	}

	void paint(Painter& painter) override {
		// TODO:
		(void)painter;
	}

private:
	WaterfallView waterfall_view;
	FrequencyScale frequency_scale;

	void on_channel_spectrum(const ChannelSpectrum& spectrum) {
		waterfall_view.on_channel_spectrum(spectrum);
		frequency_scale.set_spectrum_sampling_rate(spectrum.sampling_rate, spectrum.db_count);

		// TODO: Set with actual information.
		//taps_64_lp_042_078_tfilter
		// TODO: Pass these details, don't hard-code them.
		// Channel spectrum is channel filter input, decimated by 2 by the
		// channel filter, then by 4 by the channel spectrum decimator.
		constexpr size_t channel_spectrum_decimation = 2 * 4;
		// TODO: Rename spectrum.bandwidth to spectrum.sampling_rate so this makes sense. */
		frequency_scale.set_channel_filter(
			spectrum.sampling_rate * channel_spectrum_decimation * 42 / 1000,
			spectrum.sampling_rate * channel_spectrum_decimation * 78 / 1000
		);
	}
};

} /* namespace spectrum */
} /* namespace ui */

#endif/*__UI_SPECTRUM_H__*/
