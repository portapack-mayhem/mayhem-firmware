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

#include "event_m0.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>

namespace ui {
namespace spectrum {

class FrequencyScale : public Widget {
public:
	void on_show() override;

	void set_spectrum_sampling_rate(const int new_sampling_rate);
	void set_channel_filter(const int pass_frequency, const int stop_frequency);

	void paint(Painter& painter) override;

private:
	static constexpr int filter_band_height = 4;

	int spectrum_sampling_rate { 0 };
	const int spectrum_bins = std::tuple_size<decltype(ChannelSpectrum::db)>::value;
	int channel_filter_pass_frequency { 0 };
	int channel_filter_stop_frequency { 0 };

	void clear();
	void clear_background(Painter& painter, const Rect r);

	void draw_frequency_ticks(Painter& painter, const Rect r);
	void draw_filter_ranges(Painter& painter, const Rect r);
};

class WaterfallView : public Widget {
public:
	void on_show() override;
	void on_hide() override;

	void paint(Painter& painter) override;

	void on_channel_spectrum(const ChannelSpectrum& spectrum);

private:
	void clear();
};

class WaterfallWidget : public View {
public:
	WaterfallWidget();

	WaterfallWidget(const WaterfallWidget&) = delete;
	WaterfallWidget(WaterfallWidget&&) = delete;
	WaterfallWidget& operator=(const WaterfallWidget&) = delete;
	WaterfallWidget& operator=(WaterfallWidget&&) = delete;

	void on_show() override;
	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;

	void paint(Painter& painter) override;

private:
	WaterfallView waterfall_view { };
	FrequencyScale frequency_scale { };
	ChannelSpectrumFIFO* fifo { nullptr };

	MessageHandlerRegistration message_handler_spectrum_config {
		Message::ID::ChannelSpectrumConfig,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
			this->fifo = message.fifo;
		}
	};
	MessageHandlerRegistration message_handler_frame_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			if( this->fifo ) {
				ChannelSpectrum channel_spectrum;
				while( fifo->out(channel_spectrum) ) {
					this->on_channel_spectrum(channel_spectrum);
				}
			}
		}
	};

	void on_channel_spectrum(const ChannelSpectrum& spectrum);
};

} /* namespace spectrum */
} /* namespace ui */

#endif/*__UI_SPECTRUM_H__*/
